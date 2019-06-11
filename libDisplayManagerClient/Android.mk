# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2016 Socionext Inc.

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS :=

LOCAL_C_INCLUDES += $(LOCAL_PATH)/..
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/.. $(LOCAL_PATH)

LOCAL_SRC_FILES := \
	libDisplayManagerClient.cpp

LOCAL_SHARED_LIBRARIES += libbase
LOCAL_SHARED_LIBRARIES += libutils libcutils libbinder liblog
LOCAL_SHARED_LIBRARIES += \
    libomxil-bellagio \
    libomxprox.$(TARGET_BOOTLOADER_BOARD_NAME) \
    libbinder

LOCAL_MODULE := libDisplayManagerClient

LOCAL_MODULE_TAGS := optional
ifeq ($(BOARD_VNDK_VERSION),current)
LOCAL_VENDOR_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)

