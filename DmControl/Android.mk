LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES :=
LOCAL_SRC_FILES:= dm_cmd.cpp
LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libutils \
	libbinder \
	libDisplayManagerClient

LOCAL_SHARED_LIBRARIES += libbase
LOCAL_CFLAGS += -DUSE_VNDSERVICE

LOCAL_MODULE:= DmControlCmd
LOCAL_MODULE_TAGS := optional
ifeq ($(BOARD_VNDK_VERSION),current)
LOCAL_VENDOR_MODULE := true
endif
include $(BUILD_EXECUTABLE)
