#ifndef THREAD_LIST_H
#define THREAD_LIST_H

#include <pthread.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "socketclient.h"

struct thread_data
{
  socketclient_t* client;
  pthread_t thread_id;
  bool complete;
};typedef struct thread_data thread_data_t;


/*-------------------------------------------------*/
/*-----------Functions for Linked List-------------*/
/*-------------------------------------------------*/

void threadList_init(void);

void threadList_dtor(void);

void threadList_insert(thread_data_t* data);

bool threadList_removeAt(int pos);

int threadList_searchState(bool state);

void threadList_getAt(int position,thread_data_t* data);

#endif //THREAD_LIST_H