#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "stack.h"
#include "calc.h"
#include <sys/types.h>
#include <sys/wait.h>
#include "messageq.h"
#define NUM_THREADS 3
#define BILLION 1000000000L;


void normal_calc(void *tmp_p); // 정상적으로 계산 - 후위표기식 변환 -> 계산
void order_calc(void *tmp_p); // 연산순서 무시하고 순서대로 계산 - 후위표기식 변환 -> 계산 
void inverse_calc(void *tmp_p); // 연산순서 무시하고 거꾸고 계산 - 후위표기식 변환 -> 계산
void start_calculate(long calc_client); // client에서 파일을 읽어와 쓰레드를 생성 후 계산수행
void server_do(void *tmp_p);
void start_file_print(long clac_client);

pthread_t callThd[NUM_THREADS][NUM_THREADS];
pthread_mutex_t mutexcal_client;
long calc_client;


// main 함수가 Server 역할  
int main() {
	pid_t pid_client1, pid_client2;
	int status_waitpid;
	struct timespec start, end;
	double accum;
	printf("계산을 수행할 클라이언트를 선택해주세요 1번 - Client1, 2번 - Client2 : ");
	scanf("%ld", &calc_client);
 	if(clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
		perror("clock gettime");
		return EXIT_FAILURE;
	}

	pid_client1 = fork(); // Client 1 생성
	if(pid_client1 < 0) perror("Client 1 생성 실패");

	if(pid_client1 == 0) {  // Client 1 수행 영역
		if(calc_client != 2)  {// 2번이 아니면 파일을 읽어와 계산 수행 
			printf("Client 1에서 계산을 수행합니다. 계산을 완료 후 서버로 결과를 전송합니다. \n");
			start_calculate(calc_client);
			
		}

		else { // Server에게 계산결과를 받아 파일로 출력
			printf("Client 1에서 Server로 부터 전달받은 계산 결과를 텍스트 파일로 출력합니다.  \n");
			start_file_print(calc_client);
		}
		exit(0);
	}

	pid_client2 = fork(); // Client 2 생성

	if(pid_client2 < 0) perror("Client 2 생성 실패");


	if(pid_client2 == 0) { // Client 2 수행 영역
		if(calc_client == 2) { // 2번이면 파일을 읽어와 계산 수행 
			printf("Client 2에서 계산을 수행합니다. 계산을 완료 후 서버로 결과를 전송합니다. \n");
			start_calculate(calc_client);

		}
		
		else { // 계산결과를 받아 파일로 출력
			printf("Client 2에서 Server로 부터 전달받은 계산 결과를 텍스트 파일로 출력합니다. \n");
			start_file_print(calc_client);

		}
		exit(0);
	}
// ======= 여기서부터 Server 수행 영역 =========
int t_id;
int status;
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

if(t_id = pthread_create(&callThd[1][0], &attr, (void*)server_do, (void*)calc_client) != 0) exit(1);
if(t_id = pthread_create(&callThd[1][1], &attr, (void*)server_do, (void*)calc_client)!= 0) exit(1);
if(t_id = pthread_create(&callThd[1][2], &attr, (void*)server_do, (void*)calc_client)!= 0) exit(1);

pthread_attr_destroy(&attr);

for (int i = 0; i < NUM_THREADS; i++) {
	pthread_join(callThd[1][i], (void**)&status);
}
waitpid(pid_client1,&status_waitpid, 0);
waitpid(pid_client2,&status_waitpid, 0);
if(clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
		perror("clock gettime");
		return EXIT_FAILURE;
	}
accum = (end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / (double)BILLION;
printf("Message Passing 수행시간 : %.9f\n", accum);
pthread_exit(NULL);
}

void normal_calc(void *tmp_p) {

	char arr[MAX_SIZE];
	pthread_mutex_lock(&mutexcal_client);
	calc_thread * tp = (calc_thread *)tmp_p;
	FILE *fp = tp->fp;
	fgets(arr, sizeof(arr), fp);
	fseek(fp, 0, SEEK_SET);
	pthread_mutex_unlock(&mutexcal_client);
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
		if(Client2Server(C1toS_QKEY, answer, 'n', arr) < 0) {
			printf("메시지 전송 실패\n");
		}
	
	}
	else { // 계산하는 client가 2라면
		if(Client2Server(C2toS_QKEY, answer, 'n', arr) < 0) {
			printf("메시지 전송 실패\n");
		}
 	}
}


void order_calc(void* tmp_p) {

	char arr[MAX_SIZE];
	pthread_mutex_lock(&mutexcal_client);
	calc_thread * tp = (calc_thread *)tmp_p;
	FILE *fp = tp->fp;
	fgets(arr, sizeof(arr), fp);
	fseek(fp, 0, SEEK_SET);
	pthread_mutex_unlock(&mutexcal_client);
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
		if(Client2Server(C1toS_QKEY, answer, 'o', arr) < 0) {
			printf("메시지 전송 실패\n");
		}
	
	}
	else { // 계산하는 client가 2라면
		if(Client2Server(C2toS_QKEY, answer, 'o', arr) < 0) {
			printf("메시지 전송 실패\n");
		}
 	}

}


