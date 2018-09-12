#include <stdio.h>
#include <stdlib.h>
#include "generator-daemon.h"
#include "generator-options-parse.h"

int main() {
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
    return 0;
}
