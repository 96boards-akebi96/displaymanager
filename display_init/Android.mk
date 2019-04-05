LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS :=
LOCAL_C_INCLUDES :=
USE_BASE_NAME := bsp
LOCAL_CFLAGS += -DSC1401AJ1

GRALLOC_DIR += vendor/arm/mali_lib/pie/include
# Additional CFLAGS for "gralloc_priv.h"
LOCAL_CFLAGS += -DMALI_ION=1
LOCAL_CFLAGS += -DMALI_AFBC_GRALLOC=1
LOCAL_CFLAGS += -DUSE_HWC_V15
LOCAL_CFLAGS += -DAPI_IS_28


LOCAL_C_INCLUDES := $(GRALLOC_DIR)
LOCAL_C_INCLUDES += $(TOP)/system/core/libion
LOCAL_C_INCLUDES += frameworks/native/include/media/openmax

LOCAL_SRC_FILES :=  \
    display_init.cpp \
    ../DisplayManager.cpp \

LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += \
    libcutils libc libcutils libutils liblog libbinder libhardware \
    libomxil-bellagio \
    libomxprox.$(TARGET_BOOTLOADER_BOARD_NAME) \
    libOmxDisplayManager libOmxDisplayController \
    libDisplayManagerClient

LOCAL_MODULE := display_init
LOCAL_MODULE_TAGS := optional
ifeq ($(BOARD_VNDK_VERSION),current)
LOCAL_VENDOR_MODULE := true
endif
include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))

