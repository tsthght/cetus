#ifndef __GENERATOR_DAEMON_H__
#define __GENERATOR_DAEMON_H__

#include <glib.h>

void daemonize();
gint keepalive(int *child_exit_status, const char *pid_file);

#endif
