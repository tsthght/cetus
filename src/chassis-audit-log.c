#include "chassis-audit-log.h"
#include "network-mysqld-packet.h"
#include <sys/stat.h>
//#include <sys/types.h>
//#include<unistd.h>

static void get_current_time_str(GString *str) {
    if (!str) {
        g_critical("str is NULL when call get_current_time_str()");
        return;
    }
    struct tm *tm;
    GTimeVal tv;
    time_t  t;

    g_get_current_time(&tv);
    t = (time_t) tv.tv_sec;
    tm = localtime(&t);

    str->len = strftime(str->str, str->allocated_len, "%Y-%m-%d %H:%M:%S", tm);
    g_string_append_printf(str, ".%.3d", (int) tv.tv_usec/1000);
}

static guint rfifo_flush(struct audit_log_mgr *mgr) {
    if (!mgr) {
        g_critical("struct mgr is NULL when call rfifo_flush()");
        return -1;
    }

    struct rfifo *fifo = mgr->fifo;
    if (!fifo) {
        g_critical("struct fifo is NULL when call rfifo_flush()");
        return -1;
    }
    if (!mgr->audit_log_fp) {
        g_critical("audit_log_fp is NULL when call rfifo_flush()");
        return -1;
    }

    guint len = fifo->in - fifo->out;
    guint l = min(len, fifo->size - (fifo->out & (fifo->size -1)));
    gint fd = fileno(mgr->audit_log_fp);
    guint s1 = pwrite(fd, fifo->buffer + (fifo->out & (fifo->size - 1)),
            l, (off_t)mgr->audit_log_cursize);
    mgr->audit_log_cursize += s1;
    fifo->out += s1;
    guint s2 = pwrite(fd, fifo->buffer, len -l, (off_t)mgr->audit_log_cursize);
    mgr->audit_log_cursize += s2;
    fifo->out += s2;
    if(mgr->audit_log_switch == AUDIT_LOG_REALTIME) {
        fsync(fd);
    }
    return (s1 + s2);
}

struct audit_log_mgr *audit_log_alloc() {
    struct audit_log_mgr *mgr = (struct audit_log_mgr *)g_malloc0(sizeof(struct audit_log_mgr));

    if(!mgr) {
        return (struct audit_log_mgr *)NULL;
    }
    mgr->audit_log_fp = NULL;
    mgr->audit_log_filename = NULL;
    mgr->audit_log_path = NULL;
    mgr->audit_log_bufsize = AUDIT_LOG_BUFFER_DEF_SIZE;
    mgr->audit_log_mode = LOGIN;
    mgr->audit_log_switch = AUDIT_LOG_OFF;
    mgr->audit_log_cursize = 0;
    mgr->audit_log_maxsize = 1024;
    mgr->audit_log_fullname = NULL;
    mgr->audit_log_idletime = AUDIT_LOG_DEF_IDLETIME;
    mgr->audit_log_action = AUDIT_LOG_STOP;
    mgr->fifo = NULL;
    mgr->audit_log_filelist = NULL;
    mgr->audit_log_maxnum = 3;
    return mgr;
}

static void free_queue_manually(GQueue *queue, GDestroyNotify free_func) {
    if (!queue || !free_func) return;
    gint num = g_queue_get_length(queue);
    while(num) {
        gchar *fn = (gchar *)g_queue_pop_head(queue);
        free_func(fn);
        num --;
    }
    g_queue_free(queue);
}

void audit_log_free(struct audit_log_mgr *mgr) {
    if (!mgr) return;

    g_free(mgr->audit_log_filename);
    g_free(mgr->audit_log_path);

    g_free(mgr->audit_log_fullname);

    rfifo_free(mgr->fifo);

    free_queue_manually(mgr->audit_log_filelist, g_free);

    mgr->audit_log_filelist = NULL;

    g_free(mgr);
}

