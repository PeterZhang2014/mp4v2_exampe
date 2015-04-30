/****************************************************************************************************
 * Copyright (C) 2015/04/29, Genvision
 *
 * File Name   : fifo_operation.c
 * File Mark   :
 * Description : 
 * Others      :
 * Verision    : 1.0
 * Date        : Apr 29, 2015
 * Author      : Peter Zhang
 * Email       : zfy31415926.love@163.com OR zhangfy@genvision.cn
 *
 * History 1   :
 *     Date    :
 *     Version :
 *     Author  :
 *     Modification:
 * History 2   : ...
 ****************************************************************************************************/
#include "fifo_operation.h"
#include <pthread.h>
#include <time.h>

//#define FIFO_DEBUG
#ifdef FIFO_DEBUG
#define print printf
#else
#define print(...)
#endif

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * creta and return a new queue
 **/
queue *fifo_init(int max_size)
{
	queue *new_queue = malloc(sizeof(queue));
	if(new_queue == NULL)
	{
		print("Malloc failed creating the queue\n");
		return NULL;
	}
	new_queue->max_size = max_size;
	new_queue->first = NULL;
	new_queue->last = NULL;
	new_queue->current_size = 0;
	print("Generated the que @ %p\n",new_queue);

	return new_queue;
}

void fifo_free(queue *que)
{
	print("Enterd to queue_destroy\n");

	if(que == NULL)
	{
		return;
	}

	print("que is not null, que = %p\n",que);

	pthread_mutex_lock(&mutex);
	if(que->first == NULL)
	{
		print("que->first == NULL...\n");
		free(que);
		pthread_mutex_unlock(&mutex);
		return;
	}

	print("que is there lets try to free it...\n");
	node *_node = que->first;
	while(_node != NULL)
	{
		print("freeing : %s\n",(char *)_node->data);
		free(_node->data);
		node *tmp = _node->next;
		free(_node);
		_node = tmp;
	}

	free(que);

	pthread_mutex_unlock(&mutex);

	return;
}

/**
 * que is a queue pointer 
 * data is a heap allocated memory pointer
 **/
int fifo_push(queue *que, void *data, int size)
{
	print("Enterd to fifo_push...\n");
	node *new_node = malloc(sizeof(node));
	if(new_node == NULL)
	{
		print("Malloc failed createing a node\n");
		return -1;
	}
	// assumming data is in heap
	new_node->data = data;
	new_node->size = size;
	pthread_mutex_lock(&mutex);
	if (que->first == NULL)
	{
		que->first = new_node;
		que->last = new_node;
	}
	else
	{
		que->last->next = new_node;
		que->last = new_node;
	}
	que->current_size ++;
	pthread_mutex_unlock(&mutex);
	print("fifo_push() : push a node\n");

	return 0;
}

int fifo_full(queue *que)
{
	return (que->max_size <= que->current_size);
}

int fifo_empty(queue *que)
{
	return (que->current_size > 0?0:1);
}

void *fifo_pop(queue *que, int *datalen)
{
	print("Enterd to fifo_pop\n");
	if(que == NULL)
	{
		print("que is null exiting...");
		return NULL;
	}

	pthread_mutex_lock(&mutex);
	if (que->first == NULL)
	{
		pthread_mutex_unlock(&mutex);
		print("que->first is NULL exiting...\n");
		return NULL;
	}

	void *data;
	node *_node = que->first;
	if (que->first == que->last)
	{
		que->first = NULL;
		que->last = NULL;
	}
	else
	{
		que->first = _node->next;
	}

	data = _node->data;
	*datalen = _node->size;
	print("Freeing _node@ %p\n",_node);
	free(_node);
	que->current_size --;
	pthread_mutex_unlock(&mutex);
	print("fifo_pop() : pop the first node\n");
	
	return data;
}
