#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "stack.h"
#include "calc.h"
#include "messageq.h"
#define NUM_THREADS 3

void normal_calc(void *tmp_p); // 정상적으로 계산 - 후위표기식 변환 -> 계산
void order_calc(void *tmp_p); // 연산순서 무시하고 순서대로 계산 - 후위표기식 변환 -> 계산 
void inverse_calc(void *tmp_p); // 연산순서 무시하고 거꾸고 계산 - 후위표기식 변환 -> 계산
void start_calculate(int calc_client); // client에서 파일을 읽어와 쓰레드를 생성 후 계산수행


pthread_t callThd[NUM_THREADS];
pthread_mutex_t mutexcal;
int calc_client;

/*
int main() {
	int status;
	FILE* fp = fopen("input.txt", "r");
	if (fp == NULL) {
		fprintf(stderr, "파일이 열리지 않았습니다.\n");
		exit(1);
	}
	pthread_attr_t attr;
	
	

	pthread_mutex_init(&mutexcal, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	pthread_create(&callThd[0], &attr, (void*)normal_calc, (void*)fp);
	pthread_create(&callThd[1], &attr, (void*)order_calc, (void*)fp);
	pthread_create(&callThd[2], &attr, (void*)inverse_calc, (void*)fp);

	pthread_attr_destroy(&attr);

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(callThd[i], (void**)&status);
	}
	fclose(fp);
	pthread_mutex_destroy(&mutexcal);
	pthread_exit(NULL);

}
*/


// main 함수가 Server 역할  
int main() {
	pid_t pid;

	printf("계산을 수행할 클라이언트를 선택해주세요 1번 : Client1, 2번 : Client2");
	scanf("%d", &calc_client);
 	
	pid = fork(); // Client 1 생성
	if(pid < 0) perror("Client 1 생성 실패");

	if(pid == 0) {  // Client 1 수행 영역
		if(calc_client != 2)  {// 2번이 아니면 파일을 읽어와 계산 수행 
			printf("Client 1에서 계산을 수행합니다. 계산을 완료 후 서버로 결과를 전송합니다. \n");
			start_calculate(calc_client);
			
		}

		else { // 계산결과를 받아 파일로 출력

		}
		exit(0);
	}

	pid = fork(); // Client 2 생성

	if(pid < 0) perror("Client 2 생성 실패");


	if(pid == 0) { // Client 2 수행 영역
		if(calc_client == 2) { // 2번이면 파일을 읽어와 계산 수행 
			printf("Client 2에서 계산을 수행합니다. 계산을 완료 후 서버로 결과를 전송합니다. \n");
			start_calculate(calc_client);

		}
		
		else { // 계산결과를 받아 파일로 출력


		}
		exit(0);
	}
// 여기서부터 Server 수행 영역



	return 0;
}

void normal_calc(void *tmp_p) {

	char arr[MAX_SIZE];
	pthread_mutex_lock(&mutexcal);
	calc_thread * tp = (calc_thread *)tmp_p;
	FILE *fp = tp->fp;
	fgets(arr, sizeof(arr), fp);
	fseek(fp, 0, SEEK_SET);
	pthread_mutex_unlock(&mutexcal);
	int verified = is_verified(arr);
	if (verified == 0) {
		fprintf(stderr, "수식이 유효하지 않습니다.\n");
		exit(1); // 추후 가능하면 thread로 리턴하고 특정값을 전달하여 클라이언트에서 에러 발생을 알리는 식으로 변경 
	}

	int expr_length = (int)strlen(arr), j = 0;
	char tmp, top_op;
	char to_postfix[MAX_SIZE] = { '\0' };
	StackType s;
	init_stack(&s);

	// 후위 표기법으로 변경
	for (int i = 0; i < expr_length; i++) {
		tmp = arr[i];
		switch (tmp) {

		case '+': case '-': case '*': case '/':
			if (i >= 1 && arr[i - 1] == ')');
			else to_postfix[j++] = ' ';
			while (!is_stack_empty(&s) && (prec(tmp) <= prec(peek(&s))))
				to_postfix[j++] = pop(&s);
			push(&s, tmp);
			break;
		case '(':
			push(&s, tmp);
			break;
		case ')':
			to_postfix[j++] = ' ';
			top_op = pop(&s);
			while (top_op != '(') {
				to_postfix[j++] = top_op;
				top_op = pop(&s);
			}
			break;
		default:
			to_postfix[j++] = tmp;
		}


	}  
	if (expr_length > 0 && arr[expr_length - 1] == ')');
	else to_postfix[j++] = ' ';
	while (!is_stack_empty(&s))
		to_postfix[j++] = pop(&s);

	int answer = eval(to_postfix);


	if(tp->calc_client != 2) { // 계산하는 client가 1이라면
		if(Client2Server(C1toS_QKEY, answer, 'n') < 0) {
			printf("메시지 전송 실패\n");
		}
	
	}
	else { // 계산하는 client가 2라면
		if(Client2Server(C2toS_QKEY, answer, 'n') < 0) {
			printf("메시지 전송 실패\n");
		}
 	}
}