static void audit_log_check_filenum(struct audit_log_mgr *mgr, gchar *filename) {
    if (!mgr || !filename) {
        g_warning("struct mgr or filename is NULL when call audit_log_check_filenum()");
        return;
    }
    if (!mgr->audit_log_maxnum) {
        return;
    }
    if (!mgr->audit_log_filelist) {
        mgr->audit_log_filelist = g_queue_new();
    } else {
        GList *pos = g_queue_find_custom(mgr->audit_log_filelist, filename, (GCompareFunc)strcmp);
        if (pos) {
            g_warning("file %s is exist", filename);
            return;
        }
    }
    g_queue_push_tail(mgr->audit_log_filelist, g_strdup(filename));

    gint num = g_queue_get_length(mgr->audit_log_filelist) - mgr->audit_log_maxnum;
    while (num > 0) {
        gchar *fn = (gchar *)g_queue_pop_head(mgr->audit_log_filelist);
        if (unlink(fn) != 0) {
            g_warning("rm log file %s failed", fn);
        } else {
            g_message("rm log file %s success", fn);
        }
        g_free(fn);
        num --;
    }
    return ;
}

static void audit_log_check_rotate(struct audit_log_mgr *mgr) {
    if (!mgr) return ;
    if (mgr->audit_log_maxsize == 0) return;
    if (mgr->audit_log_cursize < ((gulong)mgr->audit_log_maxsize) * MB) return ;

    time_t t = time(NULL);
    struct tm cur_tm;
    localtime_r(&t, &cur_tm);

    gchar *rotate_filename = g_strdup_printf("%s/%s_%04d%02d%02d%02d%02d%02d.%s",
            mgr->audit_log_path, mgr->audit_log_filename,
            cur_tm.tm_year + 1900, cur_tm.tm_mon + 1, cur_tm.tm_mday, cur_tm.tm_hour,
            cur_tm.tm_min, cur_tm.tm_sec, AUDIT_LOG_DEF_SUFFIX);
    if (!rotate_filename) {
        g_critical("can not get the rotate filename");
        return ;
    }
    if (mgr->audit_log_fp) {
        fclose(mgr->audit_log_fp);
        mgr->audit_log_fp = NULL;
    }
    if (rename(mgr->audit_log_fullname, rotate_filename) != 0) {
        g_critical("rename log file name to %s failed", rotate_filename);
    } else {
        audit_log_check_filenum(mgr, rotate_filename);
    }
    g_free(rotate_filename);
    mgr->audit_log_cursize = 0;
    mgr->audit_log_fp = fopen(mgr->audit_log_fullname, "a");
    if(!mgr->audit_log_fp) {
        g_critical("rotate error, because fopen(%s) failed", mgr->audit_log_fullname);
    }
}

 gpointer audit_log_mainloop(gpointer user_data) {
    struct audit_log_mgr *mgr = (struct audit_log_mgr *)user_data;
    struct stat st;
    mgr->audit_log_fp = fopen(mgr->audit_log_fullname, "a");

    if (!mgr->audit_log_fp) {
        g_critical("audit log thread exit, because fopen(%s) failed", mgr->audit_log_fullname);
        mgr->audit_log_action = AUDIT_LOG_STOP;
        return NULL;
    }
    fstat(fileno(mgr->audit_log_fp), &st);
    mgr->audit_log_cursize = st.st_size;
    g_message("audit log thread started");
    mgr->audit_log_action = AUDIT_LOG_START;

    while(!chassis_is_shutdown()) {
        guint len = rfifo_flush(mgr);
        if (!len) {
            usleep(mgr->audit_log_idletime);
        } else {
            audit_log_check_rotate(mgr);
        }
        if (mgr->audit_log_action != AUDIT_LOG_START) {
            goto audit_log_exit;
        }
    }
audit_log_exit:
    fclose(mgr->audit_log_fp);
    g_message("audit log thread stopped");
    mgr->audit_log_cursize = 0;
    mgr->audit_log_action = AUDIT_LOG_STOP;
    return NULL;
}

