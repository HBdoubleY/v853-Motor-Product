/* $Id: queue.c 184 2020-08-16 11:48:52Z baizhijing $ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"
//#include "airplay_main.h"

#ifdef __cplusplus
extern "C" {
#endif


Queue *create_queue()
{
	Queue *queue = (Queue *)malloc(sizeof(Queue));
	if(queue != NULL)
	{
		memset(queue, 0, sizeof(Queue));
		queue->queue.front = queue->queue.rear = NULL;
		pthread_mutex_init(&queue->mutex, NULL);
	}

	return queue;
}

void destroy_queue(Queue *queue)
{
	if(queue != NULL)
	{
		pthread_mutex_destroy(&queue->mutex);
		free(queue);
		queue = NULL;
	}
}

int queue_size(Queue *queue)
{
	if(queue != NULL)
	{
		int len = 0;
		pthread_mutex_lock(&queue->mutex);
		PNode q = queue->queue.front;
		
		while(q != NULL)
		{
			len ++;
			q = q->next;
		}
		pthread_mutex_unlock(&queue->mutex);
		return len;
	}

	return 0;
}

int queue_empty(Queue *queue)
{
	if(queue != NULL)
		return (NULL == queue->queue.front) ? 1 : 0;
	return -1;
}

void queue_push(Queue *queue, void *data)
{
	if(queue != NULL)
	{
		pthread_mutex_lock(&queue->mutex);

		PNode q = (PNode)malloc(sizeof(Node));
		if(q != NULL)
		{
			memset(q, 0, sizeof(Node));

			q->data = data;
			q->next = NULL;
			QueueData *picdata = (QueueData *)data;

			if(queue->queue.rear != NULL)
			{
				queue->queue.rear->next = q;
			}

			queue->queue.rear = q;

			if(NULL == queue->queue.front)
				queue->queue.front = q;
		}
		pthread_mutex_unlock(&queue->mutex);
	}
}

void *queue_front(Queue *queue)
{
	if(queue != NULL)
	{
		pthread_mutex_lock(&queue->mutex);
		void *d = NULL;

		if(0 == queue_empty(queue))
			d = queue->queue.front->data;

		pthread_mutex_unlock(&queue->mutex);
		return d;
	}

	return NULL;
}

int queue_pop(Queue *queue)
{
	int ret = -1;
	if(queue != NULL)
	{
		pthread_mutex_lock(&queue->mutex);
		PNode q = NULL;

		if(0 == queue_empty(queue))
		{
			q = queue->queue.front;
			queue->queue.front = q->next;
			
			if(queue->queue.front == NULL)
				queue->queue.front = queue->queue.rear = NULL;
			free(q);
			ret = 0;
		}
		
		pthread_mutex_unlock(&queue->mutex);
	}
	return ret;
}


#if 1
Queue* gUiQueue = NULL;

void app_queue_create()
{
	gUiQueue = create_queue();
}

void app_queue_push(int type, unsigned char *data, int len)
{
	QueueData* qd = (QueueData*)malloc(sizeof(QueueData)+len);
	if (qd){
		qd->type = type; 
		qd->size = len;
		memcpy(qd->data, data, len);
		queue_push(gUiQueue, qd);
	}
}

void app_queue_clear()
{
	int cont = queue_size(gUiQueue);
    while (cont)
    {
        QueueData *qd = (QueueData *)queue_front(gUiQueue);
        int ret = queue_pop(gUiQueue);
        if(ret == 0){
			free(qd);
		}

        cont--;
    }
}


#else

Queue* gUiQueue = NULL;

void app_queue_create()
{
	gUiQueue = create_queue();
}

void app_queue_clear()
{
	int cont = queue_size(gUiQueue);
    while (cont)
    {
        QueueData *qd = (QueueData *)queue_front(gUiQueue);
        int ret = queue_pop(gUiQueue);
        if(ret == 0){
			//free(qd);
		}

        cont--;
    }
}

#endif

#ifdef __cplusplus
}
#endif
/* End of file */
