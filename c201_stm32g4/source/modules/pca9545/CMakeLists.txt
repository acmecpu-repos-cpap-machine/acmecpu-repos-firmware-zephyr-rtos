# SPDX-License-Identifier: Apache-2.0

if(CONFIG_I2C_MGM_PCA95XX)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(pca9545.c)
endif()
