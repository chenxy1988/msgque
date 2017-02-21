/************************************************************************
**         Message queue manager basic functions
**
** Copyright 2017 Chen Xiangyu(xiangyu.chen@aol.com) 
**
**
** Project website:
** https://github.com/chenxy1988/msgque
**
** This is a mothod for multi-thread communication under linux.
** Should you have any questions, pls. do not hesitate to 
** contact xiangyu.chen@aol.com
**
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msgque.h"

#define __MALLOC_TEST_ONLY_


static t_queue_global q_data;
static unsigned long mem_alloc_count = 0; //For memory testing


#ifdef __MALLOC_TEST_ONLY_
/*
	Testing only, for memory usage 
*/

void show_malloc_count()
{
    printf("memory alloc count :%ld \n",mem_alloc_count);
}
#endif


void *mem_malloc(unsigned long size)
{
    void *memp = malloc(size);

    if(memp == NULL)
    {
        printf("mem_malloc: failed, seems out of memory Size %ld \n",size);
		return NULL;
    }

    mem_alloc_count++;

    return (memp);

}


void mem_free(void *memp)
{
    if(memp == NULL)
    {
        printf("mem_free (): NULL memp");
    }
    else
    {
        free(memp);
    	mem_alloc_count--;
    }

}



int msgque_mutex_create(pthread_mutex_t *pmutex)
{
    if(pthread_mutex_init(pmutex,NULL) != 0)
    {
        printf("msgque_mutex_create() ; pthread_mutex_init() failed \n");
        return (M_RET_FAIL);
    }
    return (M_RET_OK);
}


int msgque_mutex_lock(pthread_mutex_t *pmutex)
{
    if(pthread_mutex_lock(pmutex) != 0)
    {
        printf("msgque_mutex_lock(); pthread_mutex_lock() failed \n");
        return (M_RET_FAIL);
    }

    return (M_RET_OK);
}


int msgque_mutex_unlock(pthread_mutex_t *pmutex)
{
    if(pthread_mutex_unlock(pmutex) != 0)
    {
        printf("msgque_mutex_unlock(); pthread_mutex_unlock() failed !\n");
        return (M_RET_FAIL);
    }

    return (M_RET_OK);
}



/* 
 * msgque cond init.
 *
 */

static int msgque_sem_create(t_semid *psemid)
{
    t_scb *pscb = NULL;
    int semindex;

    msgque_mutex_lock(&q_data.g_sem_mutex);

    semindex = q_data.semnum;    

    if(semindex >= OS_WRAP_MAX_SEMS)
    {
        msgque_mutex_unlock(&q_data.g_sem_mutex);
        printf("msgque_sem_create(), semindex > max sems limit \n");
        return (M_RET_FAIL);
    }

    pscb = &(q_data.scb_array[semindex]);   

    if(msgque_mutex_create(&pscb->sem_mutex) != M_RET_OK)
    {
        msgque_mutex_unlock(&q_data.g_sem_mutex);
        printf("msgque_sem_create(),msgque_mutex_create(&pscb->sem_mutex) failed\n");
        return (M_RET_FAIL);
    }

    if(pthread_cond_init(&pscb->cond_var,NULL) != 0)    
    {
        pthread_mutex_destroy(&pscb->sem_mutex);
        msgque_mutex_unlock(&q_data.g_sem_mutex);
        printf("msgque_sem_create(),pthread_cond_init(&pscb->cond_var,NULL) failed\n");
        return (M_RET_FAIL);
    }

    pscb->semid = semindex + 1;  //return index+1, be sure the sem id non-zero
    pscb->sem_count = 0; //resource count.
    q_data.semnum++;
    *psemid = pscb->semid;//return by args

    msgque_mutex_unlock(&q_data.g_sem_mutex);

    return (M_RET_OK);


}

/** 
 *	Release conds
 *
 */

