
# Copyright 2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.22.0)

cmake_path(APPEND conf_file_path ${CMAKE_CURRENT_LIST_DIR} ${board}_${core_id} prj.conf)
set(CONF_FILE ${conf_file_path} ${CONF_FILE})
include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(frdm_mcxn947_boot LANGUAGES C CXX ASM PROJECT_BOARD_PORT_PATH ${board_root}/${board}/demo_apps/led_blinky)

include(${SdkRootDirPath}/CMakeLists.txt)


include("${board}_${core_id}/board_files.cmake")

# mcux_add_source(
#     SOURCES led_blinky.c
# )

mcux_add_source(
    SOURCES cdfw_mcxn947_boot/cdfw_mcxn947_boot_main.c
            cdfw_mcxn947_boot/cdfw_mcxn947_boot_validation.c
)

# mcux_add_source(
#     SOURCES led_blinky/peripherals.c
#             led_blinky/peripherals.h
# )

mcux_add_include(
    INCLUDES led_blinky
)
mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)
mcux_add_config_mex_path( PATH ${board}_${core_id} )
mcux_add_include(INCLUDES .)
