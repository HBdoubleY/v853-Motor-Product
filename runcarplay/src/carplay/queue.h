/* $Id: queue.h 190 2020-08-17 07:08:28Z yangbo $ */
#ifndef QUEUE_INC_
#define QUEUE_INC_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif
// pthread_mutex_t img_lock;
// char *rgb_img_data;
typedef struct
{
	int size;
	int type;
    int width;
    int height;
	// unsigned char data[0];
    char data[0];
}QueueData;

typedef struct _Node {
   void *data;
   struct _Node *next;
} Node, *PNode;

typedef struct _LinkQueue {
    PNode front;
    PNode rear;
} LinkQueue;

typedef struct _Queue {
	LinkQueue queue;
	pthread_mutex_t mutex;
} Queue;

Queue *create_queue();
void destroy_queue(Queue *queue);
int queue_size(Queue *queue);
int queue_empty(Queue *queue);
void queue_push(Queue *queue, void *data);
void *queue_front(Queue *queue);
int queue_pop(Queue *queue);
void app_queue_create();
#ifdef __cplusplus
}
#endif

#endif /* QUEUE_INC_ */
/* End of file */
