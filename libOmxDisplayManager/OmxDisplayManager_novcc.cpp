#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>

#define LOG_TAG "OmxDisplayManager_novcc"

#include "OmxDisplayManager.h"
#include "IDisplayManager.h"
#include <hardware/hwcomposer.h>
#include <string>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

OmxDisplayManager::OmxDisplayManager(
    int w, int h, int fr,
    OMX_OUTPUTFORMATTYPE fmt,
    OMX_OUTPUTDEVICETYPE dev,
    std::string Mname,
    OMX_OUTPUTMODETYPE Omd,
    OMX_U32 FWA,
    OMX_U32 FHA
    )
{
    ATRACE_CALL();
    VdcOutWidth     = w;
    VdcOutHeight    = h;
    VdcFrameRate    = fr;
    VdcOutputFormat = fmt;
    VdcOutputDevice = dev;
    VdcOutputMode   = Omd;
    VdcAdancedtype.nFrameWidthAll   = FWA; 
    VdcAdancedtype.nFrameHeightAll  = FHA; 
    VdcModeName     = Mname;
    streamInfoWidth = -1;
    streamInfoHeight= -1;
    streamInfoFrameRate = -1;
    streamInfoColorFormat = -1;
    streamInfoIP = -1;
    surfaceX = -1;
    surfaceY = -1;
    surfaceWidth = -1;
    surfaceHeight = -1;
}

OmxDisplayManager::~OmxDisplayManager()
{
    ATRACE_CALL();
}

static int setFrameParameter(OMX_NATIVE_DEVICETYPE_VDC *devtype, int w, int h, int fr)
{
    if ((w == 3840) && (h == 2160)) {
        devtype->eOutputSignal.nPixelClock          = 594000;
        devtype->eOutputSignal.nHorizontalSync      = 88;
        devtype->eOutputSignal.nHorizontalStart     = 384;
        devtype->eOutputSignal.nHorizontalActive    = 3840;
        devtype->eOutputSignal.nVerticalStart       = 82;
        devtype->eOutputSignal.nVerticalSync        = 20;
        devtype->eOutputSignal.nVerticalActive      = 2160;
        devtype->eOutputPanel.eVboColorDepth        = OMX_PANEL_VboColor10bit;
        devtype->eOutputPanel.nSsc                  = 0;
        devtype->eOutputSignal.nVerticalTotal          = 2250;
        devtype->eOutputSignal.nVerticalProtectMaximum = 2250;
        devtype->eOutputSignal.nVerticalProtectMinimum = 2250;

        switch (fr) {
            case 0x00320000: // 50.00Hz
                devtype->eOutputSignal.ePixelClockAdjustedBy1000Over1001 = OMX_FALSE;
                devtype->eOutputSignal.nHorizontalTotal        = 5280;
                break;
            case 0x003BF000: // 59.94Hz
                devtype->eOutputSignal.ePixelClockAdjustedBy1000Over1001 = OMX_TRUE;
                devtype->eOutputSignal.nHorizontalTotal        = 4400;
                break;
            case 0x003C0000: // 60.00Hz
                devtype->eOutputSignal.ePixelClockAdjustedBy1000Over1001 = OMX_FALSE;
                devtype->eOutputSignal.nHorizontalTotal        = 4400;
                break;
            default:
                ALOGE( "%s:%d error", __func__, __LINE__);
                return ( -1 );
        }
    } else {
        ALOGE( "%s:%d error", __func__, __LINE__);
        return ( -1 );
    }
    return 0;
}


