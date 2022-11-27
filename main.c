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
#define NUM_THREADS 4
#define NUM_PROCESS 3
#define BILLION 1000000000L;
#define CLIENT_ONE 1
#define CLIENT_TWO 2
#define SERVER 0

void normal_calc(void *tmp_p); // 정상적으로 계산 - 후위표기식 변환 -> 계산
void order_calc(void *tmp_p); // 연산순서 무시하고 순서대로 계산 - 후위표기식 변환 -> 계산 
void inverse_calc(void *tmp_p); // 연산순서 무시하고 거꾸고 계산 - 후위표기식 변환 -> 계산
void start_client(long client); // client에서 파일을 읽어와 쓰레드를 생성 후 계산수행
void recv_thread(void *tmp_p);
void send_thread(void *tmp_p);
void file_print(void *tmp_p);

int is_finished_rcv1 = 0;
int is_finished_rcv2 = 0; 
char  input1_result[MAX_SIZE];
char  input2_result[MAX_SIZE];


pthread_t callThd[NUM_PROCESS][NUM_THREADS];

typedef struct {
	FILE *fp;
	int client;
	pthread_mutex_t* mutexcal_client;
} calc_thread_info;


// main 함수가 Server 역할  
int main() {
	pid_t pid_client1, pid_client2;
	int status_waitpid;
	struct timespec start, end;
	double accum;
 	if(clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
		perror("clock gettime");
		return EXIT_FAILURE;
	}

	if((pid_client1 = fork()) < 0) perror("Client 1 생성 실패");

	else if(pid_client1 == 0) {  // Client 1 수행 영역
		start_client(CLIENT_ONE);
		exit(0);
	}


	if((pid_client2 = fork()) < 0) perror("Client 2 생성 실패");

	else if(pid_client2  == 0) { // Client 2 수행 영역
		start_client(CLIENT_TWO);
		exit(0);
	}


// ======= 여기서부터 Server 수행 영역 =========
int t_id;
int status;
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

if(t_id = pthread_create(&callThd[SERVER][0], &attr, (void*)recv_thread, (void*) CLIENT_ONE)!= 0) exit(1);
if(t_id = pthread_create(&callThd[SERVER][1], &attr, (void*)recv_thread, (void*) CLIENT_TWO)!= 0) exit(1);
if(t_id = pthread_create(&callThd[SERVER][2], &attr, (void*)send_thread,(void *)CLIENT_ONE)!= 0) exit(1);
if(t_id = pthread_create(&callThd[SERVER][3], &attr, (void*)send_thread,(void*)CLIENT_TWO)!= 0) exit(1);

pthread_attr_destroy(&attr);

for (int i = 0; i < NUM_THREADS; i++) {
	pthread_join(callThd[SERVER][i], (void**)&status);
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
	calc_thread_info *info_p = (calc_thread_info *)tmp_p;

	pthread_mutex_lock(info_p->mutexcal_client);
	fgets(arr, sizeof(arr), info_p->fp);
	fseek(info_p->fp, 0, SEEK_SET);
	pthread_mutex_unlock(info_p->mutexcal_client);
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

	if(info_p->client == CLIENT_ONE) { // 계산하는 client가 1이라면
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
	calc_thread_info *info_p = (calc_thread_info *)tmp_p;

	pthread_mutex_lock(info_p->mutexcal_client);
	fgets(arr, sizeof(arr), info_p->fp);
	fseek(info_p->fp, 0, SEEK_SET);
	pthread_mutex_unlock(info_p->mutexcal_client);

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

	if(info_p->client == CLIENT_ONE) { // 계산하는 client가 1이라면
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
	calc_thread_info *info_p = (calc_thread_info *)tmp_p;

	pthread_mutex_lock(info_p->mutexcal_client);
    fgets(arr, sizeof(arr), info_p->fp);
	fseek(info_p->fp, 0, SEEK_SET);
	pthread_mutex_unlock(info_p->mutexcal_client);

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

	if( info_p->client == CLIENT_ONE) { // 계산하는 client가 1이라면
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


void start_client(long client) {

	int status;
	int t_id;
	FILE *fp;
	calc_thread_info info;
	calc_thread_info *info_p = &info;
	if(client == CLIENT_ONE)
		 fp = fopen("input1.txt", "r");
	else
		 fp = fopen("input2.txt", "r");

	if (fp == NULL) {
		fprintf(stderr, "파일이 열리지 않았습니다.\n");
		exit(1);
	}

	

	pthread_mutex_t mutexcal_client;
	pthread_attr_t attr;

	info.client = client;
	info.fp = fp;
	info.mutexcal_client = &mutexcal_client;

	
	pthread_mutex_init(&mutexcal_client, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	if(t_id = pthread_create(&callThd[client][0], &attr, (void*)normal_calc, (void*)info_p) != 0) exit(1);
	if(t_id = pthread_create(&callThd[client][1], &attr, (void*)order_calc, (void*)info_p)!= 0) exit(1);
	if(t_id = pthread_create(&callThd[client][2], &attr, (void*)inverse_calc, (void*)info_p)!= 0) exit(1);
	if(t_id = pthread_create(&callThd[client][3], &attr, (void*)file_print, (void*)client)!= 0) exit(1);

	pthread_attr_destroy(&attr);

	// 계산해서 전송하는 역할의 쓰레드 3개가 모두 수행이 완료되기까지 기다림
	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(callThd[client][i], (void**)&status);
	}
	fclose(fp);
	pthread_mutex_destroy(&mutexcal_client);
	pthread_exit(NULL);

}

void recv_thread(void *tmp_p) {

	long client = (long)tmp_p;
	int r_qid; int mlen;
	int rcv_count = 0, i = 0;
	key_t revq_key;
 	c2s_msg rcv_msg[3];
	char print_form[MAX_SIZE*2];
	char cal_result_arr[3][MAX_SIZE];

	if(client == CLIENT_ONE) revq_key = C1toS_QKEY;
	else revq_key = C2toS_QKEY;
	

	if((r_qid = init_queue(revq_key)) == -1) {
		perror("메시지 큐 생성 & 개방 실패");
		return; // 메시지큐 생성
	}

	while(1) {
		if((mlen = msgrcv(r_qid, &rcv_msg[rcv_count], sizeof(rcv_msg[0].real_msg), 0, IPC_NOWAIT )) ==  -1) {
			if(errno != ENOMSG ) {
				perror("메시지 수신 실패");
				return;
			}
		}
		else if((++rcv_count) == 3) break; 
	}

	for(i = 0; i < rcv_count; i++) {
		switch(rcv_msg[i].real_msg.calc_method) {
			case 'n':
				sprintf(cal_result_arr[0],  "%d", rcv_msg[i].real_msg.result);
				break;
			case 'o':
				sprintf(cal_result_arr[1],  "%d", rcv_msg[i].real_msg.result);
				break;
			case 'i':
				sprintf(cal_result_arr[2],  "%d", rcv_msg[i].real_msg.result);
		}
	}

	strcat(print_form, "계산식 : ");
	strcat(print_form, rcv_msg[0].real_msg.cal_formula);
	strcat(print_form, "\n");
	strcat(print_form, "정상 계산 : ");
	strcat(print_form, cal_result_arr[0]);
	strcat(print_form, "\n");
	strcat(print_form, "앞으로 계산 : ");
	strcat(print_form, cal_result_arr[1]);
	strcat(print_form, "\n");
	strcat(print_form, "뒤로 계산 : ");
	strcat(print_form, cal_result_arr[2]);

	if(client == CLIENT_ONE) {
		strcpy(input1_result, print_form);
		is_finished_rcv1 = 1;
	}
	else {
		strcpy(input2_result, print_form);
		is_finished_rcv2 = 1;
	}

}


void send_thread(void *tmp_p) {

	long client = (long)tmp_p;
	while(1) {
			if((client == CLIENT_ONE) && is_finished_rcv1 ) {
				Server2Client(StoC2_QKEY, input1_result);
				break;
			}
			else if((client == CLIENT_TWO ) && is_finished_rcv2) {
				Server2Client(StoC1_QKEY, input2_result);
				break;
			}
	}

}


void file_print(void *tmp_p) {

	int r_qid;
	int mlen;
	long client = (long)tmp_p;
	key_t revq_key;
	s2c_msg rcv_msg;
	FILE *fp;
	if(client == CLIENT_ONE) revq_key = StoC1_QKEY;
	else  revq_key = StoC2_QKEY;
	
	if((r_qid = init_queue(revq_key)) == -1) {
		perror("메시지 큐 생성 & 개방 실패");
		return; // 메시지큐 생성
	}
	
	while(1) {
		if((mlen = msgrcv(r_qid, &rcv_msg, sizeof(rcv_msg.real_msg), 0, IPC_NOWAIT )) ==  -1) {
			if(errno != ENOMSG ) {	
				perror("메시지 수신 실패");
				return;
			}
		}
		else break;
	}

	switch(client) {
		case CLIENT_ONE:
			if((fp = fopen("output2.txt", "w")) == NULL) {
				perror("파일 열기 실패");
				return;
			}
			break;

		case CLIENT_TWO:
		    if((fp = fopen("output1.txt", "w")) == NULL) {
				perror("파일 열기 실패");
				return;
			}
			break;
	}

	fprintf(fp, "%s", rcv_msg.real_msg.print_msg);
	fclose(fp);

}

