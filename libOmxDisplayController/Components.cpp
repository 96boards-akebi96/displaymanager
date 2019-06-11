// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2016 Socionext Inc.

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>

//#define LOG_NDEBUG 0   //Open ALOGV
//#define LOG_FUNCTION_DEBUG   //Open Function Trace
#define LOG_TAG "BaseComponent"

#include "Components.h"

// VDC component
BasePort *VdcComponent::AddPort(int w, int h, int fr,
    OMX_NATIVE_DEVICETYPE_VDC devtype) {
    
    ATRACE_CALL();
    int index = GetPortIndex();
    int ret = 0;
    BasePort *port = new VdcPort(this, w, h, fr, index, devtype);
    if (port == NULL) {
        ALOGE("Can't create port");
        return (BasePort *)NULL;
    }
    ret = SetParamPortDefinition(port);
    if (ret < 0) {
        delete port;
        return (BasePort *)NULL;
    }
    return port;
}

int VdcComponent::ConfigCommonPlaneBlend(BasePort *port, int depth)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_CONFIG_PLANEBLENDTYPE     plane_blend;

    /* Plane Blend Type */
    plane_blend.nSize  = sizeof( plane_blend );
    omx_init_version( &plane_blend.nVersion );
    plane_blend.nPortIndex = port->GetIndex();
    plane_blend.nDepth = depth;
    ret = OMX_SetConfig( GetHandle(), OMX_IndexConfigCommonPlaneBlend, &plane_blend );
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_SetConfig(OMX_IndexConfigCommonPlaneBlend) for %s failed : ret = 0x%x.\n", GetComponentName(), (unsigned int)ret );
        return ( -1 );
    }
    return 0;
}

int VdcComponent::ConfigCommonMute(BasePort *port, OMX_MUTETYPE mute_type, OMX_U32 color)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_CONFIG_MUTETYPE mute;

    mute.nSize = sizeof(mute);
    omx_init_version(&mute.nVersion);
    mute.nPortIndex    = port->GetIndex();
    mute.eEnableMute   = mute_type;
    mute.nMuteRGBColor = color;

    ret = OMX_SetConfig( GetHandle(), (OMX_INDEXTYPE)OMX_IndexConfigCommonMute, &mute);
    if (ret != OMX_ErrorNone) {
        ALOGE( "OMX_SetConfig(OMX_IndexConfigCommonMute) for %s failed : ret = 0x%x.\n", GetComponentName(), (unsigned int)ret );
        return ( -1 );
    }
    port->SetMuteType(mute_type);
    return 0;
}

