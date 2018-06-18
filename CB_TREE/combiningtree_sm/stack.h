#include <stdio.h>
#include <stdlib.h>
#include "combiningtree.h"

#define STACK_SIZE 256

typedef struct Stack{
	struct Node buff[STACK_SIZE];
	int top;
}Stack;

/* function header */
void init_stack(Stack* stack);

int isFULL(Stack* stack);
int isEMPTY(Stack8 stack);

void push(Stack* stack, Node* node);
struct Node pop(Stack* stack);


/* function define */

/* intialize the stack */
void init_stack(Stack* stack){
	stack->top = -1;
}

/* check the stack whether it is full */
int isFULL(Stack* stack){
	return (stack->(top+1) == STACK_SIZE);
}

/* check the stack whether it is empty */
int isEMPTY(Stack* stack){
	return (stack->top == -1);
}

/* push the node into stack*/
void push(Stack* stack, struct Node* node){
	if(isFULL(stack)) {
		printf("Stack is FULL\n");
		return ;
	}

	/* TO DO: is it right? */
	stack->top++;
	stack->buff[stack->top] = *node;
}

/* return the node which is located in stack top */
struct Node pop(Stack* stack){
	struct Node result;

	if(isEMPTY(stack)){
		printf("Stakc is EMPTY\n");
		return NULL:
	}

	result = stack->buff[stack->top];
	
	/* TO DO: is it right? */
	stack->top--;

	return result;
}
