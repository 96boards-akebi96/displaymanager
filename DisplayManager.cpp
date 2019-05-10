
//#define LOG_NDEBUG 0   //Open ALOGV
//#define LOG_FUNCTION_DEBUG   //Open Function Trace
#define LOG_TAG "DisplayManager"

#include <cutils/properties.h>
#include "DisplayManager.h"
#include "libDisplayManagerClient.h"
#include <hardware/gralloc.h>
#include "mali_gralloc_module.h"
#include "gralloc_priv.h"
#include <sys/mman.h>

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>

static dm_display_attribute gConfig[] = {
    {0, 0, 0, 0, 0.0f}, // boot attribute
    {3840, 2160, 1920, 1080, 60.000f}, // 4K 60fps
    {3840, 2160, 1920, 1080, (60.0f*1000/1001)}, // 4K 59.94fps
    {3840, 2160, 1920, 1080, 50.000f}, // 4K 50.0fps
};

DisplayManager::DisplayManager()
{
    ATRACE_CALL();
    PlaneShowStatus = 0;
    omx_display = NULL;
    OsdPortSet = NULL; 
    current_index = 0; // index of boot attribute
    pthread_mutex_init(&initialized_mutex, NULL);
    pthread_mutex_init(&change_port_setting_mutex, NULL);
    pthread_cond_init(&change_port_setting_wait, NULL);
    initialized = false;
}

DisplayManager::~DisplayManager()
{
    ATRACE_CALL();
}

void DisplayManager::init()
{
    ATRACE_CALL();
    int VdcOutWidth  = 0;
    int VdcOutHeight = 0;
    int VdcFrameRate = 0;
    OMX_OUTPUTFORMATTYPE VdcOutputFormat = OMX_OutputFormatRGB;
    OMX_OUTPUTDEVICETYPE VdcOutputDevice = OMX_OutputDeviceVBYONE;
    std::string VdcOutputDeviceName = "";
    
    ALOGI(  "DisplayManager's main thread ready to run. "
        "Initializing OpenMAX and H/W...");
        
    {
        VdcFrameRate = 0x003C0000;  // 60 Hz
        gConfig[0].fps = 60.0f;

        VdcOutWidth  = 3840;
        VdcOutHeight = 2160;
        gConfig[0].xres = VdcOutWidth;
        gConfig[0].yres = VdcOutHeight;

        VdcOutputDevice = OMX_OutputDeviceVBYONE;
        VdcOutputFormat = OMX_OutputFormatRGB;
    }

    omx_display = new OmxDisplayManager(
                        VdcOutWidth, VdcOutHeight, VdcFrameRate,
                        VdcOutputFormat, VdcOutputDevice, VdcOutputDeviceName);
    omx_display->init();

    // Set LOSD
    gConfig[0].losd_xres = 1920;
    gConfig[0].losd_yres = 1080;

    OsdPortSet   = omx_display->AddOsdPort(gConfig[0].losd_xres,gConfig[0].losd_yres,OMX_COLOR_Format32bitARGB8888, 0);

    ALOGV("%s: init finished", __FUNCTION__);
}

void DisplayManager::deinit()
{
    ALOGV("%s: deinit started", __FUNCTION__);

    omx_display->stop();
    omx_display->deinit();
    OMX_Deinit();

    delete OsdPortSet;
    delete omx_display;

    ALOGV("%s: deinit finished", __FUNCTION__);
}


void DisplayManager::run(bool block)
{
    ALOGV("%s: start", __FUNCTION__);

    omx_display->start();
    pthread_mutex_lock(&initialized_mutex);
    initialized = true;
    pthread_mutex_unlock(&initialized_mutex);
    do {
        sleep(1);
    } while (block);
}

int DisplayManager::GetComponentAndPort(int32_t type, BaseComponent **cmp, BasePort **port, PortSet **ps)
{
    ATRACE_CALL();
    if (cmp != NULL) *cmp = (BaseComponent *)NULL;
    if (port != NULL) *port = (BasePort *)NULL;
    if (ps != NULL) *ps = (PortSet *)NULL;
    switch (type) {
        case TYPE_OSD:
            if (cmp != NULL) *cmp = OsdPortSet->GetTopComponent();
            if (port != NULL) *port = OsdPortSet->GetPort();
            if (ps != NULL) *ps = OsdPortSet;
            break;
        default:
            ALOGE("GetComponentAndPort::Unknown type : %d", type);
            return -1;
    }
    return 0;
}


