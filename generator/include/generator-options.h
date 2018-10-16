#ifndef __GENERATOR_OPTIONS_H__
#define __GENERATOR_OPTIONS_H__

#include <glib.h>

#define OPTIONAL_ARG(entry) FALSE
#define NO_ARG(entry) ((entry)->arg == OPTION_ARG_NONE)

typedef gint (*generator_opt_assign_hook)(const gchar *newval, gpointer param);
typedef gchar* (*generator_opt_show_hook)(gpointer param);

typedef enum opt_errcode {
    OPT_SUCCESS = 0,
    OPT_ARG_INVALID,
    OPT_FAILED
} opt_errcode_t;

typedef enum opt_property{
    ASSIGN_OPTS_PROPERTY = 0x1,
    SHOW_OPTS_PROPERTY = 0x02,
    SAVE_OPTS_PROPERTY = 0x04,
    ALL_OPTS_PROPERTY = 0x07
}opt_property_t;

#define SHOW_SAVE_OPTS_PROPERTY (SHOW_OPTS_PROsPERTY|SAVE_OPTS_PROPERTY)
#define ALL_OPTS_PROPERTY (ASSIGN_OPTS_PROPERTY|SHOW_OPTS_PROPERTY|SAVE_OPTS_PROPERTY)

#define CAN_ASSIGN_OPTS_PROPERTY(opt_property) ((opt_property) & ASSIGN_OPTS_PROPERTY)
#define CAN_SHOW_OPTS_PROPERTY(opt_property) ((opt_property) & SHOW_OPTS_PROPERTY)
#define CAN_SAVE_OPTS_PROPERTY(opt_property) ((opt_property) & SAVE_OPTS_PROPERTY)

enum option_type {              // arg_data type
    OPTION_ARG_NONE,            // bool *
    OPTION_ARG_INT,             // int *
    OPTION_ARG_INT64,           // int64 *
    OPTION_ARG_DOUBLE,          // double *
    OPTION_ARG_STRING,          // char **
    OPTION_ARG_STRING_ARRAY,    // char **
};

enum option_flags {
    OPTION_FLAG_REVERSE = 0x04,

    OPTION_FLAG_CMDLINE = 0x10,
    OPTION_FLAG_CONF_FILE = 0x20,
    OPTION_FLAG_REMOTE_CONF = 0x40,
};

typedef struct generator_option {
    const gchar *long_name;
    gchar short_name;
    gint flags;
    
    GOptionArg arg;
    gpointer arg_data;

    const gchar *description;
    const gchar *arg_description;

    generator_opt_assign_hook assign_hook;
    generator_opt_show_hook show_hook;
    opt_property_t property;
}generator_option_t;

generator_option_t *generator_option_new();
void generator_option_free(generator_option_t *opt);
opt_errcode_t generator_option_set(generator_option_t *opt,
                          const gchar *long_name,
                          gchar short_name,
                          gint flags,
                          GOptionArg arg,
                          gpointer arg_data,
                          const gchar *description,
                          const gchar *arg_description,
                          generator_opt_assign_hook assign_hook,
                          generator_opt_show_hook show_hook,
                          opt_property_t property);

typedef struct generator_options {
    GList *options;
    gboolean help_enabled;
    gboolean ignore_unknown;
    GList *changes;
    GList *pending_nulls;
}generator_options_t;

generator_options_t *generator_options_new();
void generator_options_free(generator_options_t *opts);
opt_errcode_t generator_options_set(generator_options_t *opts,
                           const gchar *long_name,
                           gchar short_name,
                           gint flags,
                           GOptionArg arg,
                           gpointer arg_data,
                           const gchar *description,
                           const gchar *arg_description,
                           generator_opt_assign_hook assign_hook,
                           generator_opt_show_hook show_hook,
                           opt_property_t property);
opt_errcode_t generator_options_parse_cmdline(generator_options_t *opts,
                                             gint *argc,
                                             gchar ***argv,
                                             GError **error);
#endif
