# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpmsg_remote)

target_sources(app PRIVATE
  src/main.c
)

# STM32MP2X specific: Add custom linker script for resource table support, patch IPCC shim.
target_sources_ifdef(CONFIG_SOC_STM32MP2X_M33 app PRIVATE
  src/stm32_ipcc_shim.c
)

zephyr_linker_sources_ifdef(CONFIG_SOC_STM32MP2X_M33 SECTIONS
  ${CMAKE_CURRENT_SOURCE_DIR}/resource_table.ld
)
