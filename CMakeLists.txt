# (c) Copyright 2015 Hewlett Packard Enterprise Development LP
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
# src/platform/tempd/Makefile

cmake_minimum_required (VERSION 2.8)

set (TEMPD ops-tempd)
project (${TEMPD})
set (SRC_DIR src)
set (INCL_DIR include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror")

# Rules to locate needed libraries
include(FindPkgConfig)
pkg_check_modules(CONFIG_YAML REQUIRED ops-config-yaml)
pkg_check_modules(OVSCOMMON REQUIRED libovscommon)
pkg_check_modules(OVSDB REQUIRED libovsdb)

include_directories (${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/${INCL_DIR}
                     ${OVSCOMMON_INCLUDE_DIRS}
)

# Sources to build ops-tempd
set (SOURCES ${SRC_DIR}/tempd.c)

# Rules to build ops-tempd
add_executable (${TEMPD} ${SOURCES})

target_link_libraries (${TEMPD} ${CONFIG_YAML_LIBRARIES}
                       ${OVSCOMMON_LIBRARIES} ${OVSDB_LIBRARIES}
                       -lpthread -lrt -lsupportability)

# Build ops-ledd cli shared libraries.
add_subdirectory(src/cli)

# Rules to install ops-tempd binary in rootfs
install(TARGETS ${TEMPD}
        RUNTIME DESTINATION bin)
