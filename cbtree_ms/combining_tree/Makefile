CFLAGS=-g -Wall
CC=gcc
LIBS= -lpthread -pg
OBJS= ct_node.o ct_stack.o ct_tree.o
SRCS= ct_node.c ct_stack.c ct_tree.c

ct: ct_main.o ${OBJS}
	${CC} ${CFLAGS} -o ct_main ct_main.c ${OBJS} ${LIBS}


clean:
	rm ct_main ${OBJS}