static int msgque_sem_release(t_semid semid)
{
    t_scb *pscb;
    int semindex;

    /*
     * Get address of semid cond
    */
    for(semindex=0;semindex < q_data.semnum;semindex++)
    {
        if(q_data.scb_array[semindex].semid == semid)
        {
            break;
        }
    }

    if(semindex >= q_data.semnum)
    {
        printf("msgque_sem_release(); semindex > q_data.semnum ,No such semid ! \n");
        return (M_RET_FAIL);
    }

    pscb = &q_data.scb_array[semindex];

    msgque_mutex_lock(&pscb->sem_mutex);

    //v 
    pscb->sem_count++;  //free resource +1

    if(pscb->sem_count == 1)    //if this resource is 1, must be busy.
    {
        pthread_cond_signal(&pscb->cond_var);   //Send a signal
    }

    msgque_mutex_unlock(&pscb->sem_mutex);

    return (M_RET_OK);

}

/** 
 *	Waiting a cond
 *
 *
 */

static int msgque_sem_wait(t_semid semid)
{
    t_scb *pscb;
    int semindex;

    for(semindex = 0;semindex < q_data.semnum;semindex++)
    {
        if(q_data.scb_array[semindex].semid == semid)   //According semid, get address of array
        {
            break;
        }
    }

    if(semindex >= q_data.semnum)
    {
        printf("msgque_sem_wait(); semindex > q_data.semnum ,No such semid ! \n");
        return (M_RET_FAIL);
    }

    pscb = &q_data.scb_array[semindex];

    msgque_mutex_lock(&pscb->sem_mutex);

    //p
    if(pscb->sem_count == 0)    //if resource == 0, there must a task waiting
    {
        if(pthread_cond_wait(&pscb->cond_var,&pscb->sem_mutex) != 0)
        {
            return M_RET_FAIL;
        }
    }

    if(pscb->sem_count > 0)
    {
        pscb->sem_count--;   // large than 0, resource -1
    }

    msgque_mutex_unlock(&pscb->sem_mutex);
    return (M_RET_OK);
}

/*
 *    Creating a message queue,
 *    Success: return a vaild queue id,
 *    Failed : return -1
 *
 */

int msgque_create()
{
    t_qcb *pqcb = NULL;
    int qindex;
    t_queid queid;

    msgque_mutex_lock(&q_data.g_que_mutex);

    qindex = q_data.que_num; 

    if(qindex >= OS_WRAP_MAX_QUEUES)    
    {
        msgque_mutex_unlock(&q_data.g_que_mutex);
        printf("sys_msgque_create(); qindex >= OS_WARP_MAX_QUEUES \n");
        return (M_RET_FAIL);
    }

    pqcb = &(q_data.qcb_array[qindex]);

    if(msgque_sem_create(&pqcb->semid) != M_RET_OK)
    {
        printf("sys_msgque_create(); msgque_sem_create() != 0 \n");
        msgque_mutex_unlock(&q_data.g_que_mutex);
        return (M_RET_FAIL);
    }

    if(msgque_mutex_create(&pqcb->qmutex) != M_RET_OK)
    {
        printf("sys_msgque_create(); msgque_mutex_create() != 0 \n");
        msgque_mutex_unlock(&q_data.g_que_mutex);
        return (M_RET_FAIL);
    }

    pqcb->firstp = pqcb->lastp;     
    q_data.que_num++;
    queid = qindex + 1;    

    msgque_mutex_unlock(&q_data.g_que_mutex);

    return queid;

}

/*  Sending a message to queue,
 *  qid: queue id
 *  msg: data which send to queue
 *  msglen: length of data
 *  Return value:
 *  Success: M_RET_SUCCESS
 *  Failed:  M_RET_FAIL
 *
 */