void
audit_log_thread_start(struct audit_log_mgr *mgr) {
    if (!mgr) return;
    GThread *audit_log_thread = NULL;
#if !GLIB_CHECK_VERSION(2, 32, 0)
    GError *error = NULL;
    audit_log_thread = g_thread_create(audit_log_mainloop, mgr, TRUE, &error);
    if (audit_log_thread == NULL && error != NULL) {
        g_critical("Create audit log thread error: %s", error->message);
    }
    if (error != NULL) {
        g_clear_error(&error);
    }
#else
    audit_log_thread = g_thread_new("audit_log-thread", audit_log_mainloop, mgr);
    if (audit_log_thread == NULL) {
        g_critical("Create audit log thread error.");
    }
#endif

    mgr->thread = audit_log_thread;
}

 void
 cetus_audit_log_start_thread_once(struct audit_log_mgr *mgr)
 {

     if (!mgr) {
         g_critical("audit_mgr is null!");
         return;
     }
     if (mgr->audit_log_bufsize == 0) {
         mgr->audit_log_bufsize = AUDIT_LOG_BUFFER_DEF_SIZE;
     }
     if (mgr->audit_log_filename == NULL) {
         mgr->audit_log_filename = g_strdup(AUDIT_LOG_DEF_FILE_NAME);
     }
     if (mgr->audit_log_path == NULL) {
         mgr->audit_log_path = g_strdup(AUDIT_LOG_DEF_PATH);
     }
     int result = access(mgr->audit_log_path, F_OK);
     if (result != 0) {
         g_message("audit log path is not exist, try to mkdir");
         result = mkdir(mgr->audit_log_path, 0660);
         if (result != 0) {
             g_message("mkdir(%s) failed", mgr->audit_log_path);
         }
     }
     if (mgr->audit_log_fullname == NULL) {
         mgr->audit_log_fullname = g_strdup_printf("%s/%s.%s", mgr->audit_log_path, mgr->audit_log_filename, AUDIT_LOG_DEF_SUFFIX);
     }
     mgr->fifo = rfifo_alloc(mgr->audit_log_bufsize);
     mgr->audit_log_filelist = g_queue_new();

     if (mgr->audit_log_switch == AUDIT_LOG_OFF) {
         g_message("audit thread is not start");
         return;
     }
     audit_log_thread_start(mgr);
 }

 void
 audit_sql_login(network_mysqld_con *con)
 {
     if (!con || !con->srv) {
         g_critical("con or con->srv is NULL when call audit_sql_connect()");
         return;
     }
     struct audit_log_mgr *mgr = con->srv->audit_mgr;
     if (!mgr) {
         g_critical("audit mgr is NULL when call audit_sql_connect()");
         return;
     }
     if (mgr->audit_log_switch == AUDIT_LOG_OFF ||
         !(mgr->audit_log_mode & LOGIN) ||
         (mgr->audit_log_action != AUDIT_LOG_START)) {
         return;
     }
     GString *message = g_string_sized_new(sizeof("2004-01-01T00:00:00.000Z"));
     get_current_time_str(message);
     g_string_append_printf(message, ": #login# %s@%s Connect Cetus, C_id:%u C_db:%s C_charset:%u C_auth_plugin:%s C_ssl:%s C_cap:%x S_cap:%x\n",
                                              con->client->response->username->str,//C_usr
                                              con->client->src->name->str,//C_ip
                                              con->client->challenge->thread_id,//C_id
                                              con->client->response->database->str,//C_db
                                              con->client->response->charset,//C_charset
                                              con->client->response->auth_plugin_name->str,
                                              con->client->response->ssl_request == 1 ? "true" : "false",
                                              con->client->response->client_capabilities,
                                              con->client->response->server_capabilities
                                              );

     rfifo_write(mgr->fifo, message->str, message->len);
     g_string_free(message, TRUE);
 }
