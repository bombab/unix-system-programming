#ifndef _MESSAGEQ_H
#define _MESSAGEQ_H
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define C1toS_QKEY (key_t)60140 // client1->sever 작업 큐의 key 값
#define C2toS_QKEY (key_t)60141 // client2->server 작업 큐의  key 값
#define StoC1_QKEY (key_t)60142 // server->cleint1 작업 큐의 key 값
#define StoC2_QKEY (key_t)60143 // server->cleint2 작업 큐의 key  값
#define QPERM 0660              //큐의 허가
#define MSG_MTYPE 1             /*메시지큐의 메시지에 대한 모든 MTYPE  정의*/

typedef struct  { // 계산을 수행할 쓰레드에 전달해줘야할 구조체 타입            
    long calc_client;
    FILE *fp;
} calc_thread;

typedef struct
{ // 메시지 큐로 send 할 메시지중 실질 메시지 내용
    int result;
    char calc_method;
} cal_result_msg;

typedef struct
{               // 메시지 큐로 send 할 메시지 타입
    long mtype; /* message type */
    cal_result_msg real_msg;
} c2s_msg;

/* init_queue -- 큐 식별자를 회득한다. */
int init_queue(key_t type)
{

    int queue_id;

    /*메시지 큐를 생성하거나 개방하려고 시도한다*/
    if ((queue_id = msgget(type, IPC_CREAT | QPERM)) == -1)
        perror("msgget failed");

    return (queue_id);
}

int Client2Server(key_t type, int cal_result, char cal_type)
{ // client에서 server로 메시지를 전송한다.
    int s_qid;
    c2s_msg c2s;
    key_t snd_type = type;
    if ((s_qid = init_queue(snd_type)) == -1)
        return -1; // 메시지큐 생성

    c2s.real_msg.result = cal_result;
    c2s.real_msg.calc_method = cal_type;
    c2s.mtype = (long)MSG_MTYPE;

    if (msgsnd(s_qid, &c2s, sizeof(c2s.real_msg), 0) == -1)
    {

        perror("msgsnd failed");
        return -1;
    }
    else
        return 0; // 정상적으로 전송될 시 0을 반환
}

int Server2Client()
{ // server에서 client로 메시지 전송
    return 0;
}

#endif