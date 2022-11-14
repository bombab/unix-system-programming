#ifndef _CALC_H
#define _CALC_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"


int prec(char op) {

	switch (op) {
	case '(': case ')': return 0;
	case '+': case '-': return 1;
	case '*': case '/': return 2;
	}

	return -1;
}

int eval(char exp[]) {
	int op1, op2, i = 0, j = 0;
	int len = (int)strlen(exp);
	char tmp[100] = { '\0' };
	char ch;
	StackType cal;

	init_stack(&cal);
	for (i = 0; i < len; i++) {
		ch = exp[i];
		if (ch != '+' && ch != '-' && ch != '*' && ch != '/') {
			tmp[j++] = ch;
			if (ch == ' ') {
				op1 = atoi(tmp);
				push(&cal, op1);
				memset(tmp, 0, sizeof(char) * 100);
				j = 0;
			}
		}
		else {

			op2 = pop(&cal);
			op1 = pop(&cal);
			switch (ch) {
			case '+': push(&cal, op1 + op2); break;
			case '-': push(&cal, op1 - op2); break;
			case '*': push(&cal, op1 * op2); break;
			case '/':
				if (op2 == 0) {
					fprintf(stderr, "0으로 나눌수 없습니다.\n");
					exit(1);
				}
				else push(&cal, op1 / op2); break;

			}
		}
	}


	return pop(&cal);
}

int is_verified(char exp[]) {

	int exp_length = (int)strlen(exp)-1; // 리눅스 vim 편집기에서는 마지막에 개행문자를 자동으로 추가한다.
	char tmp;
	StackType s;
	init_stack(&s);

	if (exp_length == 0) return 0;

	for (int i = 0; i < exp_length; i++) {
		tmp = exp[i];
		switch (tmp) {
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			if ((i <= exp_length - 2) && exp[i + 1] == '(') return 0;
			break;

		case '+': case '-': case '*': case '/':
			if (i == 0 || i == exp_length - 1) return 0;
			if ((i >= 1) && exp[i - 1] == '+' || exp[i - 1] == '-' ||
				exp[i - 1] == '*' || exp[i - 1] == '/') return 0;
			break;
		case '(':
			push(&s, '(');
			break;
		case ')':
			if (is_stack_empty(&s)) return 0;
			if (exp[i - 1] == '(') return 0;
			pop(&s);
			break;
		default:
			return 0;

		}


	}
	if (!is_stack_empty(&s)) return 0;

	return 1;
} // 유효한 수식인지 판단

#endif