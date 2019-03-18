#ifndef __COMPONENT_H__
#define __COMPONENT_H__

#include "BaseComponent.h"

#define OMX_PREFIX "OMX.SNI"

class VdcComponent : public BaseComponent {
    public:
        VdcComponent() : BaseComponent(OMX_PREFIX".VDC") {};
        ~VdcComponent(){};
        BasePort *AddPort(int w, int h, unsigned int CFormat, OMX_DIRTYPE dir) {
            return BaseComponent::AddPort(w, h, CFormat, dir);
        }
        BasePort *AddPort(int w, int h, int fr,
            OMX_NATIVE_DEVICETYPE_VDC devtype);
        int ConfigCommonPlaneBlend(BasePort *port, int depth);
        int ConfigCommonMute(BasePort *port, OMX_MUTETYPE mute_type, OMX_U32 color);                                                                                                              
};

#endif /* __COMPONENT_H__ */
