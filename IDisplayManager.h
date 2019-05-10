#ifndef __IDISPLAY_MANAGER_H__
#define __IDISPLAY_MANAGER_H__

#include <stdlib.h>

#include "utils/RefBase.h"
#include <log/log.h>

#include <binder/IInterface.h>
#include <binder/IBinder.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include <libDisplayManagerClient.h>

using namespace android;

typedef struct dm_stream_info_ {
    int32_t width;
    int32_t height;
#if 1 // for No.128 backlight base
    int32_t input_frame_rate;
    int32_t output_frame_rate;
#endif
    int32_t color_format;
    int32_t ip;
    int32_t surfaceX;
    int32_t surfaceY;
    int32_t surfaceWidth;
    int32_t surfaceHeight;
    int32_t plane_show_status;
} dm_stream_info;

#if 1 // for No.149
typedef struct {
    uint32_t x;
    uint32_t y;
} dm_DisplayPrimaries;

typedef struct {
    dm_DisplayPrimaries display_primaries[3];
    uint32_t white_point_x;
    uint32_t white_point_y;
    uint32_t max_display_mastering_luminance;
    uint32_t min_display_mastering_luminance;
} dm_MasteringDisplayColourVolume;

typedef struct
{
    uint32_t max_content_light_level;
    uint32_t max_pic_average_light_level;
} dm_ContentLightLevelInfo;

typedef struct
{
    uint32_t                        preferred_transfer_characteristics_present;
    uint32_t                        preferred_transfer_characteristics;

    uint32_t                        mastering_display_colour_volume_present;
    dm_MasteringDisplayColourVolume mastering_display_colour_volume;

    uint32_t                        content_light_level_info_present;
    dm_ContentLightLevelInfo        content_light_level_info;
} dm_HDRExtInfo;

typedef struct {
    uint32_t      colour_primaries;
    uint32_t      transfer_characteristics;
#if 1 // for No.149 OMX I/F add
    uint32_t      dynamic_range;
    uint32_t      input_source;
#endif
    dm_HDRExtInfo hdr_ext_info;
} dm_hdr_info_t;
#endif // for No.149

typedef struct dm_display_attribute_ {
    int32_t xres;
    int32_t yres;
    int32_t losd_xres;
    int32_t losd_yres;
    float fps;
} dm_display_attribute;

// Interface (our AIDL) - Shared by server and client
class IDisplayManager : public IInterface {
    public:
        enum {
            EMPTY_BUFFER = IBinder::FIRST_CALL_TRANSACTION,
            CHANGE_PLANE_SHOW,
            SET_POWER_MODE,
            GET_DISPLAY_CONFIGS,
            SET_ACTIVE_CONFIG,
            DUMP,
        };

        enum {
            TYPE_OSD   = 0x01,
        } BUFFER_TYPE;

        virtual int32_t EmptyBuffer(int32_t type, native_handle_t const*hnd, uint32_t flag = 0) = 0;
        virtual int32_t ChangePlaneShow(int32_t flags) = 0;
        virtual int32_t SetPowerMode(int32_t mode) = 0;
        virtual int32_t GetDisplayConfigs(int32_t *num_config, dm_display_attribute **configs) = 0;
        virtual int32_t setActiveConfig(int32_t index) = 0;
        virtual String8 Dump(void) = 0;
        DECLARE_META_INTERFACE(DisplayManager);  // Expands to 5 lines below:
};

extern sp<IDisplayManager> getDemoServ();

#endif /* __IDISPLAY_MANAGER_H__ */
