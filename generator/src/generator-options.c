#include "generator-options.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct opt_change {
    enum option_type arg_type;
    gpointer arg_data;
    union {
        gboolean bool;
        gint integer;
        gchar *str;
        gchar **array;
        gdouble dbl;
        gint64 int64;
    } prev;
    union {
        gchar *str;
        struct {
            gint len;
            gchar **data;
        } array;
    } allocated;
};

struct pending_null {
    gchar **ptr;
    gchar *value;
};


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

static void
free_changes_list(generator_options_t *opts, gboolean revert)
{
    GList *list;
    for (list = opts->changes; list != NULL; list = list->next) {
        struct opt_change *change = list->data;

        if (revert) {
            switch (change->arg_type) {
            case OPTION_ARG_NONE:
                *(gboolean *)change->arg_data = change->prev.bool;
                break;
            case OPTION_ARG_INT:
                *(gint *)change->arg_data = change->prev.integer;
                break;
            case OPTION_ARG_STRING:
                g_free(change->allocated.str);
                *(gchar **)change->arg_data = change->prev.str;
                break;
            case OPTION_ARG_STRING_ARRAY:
                g_strfreev(change->allocated.array.data);
                *(gchar ***)change->arg_data = change->prev.array;
                break;
            case OPTION_ARG_DOUBLE:
                *(gdouble *)change->arg_data = change->prev.dbl;
                break;
            case OPTION_ARG_INT64:
                *(gint64 *)change->arg_data = change->prev.int64;
                break;
            default:
                g_assert_not_reached();
            }
        }
        g_free(change);
    }
    g_list_free(opts->changes);
    opts->changes = NULL;
}

static void
free_pending_nulls(generator_options_t *opts, gboolean perform_nulls)
{
    GList *list;
    for (list = opts->pending_nulls; list != NULL; list = list->next) {
        struct pending_null *n = list->data;

        if (perform_nulls) {
            if (n->value) {
                /* Copy back the short options */
                *(n->ptr)[0] = '-';
                strcpy(*n->ptr + 1, n->value);
            } else {
                *n->ptr = NULL;
            }
        }
        g_free(n->value);
        g_free(n);
    }
    g_list_free(opts->pending_nulls);
    opts->pending_nulls = NULL;
}

generator_options_t *generator_options_new() {
    generator_options_t *opts = g_slice_new0(generator_options_t);
    return opts;
}

void generator_options_free(generator_options_t *opts) {
    if (!opts) return;
    g_list_free_full(opts->options, (GDestroyNotify)generator_option_free);
    free_changes_list(opts, FALSE);
    free_pending_nulls(opts, FALSE);
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

static gboolean opts_has_h_entry(generator_options_t *opts) {
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
    char token = opts_has_h_entry(opts) ? '?' : 'h';
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

static gboolean
parse_int(const gchar *arg_name, const gchar *arg, gint *result, GError **error)
{
    gchar *end;
    glong tmp;

    errno = 0;
    tmp = strtol(arg, &end, 0);

    if (*arg == '\0' || *end != '\0') {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                    "Cannot parse integer value '%s' for %s", arg, arg_name);
        return FALSE;
    }

    *result = tmp;
    if (*result != tmp || errno == ERANGE) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                    "Integer value '%s' for %s out of range", arg, arg_name);
        return FALSE;
    }
    return TRUE;
}

static gboolean
parse_double(const gchar *arg_name, const gchar *arg, gdouble *result, GError **error)
{
    gchar *end;
    gdouble tmp;

    errno = 0;
    tmp = g_strtod(arg, &end);

    if (*arg == '\0' || *end != '\0') {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                    "Cannot parse double value '%s' for %s", arg, arg_name);
        return FALSE;
    }
    if (errno == ERANGE) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                    "Double value '%s' for %s out of range", arg, arg_name);
        return FALSE;
    }
    *result = tmp;
    return TRUE;
}

static gboolean
parse_int64(const gchar *arg_name, const gchar *arg, gint64 *result, GError **error)
{
    gchar *end;
    gint64 tmp;

    errno = 0;
    tmp = g_ascii_strtoll(arg, &end, 0);

    if (*arg == '\0' || *end != '\0') {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                    "Cannot parse integer value '%s' for %s", arg, arg_name);
        return FALSE;
    }
    if (errno == ERANGE) {
        g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                    "Integer value '%s' for %s out of range", arg, arg_name);
        return FALSE;
    }
    *result = tmp;
    return TRUE;
}

