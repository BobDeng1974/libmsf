
#ifndef __MSF_RINGBUFF_H__
#define __MSF_RINGBUFF_H__

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ringbuffer {
    void *buffer;
    int length;
    size_t start;
    size_t end;
} ringbuffer;

struct ringbuffer *rb_create(int len);
void rb_destroy(struct ringbuffer *rb);
ssize_t rb_write(struct ringbuffer *rb, const void *buf, size_t len);
ssize_t rb_read(struct ringbuffer *rb, void *buf, size_t len);
void *rb_dump(struct ringbuffer *rb, size_t *len);
void rb_cleanup(struct ringbuffer *rb);

#ifdef __cplusplus
}
#endif

#endif