int32_t DisplayManager::EmptyBuffer(int32_t type, native_handle_t const* hnd, uint32_t flag)
{
    ATRACE_CALL();
    int32_t ret = 0;
    BaseComponent *cmp = (BaseComponent *)NULL;
    BasePort *port = (BasePort *)NULL;
    PortSet *ps = (PortSet *)NULL;
    char *buffer = (char *)NULL;
    int len = 0;

    OMX_SNIVS_OUTPORT_PRIVATE_t *option = (OMX_SNIVS_OUTPORT_PRIVATE_t *)NULL;
    private_handle_t const* handle = reinterpret_cast<private_handle_t const*>(hnd);
    bool blocked = false;
    
    ret = GetComponentAndPort(type, &cmp, &port, &ps);
    if (ret < 0) {
        return -1;
    }
    switch (type) {
        case TYPE_OSD:
            buffer = (char *)handle->pbase;
            len = handle->size;
            blocked = true;
            break;
        default:
            ALOGE("EmptyBuffer::Unknown type : %d", type);
            break;
    }
    // ALOGI("DisplayManager::EmptyBuffer(%d, %08x, %d %p, %d)", type, buffer, len, option, cmp->GetState());
    ret = cmp->EmptyBuffer(port, (char*)buffer, len, option, blocked, flag);

    switch (type) {
        case TYPE_OSD:
            // Mute OFf
            omx_display->ChangeMute(ps, OMX_MuteDisable, 0);
            break;
    }

    return  ret;
}

int32_t DisplayManager::ChangePlaneShow(int32_t flags)
{
    ATRACE_CALL();
    int32_t ret;
    BaseComponent *cmp = (BaseComponent *)NULL;
    BasePort *port = (BasePort *)NULL;
    PortSet *ps = (PortSet *)NULL;

//    ALOGI("Plane Show %02x", flags);
    ret = GetComponentAndPort(TYPE_OSD,&cmp,&port,&ps);
    if (ret < 0) {
        return -1;
    }
    if (flags & TYPE_OSD) {
        if ((PlaneShowStatus & TYPE_OSD) == 0) {
            omx_display->ChangePlaneShow(ps, OMX_Show);
            PlaneShowStatus |= TYPE_OSD;
            ALOGI("%s:%d Plane Show %02x", __FUNCTION__, __LINE__, flags);
        }
    } else {
        if (PlaneShowStatus & TYPE_OSD) {
            omx_display->ChangePlaneShow(ps, OMX_Hide);
            PlaneShowStatus &= ~TYPE_OSD;
            ALOGI("%s:%d Plane Show %02x", __FUNCTION__, __LINE__, flags);
        }
    }
    if (PlaneShowStatus & TYPE_OSD) {
        omx_display->ChangeColorBlend(ps, OMX_ColorBlendNone);
    }
    ATRACE_INT("PlaneShowStatus", PlaneShowStatus);
//    ALOGI("%s:%d Plane Show PlaneShowStatus :%02x", __FUNCTION__, __LINE__, PlaneShowStatus);
    return ret;
}

int32_t DisplayManager::SetPowerMode(int32_t mode)
{
    ATRACE_CALL();
    int32_t ret;

    ALOGI("SetPowerMode %04x", mode);
    ret = omx_display->SetPowerMode(mode);
    return ret;
}

int32_t DisplayManager::GetDisplayConfigs(int32_t *num_config, dm_display_attribute **configs)
{
    ATRACE_CALL();
    int ret = 0;
    *num_config = (sizeof(gConfig)/sizeof(gConfig)[0]);
    *configs = &gConfig[0];
    return ret;
}

int32_t DisplayManager::setActiveConfig(int32_t index)
{
    ATRACE_CALL();
    int ret = 0;
    if (index == current_index) {
        // Not change
        ALOGW("setActiveConfig : no need to change : cur_index = %d, request = %d", current_index, index);
        return ret;
    }
    if ((index < 0) || (index >= (int)(sizeof(gConfig)/sizeof(gConfig)[0]))) {
        ALOGE("%s:%d index error : %d", __func__, __LINE__, index);
        return -1;
    }

    ret = omx_display->setActiveConfig(&gConfig[index]);
    if (ret < 0) {
        ALOGE("%s:%d setActiveConfig error : %d", __func__, __LINE__, index);
        return -1;
    }
    current_index = index;
    return ret;
}