static void
add_pending_null(generator_options_t *opts, gchar **ptr, gchar *value)
{
    struct pending_null *n;

    n = g_new0(struct pending_null, 1);
    n->ptr = ptr;
    n->value = value;

    opts->pending_nulls = g_list_prepend(opts->pending_nulls, n);
}

static struct opt_change *
get_change(generator_options_t *opts, enum option_type arg_type, gpointer arg_data)
{
    GList *list;
    struct opt_change *change = NULL;

    for (list = opts->changes; list != NULL; list = list->next) {
        change = list->data;

        if (change->arg_data == arg_data)
            goto found;
    }

    change = g_new0(struct opt_change, 1);
    change->arg_type = arg_type;
    change->arg_data = arg_data;

    opts->changes = g_list_prepend(opts->changes, change);

  found:
    return change;
}

static gboolean
parse_arg(generator_options_t *opts,
          generator_option_t *entry, const gchar *value, const gchar *option_name, GError **error)
{
    struct opt_change *change;

    switch (entry->arg) {
    case OPTION_ARG_NONE:{
        (void)get_change(opts, OPTION_ARG_NONE, entry->arg_data);

        *(gboolean *)entry->arg_data = !(entry->flags & OPTION_FLAG_REVERSE);
        break;
    }
    case OPTION_ARG_STRING:{
        gchar *data = g_locale_to_utf8(value, -1, NULL, NULL, error);
        if (!data)
            return FALSE;

        change = get_change(opts, OPTION_ARG_STRING, entry->arg_data);
        g_free(change->allocated.str);

        change->prev.str = *(gchar **)entry->arg_data;
        change->allocated.str = data;

        *(gchar **)entry->arg_data = data;
        break;
    }
    case OPTION_ARG_STRING_ARRAY:{
        gchar *data = g_locale_to_utf8(value, -1, NULL, NULL, error);
        if (!data)
            return FALSE;

        change = get_change(opts, OPTION_ARG_STRING_ARRAY, entry->arg_data);

        if (change->allocated.array.len == 0) {
            change->prev.array = *(gchar ***)entry->arg_data;
            change->allocated.array.data = g_new(gchar *, 2);
        } else {
            change->allocated.array.data =
                g_renew(gchar *, change->allocated.array.data, change->allocated.array.len + 2);
        }
        change->allocated.array.data[change->allocated.array.len] = data;
        change->allocated.array.data[change->allocated.array.len + 1] = NULL;

        change->allocated.array.len++;

        *(gchar ***)entry->arg_data = change->allocated.array.data;

        break;
    }
    case OPTION_ARG_INT:{
        gint data;
        if (!parse_int(option_name, value, &data, error))
            return FALSE;

        change = get_change(opts, OPTION_ARG_INT, entry->arg_data);
        change->prev.integer = *(gint *)entry->arg_data;
        *(gint *)entry->arg_data = data;
        break;
    }
    case OPTION_ARG_DOUBLE:{
        gdouble data;
        if (!parse_double(option_name, value, &data, error)) {
            return FALSE;
        }
        change = get_change(opts, OPTION_ARG_DOUBLE, entry->arg_data);
        change->prev.dbl = *(gdouble *)entry->arg_data;
        *(gdouble *)entry->arg_data = data;
        break;
    }
    case OPTION_ARG_INT64:{
        gint64 data;
        if (!parse_int64(option_name, value, &data, error)) {
            return FALSE;
        }
        change = get_change(opts, OPTION_ARG_INT64, entry->arg_data);
        change->prev.int64 = *(gint64 *)entry->arg_data;
        *(gint64 *)entry->arg_data = data;
        break;
    }
    default:
        g_assert_not_reached();
    }
    entry->flags |= OPTION_FLAG_CMDLINE;
    return TRUE;
}

