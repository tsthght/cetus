#include <stdio.h>
#include <stdlib.h>
#include "generator-daemon.h"
#include "generator-options.h"
#include "generator-parameters.h"

int main() {
    generator_parameters_t *param = generator_parameters_new();
    if (!param) goto gen_exit;
    daemonize();
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
    return 0;
}
