#ifndef __GENERATOR_PARAMETERS_H__
#define __GENERATOR_PARAMETERS_H__

#include <glib.h>
#include "generator-options.h"

typedef struct generator_parameters {
    gint daemon;
    gint keepalive;
}generator_parameters_t;

typedef struct external_param {
    generator_parameters_t *param;
    gint opt_type;
} external_param_t;

generator_parameters_t *generator_parameters_new();
void generator_parameters_free(generator_parameters_t *param);

gint generator_parameters_set_options(generator_parameters_t *param, 
                                      generator_options_t *opts);

gchar* show_daemon_mode(gpointer p);
#endif