static gboolean
parse_long_option(generator_options_t *opts,
                  gint *idx, gchar *arg, gboolean aliased, gint *argc, gchar ***argv, GError **error, gboolean *parsed)
{
    GList *l;
    for (l = opts->options; l; l = l->next) {
        generator_option_t *entry = l->data;
        if (*idx >= *argc)
            return TRUE;

        if (NO_ARG(entry) && strcmp(arg, entry->long_name) == 0) {
            gchar *option_name = g_strconcat("--", entry->long_name, NULL);
            gboolean retval = parse_arg(opts, entry, NULL, option_name, error);
            g_free(option_name);

            add_pending_null(opts, &((*argv)[*idx]), NULL);
            *parsed = TRUE;

            return retval;
        } else {
            gint len = strlen(entry->long_name);

            if (strncmp(arg, entry->long_name, len) == 0 && (arg[len] == '=' || arg[len] == 0)) {
                gchar *value = NULL;
                gchar *option_name;

                add_pending_null(opts, &((*argv)[*idx]), NULL);
                option_name = g_strconcat("--", entry->long_name, NULL);

                if (arg[len] == '=') {
                    value = arg + len + 1;
                } else if (*idx < *argc - 1) {
                    if (!OPTIONAL_ARG(entry)) {
                        value = (*argv)[*idx + 1];
                        add_pending_null(opts, &((*argv)[*idx + 1]), NULL);
                        (*idx)++;
                    } else {
                        if ((*argv)[*idx + 1][0] == '-') {
                            gboolean retval = parse_arg(opts, entry,
                                                        NULL, option_name, error);
                            *parsed = TRUE;
                            g_free(option_name);
                            return retval;
                        } else {
                            value = (*argv)[*idx + 1];
                            add_pending_null(opts, &((*argv)[*idx + 1]), NULL);
                            (*idx)++;
                        }
                    }
                } else if (*idx >= *argc - 1 && OPTIONAL_ARG(entry)) {
                    gboolean retval = parse_arg(opts, entry, NULL, option_name, error);
                    *parsed = TRUE;
                    g_free(option_name);
                    return retval;
                } else {
                    g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                                "Missing argument for %s", option_name);
                    g_free(option_name);
                    return FALSE;
                }
                if (!parse_arg(opts, entry, value, option_name, error)) {
                    g_free(option_name);
                    return FALSE;
                }
                g_free(option_name);
                *parsed = TRUE;
            }
        }
    }
    return TRUE;
}

static gboolean
parse_short_option(generator_options_t *opts,
                   gint idx, gint *new_idx, gchar arg, gint *argc, gchar ***argv, GError **error, gboolean *parsed)
{
    GList *l;
    for (l = opts->options; l; l = l->next) {
        generator_option_t *entry = l->data;

        if (arg == entry->short_name) {
            gchar *option_name = g_strdup_printf("-%c", entry->short_name);
            gchar *value = NULL;
            if (NO_ARG(entry)) {
                value = NULL;
            } else {
                if (*new_idx > idx) {
                    g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_FAILED, "Error parsing option %s", option_name);
                    g_free(option_name);
                    return FALSE;
                }

                if (idx < *argc - 1) {
                    if (!OPTIONAL_ARG(entry)) {
                        value = (*argv)[idx + 1];
                        add_pending_null(opts, &((*argv)[idx + 1]), NULL);
                        *new_idx = idx + 1;
                    } else {
                        if ((*argv)[idx + 1][0] == '-') {
                            value = NULL;
                        } else {
                            value = (*argv)[idx + 1];
                            add_pending_null(opts, &((*argv)[idx + 1]), NULL);
                            *new_idx = idx + 1;
                        }
                    }
                } else if (idx >= *argc - 1 && OPTIONAL_ARG(entry)) {
                    value = NULL;
                } else {
                    g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
                                "Missing argument for %s", option_name);
                    g_free(option_name);
                    return FALSE;
                }
            }

            if (!parse_arg(opts, entry, value, option_name, error)) {
                g_free(option_name);
                return FALSE;
            }
            g_free(option_name);
            *parsed = TRUE;
        }
    }
    return TRUE;
}

static gboolean
parse_remaining_arg(generator_options_t *opts, gint *idx, gint *argc, gchar ***argv, GError **error, gboolean *parsed)
{
    GList *l;
    for (l = opts->options; l; l = l->next) {
        generator_option_t *entry = l->data;
        if (*idx >= *argc)
            return TRUE;

        if (entry->long_name[0])
            continue;

        g_return_val_if_fail(entry->arg == OPTION_ARG_STRING_ARRAY, FALSE);

        add_pending_null(opts, &((*argv)[*idx]), NULL);

        if (!parse_arg(opts, entry, (*argv)[*idx], "", error))
            return FALSE;

        *parsed = TRUE;
        return TRUE;
    }
    return TRUE;
}