void order_calc(void* tmp_p) {

	char arr[MAX_SIZE];
	pthread_mutex_lock(&mutexcal);
	calc_thread * tp = (calc_thread *)tmp_p;
	FILE *fp = tp->fp;
	fgets(arr, sizeof(arr), fp);
	fseek(fp, 0, SEEK_SET);
	pthread_mutex_unlock(&mutexcal);
	int verified = is_verified(arr);
	if (verified == 0) {
		fprintf(stderr, "수식이 유효하지 않습니다.\n");
		exit(1); // 추후 가능하면 thread로 리턴하고 특정값을 전달하여 클라이언트에서 에러 발생을 알리는 식으로 변경 
	}

	int expr_length = (int)strlen(arr), j = 0;
	char tmp;
	char to_postfix[MAX_SIZE] = { '\0' };
	StackType s;
	init_stack(&s);

	for (unsigned int i = 0; i < strlen(arr); i++) {
		tmp = arr[i];
		switch (tmp) {
		case '(': case ')': continue;
		case '+': case '-': case '*': case '/':
			to_postfix[j++] = ' ';
			if (!is_stack_empty(&s)) to_postfix[j++] = pop(&s);
			push(&s, tmp);
			break;
		default:
			to_postfix[j++] = tmp;
		}
	}
	to_postfix[j++] = ' ';
	to_postfix[j++] = pop(&s);

	int answer = eval(to_postfix);
	if(tp->calc_client != 2) { // 계산하는 client가 1이라면
		if(Client2Server(C1toS_QKEY, answer, 'o') < 0) {
			printf("메시지 전송 실패\n");
		}
	
	}
	else { // 계산하는 client가 2라면
		if(Client2Server(C2toS_QKEY, answer, 'o') < 0) {
			printf("메시지 전송 실패\n");
		}
 	}

}


void inverse_calc(void* tmp_p) {

	char arr[MAX_SIZE];
	pthread_mutex_lock(&mutexcal);
	calc_thread * tp = (calc_thread *)tmp_p;
	FILE *fp = tp->fp;
	fgets(arr, sizeof(arr), fp);
	fseek(fp, 0, SEEK_SET);
	pthread_mutex_unlock(&mutexcal);
	int verified = is_verified(arr);
	if (verified == 0) {
		fprintf(stderr, "수식이 유효하지 않습니다.\n");
		exit(1); // 추후 가능하면 thread로 리턴하고 특정값을 전달하여 클라이언트에서 에러 발생을 알리는 식으로 변경 
	}

	int expr_length = (int)strlen(arr), j = 0;
	char tmp;
	char to_postfix[MAX_SIZE] = { '\0' };
	StackType s; StackType num;
	init_stack(&s); init_stack(&num);

	for (int i = (int)strlen(arr) - 1; i >= 0; i--) {
		tmp = arr[i];
		switch (tmp) {
		case '(': case ')': continue;
		case '+': case '-': case '*': case '/':
			while (!is_stack_empty(&num)) {
				to_postfix[j++] = pop(&num);
			}
			to_postfix[j++] = ' ';
			if (!is_stack_empty(&s)) to_postfix[j++] = pop(&s);
			push(&s, tmp);
			break;
		default:
			push(&num, tmp);
		}
	}
	while (!is_stack_empty(&num)) {
		to_postfix[j++] = pop(&num);
	}
	to_postfix[j++] = ' ';
	to_postfix[j++] = pop(&s);

	int answer = eval(to_postfix);
	if(tp->calc_client != 2) { // 계산하는 client가 1이라면
		if(Client2Server(C1toS_QKEY, answer, 'i') < 0) {
			printf("메시지 전송 실패\n");
		}
	
	}
	else { // 계산하는 client가 2라면
		if(Client2Server(C2toS_QKEY, answer, 'i') < 0) {
			printf("메시지 전송 실패\n");
		}
 	}
}


void start_calculate(int calc_client) {

	int status;
	int t_id;
	FILE* fp = fopen("input.txt", "r");
	if (fp == NULL) {
		fprintf(stderr, "파일이 열리지 않았습니다.\n");
		exit(1);
	}
	pthread_attr_t attr;
	
	calc_thread info;
	calc_thread *info_p = &info;
	info_p->calc_client =  calc_client;
	info_p->fp =  fp;
	pthread_mutex_init(&mutexcal, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(t_id = pthread_create(&callThd[0], &attr, (void*)normal_calc, (void*)info_p) != 0) exit(1);
	if(t_id = pthread_create(&callThd[1], &attr, (void*)order_calc, (void*)info_p)!= 0) exit(1);
	if(t_id = pthread_create(&callThd[2], &attr, (void*)inverse_calc, (void*)info_p)!= 0) exit(1);

	pthread_attr_destroy(&attr);

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(callThd[i], (void**)&status);
	}
	fclose(fp);
	pthread_mutex_destroy(&mutexcal);
	pthread_exit(NULL);

}