int OmxDisplayManager::init()
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    int err = 0;
    
    ALOGI("init_omx() in hwcomposer_omx.cpp START");

    ret = OMX_Init();
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_Init failed.\n" );
        return ( -1 );
    }

    vdc = new VdcComponent();
    
    err = vdc->init();
    if (err < 0) {
        ALOGE( "%s:%d error", __func__, __LINE__);
        return ( err );
    }
    
    memset(&devtype, 0, sizeof(devtype));
    devtype.eOutputDevice     = VdcOutputDevice;
    devtype.eOutputFormat     = VdcOutputFormat;
    devtype.eOutputMode       = VdcOutputMode;
    devtype.nCompositePattern  = (OMX_COMPOSITEPATTERNTYPE)(OMX_CompositePatternPort1Video | OMX_CompositePatternPort2Video 
                                        | OMX_CompositePatternPort3ImagePostPQ | OMX_CompositePatternPort4ImagePostPQ);
    devtype.eOutputPanel.nHorizontalDivision       = 1;
    devtype.eOutputPanel.bHorizontalReverse        = OMX_FALSE;
    devtype.eOutputPanel.bVerticalReverse          = OMX_FALSE;
    devtype.eOutputPanel.nVboLaneSwap[0]       = 0;
    devtype.eOutputPanel.nVboLaneSwap[1]       = 1;
    devtype.eOutputPanel.nVboLaneSwap[2]       = 2;
    devtype.eOutputPanel.nVboLaneSwap[3]       = 3;
    devtype.eOutputPanel.nVboLaneSwap[4]       = 4;
    devtype.eOutputPanel.nVboLaneSwap[5]       = 5;
    devtype.eOutputPanel.nVboLaneSwap[6]       = 6;
    devtype.eOutputPanel.nVboLaneSwap[7]       = 7;
    devtype.eOutputPanel.eVboLaneSwitch[0]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSwitch[1]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSwitch[2]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSwitch[3]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSwitch[4]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSwitch[5]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSwitch[6]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSwitch[7]     = OMX_PANEL_LaneSwitchOn;
    devtype.eOutputPanel.eVboLaneSubSwitch[0]  = OMX_PANEL_LaneSwitchOff;
    devtype.eOutputPanel.eVboLaneSubSwitch[1]  = OMX_PANEL_LaneSwitchOff;
    devtype.eOutputPanel.eVboLaneSubSwitch[2]  = OMX_PANEL_LaneSwitchOff;
    devtype.eOutputPanel.eVboLaneSubSwitch[3]  = OMX_PANEL_LaneSwitchOff;
    devtype.eOutputPanel.eVboLaneSrsSig[0]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneSrsSig[1]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneSrsSig[2]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneSrsSig[3]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneSrsSig[4]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneSrsSig[5]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneSrsSig[6]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneSrsSig[7]     = OMX_PANEL_VboSrsSig450mV;
    devtype.eOutputPanel.eVboLaneEmLevel[0]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneEmLevel[1]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneEmLevel[2]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneEmLevel[3]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneEmLevel[4]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneEmLevel[5]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneEmLevel[6]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneEmLevel[7]    = OMX_PANEL_EmLevel0mV;
    devtype.eOutputPanel.eVboLaneCommonEmTimeLevel = OMX_PANEL_EmTimeLevel0;
    devtype.eOutputPanel.eVboLanePolarity[0]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eVboLanePolarity[1]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eVboLanePolarity[2]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eVboLanePolarity[3]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eVboLanePolarity[4]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eVboLanePolarity[5]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eVboLanePolarity[6]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eVboLanePolarity[7]   = OMX_PANEL_PolarityPositive;
    devtype.eOutputPanel.eOutputClock          = OMX_PANEL_ClockTypeInternal;
    devtype.eOutputPanel.bDoublerSwitch        = OMX_FALSE;
    
    err = setFrameParameter(&devtype, VdcOutWidth, VdcOutHeight, VdcFrameRate);
    if (err < 0 ) {
        ALOGE( "%s:%d error", __func__, __LINE__);
        return ( err );
    }

    VdcPort0 = (VdcPort *)vdc->AddPort(VdcOutWidth, VdcOutHeight, VdcFrameRate,
                                       devtype);
    if (VdcPort0 == (VdcPort *)NULL) {
        ALOGE( "%s:%d error", __func__, __LINE__);
        return ( -1 );
    }

    return 0;
}

