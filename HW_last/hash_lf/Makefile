all:
	g++ -g -o a.out main.cpp hashtable_lockfree.cpp -lstdc++ -std=c++11 -lpthread -mrtm

test: all
	./a.out

clean:
	rm -rf a.out*                     