int msgque_send(unsigned long qid,void *msg,unsigned short msglen)
{
    t_msg_block *pbmsg;
    t_qcb *pqcb;
    unsigned long qindex;

    if(msg == NULL)
    {
        printf("sys_msgque_send(),msg == NULL \n");
        return (M_RET_FAIL);
    }

    qindex = qid - 1;

    if(qindex >= OS_WRAP_MAX_QUEUES)
    {
        printf("sys_msgque_send();qid error\n");
        return (M_RET_FAIL);
    }

    pqcb = &(q_data.qcb_array[qindex]);

    pbmsg = (t_msg_block *)mem_malloc(sizeof(t_msg_block));     //Store message block

    if(pbmsg == NULL)
    {
        printf("sys_msgque_send(),mem_malloc pbmsg == NULL \n");
        return (M_RET_FAIL);
    }

    pbmsg->nextp = NULL;
    pbmsg->msg = msg;
    pbmsg->msglen = msglen;

    msgque_mutex_lock(&pqcb->qmutex);

    if(pqcb->lastp != NULL)
    {
        pqcb->lastp->nextp = pbmsg;
        pqcb->lastp = pbmsg;
    }
    else
    {
        pqcb->firstp = pqcb->lastp = pbmsg;
    }

    msgque_sem_release(pqcb->semid);

    msgque_mutex_unlock(&pqcb->qmutex);

    return (M_RET_OK);

}


/** 
 *	Receive a message from queue
 *  Args:
 *  qid: queue id
 *  msg: pointer of message pointer
 *  msglen: length of message
 *  Return value:
 *  Failed: M_RET_FAIL
 *  Success: M_RET_OK
 *
 */


int msgque_recv(unsigned long qid,void **msg,unsigned short *msglen)
{
    t_msg_block *pbmsg;
    t_qcb *pqcb;
    unsigned long qindex;

    pbmsg = NULL;
    qindex = qid - 1;

    if(qindex >= OS_WRAP_MAX_QUEUES)
    {
        printf("msgque_recv(); qindex >= limit \n");
        return (M_RET_FAIL);
    }

    pqcb = &q_data.qcb_array[qindex];

    if(msgque_sem_wait(pqcb->semid) != M_RET_OK)
    {
        printf("msgque_recv(); msgque_sem_wait () failed \n");
        return (M_RET_FAIL);
    }

    msgque_mutex_lock(&pqcb->qmutex);

    pbmsg = pqcb->firstp;

    if(pbmsg == NULL)
    {
        printf("msgque_recv(); pbmsg == NULL \n");
        msgque_mutex_unlock(&pqcb->qmutex);
        return (M_RET_FAIL);
    }

    *msg = pbmsg->msg;
    *msglen = pbmsg->msglen;

    if(pqcb->lastp == pbmsg)
    {
        pqcb->lastp = NULL;
    }
    pqcb->firstp = pbmsg->nextp;

    mem_free(pbmsg);

    msgque_mutex_unlock(&pqcb->qmutex);

    return (M_RET_OK);

}


/** 
 *  MSGQUE:
 *	message queue init function.
 *  
 *  Success: M_RET_OK
 *  Failure: M_RET_FAIL
 */
int msgque_init()
{

    /*Mutex and lock init*/

    if(pthread_mutex_init(&q_data.g_mutex,NULL) != 0)
    {
        printf("msgque_init(), msgque_mutex_create(q_data.g_mutex) failed !\n");
        return (M_RET_FAIL);
    }

    if(pthread_mutex_init(&q_data.g_que_mutex,NULL) != 0)
    {
        printf("msgque_init(), msgque_mutex_create(que_mutex) failed !\n");
        return (M_RET_FAIL);
    }


    if(pthread_mutex_init(&q_data.g_sem_mutex,NULL))
    {
        printf("msgque_init(), msgque_mutex_create(sem_mutex) failed !\n");
        return (M_RET_FAIL);
    }

    /*Clean the queue counters*/
    q_data.que_num = 0;
    q_data.semnum = 0;

    return M_RET_OK;

}




