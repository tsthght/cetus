#ifndef __GENERATOR_DAEMON__
#define __GENERATOR_DAEMON__

#include <glib.h>

void daemonize();
gint keepalive(int *child_exit_status, const char *pid_file);

#endif