int OmxDisplayManager::SetupTunnelConnect(
    BaseComponent *cmp1, BasePort *cmp1_port,
    BaseComponent *cmp2, BasePort *cmp2_port)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    ret = OMX_SetupTunnel( cmp1->GetHandle(), cmp1_port->GetIndex(),
            cmp2->GetHandle(), cmp2_port->GetIndex() );
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_SetupTunnel() for %s and %s failed : ret = 0x%x.\n",
            cmp1->GetComponentName(), cmp2->GetComponentName(), (unsigned int)ret );
        return ( -1 );
    }
    return 0;
}

int OmxDisplayManager::deinit()
{
    ATRACE_CALL();
    vdc->deinit();
    
    delete VdcPort0;
    delete vdc;
    return 0;
}

int OmxDisplayManager::start()
{
    ATRACE_CALL();
    int ret = 0;
    ret = vdc->SetState(OMX_StateIdle, true);
    if (ret < 0) {
        ALOGE("%s:%d Error", __func__, __LINE__);
    }

    ret = vdc->SetState(OMX_StateExecuting, true);
    if (ret < 0) {
        ALOGE("%s:%d Error", __func__, __LINE__);
    }
    
    return 0;
}

int OmxDisplayManager::stop()
{
    ATRACE_CALL();
    int ret = 0;
    ret = vdc->SetState(OMX_StateIdle, true);
    if (ret < 0) {
        ALOGE("%s:%d Error", __func__, __LINE__);
    }

    ret = vdc->SetState(OMX_StateLoaded, true);
    if (ret < 0) {
        ALOGE("%s:%d Error", __func__, __LINE__);
    }
    
    return 0;
}

PortSet *OmxDisplayManager::AddOsdPort(int w, int h, unsigned int CFormat, int PlaneOrder)
{
    ATRACE_CALL();
    OMX_CONFIG_RECTTYPE in, out;

    // Dummy Inc
    vdc->GetPortIndex();
    vdc->GetPortIndex();

    BasePort *VdcPort = vdc->AddPort(w, h, CFormat, OMX_DirInput);
    
    vdc->SetRect(&in,  VdcPort, 0, 0, w, h);
    vdc->SetRect(&out, VdcPort, 0, 0, VdcOutWidth, VdcOutHeight);
    vdc->UpdateCrop(&in, &out);
    OsdWidth  = w;
    OsdHeight = h;
    
    vdc->ConfigCommonPlaneBlend(VdcPort, PlaneOrder);
    vdc->ConfigCommonMute(VdcPort, OMX_MuteBlack, 0);

    PortSet *ps = new PortSet(vdc, VdcPort);
    if (ps == NULL) {
         ALOGE( "%s:%d error", __func__, __LINE__);
    }
    return ps;
}

int OmxDisplayManager::UpdateCrop(PortSet *ps,
    int in_x,  int in_y,  int in_w,  int in_h,
    int out_x, int out_y, int out_w, int out_h,
    int /* transform */, int blending, int planeAlpha)
{
    ATRACE_CALL();
    OMX_CONFIG_RECTTYPE in, out;
    BaseComponent *cmp = ps->GetTopComponent();
    int magnification = 1;
    
    if ( (OsdWidth == 1920) && (OsdHeight == 1080)
        && (VdcOutWidth == 3840) && (VdcOutHeight == 2160) ) {
        magnification = 2; 
    }

    surfaceX = out_x;
    surfaceY = out_y;
    surfaceWidth = out_w;
    surfaceHeight = out_h;

    cmp->SetRect(&in,  ps->GetPort(), in_x, in_y, in_w, in_h);
    cmp->SetRect(&out, ps->GetPort(), out_x*magnification, out_y*magnification, out_w*magnification, out_h*magnification);
    cmp->UpdateCrop(&in, &out);
    ChangeColorBlend(ps, (OMX_COLORBLENDTYPE)blending, planeAlpha);
    return 0;
}

