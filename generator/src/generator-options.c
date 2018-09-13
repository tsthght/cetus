#include "generator-options.h"
#include <string.h>
#include <stdlib.h>

generator_option_t *generator_option_new() {
    generator_option_t *opt = g_slice_new0(generator_option_t);
    return opt;
}

void generator_option_free(generator_option_t *opt) {
    if (!opt) return;
    g_free((gchar *)opt->long_name);
    g_free((gchar *)opt->description);
    g_free((gchar *)opt->arg_description);
    g_slice_free(generator_option_t, opt);
}

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
                          opt_property_t property) {
    if (!opt) return OPT_ARG_INVALID;
    opt->long_name = long_name;
    opt->short_name = short_name;
    opt->flags = flags;
    opt->arg = arg;
    opt->arg_data = arg_data;
    opt->description = description;
    opt->arg_description = arg_description;
    opt->assign_hook = assign_hook;
    opt->show_hook = show_hook;
    opt->property = property;
    return OPT_SUCCESS;
}

generator_options_t *generator_options_new() {
    generator_options_t *opts = g_slice_new0(generator_options_t);
    return opts;
}

void generator_options_free(generator_options_t *opts) {
    if (!opts) return;
    g_list_free_full(opts->options, (GDestroyNotify)generator_option_free);
    g_slice_free(generator_options_t, opts);
}

static opt_errcode_t generator_options_add_option(generator_options_t *opts, 
                                  generator_option_t *opt) {
    if (!opts || !opt) return OPT_ARG_INVALID;
    opts->options = g_list_append(opts->options, opt);
    return OPT_SUCCESS;
}

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
                                  opt_property_t property) {
    generator_option_t *opt = generator_option_new();
    opt_errcode_t ret = generator_option_set(opt,
                                           long_name,
                                           short_name,
                                           flags,
                                           arg,
                                           arg_data,
                                           description,
                                           arg_description,
                                           assign_hook,
                                           show_hook,
                                           property);
    if (ret == OPT_SUCCESS) {
        ret = generator_options_add_option(opts, opt);
    }
    return ret;
}

static gboolean context_has_h_entry(generator_options_t *opts) {
    GList *l;
    for (l = opts->options; l; l = l->next) {
        generator_option_t *entry = l->data;
        if (entry->short_name == 'h') {
            return TRUE;
        }
    }
    return FALSE;
}

static void generator_print_help(generator_options_t *opts) {
    gint max_length = 50;
    g_print("%s\n  %s", "Usage:", g_get_prgname());
    if (opts->options)
        g_print(" %s", "[OPTION...]");

    g_print("\n\nHelp Options:\n");
    char token = context_has_h_entry(opts) ? '?' : 'h';
    g_print("  -%c, --%-*s %s\n", token, max_length - 12, "help", "Show help options");

    g_print("\n\nApplication Options:\n");
    GList *l;
    for (l = opts->options; l; l = l->next) {
        generator_option_t *entry = l->data;
        int len = 0;
        if (entry->short_name) {
            g_print("  -%c, --%s", entry->short_name, entry->long_name);
            len = 8 + strlen(entry->long_name);
        } else {
            g_print("  --%s", entry->long_name);
            len = 4 + strlen(entry->long_name);
        }
        if (entry->arg_description) {
            g_print("=%s", entry->arg_description);
            len += 1 + strlen(entry->arg_description);
        }
        g_print("%*s %s\n", max_length - len, "", entry->description ? entry->description : "");
    }
    exit(0);    
}

opt_errcode_t generator_options_parse_cmdline(generator_options_t *opts,
                                             gint *argc,
                                             gchar **argv,
                                             GError **error) {
    if (!argc || !argv) return OPT_ARG_INVALID;
    return OPT_SUCCESS;
}
