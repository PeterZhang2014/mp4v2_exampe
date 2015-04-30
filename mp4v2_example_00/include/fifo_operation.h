/****************************************************************************************************
 * Copyright (C) 2015/04/29, Genvision
 *
 * File Name   : fifo_operation.h
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
#ifndef __FIFO_OPERATION_H__
#define __FIFO_OPERATION_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fifo_node{
	void *data;
	int size;
	struct fifo_node *next;
};
typedef struct fifo_node node;
struct fifo_queue{
	int max_size;
	int current_size;
	node *first;
	node *last;
};
typedef struct fifo_queue queue;

//create and return the queue
queue *fifo_init(int max_size);

//destory the queue (free all the memory associate with the queue
//even the data)
void fifo_free(queue *que);

//enque the data into queue
int fifo_push(queue *que, void *data, int size);

//return the data from the queue
//and free up all the internally allocated memory
//but user have to free the returning data pointer
void *fifo_pop(queue *que, int *datalen);

//If que is full return 1, else return 0
int fifo_full(queue *que);

//If que is empty return 1, else return 0
int fifo_empty(queue *que);

#endif/*ifndef __FIFO_OPERATION_H__*/
