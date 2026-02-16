# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PCA9632)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(pca9632.c)
endif()
