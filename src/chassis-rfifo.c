#include "chassis-rfifo.h"
#include "cetus-util.h"

struct rfifo *rfifo_alloc(guint size) {
    struct rfifo *ret = (struct rfifo *)g_malloc0(sizeof(struct rfifo));;

    if (!ret) {
        return NULL;
    }

    if (size & (size - 1)) {
        size = roundup_pow_of_two(size);
    }
    ret->buffer = (unsigned char *)g_malloc0(size);
    if (!ret->buffer) {
        g_free(ret);
        return NULL;
    }
    ret->size = size;
    ret->in = ret->out = 0;
    return ret;
}

void rfifo_free(struct rfifo *fifo) {
    if (!fifo) return;
    g_free(fifo->buffer);
    g_free(fifo);
}

guint rfifo_write(struct rfifo *fifo, guchar *buffer, guint len) {
    if (!fifo) {
        g_critical("struct fifo is NULL when call rfifo_write()");
        return -1;
    }
    guint l;
    len = min(len, (fifo->size - fifo->in + fifo->out));
    l = min(len, fifo->size - (fifo->in & (fifo->size -1)));
    memcpy(fifo->buffer + (fifo->in & (fifo->size -1)), buffer, l);//g_strlcpy
    memcpy(fifo->buffer, buffer + l, len - l);
    fifo->in += len;
    return len;
}