String8 DisplayManager::Dump(void)
{
    String8 out;
    out.append("PlaneShowStatus : ");
    out.appendFormat("%s", ((PlaneShowStatus & TYPE_OSD) == TYPE_OSD) ? "OSD" : "");
    out.append("\n");
    omx_display->dump(out);
    if (OsdPortSet != NULL) {
        out.append("OSD Port Set ---------------------------\n");
        OsdPortSet->dump(out);
    }
    return out;
}

status_t DisplayManager::dump(int fd, const Vector<String16>& /* args */)
{
    String8 out = Dump();
    write(fd, out.string(), out.size());
    return OK;
}

status_t BnDisplayManager::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags) {
    ATRACE_CALL();
    // ALOGD("BnDisplayManager::onTransact(%i) %i", code, flags);

    switch(code) {
        case EMPTY_BUFFER: {
            int ret;
            if (initialized == false) {
                ALOGV("onTransact still not initialized : %d", initialized);
                return NO_ERROR;
            }
            pthread_mutex_lock(&change_port_setting_mutex);
            while (change_port_setting == 1) {
                // wait
                pthread_cond_wait( &change_port_setting_wait, &change_port_setting_mutex );
            }
            empty_buffer = 1;
            pthread_mutex_unlock(&change_port_setting_mutex);
            data.checkInterface(this);
            int32_t type = data.readInt32();
            native_handle_t *hnd = data.readNativeHandle();
            if (hnd == NULL) {
                ALOGV("onTransact EMPTY_BUFFER native_handle NULL");
                return NO_ERROR;
            }
            uint32_t flag = data.readInt32();
            ret = EmptyBuffer(type, hnd, flag);
            reply->writeInt32(ret);
            native_handle_close(hnd);
            native_handle_delete(hnd);
            pthread_mutex_lock(&change_port_setting_mutex);
            empty_buffer = 0;
            pthread_cond_signal( &change_port_setting_wait );  /* pthread_cond_wait() before hwc_Vec_emptyThisBuffer */
            pthread_mutex_unlock(&change_port_setting_mutex);
            return NO_ERROR;
        } break;
        case CHANGE_PLANE_SHOW: {
            int ret;
            pthread_mutex_lock(&initialized_mutex);
            data.checkInterface(this);
            int32_t flags = data.readInt32();
            ret = ChangePlaneShow(flags);
            reply->writeInt32(ret);
            pthread_mutex_unlock(&initialized_mutex);
            return NO_ERROR;
        } break;
        case SET_POWER_MODE: {
            int ret;
            pthread_mutex_lock(&initialized_mutex);
            data.checkInterface(this);
            int32_t mode = data.readInt32();
            ret = SetPowerMode(mode);
            reply->writeInt32(ret);
            pthread_mutex_unlock(&initialized_mutex);
            return NO_ERROR;
        } break;
        case GET_DISPLAY_CONFIGS:{
            dm_display_attribute *dda;
            int32_t num_config;
            int ret = 0;
            if (initialized == false) {
                ALOGV("onTransact still not initialized : %d", initialized);
                reply->writeInt32(0);
                reply->writeInt32(INVALID_OPERATION);
                return NO_ERROR;
            }
            data.checkInterface(this);
            ret = GetDisplayConfigs(&num_config, &dda);
            reply->writeInt32(num_config);
            void *buf = reply->writeInplace((size_t)(num_config*sizeof(dm_display_attribute)));
            memcpy(buf, dda, num_config*sizeof(dm_display_attribute));
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case SET_ACTIVE_CONFIG:{
            int ret = 0;
            if (initialized == false) {
                ALOGV("onTransact still not initialized : %d", initialized);
                return NO_ERROR;
            }
            data.checkInterface(this);
            int32_t index = data.readInt32();
            ret = setActiveConfig(index);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case DUMP:{
            String8 out = Dump();
            reply->writeString8(out);
            return NO_ERROR;
        }
        default:
            int ret;
            pthread_mutex_lock(&initialized_mutex);
            ret = BBinder::onTransact(code, data, reply, flags);
            pthread_mutex_unlock(&initialized_mutex);
            return ret;
    }
    return -1;
}

