#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h> /* wait4 */
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h> /* getrusage */
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
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

/**
 * forward the signal to the process group, but not us
 */
static void signal_forward(int sig) {
    signal(sig, SIG_IGN); /* we don't want to create a loop here */
    kill(0, sig);
}

gint keepalive(int *child_exit_status, const char *pid_file) {
    int nprocs = 0;
    pid_t child_pid = -1;

    /* we ignore SIGINT and SIGTERM and just let it be forwarded to the child instead
     * as we want to collect its PID before we shutdown too
     *
     * the child will have to set its own signal handlers for this
     */

    for (;;) {
        /* try to start the children */
        while (nprocs < 1) {
            pid_t pid = fork();

            if (pid == 0) {
                /* child */
                return 0;
            } else if (pid < 0) {
                /* fork() failed */
                return -1;
            } else {
                /* we are the angel, let's see what the child did */
                /* forward a few signals that are sent to us to the child instead */
                signal(SIGINT, signal_forward);
                signal(SIGTERM, signal_forward);
                signal(SIGHUP, signal_forward);
                signal(SIGUSR1, signal_forward);
                signal(SIGUSR2, signal_forward);

                child_pid = pid;
                nprocs++;
            }
        }

        if (child_pid != -1) {
            struct rusage rusage;
            int exit_status;
            pid_t exit_pid;

#ifdef HAVE_WAIT4
            exit_pid = wait4(child_pid, &exit_status, 0, &rusage);
#else
            memset(&rusage, 0, sizeof(rusage)); /* make sure everything is zero'ed out */
            exit_pid = waitpid(child_pid, &exit_status, 0);
#endif
            if (exit_pid == child_pid) {

                /* delete pid file */
                if (pid_file) {
                    unlink(pid_file);
                }

                /* our child returned, let's see how it went */
                if (WIFEXITED(exit_status)) {
                    if (child_exit_status) *child_exit_status = WEXITSTATUS(exit_status);

                    return 1;
                } else if (WIFSIGNALED(exit_status)) {
                    int time_towait = 2;
                    /* our child died on a signal
                     *
                     * log it and restart */

                    /**
                     * to make sure we don't loop as fast as we can, sleep a bit between
                     * restarts
                     */

                    signal(SIGINT, SIG_DFL);
                    signal(SIGTERM, SIG_DFL);
                    signal(SIGHUP, SIG_DFL);
                    while (time_towait > 0) time_towait = sleep(time_towait);

                    nprocs--;
                    child_pid = -1;
                } else if (WIFSTOPPED(exit_status)) {
                } else {
                    g_assert_not_reached();
                }
            } else if (-1 == exit_pid) {
                /* EINTR is ok, all others bad */
                if (EINTR != errno) {
                    /* how can this happen ? */

                    return -1;
                }
            } else {
                g_assert_not_reached();
            }
        }
    }
}
