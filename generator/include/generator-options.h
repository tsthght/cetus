#ifndef __GENERATOR_OPTIONS_H__
#define __GENERATOR_OPTIONS_H__

#include <glib.h>

typedef gint (*generator_opt_assign_hook)(const gchar *newval, gpointer param);
typedef gchar* (*generator_opt_show_hook)(gpointer param);

typedef enum opt_errcode {
    OPT_SUCCESS = 0,
    OPT_ARG_INVALID
} opt_errcode_t;

typedef enum opt_property{
    ASSIGN_OPTS_PROPERTY = 0x1,
    SHOW_OPTS_PROPERTY = 0x02,
    SAVE_OPTS_PROPERTY = 0x04,
    ALL_OPTS_PROPERTY = 0x07
}opt_property_t;

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
                                             gchar **argv,
                                             GError **error);
#endif
