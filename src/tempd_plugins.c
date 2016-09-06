/*
 * Copyright Mellanox Technologies, Ltd. 2001-2016.
 * This software product is licensed under Apache version 2, as detailed in
 * the COPYING file.
 */

#include <ltdl.h>
#include <limits.h>
#include <stdio.h>
#include <openvswitch/vlog.h>
#include "tempd_interface.h"
#include "tempd_plugins.h"

VLOG_DEFINE_THIS_MODULE(tempd_plugins);

#define CHECK_LT_RC(rc, msg)                    \
    do {                                        \
        if (rc) {                               \
            VLOG_ERR(msg ": %s", lt_dlerror()); \
            goto exit;                          \
        }                                       \
    } while (0);

struct plugin_class {
    char type[NAME_MAX];
    void (*plugin_init)(void);
    void (*plugin_deinit)(void);
    void (*plugin_run)(void);
    void (*plugin_wait)(void);
    const struct tempd_subsystem_class *(*subsystem_class_get)(void);
    const struct tempd_sensor_class *(*sensor_class_get)(void);
};

static lt_dlinterface_id interface_id;

static int
plugin_open(const char *filename, void *data)
{
    int rc = 0;
    const char *libname = strrchr(filename, '/') + 1;
    struct plugin_class *pc = xzalloc(sizeof *pc);
    lt_dlhandle dlhandle;

    VLOG_INFO("Loading symbols from %s %s", filename, libname);

    rc = !(dlhandle = lt_dlopenadvise(filename, *(lt_dladvise *)data));
    CHECK_LT_RC(rc, "lt_dlopenadvise");

    rc = !(pc->plugin_init = lt_dlsym(dlhandle, "tempd_plugin_init"));
    CHECK_LT_RC(rc, "Load tempd_plugin_init");

    rc = !(pc->plugin_deinit = lt_dlsym(dlhandle, "tempd_plugin_deinit"));
    CHECK_LT_RC(rc, "Load tempd_plugin_deinit");

    rc = !(pc->plugin_run = lt_dlsym(dlhandle, "tempd_plugin_run"));
    CHECK_LT_RC(rc, "Load tempd_plugin_run");

    rc = !(pc->plugin_wait = lt_dlsym(dlhandle, "tempd_plugin_wait"));
    CHECK_LT_RC(rc, "Load tempd_plugin_wait");

    rc = !(pc->subsystem_class_get = lt_dlsym(dlhandle, "tempd_subsystem_class_get"));
    CHECK_LT_RC(rc, "Load tempd_subsystem_class_get");

    rc = !(pc->sensor_class_get = lt_dlsym(dlhandle, "tempd_sensor_class_get"));
    CHECK_LT_RC(rc, "Load tempd_sensor_class_get");

    snprintf(pc->type, NAME_MAX, libname);

    rc = lt_dlcaller_set_data(interface_id, dlhandle, pc) != NULL;
    CHECK_LT_RC(rc, "lt_dlcaller_set_data already called for this handle");

exit:
    if (rc) {
        free(pc);
    }

    /* We skip plugins that do not have required interface, e. g. for other
     * daemons, and always return 0. */
    return 0;
}

/**
 * Loads platform plugin. Initializes callbacks from plugin.
 *
 * @return 0 on success, non-zero value on failure.
 */
int
tempd_plugins_load(void)
{
    int rc = 0;
    lt_dladvise advise;

    rc = lt_dlinit() || lt_dlsetsearchpath(PLATFORM_PLUGINS_PATH) ||
        lt_dladvise_init(&advise);
    CHECK_LT_RC(rc, "ltdl initializations");

    rc = !(interface_id = lt_dlinterface_register("tempd-plugin", NULL));
    CHECK_LT_RC(rc, "Interface register");

    rc = lt_dladvise_local(&advise) || lt_dladvise_ext(&advise) ||
        lt_dlforeachfile(lt_dlgetsearchpath(), &plugin_open, &advise);
    CHECK_LT_RC(rc, "Setting ltdl advice");

exit:
    if (rc && interface_id) {
        lt_dlinterface_free(interface_id);
    }

    return rc;
}

/**
 * Unload plugin - deinitialize symbols by destroing handle.
 */
void
tempd_plugins_unload(void)
{
    lt_dlinterface_free(interface_id);
    lt_dlexit();
}

void
tempd_plugins_init(void)
{
    lt_dlhandle iter_handle = 0;
    struct plugin_class *pc = NULL;

    while ((iter_handle = lt_dlhandle_iterate(interface_id, iter_handle))) {
        pc = (struct plugin_class *)lt_dlcaller_get_data(interface_id, iter_handle);
        if (pc) {
            pc->plugin_init();
        }
    }
}

void
tempd_plugins_deinit(void)
{
    lt_dlhandle iter_handle = 0;
    struct plugin_class *pc = NULL;

    while ((iter_handle = lt_dlhandle_iterate(interface_id, iter_handle))) {
        pc = (struct plugin_class *)lt_dlcaller_get_data(interface_id, iter_handle);
        if (pc) {
            pc->plugin_deinit();
        }
    }
}

void
tempd_plugins_run(void)
{
    lt_dlhandle iter_handle = 0;
    struct plugin_class *pc = NULL;

    while ((iter_handle = lt_dlhandle_iterate(interface_id, iter_handle))) {
        pc = (struct plugin_class *)lt_dlcaller_get_data(interface_id, iter_handle);
        if (pc) {
            pc->plugin_run();
        }
    }
}

void
tempd_plugins_wait(void)
{
    lt_dlhandle iter_handle = 0;
    struct plugin_class *pc = NULL;

    while ((iter_handle = lt_dlhandle_iterate(interface_id, iter_handle))) {
        pc = (struct plugin_class *)lt_dlcaller_get_data(interface_id, iter_handle);
        if (pc) {
            pc->plugin_wait();
        }
    }
}

const struct tempd_subsystem_class *
tempd_subsystem_class_get(const char *platform_type)
{
    lt_dlhandle iter_handle = 0;
    struct plugin_class *pc = NULL;

    while ((iter_handle = lt_dlhandle_iterate(interface_id, iter_handle))) {
        pc = (struct plugin_class *)lt_dlcaller_get_data(interface_id, iter_handle);
        if (pc && !strcmp(pc->type, platform_type)) {
            return pc->subsystem_class_get();
        }
    }

    return NULL;
}

const struct tempd_sensor_class *
tempd_sensor_class_get(const char *platform_type)
{
    lt_dlhandle iter_handle = 0;
    struct plugin_class *pc = NULL;

    while ((iter_handle = lt_dlhandle_iterate(interface_id, iter_handle))) {
        pc = (struct plugin_class *)lt_dlcaller_get_data(interface_id, iter_handle);
        if (pc && !strcmp(pc->type, platform_type)) {
            return pc->sensor_class_get();
        }
    }

    return NULL;
}
