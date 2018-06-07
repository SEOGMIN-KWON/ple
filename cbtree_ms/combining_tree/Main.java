import java.util.ArrayList;
import java.util.Stack;
import java.lang.Exception;


class Node{
    enum CStatus{IDLE, FIRST, SECOND, RESULT, ROOT}; 
    boolean locked;
    CStatus cStatus;
    int firstValue, secondValue;
    int result;
    Node parent;
    int no;
    Node(int no) {
        this.no = no;
        parent = null;
        cStatus = CStatus.ROOT;
        locked = false; 
        firstValue = secondValue = result = 0;
    }
    
    Node(int no, Node myParent) { 
        this.no = no;
        parent = myParent;
        cStatus = CStatus.IDLE; 
        locked = false;
        firstValue = secondValue = result = 0;
    }

    synchronized boolean precombine() throws Exception{
        while(locked) {
            wait();
        }
        switch (cStatus){
            case IDLE:
                cStatus = CStatus.FIRST;
                return true;
            case FIRST:
                locked = true;
                cStatus = CStatus.SECOND;
                return false;
            case ROOT:
                return false;
            default:
                throw new Exception();
        }
    }

    synchronized int combine(int combined) throws Exception{
        while(locked){
            wait();
        }
        locked = true;
        firstValue = combined;
        switch(cStatus){
            case FIRST:
                return firstValue;
            case SECOND:
                return firstValue + secondValue;
            default:
                throw new Exception();
        }
    }

    synchronized int op(int combined) throws Exception{
        switch(cStatus){
            case ROOT:
                int prior = result;
                result += combined;
                return prior;
            case SECOND:
                secondValue = combined;
                locked = false;
                notifyAll();
                while(cStatus != CStatus.RESULT){
                    wait();
                }
                locked = false;
                notifyAll();
                cStatus = CStatus.IDLE;
                return result;
            default:
                throw new Exception();
        }
    }

    synchronized void distribute(int prior) throws Exception{
        switch(cStatus){
            case FIRST:
                cStatus = CStatus.IDLE;
                locked = false;
                break;
            case SECOND:
                result = prior + firstValue;
                cStatus = CStatus.RESULT;
                break;
            default:
                throw new Exception();
        }
        notifyAll();
    }
}

class CombiningTree{
    Node[] leaf;
    Node[] nodes;

    CombiningTree(int width){
        nodes = new Node[width - 1]; 
        nodes[0] = new Node(0);
        for (int i = 1; i < nodes.length; i++) {
            nodes[i] = new Node(i, nodes[(i-1)/2]);
        }
        leaf = new Node[(width + 1)/2];
        for (int i = 0; i < leaf.length; i++) {
          leaf[i] = nodes[nodes.length - i - 1];
        }
    }

    int getAndIncrement(int tid) throws Exception{
        Stack<Node> stack = new Stack<Node>();
        Node myLeaf = leaf[tid/2];
        Node node = myLeaf;

        
        try{
            while(node.precombine()) node = node.parent;
        } catch (Exception e){
            System.out.println(node.no + " precombine " + node.cStatus);
        }
        Node stop = node;

        node = myLeaf;
        int combined = 1;
        while(node != stop){
            try{
                combined = node.combine(combined);
            } catch (Exception e){
                System.out.println(node.no + " combine " + node.cStatus);
            }
            stack.push(node);
            node = node.parent;
        }
        
        int prior = 0;
        try{
            prior = stop.op(combined);
        } catch (Exception e){
            System.out.println(node.no + " op " + node.cStatus);
        }

        while(!stack.empty()){
            node = stack.pop();

            try{
                node.distribute(prior);
            } catch (Exception e){
                System.out.println(node.no + " distribute " + node.cStatus);
            }
        }
        return prior;
    }
}

class AtomicInc implements Runnable{
    int v;
    AtomicInc(){
        v = 0;
    }

    synchronized int getAndIncrement(){
        int ret = v;
        v++;
        return ret;
    }

    public void run(){
        
    }
}

class Thtest implements Runnable {
    CombiningTree ct;
    int tid;
    int v;
    Thtest(int tid, CombiningTree ct, int v){
        this.tid = tid;
        this.ct = ct;
        this.v = v;
    }

    public void run() {

        for(int i=0; i<v; i++){
            try{
                ct.getAndIncrement(tid);
            } catch (Exception e){
                System.out.println( "gai error");
            }
        }
    }
}
public class Main {
    static CombiningTree ct;
    static int nThread = 8;
    static int v = 100000;
    
    public static void main(String args[]){
        ct = new CombiningTree(nThread-1);
        ArrayList<Thread> threads = new ArrayList<Thread>();
        long start = System.currentTimeMillis();
        for(int i=0; i<nThread; i++) {
            System.out.println("tid " + i);
            Thread t = new Thread(new Thtest(i, ct, v));
            t.start();
            threads.add(t);
        }

        for(int i=0; i<threads.size(); i++) {
            try{
                threads.get(i).join();
            } catch (Exception e){
                System.out.println( "join error");
            }
        }
        long end = System.currentTimeMillis();
        System.out.println( "실행 시간 : " + ( end - start ) );
        
    }

}