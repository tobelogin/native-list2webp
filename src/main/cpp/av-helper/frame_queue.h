#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H
#include <libavcodec/avcodec.h>

struct FrameNode;

typedef struct FrameNode FrameNode;

typedef struct FrameQueue FrameQueue;

/**
* @return 成功返回队列头部地址，否则返回 NULL
*/
FrameQueue *framequeue_alloc();

/**
* @return 成功返回0,无法分配内存时返回 -1
*/
int framequeue_add(FrameQueue *fq, AVFrame *frame);

AVFrame *framequeue_pop(FrameQueue *fq);

void framequeue_free(FrameQueue *fq);

#endif // FRAME_QUEUE_H