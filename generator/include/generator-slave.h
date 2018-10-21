#ifndef __GENERATOR_SLAVE_H__
#define __GENERATOR_SLAVE_H__
#include <mysql.h>
#include <glib.h>

typedef struct generator_master_info {
    MYSQL *mysql;
    guint version;
}generator_master_info_t;

generator_master_info_t* generator_master_info_new();
gpointer generator_master_info_free();

gboolean check_master_version(generator_master_info_t *master_info);

gboolean slave_init();

#endif
