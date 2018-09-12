#include <stdlib.h>
#include <signal.h>
#include "generator-daemon.h"

void daemonize()
{
#ifdef SIGTTOU
    signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
    signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
#endif
    if (fork() != 0)
        exit(0);

    if (setsid() == -1)
        exit(0);

    signal(SIGHUP, SIG_IGN);

    if (fork() != 0)
        exit(0);

    umask(0);
}
