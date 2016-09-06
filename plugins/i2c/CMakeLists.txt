# Copyright Mellanox Technologies, Ltd. 2001-2016.
# This software product is licensed under Apache version 2, as detailed in
# the COPYING file.

cmake_minimum_required (VERSION 2.8)

set (TEMPD_PLUGIN tempd-i2c-plugin)
project (${TEMPD_PLUGIN})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror")

# Rules to locate needed libraries
include(FindPkgConfig)
pkg_check_modules(OVSCOMMON REQUIRED libovscommon)
pkg_check_modules(CONFIG_YAML REQUIRED ops-config-yaml)

include_directories (${PROJECT_BINARY_DIR}
                     ${OVSCOMMON_INCLUDE_DIRS}
)

# Source files to build tempd plugin
set (SOURCES i2c-plugin.c)

# Rules to build tempd plugin
add_library (${TEMPD_PLUGIN} SHARED ${SOURCES})

target_link_libraries (${TEMPD_PLUGIN} ${CONFIG_YAML_LIBRARIES}
                       ${OVSCOMMON_LIBRARIES}
                       sensors
)

# Rules to install ops-tempd binary in rootfs
install(TARGETS ${TEMPD_PLUGIN}
        LIBRARY DESTINATION lib/platform/plugins
)
