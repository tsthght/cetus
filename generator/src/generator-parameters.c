#include "generator-parameters.h"

generator_parameters_t *generator_parameters_new() {
    generator_parameters_t *param = g_slice_new0(generator_parameters_t);
    if (!param) return NULL;
    param->daemon = 1;
    param->keepalive = 1;
    return param;
}

void generator_parameters_free(generator_parameters_t *param) {
    if (!param) return;
    g_slice_free(generator_parameters_t, param);
}
