LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS :=
LOCAL_SHARED_LIBRARIES :=
ENABLE_RECSPERF_LOG :=
ENABLE_VCC :=

LOCAL_CFLAGS += -DSC1401AJ1

LOCAL_SRC_FILES   += \
    OmxDisplayManager_novcc.cpp

LOCAL_CFLAGS += -DUSE_HWC_V15

LOCAL_C_INCLUDES := $(LOCAL_PATH)/..

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES += \
    libcutils libc liblog libutils libbinder libhardware \
    libomxil-bellagio \
    libomxprox.$(TARGET_BOOTLOADER_BOARD_NAME) \
    libOmxDisplayController \
    libDisplayManagerClient

ifeq (1,$(filter 1,$(shell echo "$$(( $(PLATFORM_SDK_VERSION) > 19 ))" )))
LOCAL_SHARED_LIBRARIES += libbase
endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libOmxDisplayManager
ifeq ($(BOARD_VNDK_VERSION),current)
LOCAL_VENDOR_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)
