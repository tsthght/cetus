#ifndef _CHASSIS_AUDIT_LOG_H
#define _CHASSIS_AUDIT_LOG_H

#include "chassis-rfifo.h"
#include "network-mysqld.h"
#include "cetus-util.h"

#define AUDIT_LOG_BUFFER_DEF_SIZE 1024*1024*10
#define AUDIT_LOG_DEF_FILE_NAME "audit"
#define AUDIT_LOG_DEF_SUFFIX "log"
#define AUDIT_LOG_DEF_PATH "/var/log/"
#define AUDIT_LOG_DEF_IDLETIME 500

typedef enum {
    AUDIT_LOG_OFF,
    AUDIT_LOG_ON,
    AUDIT_LOG_REALTIME
} AUDIT_LOG_SWITCH;

typedef enum {
    LOGIN
} AUDIT_LOG_MODE;

typedef enum {
    AUDIT_LOG_UNKNOWN,
    AUDIT_LOG_START,
    AUDIT_LOG_STOP
} AUDIT_LOG_ACTION;

struct audit_log_mgr {
    guint audit_log_bufsize;
    AUDIT_LOG_SWITCH audit_log_switch;
    AUDIT_LOG_MODE audit_log_mode;
    guint audit_log_maxsize;
    gulong audit_log_cursize;
    volatile AUDIT_LOG_ACTION audit_log_action;

    volatile guint audit_log_idletime;
    volatile guint audit_log_maxnum;

    gchar *audit_log_filename;
    gchar *audit_log_path;
    GThread *thread;
    FILE *audit_log_fp;
    gchar *audit_log_fullname;
    struct rfifo *fifo;
    GQueue *audit_log_filelist;
};

struct audit_log_mgr *audit_log_alloc();
void audit_log_free(struct audit_log_mgr *mgr);

gpointer audit_log_mainloop(gpointer user_data);
void cetus_audit_log_start_thread_once(struct audit_log_mgr *mgr);
void audit_log_thread_start(struct audit_log_mgr *mgr);

void audit_sql_login(network_mysqld_con *con);
#endif
