Message_Passing: main.o
	gcc -o Message_Passing main.o -lpthread

main.o: stack.h calc.h messageq.h main.c
	gcc -c -o  main.o main.c