int OmxDisplayManager::ChangePlaneShow(PortSet *ps, OMX_PLANESHOWTYPE type)
{
    ATRACE_CALL();
    if (type == OMX_Show) {
        vdc->ConfigCommonMute(ps->GetPort(vdc), OMX_MuteDisable, 0);
    } else {
        vdc->ConfigCommonMute(ps->GetPort(vdc), OMX_MuteColored, 0x00000000);
    }
    return 0;
}

int OmxDisplayManager::ChangeColorBlend(PortSet * /* ps */, OMX_COLORBLENDTYPE /* type */, OMX_U32 /* constant */)
{
    ATRACE_CALL();
    // vdc->ConfigCommonColorBlend(ps->GetPort(vdc), type, constant);
    return 0;
}

int OmxDisplayManager::ChangeMute(PortSet *ps, OMX_MUTETYPE mute_type, OMX_U32 color)
{
    ATRACE_CALL();
    BasePort *port = ps->GetPort(vdc);
    if (port->GetMuteType() != mute_type) {
        vdc->ConfigCommonMute(port, mute_type, color);
    }
    return 0;
}

int OmxDisplayManager::l_SetPowerMode(int mode)
{
    ATRACE_CALL();
    int ret = 0;
    switch (mode) {
        case HWC_POWER_MODE_OFF:
            // Save current value
            ALOGI("SetPowerMode OFF");
            break;
        default:
            ALOGI("SetPowerMode ON");
            break;
    }
    return ret;
}

int OmxDisplayManager::SetPowerMode(int mode)
{
    ATRACE_CALL();
    int ret = 0;
    ret = l_SetPowerMode(mode);
    return ret;
}


int OmxDisplayManager::setActiveConfig(dm_display_attribute *dda)
{
    int ret = 0;
    int fr = 0;

    if (dda->fps == 60.0f) {
        fr = 0x003C0000;
    } else if (dda->fps == (60.0f*1000/1001)) {
        fr = 0x003BF000;
    } else if (dda->fps == 50.0f) {
        fr = 0x00320000;
    } else {
        ALOGE( "%s:%d error : fps : %f", __func__, __LINE__, dda->fps);
        return ( -1 );
    }

    ret = setFrameParameter(&devtype, dda->xres, dda->yres, fr);
    if (ret < 0 ) {
        ALOGE( "%s:%d error", __func__, __LINE__);
        return ( ret );
    }

    // Port Disable
    VdcPort0->Disable(vdc);
    VdcPort0->UpdateDeviceType(devtype);
    ret = VdcPort0->UpdateInfo(dda->xres, dda->yres, fr, OMX_COLOR_Format32bitARGB8888);
    if (ret < 0) {
        ALOGE("%s:%d Error", __func__, __LINE__);
        return (-1);
    }
    ret = vdc->SetParamPortDefinition(VdcPort0);
    if (ret < 0) {
        ALOGE("%s:%d Error", __func__, __LINE__);
        return (-1);
    }
    // Port Enable
    VdcPort0->Enable(vdc);

    return 0;
}

int32_t OmxDisplayManager::dump(android::String8 &out)
{
    out.append("OmxDisplayManager:\n");
    if (VdcPort0 != NULL) {
        out.append("VdcPort0:\n");
        VdcPort0->dump(out);
    }
    return 0;
}

int PortSet::stop()
{
    ATRACE_CALL();
    int ret = 0;
    for(auto item = ComponentList.begin(); item != ComponentList.end(); --item) {
        BaseComponent *comp = ((ComponentItem *)(*item))->Component;
        if (comp->GetState() == OMX_StateExecuting) {
            ret = comp->SetState(OMX_StateIdle, true);
            if (ret < 0) {
                ALOGE("Can't change OMX_StateIdle in %s", comp->GetComponentName());
                return (-1);
            }
        }
    }
    for(auto item = ComponentList.begin(); item != ComponentList.end(); --item) {
        BaseComponent *comp = ((ComponentItem *)(*item))->Component;
        if (comp->GetState() == OMX_StateIdle) {
            ret = comp->SetState(OMX_StateLoaded, true);
            if (ret < 0) {
                ALOGE("Can't change OMX_StateLoaded in %s", comp->GetComponentName());
                return (-1);
            }
        }
    }
    return 0;
}

