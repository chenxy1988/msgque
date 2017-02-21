#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msgque.h"


const char *str="THIS IS A STRING TEST....:)";

int main()
{
	unsigned long ppcsend_id;
	unsigned char *msg ;
	unsigned short length;

    if((ppcsend_id = msgque_create()) == M_RET_FAIL)
    {
        printf("sys_msgque_init(); msg_ppc_send init failed!\n");
    }  

	msg = (unsigned char *) malloc(100);
	if(!msg)
		return -1;
	memset(msg,0x0,100);
//	int msgque_send(unsigned long qid,void *msg,unsigned short msglen)
	msgque_send(ppcsend_id,(void *)str,strlen(str));
    if(msgque_recv(ppcsend_id,(void *)&msg,&length) != M_RET_FAIL){
		printf("recv %d bytes from queue :%s\n",length,msg);
		
	}

	return 0;

}
