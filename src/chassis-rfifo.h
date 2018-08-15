#ifndef _CHASSIS_RFIFO_H
#define _CHASSIS_RFIFO_H

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

struct rfifo {
    guchar *buffer;
    guint size;
    guint in;
    guint out;
};

struct rfifo *rfifo_alloc(guint size);
void rfifo_free(struct rfifo *fifo);
guint rfifo_write(struct rfifo *fifo, guchar *buffer, guint len);

#endif
