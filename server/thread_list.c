#include <syslog.h>
#include "thread_list.h"


struct thread_node
{
  thread_data_t* data;
  SLIST_ENTRY(thread_node) next; //next node element
};typedef struct thread_node thread_node_t;

static SLIST_HEAD(slist_head, thread_node) thread_list; //LIST OF THREADS


static struct thread_node* new_node(thread_data_t* data);
static void node_dtor(thread_node_t* this);

/*-------------------------------------------------*/
/*------Internal Node Functions for Linked List----*/
/*-------------------------------------------------*/
static struct thread_node* new_node(thread_data_t* data)
{
    struct thread_node* new_thread_node = (struct thread_node*) malloc(sizeof(struct thread_node));
    if(new_thread_node != NULL)
    {
        new_thread_node->data = data;
        new_thread_node->next.sle_next=NULL;
    }
    return new_thread_node;

}

static void node_dtor(thread_node_t* this)
{
    if(!this){return;}
    free(this);
}

/*-------------------------------------------------*/
/*-----------Functions for Linked List-------------*/
/*-------------------------------------------------*/

void threadList_init(void)
{

    SLIST_INIT(&thread_list);

}

void threadList_dtor(void)
{
    thread_node_t* head_thread;
    while(!SLIST_EMPTY(&thread_list))
    {
        head_thread = SLIST_FIRST(&thread_list);

        SLIST_REMOVE_HEAD(&thread_list, next);
        node_dtor(head_thread);
    }
}

void threadList_insert(thread_data_t* data)
{
    thread_node_t* node = new_node(data);
    if (SLIST_EMPTY(&thread_list))
    {
        SLIST_INSERT_HEAD(&thread_list, node, next);
    }
    else
    {
        thread_node_t* current_node;
        SLIST_FOREACH(current_node, &thread_list, next)
        {
            if(SLIST_NEXT(current_node, next) == NULL)
            {
                SLIST_INSERT_AFTER(current_node, node, next);
                break;
            }
        }
    }
}

bool threadList_removeAt(int pos)
{
    if(SLIST_EMPTY(&thread_list))
    {
        return false;
    }

    thread_node_t* tmp_node = SLIST_FIRST(&thread_list);
    if (pos == 0)
    {
        SLIST_REMOVE_HEAD(&thread_list, next);
        node_dtor(tmp_node);
        return true;
    }

    for (int i = 0; tmp_node != NULL && i < pos-1; i++) {
        tmp_node=SLIST_NEXT(tmp_node, next);
    }

    if (tmp_node == NULL || SLIST_NEXT(tmp_node, next) == NULL)
    {
        return false;
    }

    thread_node_t* next_node = SLIST_NEXT(SLIST_NEXT(tmp_node, next), next);
    node_dtor(SLIST_NEXT(tmp_node, next));
    tmp_node->next.sle_next = next_node;
    return true;
}

eSearchState threadList_searchState(bool state, int* pos)
{
    int index=0;

    if(SLIST_EMPTY(&thread_list))
    {
        return LIST_EMPTY;
    }

    thread_node_t* current_node;
    SLIST_FOREACH(current_node, &thread_list, next)
    {
        if(current_node->data->complete == state)
        {
            *pos=index;
            return SRCH_FOUND;
        }
        index++;
    }
    return SRCH_NOT_FOUND;
}

bool threadList_getAt(int position,thread_data_t** data)
{

    if(SLIST_EMPTY(&thread_list))
    {
        return false;
    }

    thread_node_t* current_node= SLIST_FIRST(&thread_list);

        for (int i = 0; i < position; i++)
        {
            current_node=SLIST_NEXT(current_node, next);
            if (current_node == NULL)
            {
                return false;
            }
        }

    *data=current_node->data;
    return true;
}
