/*
 * Copyright Mellanox Technologies, Ltd. 2001-2016.
 * This software product is licensed under Apache version 2, as detailed in
 * the COPYING file.
 */

#include <errno.h>
#include <sensors/sensors.h>
#include <sensors/error.h>

#include "tempd_interface.h"
#include "tempd.h"
#include "openvswitch/vlog.h"

#define CHECK_RC(rc, msg, args...)                            \
    do {                                                      \
        if (rc) {                                             \
            VLOG_ERR("%s. " msg, sensors_strerror(rc), args); \
            goto exit;                                        \
        }                                                     \
    } while (0);

VLOG_DEFINE_THIS_MODULE(tempd_sysfs);

struct sysfs_sensor {
    struct locl_sensor up;
    const struct sensors_chip_name *chip;
    const sensors_feature *feature;
    const sensors_subfeature *input;
};

static inline struct sysfs_sensor *sysfs_sensor_cast(const struct locl_sensor *);
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

static const struct tempd_subsystem_class sysfs_sybsystem_class = {
    .tempd_subsystem_alloc     = __subsystem_alloc,
    .tempd_subsystem_construct = __subsystem_construct,
    .tempd_subsystem_destruct  = __subsystem_destruct,
    .tempd_subsystem_dealloc   = __subsystem_dealloc,
};

const struct tempd_sensor_class sysfs_sensor_class = {
    .tempd_sensor_alloc        = __sensor_alloc,
    .tempd_sensor_construct    = __sensor_construct,
    .tempd_sensor_destruct     = __sensor_destruct,
    .tempd_sensor_dealloc      = __sensor_dealloc,
    .tempd_status_get          = __status_get,
    .tempd_temperature_get     = __temperature_get,
    .tempd_threshold_get       = __threshold_get,
};

static inline struct sysfs_sensor *
sysfs_sensor_cast(const struct locl_sensor *sensor_)
{
    ovs_assert(sensor_);

    return CONTAINER_OF(sensor_, struct sysfs_sensor, up);
}

/**
 * Get tempd subsystem class.
 */
const struct tempd_subsystem_class *tempd_subsystem_class_get(void)
{
    return &sysfs_sybsystem_class;
}

/**
 * Get tempd sensor class.
 */
const struct tempd_sensor_class *tempd_sensor_class_get(void)
{
    return &sysfs_sensor_class;
}

/**
 * Initialize ops-tempd platform support plugin.
 */
void tempd_plugin_init(void)
{
    /* We use default config file because nothing additional is needed here. */
    if (sensors_init(NULL)) {
        VLOG_ERR("Failed to initialize sensors library.");
    }
}

/**
 * Deinitialize ops-tempd platform support plugin.
 * plugin.
 */
void tempd_plugin_deinit(void)
{
    sensors_cleanup();
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
    struct sysfs_sensor *sensor = xzalloc(sizeof(struct sysfs_sensor));

    return &sensor->up;
}

static int
__sensor_construct(struct locl_sensor *sensor_)
{
    int rc = 0;
    char dev_name[NAME_MAX] = { };
    int chip_num = 0, feature_num = 0;
    const sensors_chip_name *chip = NULL;
    const sensors_feature *feature = NULL;
    struct sysfs_sensor *sensor = sysfs_sensor_cast(sensor_);

    chip_num = 0;
    while ((chip = sensors_get_detected_chips(NULL, &chip_num))) {
        feature_num = 0;
        while ((feature = sensors_get_features(chip, &feature_num))) {
            if (!chip->dev_name) {
                /* Virtual. No dev_name. */
                continue;
            }
            /* Device name in config file has format <sensor>-<device>. */
            snprintf(dev_name, NAME_MAX, "%s-%s", feature->name, chip->dev_name);
            if (!strcmp(sensor_->yaml_sensor->device, dev_name)) {
                sensor->feature = feature;
                sensor->input = sensors_get_subfeature(chip, feature,
                                                       SENSORS_SUBFEATURE_TEMP_INPUT);
                sensor->chip = chip;
                goto exit;
            }
        }
    }

    if (!sensor->input) {
        rc = -1;
        VLOG_ERR("%s does not have input subfeature.", sensor_->name);
        goto exit;
    }

exit:
    return rc;
}

static void
__sensor_destruct(struct locl_sensor *sensor_)
{
}

static void
__sensor_dealloc(struct locl_sensor *sensor_)
{
    struct sysfs_sensor *sensor = sysfs_sensor_cast(sensor_);

    free(sensor);
}

static int
__temperature_get(const struct locl_sensor *sensor_, int *mdegrees)
{
    int rc = 0;
    double input = 0.0;
    struct sysfs_sensor *sensor = sysfs_sensor_cast(sensor_);

    if (sensor->input == NULL) {
        goto exit;
    }

    rc = sensors_get_value(sensor->chip, sensor->input->number, &input);
    CHECK_RC(rc, "Get temperature for %s", sensor_->name);

    *mdegrees = (int)(input * 1000.0);

exit:
    return rc ? -1 : 0;
}

static int
__status_get(const struct locl_sensor *sensor, bool *operable)
{
    *operable = true;

    return 0;
}

static int
__threshold_get(const struct locl_sensor *sensor_,
                enum threshold_type type,
                float *threshold)
{
    struct sysfs_sensor *sensor = sysfs_sensor_cast(sensor_);
    const sensors_subfeature *subfeature = NULL;
    int subfeature_type = 0, rc = 0;
    double value = 0.0, step = 0.0;

    switch (type) {
        case THRESHOLD_ALARM_EMERGENCY_ON:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_EMERGENCY;
            step = 2.0;
            break;
        case THRESHOLD_ALARM_EMERGENCY_OFF:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_EMERGENCY;
            break;
        case THRESHOLD_ALARM_CRITICAL_ON:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_CRIT;
            step = 5.0;
            break;
        case THRESHOLD_ALARM_CRITICAL_OFF:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_CRIT;
            break;
        case THRESHOLD_ALARM_MAX_ON:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            step = 5.0;
            break;
        case THRESHOLD_ALARM_MAX_OFF:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            break;
        case THRESHOLD_ALARM_MIN:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MIN;
            break;
        case THRESHOLD_ALARM_LOW_CRIT:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_LCRIT;
            break;
        case THRESHOLD_FAN_MAX_ON:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            step = 5.0;
            break;
        case THRESHOLD_FAN_MAX_OFF:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            break;
        case THRESHOLD_FAN_FAST_ON:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            step = -3.0;
            break;
        case THRESHOLD_FAN_FAST_OFF:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            step = -6.0;
            break;
        case THRESHOLD_FAN_MEDIUM_ON:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            step = -9.0;
            break;
        case THRESHOLD_FAN_MEDIUM_OFF:
            subfeature_type = SENSORS_SUBFEATURE_TEMP_MAX;
            step = -12.0;
            break;
        default:
            rc = EINVAL;
            VLOG_ERR("Unknown feature type %d", type);
            goto exit;
    }

    subfeature = sensors_get_subfeature(sensor->chip,
                                        sensor->feature,
                                        subfeature_type);
    rc = subfeature ? 0 : ENOENT;
    CHECK_RC(rc, "Subfeature type %d not available", type);

    rc = sensors_get_value(sensor->chip, subfeature->number, &value);
    CHECK_RC(rc, "Get subfeature type %d for %s",
             subfeature_type,
             sensor_->name);

    *threshold = value + step;

exit:
    return rc ? -1 : 0;
}
