#include "generator-parameters.h"

generator_parameters_t *generator_parameters_new() {
    generator_parameters_t *param = g_slice_new0(generator_parameters_t);
    if (!param) return NULL;
    param->daemon = 0;
    param->keepalive = 0;
    return param;
}

void generator_parameters_free(generator_parameters_t *param) {
    if (!param) return;
    g_slice_free(generator_parameters_t, param);
}

gint generator_parameters_set_options(generator_parameters_t *param,
                                      generator_options_t *opts) {
    generator_options_set(opts,
                          "daemon", 0, 0,
                          OPTION_ARG_NONE, &(param->daemon),
                          "daemon mode", NULL,
                          NULL, show_daemon_mode, SHOW_OPTS_PROPERTY|SAVE_OPTS_PROPERTY);
    generator_options_set(opts,
                          "keepalive", 0, 0,
                          OPTION_ARG_NONE, &(param->keepalive),
                          "keepalive mode", NULL,
                          NULL, NULL, SHOW_OPTS_PROPERTY|SAVE_OPTS_PROPERTY);
    generator_options_set(opts,
                          "master-host", 0, 0,
                          OPTION_ARG_STRING, &(param->master_host),
                          "master host", "<string>",
                          NULL, NULL, SHOW_OPTS_PROPERTY|SAVE_OPTS_PROPERTY);
    return 0;
}

gchar* show_daemon_mode(gpointer p) {
    external_param_t *opt_param = (external_param_t *)p;
    generator_parameters_t *param = opt_param->param;
    gint opt_type = opt_param->opt_type;
    if (CAN_SHOW_OPTS_PROPERTY(opt_type)) {
        return g_strdup_printf("%s", param->daemon ? "true" : "false");
    }
    return NULL;
}