void inverse_calc(void* tmp_p) {

	char arr[MAX_SIZE];
	pthread_mutex_lock(&mutexcal_client);
	calc_thread * tp = (calc_thread *)tmp_p;
	FILE *fp = tp->fp;
	fgets(arr, sizeof(arr), fp);
	fseek(fp, 0, SEEK_SET);
	pthread_mutex_unlock(&mutexcal_client);
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
		if(Client2Server(C1toS_QKEY, answer, 'i', arr) < 0) {
			printf("메시지 전송 실패\n");
		}
	
	}
	else { // 계산하는 client가 2라면
		if(Client2Server(C2toS_QKEY, answer, 'i', arr) < 0) {
			printf("메시지 전송 실패\n");
		}
 	}
}


void start_calculate(long calc_client) {

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
	pthread_mutex_init(&mutexcal_client, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(t_id = pthread_create(&callThd[0][0], &attr, (void*)normal_calc, (void*)info_p) != 0) exit(1);
	if(t_id = pthread_create(&callThd[0][1], &attr, (void*)order_calc, (void*)info_p)!= 0) exit(1);
	if(t_id = pthread_create(&callThd[0][2], &attr, (void*)inverse_calc, (void*)info_p)!= 0) exit(1);

	pthread_attr_destroy(&attr);

	// 계산해서 전송하는 역할의 쓰레드 3개가 모두 수행이 완료되기까지 기다림
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(callThd[0][i], (void**)&status);
	}
	fclose(fp);
	pthread_mutex_destroy(&mutexcal_client);
	pthread_exit(NULL);

}
  
// tmp_p 는 int형 client_calc 을 받는다.
void server_do(void *tmp_p)  { 

	long calc_client = (long)tmp_p;
	int r_qid;
	int mlen;
	char print_form[MAX_SIZE*2];
	char cal_result_arr[MAX_SIZE];
	key_t revq_key, sndq_key;
	c2s_msg rcv_msg;

	if(calc_client != 2) {
		revq_key = C1toS_QKEY;
		sndq_key = StoC2_QKEY;
	}
	else {
		revq_key = C2toS_QKEY;
		sndq_key = StoC1_QKEY;
	}
	
	if((r_qid = init_queue(revq_key)) == -1) {
		perror("메시지 큐 생성 & 개방 실패");
		return; // 메시지큐 생성
	}
	
	if((mlen = msgrcv(r_qid, &rcv_msg, sizeof(rcv_msg.real_msg), 0, 0 )) ==  -1) {
		perror("메시지 수신 실패");
		return;
	}

	sprintf(cal_result_arr,  "%d", rcv_msg.real_msg.result);

	strcat(print_form, "계산식 : ");
	strcat(print_form, rcv_msg.real_msg.cal_formula);
	strcat(print_form, "계산 방법 : ");
	
	switch(rcv_msg.real_msg.calc_method) {
		case 'n':
		strcat(print_form, "정상 계산");
		break;
		case 'o':
		strcat(print_form, "앞으로 계산");
		break;
		case 'i':
		strcat(print_form, "거꾸로 계산");
	}

	strcat(print_form, "\n");
	strcat(print_form, "계산 결과 : ");
	strcat(print_form, cal_result_arr);
	strcat(print_form, "\n");

	if(Server2Client(sndq_key, rcv_msg.real_msg.calc_method , print_form) < 0) {
			printf("메시지 전송 실패\n");
		}

}

void file_print(void *tmp_p) {
	long calc_client = (long)tmp_p;
	int r_qid;
	int mlen;
	key_t revq_key;
	s2c_msg rcv_msg;
	FILE *fp;
	if(calc_client != 2) revq_key = StoC2_QKEY;
	else  revq_key = StoC1_QKEY;
	
	if((r_qid = init_queue(revq_key)) == -1) {
		perror("메시지 큐 생성 & 개방 실패");
		return; // 메시지큐 생성
	}
	
	if((mlen = msgrcv(r_qid, &rcv_msg, sizeof(rcv_msg.real_msg), 0, 0 )) ==  -1) {
		perror("메시지 수신 실패");
		return;
	}

	switch(rcv_msg.real_msg.calc_method) {
		case 'n':
			if((fp = fopen("output3.txt", "w")) == NULL) {
				perror("파일 열기 실패");
				return;
			}
			fprintf(fp, "%s", rcv_msg.real_msg.print_msg);
			fclose(fp);
			break;
		case 'o':
		    if((fp = fopen("output1.txt", "w")) == NULL) {
				perror("파일 열기 실패");
				return;
			}
			fprintf(fp, "%s", rcv_msg.real_msg.print_msg);
			fclose(fp);
			break;
		case 'i':
		   		if((fp = fopen("output2.txt", "w")) == NULL) {
				perror("파일 열기 실패");
				return;
			}
			fprintf(fp, "%s", rcv_msg.real_msg.print_msg);
			fclose(fp);
	}

}

void start_file_print(long calc_client) {
	
	int t_id;
	int status;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(t_id = pthread_create(&callThd[2][0], &attr, (void*)file_print, (void*)calc_client) != 0) exit(1);
	if(t_id = pthread_create(&callThd[2][1], &attr, (void*)file_print, (void*)calc_client)!= 0) exit(1);
	if(t_id = pthread_create(&callThd[2][2], &attr, (void*)file_print, (void*)calc_client)!= 0) exit(1);

	pthread_attr_destroy(&attr);

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(callThd[2][i], (void**)&status);
	}
	pthread_exit(NULL);
}