opt_errcode_t generator_options_parse_cmdline(generator_options_t *opts,
                                             gint *argc,
                                             gchar ***argv,
                                             GError **error) {
    if (!argc || !argv) return OPT_ARG_INVALID;
    if (!g_get_prgname()) {
        if (*argc) {
            gchar *prgname = g_path_get_basename((*argv)[0]);
            g_set_prgname(prgname ? prgname : "<unknown>");
            g_free(prgname);
        }
    }

    gboolean stop_parsing = FALSE;
    gboolean has_unknown = FALSE;
    gint separator_pos = 0;
    int i, j, k;
    for (i = 1; i < *argc; i++) {
        gchar *arg;
        gboolean parsed = FALSE;

        if ((*argv)[i][0] == '-' && (*argv)[i][1] != '\0' && !stop_parsing) {
            if ((*argv)[i][1] == '-') {
                /* -- option */

                arg = (*argv)[i] + 2;

                /* '--' terminates list of arguments */
                if (*arg == 0) {
                    separator_pos = i;
                    stop_parsing = TRUE;
                    continue;
                }

                /* Handle help options */
                if (opts->help_enabled && strcmp(arg, "help") == 0)
                    generator_print_help(opts);

                if (!parse_long_option(opts, &i, arg, FALSE, argc, argv, error, &parsed))
                    goto fail;

                if (parsed)
                    continue;

                if (opts->ignore_unknown)
                    continue;
            } else {            /* short option */
                gint new_i = i, arg_length;
                gboolean *nulled_out = NULL;
                gboolean has_h_entry = opts_has_h_entry(opts);
                arg = (*argv)[i] + 1;
                arg_length = strlen(arg);
                nulled_out = g_newa(gboolean, arg_length);
                memset(nulled_out, 0, arg_length * sizeof(gboolean));

                for (j = 0; j < arg_length; j++) {
                    if (opts->help_enabled && (arg[j] == '?' || (arg[j] == 'h' && !has_h_entry)))
                        generator_print_help(opts);
                    parsed = FALSE;
                    if (!parse_short_option(opts, i, &new_i, arg[j], argc, argv, error, &parsed))
                        goto fail;

                    if (opts->ignore_unknown && parsed)
                        nulled_out[j] = TRUE;
                    else if (opts->ignore_unknown)
                        continue;
                    else if (!parsed)
                        break;
                    /* !opts->ignore_unknown && parsed */
                }
                if (opts->ignore_unknown) {
                    gchar *new_arg = NULL;
                    gint arg_index = 0;
                    for (j = 0; j < arg_length; j++) {
                        if (!nulled_out[j]) {
                            if (!new_arg)
                                new_arg = g_malloc(arg_length + 1);
                            new_arg[arg_index++] = arg[j];
                        }
                    }
                    if (new_arg)
                        new_arg[arg_index] = '\0';
                    add_pending_null(opts, &((*argv)[i]), new_arg);
                    i = new_i;
                } else if (parsed) {
                    add_pending_null(opts, &((*argv)[i]), NULL);
                    i = new_i;
                }
            }

            if (!parsed)
                has_unknown = TRUE;

            if (!parsed && !opts->ignore_unknown) {
                g_set_error(error, G_OPTION_ERROR, G_OPTION_ERROR_UNKNOWN_OPTION, "Unknown option %s", (*argv)[i]);
                goto fail;
            }
        } else {
            //if (opts->strict_posix)
            stop_parsing = TRUE;

            /* Collect remaining args */
            if (!parse_remaining_arg(opts, &i, argc, argv, error, &parsed))
                goto fail;
            if (!parsed && (has_unknown || (*argv)[i][0] == '-'))
                separator_pos = 0;
        }
    }

    if (separator_pos > 0)
        add_pending_null(opts, &((*argv)[separator_pos]), NULL);
    if (argc && argv) {
        free_pending_nulls(opts, TRUE);
        for (i = 1; i < *argc; i++) {
            for (k = i; k < *argc; k++)
                if ((*argv)[k] != NULL)
                    break;

            if (k > i) {
                k -= i;
                for (j = i + k; j < *argc; j++) {
                    (*argv)[j - k] = (*argv)[j];
                    (*argv)[j] = NULL;
                }
                *argc -= k;
            }
        }
    }
    return OPT_SUCCESS;

  fail:
    free_changes_list(opts, TRUE);
    free_pending_nulls(opts, FALSE);
    return OPT_FAILED;
}
