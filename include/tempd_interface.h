/*
 * Copyright Mellanox Technologies, Ltd. 2001-2016.
 * This software product is licensed under Apache version 2, as detailed in
 * the COPYING file.
 */

#ifndef _TEMPD_INTERFACE_H_
#define _TEMPD_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>
#include "sset.h"

struct alarm_threshold_values;
struct fan_threshold_values;

enum threshold_type {
    THRESHOLD_ALARM_EMERGENCY_ON = 0,
    THRESHOLD_ALARM_EMERGENCY_OFF,
    THRESHOLD_ALARM_CRITICAL_ON,
    THRESHOLD_ALARM_CRITICAL_OFF,
    THRESHOLD_ALARM_MAX_ON,
    THRESHOLD_ALARM_MAX_OFF,
    THRESHOLD_ALARM_MIN,
    THRESHOLD_ALARM_LOW_CRIT,
    THRESHOLD_FAN_MAX_ON,
    THRESHOLD_FAN_MAX_OFF,
    THRESHOLD_FAN_FAST_ON,
    THRESHOLD_FAN_FAST_OFF,
    THRESHOLD_FAN_MEDIUM_ON,
    THRESHOLD_FAN_MEDIUM_OFF,
};

struct tempd_subsystem_class {

    /**
     * Allocation of temperature sensor subsystem on adding to ovsdb.
     * Implementation should define its own struct that contains parent struct
     * locl_subsystem, and return pointer to parent.
     *
     * @return pointer to allocated subsystem.
     */
    struct locl_subsystem *(*tempd_subsystem_alloc)(void);

    /**
     * Construction of temperature sensor subsystem on adding to ovsdb.
     * Implementation should initialize all fields in derived structure from
     * locl_subsystem.
     *
     * @param[out] subsystem - pointer to subsystem.
     *
     * @return 0     on success.
     * @return errno on failure.
     */
    int (*tempd_subsystem_construct)(struct locl_subsystem *subsystem);

    /**
     * Destruction of temperature sensor subsystem on removing from ovsdb.
     * Implementation should deinitialize all fields in derived structure from
     * locl_subsystem.
     *
     * @param[in] subsystem - pointer to subsystem.
     */
    void (*tempd_subsystem_destruct)(struct locl_subsystem *subsystem);

    /**
     * Deallocation of temperature sensor subsystem on removing from ovsdb.
     * Implementation should free memory from derived structure.
     *
     * @param[in] subsystem - pointer to subsystem.
     */
    void (*tempd_subsystem_dealloc)(struct locl_subsystem *subsystem);
};

struct tempd_sensor_class {
    /**
     * Allocation of sensor. Implementation should define its own struct that
     * contains parent struct locl_sensor, and return pointer to parent.
     *
     * @return pointer to allocated sensor.
     */
    struct locl_sensor *(*tempd_sensor_alloc)(void);

    /**
     * Construction of sensor on adding to ovsdb. Implementation should
     * initialize all fields in derived structure from locl_sensor.
     *
     * @param[out] sensor - pointer to sensor.
     *
     * @return 0     on success.
     * @return errno on failure.
     */
    int (*tempd_sensor_construct)(struct locl_sensor *sensor);

    /**
     * Destruction of sensor. Implementation should deinitialize all fields in
     * derived structure from locl_sensor.
     *
     * @param[in] sensor - pointer to sensor.
     */
    void (*tempd_sensor_destruct)(struct locl_sensor *sensor);

    /**
     * Deallocation of sensor. Implementation should free memory from derived
     * structure.
     *
     * @param[in] sensor - pointer to sensor.
     */
    void (*tempd_sensor_dealloc)(struct locl_sensor *sensor);

    /**
     * Get sensor status.
     *
     * @param[in] sensor - pointer to sensor.
     * @param[out] operable - pointer to boolean variable to be filled.
     *
     * @return 0     on success.
     * @return errno on failure.
     */
    int (*tempd_status_get)(const struct locl_sensor *sensor, bool *operable);

    /**
     * Get sensor temperature.
     *
     * @param[in] sensor - pointer to sensor.
     * @param[out] temp - pointer to temperature variable to be filled.
     *
     * @return 0     on success.
     * @return errno on failure.
     */
    int (*tempd_temperature_get)(const struct locl_sensor *sensor,
                                 int *mdegrees);

    /**
     * Get sensor thresholds.
     *
     * @param[in] sensor - pointer to sensor.
     * @param[in] threshold_type - type of requested threshold.
     * @param[out] threshold - pointer to threshold that has to be filled.
     *
     * @return 0     on success.
     * @return errno on failure.
     */
    int (*tempd_threshold_get)(const struct locl_sensor *sensor,
                               enum threshold_type type,
                               float *threshold);
};

#endif /* _TEMPD_INTERFACE_H_ */
