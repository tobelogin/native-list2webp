#include <stdlib.h>

#include "frame_queue.h"

struct FrameNode
{
    AVFrame *frame;
    struct FrameNode *next;
};

struct FrameQueue
{
    struct FrameNode *head;
    struct FrameNode *tail;
};

FrameQueue *framequeue_alloc()
{
    FrameQueue *q = (FrameQueue*)malloc(sizeof(FrameQueue));
    if(q == NULL)
    {
        fprintf(stderr, "Unable to allocate frame queue.\n");
        return NULL;
    }
    q->head = NULL;
    q->tail = NULL;
    return q;
}

int framequeue_add(FrameQueue *fq, AVFrame *frame)
{
    FrameNode *node = (FrameNode*)malloc(sizeof(FrameNode));
    if(node == NULL)
    {
        fprintf(stderr, "Unable to allocate frame node.\n");
        return -1;
    }
    node->frame = frame;
    node->next = NULL;
    if (fq->head == NULL)
    {
        fq->head = node;
        fq->tail = node;
    }
    else
    {
        node->next = fq->tail->next;
        fq->tail->next = node;
        fq->tail = node;
    }
    return 0;
}

AVFrame *framequeue_pop(FrameQueue *fq)
{
    if(fq->head != NULL)
    {
        AVFrame *frame = fq->head->frame;
        FrameNode *del = fq->head;
        if (fq->head == fq->tail)
        {
            fq->head = NULL;
            fq->tail = NULL;
        }
        else
        {
            fq->head = fq->head->next;
        }
        free(del);
        return frame;
    }
    return NULL;
}

void framequeue_free(FrameQueue *fq)
{
    FrameNode *del = fq->head;
    while(del != NULL)
    {
        fq->head = del->next;
        free(del);
        del = fq->head;
    }
    free(del);
    free(fq);
}