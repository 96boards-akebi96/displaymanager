LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS :=
LOCAL_C_INCLUDES :=
LOCAL_SHARED_LIBRARIES :=
ENABLE_RECSPERF_LOG :=
ENABLE_VCC :=

LOCAL_CFLAGS += -DSC1401AJ1

LOCAL_SRC_FILES   += \
    BaseComponent.cpp \
    Components.cpp

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES += \
	libcutils libc liblog libutils \
	libomxil-bellagio \
	libomxprox.$(TARGET_BOOTLOADER_BOARD_NAME)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOmxDisplayController
ifeq ($(BOARD_VNDK_VERSION),current)
LOCAL_VENDOR_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)
