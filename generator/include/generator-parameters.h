#ifndef __GENERATOR_PARAMETERS_H__
#define __GENERATOR_PARAMETERS_H__

#include <glib.h>

typedef struct generator_parameters {
    gint daemon;
    gint keepalive;
}generator_parameters_t;

generator_parameters_t *generator_parameters_new();
void generator_parameters_free(generator_parameters_t *param);



#endif
