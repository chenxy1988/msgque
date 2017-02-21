#ifndef __MSGQUE_H__
#define __MSGQUE_H__

#define __need_timeval 

#include <pthread.h>

#ifndef TRUE
#define TRUE    (1 == 1)
#define FALSE   (0 == 1)
#endif


#define OS_WRAP_MAX_TASKS  50
#define OS_WRAP_MAX_QUEUES (OS_WRAP_MAX_TASKS * 2)
#define OS_WRAP_MAX_SEMS   (OS_WRAP_MAX_TASKS * 10)
#define OS_NAME_MAX_LEN    10

#define M_RET_OK	0
#define M_RET_FAIL	-1


typedef unsigned long t_queid;
typedef unsigned long t_semid;

typedef void * t_msg;
typedef void * (*t_malloc_handler)(unsigned long size);
typedef void (*t_free_handler)(void *mem_p);

typedef struct t_msg_block
{
    struct t_msg_block *nextp;
    t_msg   msg;
    unsigned short msglen;
}t_msg_block;

typedef struct
{
    t_semid semid;
    pthread_mutex_t qmutex;
    t_msg_block *firstp;
    t_msg_block *lastp;

}t_qcb;

typedef struct
{
    t_semid semid;
    pthread_cond_t cond_var;
    pthread_mutex_t sem_mutex;
    int sem_count;
}t_scb;


typedef struct
{
    pthread_mutex_t g_mutex;
    int que_num;
    t_qcb   qcb_array[OS_WRAP_MAX_QUEUES];
    pthread_mutex_t g_que_mutex;
    int semnum;
    t_scb   scb_array[OS_WRAP_MAX_SEMS];
    pthread_mutex_t g_sem_mutex;

}t_queue_global ;

int msgque_init();
unsigned long msgque_create();
int msgque_send(unsigned long qid,void *msg,unsigned short msglen);
int msgque_recv(unsigned long qid,void **msg,unsigned short *msglen);


#endif
