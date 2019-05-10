
#include "utils/RefBase.h"
#include <log/log.h>
#include <cutils/compiler.h>

#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include <OmxDisplayManager.h>
#include <IDisplayManager.h>
#include <pthread.h>

using namespace android;

// Server
class BnDisplayManager : public BnInterface<IDisplayManager> {
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
    protected:
        pthread_mutex_t initialized_mutex;
        pthread_mutex_t change_port_setting_mutex;
        pthread_cond_t change_port_setting_wait;
        int change_port_setting = 0;
        int empty_buffer = 0;

        bool initialized;
};

class DisplayManager : public BnDisplayManager
{
    private:
        OmxDisplayManager *omx_display;
        PortSet *OsdPortSet;
        int GetComponentAndPort(int32_t type, BaseComponent **cmp = NULL, BasePort **port = NULL,  PortSet **ps = NULL);
        int PlaneShowStatus;
        int current_index;

    public:
        static char const* getServiceName() ANDROID_API {
            return "DisplayManager";
        };
        DisplayManager() ANDROID_API;
        ~DisplayManager();
        void init() ANDROID_API;
        void run(bool block = true) ANDROID_API;
        void deinit() ANDROID_API;
        status_t dump(int fd, const Vector<String16>& args);

        // Client API
        virtual int32_t EmptyBuffer(int32_t type, native_handle_t const* hnd, uint32_t flag = 0);
        virtual int32_t ChangePlaneShow(int32_t flags);
        virtual int32_t SetPowerMode(int32_t mode);
        virtual int32_t GetDisplayConfigs(int32_t *num_config, dm_display_attribute **configs);
        virtual int32_t setActiveConfig(int32_t index);
        virtual String8 Dump(void);
};
