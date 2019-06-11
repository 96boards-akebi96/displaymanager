// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2016 Socionext Inc.

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>

#define LOG_TAG "BpDisplayManager"
#include <IDisplayManager.h>
#include <libDisplayManagerClient.h>
#include <list>

#define LOG_TAG "BpDisplayManager"

using namespace android;

// Client
class BpDisplayManager : public BpInterface<IDisplayManager> {
    public:
        BpDisplayManager(const sp<IBinder>& impl) : BpInterface<IDisplayManager>(impl) {
            ALOGD("BpDisplayManager::BpDisplayManager()");
        }
        virtual int32_t EmptyBuffer(int32_t type, native_handle_t const* hnd, uint32_t flag) {
            ATRACE_CALL();
            Parcel data, reply;
            data.writeInterfaceToken(IDisplayManager::getInterfaceDescriptor());
            data.writeInt32(type);
            data.writeNativeHandle(hnd);
            data.writeInt32(flag);

            // ALOGI("BpDisplayManager::push parcel to be sent:\n");

            int ret = remote()->transact(EMPTY_BUFFER, data, &reply);
            if (ret != NO_ERROR) {
                // ALOGI("%s:%d ----------------", __func__, __LINE__);
                return -1;
            }

            // ALOGI("BpDisplayManager::push parcel reply:\n");
            
            int32_t res;
            reply.readInt32(&res);
            // ALOGD("BpDisplayManager::EmptyBuffer = %i (status: %i)", res, status);
            return res;
        }
        
        virtual int32_t ChangePlaneShow(int32_t flags) {
            ATRACE_CALL();
            Parcel data, reply;
            data.writeInterfaceToken(IDisplayManager::getInterfaceDescriptor());
            data.writeInt32(flags);
            int ret = remote()->transact(CHANGE_PLANE_SHOW, data, &reply);
            if (ret != NO_ERROR) {
                // ALOGI("%s:%d ----------------", __func__, __LINE__);
                return -1;
            }

            int32_t res;
            reply.readInt32(&res);
            return res;
        }

        virtual int32_t SetPowerMode(int32_t mode) {
            ATRACE_CALL();
            Parcel data, reply;
            data.writeInterfaceToken(IDisplayManager::getInterfaceDescriptor());
            data.writeInt32(mode);
            int ret = remote()->transact(SET_POWER_MODE, data, &reply);
            if (ret != NO_ERROR) {
                // ALOGI("%s:%d ----------------", __func__, __LINE__);
                return -1;
            }
            
            int32_t res;
            reply.readInt32(&res);
            return res;
        }

        virtual int32_t GetDisplayConfigs(int32_t *num_config, dm_display_attribute **configs){
            ATRACE_CALL();
            Parcel data, reply;
            data.writeInterfaceToken(IDisplayManager::getInterfaceDescriptor());
            int ret = remote()->transact(GET_DISPLAY_CONFIGS, data, &reply);
            if (ret != NO_ERROR) {
                // ALOGI("%s:%d ----------------", __func__, __LINE__);
                return -1;
            }
            *num_config = reply.readInt32();
            if ((*num_config > 0) && (configs != NULL) && (*configs != NULL)) {
                const void *buf = reply.readInplace((*num_config)*sizeof(dm_display_attribute));
                memcpy(*configs, buf, ((*num_config)*sizeof(dm_display_attribute)));
            }
            return reply.readInt32();
        }
        virtual int32_t setActiveConfig(int32_t index){
            ATRACE_CALL();
            Parcel data, reply;
            data.writeInterfaceToken(IDisplayManager::getInterfaceDescriptor());
            data.writeInt32(index);
            int ret = remote()->transact(SET_ACTIVE_CONFIG, data, &reply);
            if (ret != NO_ERROR) {
                // ALOGI("%s:%d ----------------", __func__, __LINE__);
                return -1;
            }
            return reply.readInt32();
        }
        virtual String8 Dump(void){
            ATRACE_CALL();
            Parcel data, reply;
            data.writeInterfaceToken(IDisplayManager::getInterfaceDescriptor());
            int ret = remote()->transact(DUMP, data, &reply);
            if (ret != NO_ERROR) {
                // ALOGI("%s:%d ----------------", __func__, __LINE__);
                String8 ret;
                ret.append("\n");
                return ret;
            }
            return reply.readString8();
        }
};
IMPLEMENT_META_INTERFACE(DisplayManager, "BpDisplayManager");


// Helper function to get a hold of the "DisplayManager" service.
sp<IDisplayManager> getDemoServ() {
    ATRACE_CALL();
    sp<IServiceManager> sm = defaultServiceManager();
    ALOGE_IF(sm == 0, "binder is null");
    sp<IBinder> binder = sm->getService(String16("DisplayManager"));
    // TODO: If the "Demo" service is not running, getService times out and binder == 0.
    ALOGE_IF(binder == 0, "binder is null");
    sp<IDisplayManager> demo = interface_cast<IDisplayManager>(binder);
    ALOGE_IF(demo == 0, "demo is null");
    return demo;
}
