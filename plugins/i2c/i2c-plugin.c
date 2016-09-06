/*
 * Copyright Mellanox Technologies, Ltd. 2001-2016.
 * This software product is licensed under Apache version 2, as detailed in
 * the COPYING file.
 */

#include <errno.h>

#include "tempd_interface.h"
#include "tempd.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(tempd_i2c);

struct i2c_sensor {
    struct locl_sensor up;
    uint32_t fault_count;
};

static inline struct i2c_sensor *i2c_sensor_cast(const struct locl_sensor *);
static struct locl_subsystem *__subsystem_alloc(void);
static int __subsystem_construct(struct locl_subsystem *subsystem);
static void __subsystem_destruct(struct locl_subsystem *subsystem);
static void __subsystem_dealloc(struct locl_subsystem *subsystem);

static struct locl_sensor * __sensor_alloc(void);
static int __sensor_construct(struct locl_sensor *sensor);
static void __sensor_destruct(struct locl_sensor *sensor);
static void __sensor_dealloc(struct locl_sensor *sensor);
static int __status_get(const struct locl_sensor *sensor, bool *operable);
static int __temperature_get(const struct locl_sensor *sensor, int *mdegrees);
static int __threshold_get(const struct locl_sensor *sensor,
                           enum threshold_type type,
                           float *threshold);

static const struct tempd_subsystem_class i2c_sybsystem_class = {
    .tempd_subsystem_alloc     = __subsystem_alloc,
    .tempd_subsystem_construct = __subsystem_construct,
    .tempd_subsystem_destruct  = __subsystem_destruct,
    .tempd_subsystem_dealloc   = __subsystem_dealloc,
};

const struct tempd_sensor_class i2c_sensor_class = {
    .tempd_sensor_alloc        = __sensor_alloc,
    .tempd_sensor_construct    = __sensor_construct,
    .tempd_sensor_destruct     = __sensor_destruct,
    .tempd_sensor_dealloc      = __sensor_dealloc,
    .tempd_status_get          = __status_get,
    .tempd_temperature_get     = __temperature_get,
    .tempd_threshold_get       = __threshold_get,
};

static inline struct i2c_sensor *
i2c_sensor_cast(const struct locl_sensor *sensor_)
{
    ovs_assert(sensor_);

    return CONTAINER_OF(sensor_, struct i2c_sensor, up);
}

/**
 * Get tempd subsystem class.
 */
const struct tempd_subsystem_class *tempd_subsystem_class_get(void)
{
    return &i2c_sybsystem_class;
}

/**
 * Get tempd sensor class.
 */
const struct tempd_sensor_class *tempd_sensor_class_get(void)
{
    return &i2c_sensor_class;
}

/**
 * Initialize ops-tempd platform support plugin.
 */
void tempd_plugin_init(void)
{
}

/**
 * Deinitialize ops-tempd platform support plugin.
 * plugin.
 */
void tempd_plugin_deinit(void)
{
}

void
tempd_plugin_run(void)
{
}

void tempd_plugin_wait(void)
{
}

static struct locl_subsystem *
__subsystem_alloc(void)
{
    return xzalloc(sizeof(struct locl_subsystem));
}

static int
__subsystem_construct(struct locl_subsystem *subsystem)
{
    return 0;
}

static void
__subsystem_destruct(struct locl_subsystem *subsystem)
{
}

static void
__subsystem_dealloc(struct locl_subsystem *subsystem)
{
    free(subsystem);
}

static struct locl_sensor *
__sensor_alloc(void)
{
    struct i2c_sensor *sensor = xzalloc(sizeof(struct i2c_sensor));

    return &sensor->up;
}

static int
__sensor_construct(struct locl_sensor *sensor_)
{
    const YamlSensor *yaml_sensor = sensor_->yaml_sensor;

    if (strcmp(yaml_sensor->type, "lm75")) {
        VLOG_ERR("Unrecognized sensor type %s", yaml_sensor->type);
        return -1;
    }

    return 0;
}

static void
__sensor_destruct(struct locl_sensor *sensor_)
{
}

static void
__sensor_dealloc(struct locl_sensor *sensor_)
{
    struct i2c_sensor *sensor = i2c_sensor_cast(sensor_);

    free(sensor);
}

static int
__temperature_get(const struct locl_sensor *sensor_, int *mdegrees)
{
    int rc = 0;
    char buf[2];
    struct i2c_sensor *sensor = i2c_sensor_cast(sensor_);

    const YamlDevice *device = yaml_find_device(sensor_->subsystem->yaml_handle,
                                                sensor_->subsystem->name,
                                                sensor_->yaml_sensor->device);

    rc = i2c_data_read(sensor_->subsystem->yaml_handle, device,
                       sensor_->subsystem->name, 0, sizeof(buf), buf);

    if (rc) {
        sensor->fault_count++;
        /* Avoid overflow. */
        sensor->fault_count = sensor->fault_count > MAX_FAIL_RETRY ?
                                                    MAX_FAIL_RETRY :
                                                    sensor->fault_count;
        goto exit;
    }

    sensor->fault_count = 0;
    /* Convert to millidegrees (C). */
    *mdegrees = (int)buf[0] * MILI_DEGREES;
    /* High bit in second byte is half-degree indicator. */
    if (buf[1] < 0) {
        *mdegrees += 500;
    }

exit:
    return rc ? -1 : 0;
}

static int
__status_get(const struct locl_sensor *sensor_, bool *operable)
{
    int rc = 0;
    char buf[2];
    const YamlDevice *device = NULL;
    struct i2c_sensor *sensor = i2c_sensor_cast(sensor_);

    /* Didn't exceed fault count. Sensor is ok. */
    if (sensor->fault_count <= MAX_FAIL_RETRY) {
        *operable = true;
        goto exit;
    }

    device = yaml_find_device(sensor_->subsystem->yaml_handle,
                              sensor_->subsystem->name,
                              sensor_->yaml_sensor->device);
    /* Try to read from i2c. If doesn't fail, we're good. */
    rc = i2c_data_read(sensor_->subsystem->yaml_handle, device,
                       sensor_->subsystem->name, 0, sizeof(buf), buf);

    *operable = rc ? false : true;

exit:
    return 0;
}

static int
__threshold_get(const struct locl_sensor *sensor_,
                enum threshold_type type,
                float *threshold)
{
    /* i2c does not provide thresholds. Use defaults from yaml file. */

    return -1;
}
