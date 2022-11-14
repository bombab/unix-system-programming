#ifndef _STACK_H
#define _STACK_H
#include <stdio.h>
//============= 스택 ADT 정의 ===================

#define MAX_SIZE 1000

typedef int element;
typedef struct {

	element data[MAX_SIZE];
	int top;
} StackType;

void init_stack(StackType* s) {
	s->top = -1;
}

int is_stack_empty(StackType* s) {
	return (s->top == -1);
}

int is_stack_full(StackType* s) {
	return (s->top == MAX_SIZE - 1);
}

void push(StackType* s, element item) {
	if (is_stack_full(s)) {
		fprintf(stderr, "해당 스택이 포화상태입니다.");
		return;
	}
	else s->data[++(s->top)] = item;
}

element pop(StackType* s) {
	if (is_stack_empty(s)) {
		fprintf(stderr, "해당 스택이 공백상태입니다.");
		exit(1);
	}
	else return s->data[(s->top)--];
}


element peek(StackType* s) {
	if (is_stack_empty(s)) {
		fprintf(stderr, "해당 스택이 공백상태입니다.");
		exit(1);
	}
	else return s->data[s->top];
}

// =====================스택 끝=========================

#endif