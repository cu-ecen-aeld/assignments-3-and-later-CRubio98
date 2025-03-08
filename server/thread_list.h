#ifndef THREAD_LIST_H
#define THREAD_LIST_H

#include <pthread.h>
#include <sys/queue.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "socketclient.h"

typedef enum
{
  LIST_EMPTY,
  SRCH_NOT_FOUND,
  SRCH_FOUND
}eSearchState;

typedef enum
{
  eThreadNotFinish,
  eThreadFinish
}eThreadState;

struct thread_data
{
  socketclient_t* client;
  pthread_t thread_id;
  bool complete;
};typedef struct thread_data thread_data_t;


/*-------------------------------------------------*/
/*-----------Functions for Linked List-------------*/
/*-------------------------------------------------*/

/**
 * @brief Initialize LinkedList of thread data
 *
 */
void threadList_init(void);

/**
 * @brief Destroy all the nodes in the LinkedList
 *
 */
void threadList_dtor(void);

/**
 * @brief Insert a new node in the LinkedList that contains
 * a pointer to a dinamically allocated thread data
 * @param data Thread data to be inserted
 */
void threadList_insert(thread_data_t* data);

/**
 * @brief Remove a node from the LinkedList
 * @param pos Position of the node to be removed
 * @return true if the node was removed, false otherwise
 */
bool threadList_removeAt(int pos);

/**
 * @brief Search for a thread_data with a specific state
 * @param state State to be searched
 * @param pos Position of the node found
 * @return eSearchState
 */
eSearchState threadList_searchState(bool state, int* pos);

/**
 * @brief Get the thread data at a specific position
 * @param position Position of the node to be retrieved
 * @param data Pointer to the  dynamic allocated thread_data_t pointer
 * @return true if the node was retrieved, false otherwise
 */
bool threadList_getAt(int position,thread_data_t** data);

#endif //THREAD_LIST_H
