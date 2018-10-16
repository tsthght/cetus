#include <stdio.h>
#include <stdlib.h>
#include "generator-daemon.h"
#include "generator-options.h"
#include "generator-parameters.h"

gint main(gint argc, gchar **argv) {
    generator_parameters_t *param = generator_parameters_new();
    if (!param) goto gen_exit;
    generator_options_t *opts = generator_options_new();
    if (!opts) goto gen_exit;
    generator_parameters_set_options(param, opts);
    GError *gerr = NULL;
    if (FALSE == generator_options_parse_cmdline(opts, &argc, &argv, &gerr)) {
        goto gen_exit;
    }
    if (param->daemon) {
        //daemonize();
        printf("## daemon\n");
    }
    if (param->keepalive) {
        printf("## keepalive\n");
    }
    gint child_exit_status = EXIT_SUCCESS;
    gint ret = keepalive(&child_exit_status, NULL);
    if (ret > 0) {
        return 0;
    } else if (ret < 0) {
        return 0;
    } else {

    }
    printf("generator driver\n");
    sleep(1000);
gen_exit:
    generator_parameters_free(param);
    generator_options_free(opts);
    return 0;
}