int PortSet::start()
{
    ATRACE_CALL();
    int ret = 0;
    for(auto item = ComponentList.begin(); item != ComponentList.end(); ++item) {
        BaseComponent *comp = ((ComponentItem *)(*item))->Component;
        if (!comp->isVdc()) {
            if (comp->GetState() == OMX_StateLoaded) {
                ret = comp->SetState(OMX_StateIdle, true);
                if (ret < 0) {
                    ALOGE("Can't change OMX_StateIdle in %s", comp->GetComponentName());
                    return (-1);
                }
            }
        }
    }
    for(auto item = ComponentList.begin(); item != ComponentList.end(); ++item) {
        BaseComponent *comp = ((ComponentItem *)(*item))->Component;
        if (!comp->isVdc()) {
            if (comp->GetState() == OMX_StateIdle) {
                ret = comp->SetState(OMX_StateExecuting, true);
                if (ret < 0) {
                    ALOGE("Can't change OMX_StateExecuting in %s", comp->GetComponentName());
                    return (-1);
                }
            }
        }
    }
    return 0;
}


BasePort *PortSet::GetPort(BaseComponent *comp, int portid)
{
    ATRACE_CALL();
    ComponentItem *item = NULL;
    if (comp == NULL) {
        item = *ComponentList.begin();
    } else {
        std::list<ComponentItem *>::iterator cmp = ComponentList.begin();
        for(; cmp != ComponentList.end(); ++cmp) {
            item = *cmp;
            if (comp == item->Component) {
                break;
            }
        }
        if (cmp == ComponentList.end()) {
            ALOGE("Can't find component!!!");
            return (BasePort *)NULL;
        } 
    }
    switch (portid) {
        case PORTSET_COMP_IN:
            return item->In;
        case PORTSET_COMP_OUT:
            return item->Out;
    }
    return (BasePort *)NULL;
}

int32_t PortSet::dump(android::String8 &out)
{
    for(auto item = ComponentList.begin(); item != ComponentList.end(); ++item) {
        BaseComponent *comp = ((ComponentItem *)(*item))->Component;
        BasePort *In = ((ComponentItem *)(*item))->In;
        BasePort *Out = ((ComponentItem *)(*item))->Out;
        out.appendFormat("Component name : %s\n", comp->GetComponentName());
        if (In != NULL) {
            out.append("In port\n");
            In->dump(out);
        }
        if (Out != NULL) {
            out.append("Out port\n");
            Out->dump(out);
        }
    }
    return 0;
}

int PortSet::enable()
{
    ATRACE_CALL();
    android::String8 out;
    for(auto item = ComponentList.rbegin(); item != ComponentList.rend(); ++item) {
        BaseComponent *comp = ((ComponentItem *)(*item))->Component;
        out.appendFormat("%s -> ", comp->GetComponentName());
        if (comp->isVdc()) {
            GetPort(comp, PORTSET_COMP_IN)->Enable(comp);
        }
    }
    ALOGI("%s:%d : %s", "PortSet::enable", __LINE__, out.string());
    return 0;
}

int PortSet::disable()
{
    ATRACE_CALL();
    android::String8 out;
    for(auto item = ComponentList.begin(); item != ComponentList.end(); ++item) {
        BaseComponent *comp = ((ComponentItem *)(*item))->Component;
        out.appendFormat("%s <- ", comp->GetComponentName());
        if (comp->isVdc()) {
            GetPort(comp, PORTSET_COMP_IN)->Disable(comp);
        }
    }
    ALOGI("%s:%d : %s", "PortSet::disable", __LINE__, out.string());
    return 0;
}
