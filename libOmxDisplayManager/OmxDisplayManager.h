// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2016 Socionext Inc.

#ifndef __OMX_DISPLAY_MANAGER_H__
#define __OMX_DISPLAY_MANAGER_H__

#include "Components.h"
#include <list>

#include "IDisplayManager.h"

class PortSet {
    private:
    public:
        enum {
            PORTSET_COMP_IN = 0,
            PORTSET_COMP_OUT
        };
        typedef struct ComponentItem {
            BaseComponent *Component;
            BasePort *In;
            BasePort *Out;
        } ComponentItem;
        std::list<ComponentItem *> ComponentList;
        
        PortSet() {};
        ~PortSet() {};

        void PushBack(BaseComponent *comp,
                BasePort *PortIn,
                BasePort *PortOut)
        {
            ComponentItem *item = new ComponentItem;
            item->Component = comp;
            item->In  = PortIn;
            item->Out = PortOut;
            ComponentList.push_back(item);
        }
        void PushFront(BaseComponent *comp,
                BasePort *PortIn,
                BasePort *PortOut)
        {
            ComponentItem *item = new ComponentItem;
            item->Component = comp;
            item->In  = PortIn;
            item->Out = PortOut;
            ComponentList.push_front(item);
        }
        
        BaseComponent *GetComponent(int count) {
            std::list<ComponentItem *>::iterator it = ComponentList.begin();
            ComponentItem *item;
            while (count > 0) {
                it++;
                count--;
            }
            item = *it;
            return item->Component;
        };
        
        BaseComponent *GetTopComponent() {
            return GetComponent(0);
        };
        
        PortSet(BaseComponent *comp,
                BasePort *PortIn,
                BasePort *PortOut = NULL) {
            PushBack(comp, PortIn, PortOut);
        };
        int start();
        int stop();
        int ChangeToLoaded(BaseComponent *comp);
        int ChangeToExecuting(BaseComponent *comp);
        int disable();
        int enable();

        BasePort *GetPort(BaseComponent *comp, int portid = PORTSET_COMP_IN);
        BasePort *GetPort(int portid = PORTSET_COMP_IN) {
            return GetPort(NULL, portid);
        };
        int32_t dump(android::String8 &out);
};

class OmxDisplayManager {
    private:
        int VdcOutWidth;
        int VdcOutHeight;
        int VdcFrameRate;
        OMX_OUTPUTFORMATTYPE VdcOutputFormat;
        OMX_OUTPUTDEVICETYPE VdcOutputDevice;
        OMX_OUTPUTMODETYPE VdcOutputMode;
        OMX_OUTPUTADVANCEDTYPE VdcAdancedtype;
        std::string VdcModeName;
        int streamInfoWidth;
        int streamInfoHeight;
        int streamInfoFrameRate;
        int streamInfoColorFormat;
        int streamInfoIP;
        int surfaceX;
        int surfaceY;
        int surfaceWidth;
        int surfaceHeight;

        int VdecOutWitdh = 3840;
        int VdecOutHeight = 2160;
        // int VdecOutFormat = OMX_COLOR_FormatYUV10_TC_422PK;
        int VdecOutFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        OMX_NATIVE_DEVICETYPE_VDC devtype;
        
        int OsdWidth  = 1920;
        int OsdHeight = 1080;
        
        VdcComponent *vdc;
        VdcPort *VdcPort0;

    public:
        OmxDisplayManager(int w = 3840, int h = 2160, int fr = 0x003C0000,
                            OMX_OUTPUTFORMATTYPE fmt = OMX_OutputFormatRGB,
                            OMX_OUTPUTDEVICETYPE dev = OMX_OutputDeviceVBYONE,
                            std::string Mname = "",
                            OMX_OUTPUTMODETYPE Omd = OMX_OutputMode2D,
                            OMX_U32 FWA = 0,
                            OMX_U32 FHA = 0);
        ~OmxDisplayManager();
        int init();
        int start();
        int stop();
        int deinit();
        PortSet *AddOsdPort(int w, int h, unsigned int CFormat = OMX_COLOR_Format32bitARGB8888, int PlaneOrder = 0);
        int SetupTunnelConnect(
            BaseComponent *cmp1, BasePort *cmp1_port,
            BaseComponent *cmp2, BasePort *cmp2_port);
        int ChangeInputPortDef(
            PortSet *ps, int w, int h, unsigned int color_format, int iptype);
        int ChangeInputPortDef(
            PortSet *ps, int w, int h, int stride, int sliceheight, int fr, unsigned int color_format, int iptype);
        int UpdateCrop(PortSet *ps,
            int in_x,  int in_y,  int in_w,  int in_h,
            int out_x, int out_y, int out_w, int out_h,
            int transform, int blending, int planeAlpha);
        int ChangePlaneShow(PortSet *ps, OMX_PLANESHOWTYPE type);
        int ChangeColorBlend(PortSet *ps, OMX_COLORBLENDTYPE type, OMX_U32 constant = (255<<24));
        int ChangePlaneBlend(PortSet *ps, int32_t depth);
        int SetColorSpace(PortSet *ps, OMX_COLORSPACETYPE colorspace);
        int l_SetPowerMode(int mode);
        int SetPowerMode(int mode);
        int ChangeMute(PortSet *ps, OMX_MUTETYPE mute_type, OMX_U32 color);

        int setActiveConfig(dm_display_attribute *dda);
        int setOutputClock(int clock_mode);

        int32_t dump(android::String8 &out);
};
#endif /* __OMX_DISPLAY_MANAGER_H__ */
