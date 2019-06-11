# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016 Socionext Inc.

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_CFLAGS :=

LOCAL_CFLAGS += -DSC1401AJ1
GRALLOC_DIR += vendor/arm/gralloc/driver/product/android/gralloc/src
# Additional CFLAGS for "gralloc_priv.h"
LOCAL_CFLAGS += -DMALI_ION=1
LOCAL_CFLAGS += -DMALI_AFBC_GRALLOC=1

LOCAL_C_INCLUDES := $(GRALLOC_DIR)
LOCAL_C_INCLUDES += $(TOP)/system/core/libion

LOCAL_SRC_FILES := 	\
    DisplayManager.cpp \
    displaymanager_main.cpp
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += \
    libcutils libc libcutils libutils liblog libbinder libhardware \
    libomxil-bellagio \
    libomxprox.$(TARGET_BOOTLOADER_BOARD_NAME) \
    libOmxDisplayManager libOmxDisplayController \
    libDisplayManagerClient

LOCAL_CFLAGS += -DUSE_VNDSERVICE
LOCAL_MODULE := display_manager
LOCAL_MODULE_TAGS := optional
ifeq ($(BOARD_VNDK_VERSION),current)
LOCAL_VENDOR_MODULE := true
endif
include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))


