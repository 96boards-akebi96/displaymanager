/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2016 Socionext Inc. */

#ifndef __BaseComponent_H__
#define __BaseComponent_H__

#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_Index_SNI.h>
#include <OMX_IVCommon_SNI.h>
#include <OMX_Video_SNI.h>
#include <pthread.h>
#include <stdint.h>

#include <string>
#include <list>

#include <log/log.h>
#include <utils/String8.h>
#include <utils/Mutex.h>

class BaseComponent;

class Omx_BufferHeader {
    private:
        int self_alloc;
    public:
        OMX_BUFFERHEADERTYPE *omx_buffer;
        void _Omx_BufferHeader(OMX_BUFFERHEADERTYPE *omx_buffer)
        {
            omx_buffer->nSize                = sizeof( OMX_BUFFERHEADERTYPE );
            omx_buffer->nVersion.s.nVersionMajor = 1;
            omx_buffer->nVersion.s.nVersionMinor = 1;
            omx_buffer->nVersion.s.nRevision     = 0;
            omx_buffer->nVersion.s.nStep         = 0;
    
            omx_buffer->pBuffer              = (OMX_U8 *)NULL; //hnd->pbase;
            omx_buffer->nAllocLen            = 0; //roundUpToPageSize(m->finfo.line_length * m->info.yres);
            omx_buffer->nFilledLen           = 0; //roundUpToPageSize(m->finfo.line_length * m->info.yres);
            omx_buffer->nOffset              = 0;
            omx_buffer->pAppPrivate          = NULL;
            omx_buffer->pPlatformPrivate     = NULL;
            omx_buffer->pOutputPortPrivate   = NULL;
            omx_buffer->pInputPortPrivate    = NULL;
            omx_buffer->hMarkTargetComponent = NULL;
            omx_buffer->pMarkData            = NULL;
            omx_buffer->nTickCount           = 0;
            omx_buffer->nTimeStamp           = 16666;
            omx_buffer->nFlags               = 0x0;
            omx_buffer->nOutputPortIndex     = 0xffffffff;
            omx_buffer->nInputPortIndex      = 0xffffffff;
        }
        Omx_BufferHeader()
        {
            self_alloc = 1;
            omx_buffer = (OMX_BUFFERHEADERTYPE *)malloc(sizeof(OMX_BUFFERHEADERTYPE));
            _Omx_BufferHeader(omx_buffer);
        };
        Omx_BufferHeader(OMX_BUFFERHEADERTYPE *buf)
        {
            self_alloc = 0;
            omx_buffer = buf;
            _Omx_BufferHeader(omx_buffer);
        };
        ~Omx_BufferHeader()
        {
            if ((self_alloc) && (omx_buffer)) {
                self_alloc = 0;
                free(omx_buffer);
                omx_buffer = NULL;
            }
        };
};

class BufferHeaderList {
    private:
        android::Mutex mLock;
        std::list<OMX_BUFFERHEADERTYPE *> BufferHeaderList;

        void del_NoLock(OMX_BUFFERHEADERTYPE *buf)
        {
            for(auto itr = BufferHeaderList.begin(); itr != BufferHeaderList.end(); itr++) {
                if (*itr == buf) {
                    BufferHeaderList.erase(itr);
                    break;
                }
            }
        }

    public:
        void Add(OMX_BUFFERHEADERTYPE *buf)
        {
            android::Mutex::Autolock lock(mLock);
            del_NoLock(buf); // remove the previous buffer if exist
            BufferHeaderList.push_back(buf);
        }
        void del(OMX_BUFFERHEADERTYPE *buf)
        {
            android::Mutex::Autolock lock(mLock);
            del_NoLock(buf);
        }
        OMX_BUFFERHEADERTYPE *Get(void)
        {
            android::Mutex::Autolock lock(mLock);
            OMX_BUFFERHEADERTYPE *buf = NULL;
            if (BufferHeaderList.size() > 0) {
                buf = BufferHeaderList.front();
                BufferHeaderList.pop_front();
            }
            return buf;
        }
        OMX_BUFFERHEADERTYPE *Get(OMX_U8 *pBuffer)
        {
            OMX_BUFFERHEADERTYPE *buf = NULL;
            for(auto itr = BufferHeaderList.begin(); itr != BufferHeaderList.end(); itr++) {
                if (((OMX_BUFFERHEADERTYPE*)*itr)->pBuffer == pBuffer) {
                    buf = *itr;
                    break;
                }
            }
            return buf;
        }
        int isPortBuffer(OMX_BUFFERHEADERTYPE *buf) {
            android::Mutex::Autolock lock(mLock);
            for(auto buff = BufferHeaderList.begin(); buff != BufferHeaderList.end(); ++buff) {
                if (((OMX_BUFFERHEADERTYPE *)*buff) == buf) {
                    return 1;
                }
            }
            return 0;
        }
        int GetSize(void)
        {
            android::Mutex::Autolock lock(mLock);
            return BufferHeaderList.size();
        }
};

class BasePort {
    private:
        OMX_PARAM_PORTDEFINITIONTYPE def;
        OMX_MUTETYPE mute;
        int EmptyBufferCounter;
        bool blocked;
    public:
        pthread_mutex_t     lock;
        pthread_cond_t      wait;
        int                 ev;
        class BufferHeaderList BufferHeaderList;
        class BufferHeaderList Used;

        int wait_event(int event, struct timespec *timeout = NULL);
        void notify_event(int event);
        
        OMX_CONFIG_RECTTYPE in_crop, out_crop;
        BasePort(BaseComponent *comp, int w, int h, int fr, unsigned int CFormat, OMX_DIRTYPE dir, int index);
        BasePort(BaseComponent *comp, int w, int h, unsigned int CFormat, OMX_DIRTYPE dir, int index);
        virtual ~BasePort(){};
        OMX_PARAM_PORTDEFINITIONTYPE *GetPortDifinitionType()
        {
            return &def;
        }
        int GetIndex() {
            return def.nPortIndex;
        }
        OMX_DIRTYPE GetDir() {
            return def.eDir;
        }
        void AddPortBuffer(OMX_BUFFERHEADERTYPE *buf)
        {
            BufferHeaderList.Add(buf);
        }
        void RemovePortBuffer(OMX_BUFFERHEADERTYPE *buf)
        {
            BufferHeaderList.del(buf);
        }
        OMX_BUFFERHEADERTYPE *GetPortBuffer(void)
        {
            OMX_BUFFERHEADERTYPE *buf = NULL;
            buf = BufferHeaderList.Get();
            BufferHeaderList.Add(buf);
            return buf;
        }
        OMX_BUFFERHEADERTYPE *GetPortBuffer(OMX_U8 *pBuffer)
        {
            OMX_BUFFERHEADERTYPE *buf = NULL;
            buf = BufferHeaderList.Get(pBuffer);
            return buf;
        }
        int isPortBuffer(OMX_BUFFERHEADERTYPE *buf)
        {
            return BufferHeaderList.isPortBuffer(buf);
        }
        int GetNumOfPortBuffer(void)
        {
            return BufferHeaderList.GetSize();
        }
        void SetMuteType(OMX_MUTETYPE type) {
            mute = type;
        }
        OMX_MUTETYPE GetMuteType(void) {
            return mute;
        }
        void AddEmptyBufferCounter(void) {
            EmptyBufferCounter++;
        }
        bool GetBlockedFlag(void) {
            return blocked;
        }
        void SetBlockedFlag(bool flag) {
            blocked = flag;
        }
        int omx_init_vportdef(OMX_PARAM_PORTDEFINITIONTYPE *lp_def, int dir, int index);
        int omx_init_iportdef(OMX_PARAM_PORTDEFINITIONTYPE *lp_def, int dir, int index);
        int UpdateInfo(int w, int h, unsigned int color_format);
        int UpdateInfo(int w, int h, int fr, unsigned int CFormat);
        int Enable(BaseComponent *comp);
        int Disable(BaseComponent *comp);
        int32_t dump(android::String8 &out);
        const char *getColorFormatName(OMX_COLOR_FORMATTYPE colorformat);
};

class VdcPort : public BasePort {
    private:
    public:
        OMX_NATIVE_DEVICETYPE_VDC     devicetype;
        VdcPort(BaseComponent *comp, int w, int h, int fr,
            int index,
            OMX_NATIVE_DEVICETYPE_VDC devtype)
                : BasePort(comp, w, h, fr, OMX_COLOR_Format32bitARGB8888, OMX_DirInput, index) {
            OMX_PARAM_PORTDEFINITIONTYPE *def = BasePort::GetPortDifinitionType();
            devicetype                             = devtype;
            def->format.video.pNativeRender        = (OMX_NATIVE_DEVICETYPE)&devicetype;
        };
        ~VdcPort(){};
        void UpdateDeviceType(OMX_NATIVE_DEVICETYPE_VDC dt) {
            devicetype = dt;
        }
        int32_t dump(android::String8 &out) {
            BasePort::dump(out);
            if (devicetype.eOutputPanel.eOutputClock == OMX_PANEL_ClockTypeInternal) {
                out.appendFormat("devicetype.eOutputPanel.eOutputClock : OMX_PANEL_ClockTypeInternal\n");
            } else if (devicetype.eOutputPanel.eOutputClock == OMX_PANEL_ClockTypeExternal) {
                out.appendFormat("devicetype.eOutputPanel.eOutputClock : OMX_PANEL_ClockTypeExternal\n");
            } else if (devicetype.eOutputPanel.eOutputClock == OMX_PANEL_ClockTypeFreeRun) {
                out.appendFormat("devicetype.eOutputPanel.eOutputClock : OMX_PANEL_ClockTypeFreeRun\n");
            }
            return 0;
        }
};

class Omx_BufferHeaderList {
    private:
        android::Mutex mLock;
        std::list<Omx_BufferHeader *> BufferHeaderList;

        void del_NoLock(Omx_BufferHeader *buf)
        {
            for(auto itr = BufferHeaderList.begin(); itr != BufferHeaderList.end(); itr++) {
                if (*itr == buf) {
                    BufferHeaderList.erase(itr);
                    break;
                }
            }
        }

    public:
        void Add(Omx_BufferHeader *buf)
        {
            android::Mutex::Autolock lock(mLock);
            del_NoLock(buf); // remove the previous buffer if exist
            BufferHeaderList.push_back(buf);
        }
        void del(Omx_BufferHeader *buf)
        {
            android::Mutex::Autolock lock(mLock);
            del_NoLock(buf);
        }
        Omx_BufferHeader *Get(OMX_BUFFERHEADERTYPE *pBuffer)
        {
            Omx_BufferHeader *buf = NULL;
            for(auto itr = BufferHeaderList.begin(); itr != BufferHeaderList.end(); itr++) {
                if (((Omx_BufferHeader*)*itr)->omx_buffer == pBuffer) {
                    buf = *itr;
                    break;
                }
            }
            return buf;
        }
        int GetSize(void)
        {
            android::Mutex::Autolock lock(mLock);
            return BufferHeaderList.size();
        }
};

class BaseComponent {
    private:
        OMX_HANDLETYPE      pHandle;
        OMX_PTR             app_data;
        static OMX_CALLBACKTYPE    DefaultCallbacks;
        std::string         CompName;
        int PortIndex;
        OMX_STATETYPE status;
        std::list<BasePort *> PortList;
        class Omx_BufferHeaderList Omx_BufferHeaderList;
        android::Mutex mEmptyBuffer_lock;

    public:
        enum {
            HWC_OMX_EVENT_NONE=0,
            HWC_OMX_CHANGE_STATE,
            HWC_OMX_EMPTY_BUFFER_DONE,
            HWC_OMX_FILL_BUFFER_DONE,
            HWC_OMX_TIMEOUT,
            HWC_OMX_NUM_EVENT
        };
        
        pthread_mutex_t     lock;
        pthread_cond_t      wait;
        int                 ev;
        // static
        static OMX_ERRORTYPE default_event_handler(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_PTR pAppData,
            OMX_IN OMX_EVENTTYPE eEvent,
            OMX_IN OMX_U32 nData1,
            OMX_IN OMX_U32 nData2,
            OMX_IN OMX_PTR pEventData
        );
        static OMX_ERRORTYPE default_empty_buffer_done(
            OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_PTR pAppData,
            OMX_IN OMX_BUFFERHEADERTYPE* pBuffer
        );
        static OMX_ERRORTYPE default_fill_buffer_done(
            OMX_OUT OMX_HANDLETYPE hComponent,
            OMX_OUT OMX_PTR pAppData,
            OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer
        );
        void wait_event(BaseComponent *base, int event);
        void notify_event(BaseComponent *base, int event);
        
        BaseComponent();
        BaseComponent(std::string Name);
        virtual ~BaseComponent();
        
        int init(OMX_CALLBACKTYPE *cb = &DefaultCallbacks);
        int deinit(void);
        int SetState(OMX_STATETYPE state, bool blocked = false);
        int EmptyBuffer(BasePort *port, char* buff, int buff_len,
                OMX_SNIVS_OUTPORT_PRIVATE_t *option = (OMX_SNIVS_OUTPORT_PRIVATE_t *)NULL,
                bool blocked = false, uint32_t flag = 0);
        int SetParamPortDefinition(BasePort *port);
        BasePort *AddPort(int w, int h, int fr, unsigned int CFormat, OMX_DIRTYPE dir);
        BasePort *AddPort(int w, int h, unsigned int CFormat, OMX_DIRTYPE dir);
        int SetRect(OMX_CONFIG_RECTTYPE *rect, BasePort *port, int x, int y, int w, int h);
        int UpdateCrop(int InOut, OMX_CONFIG_RECTTYPE *rect);
        int UpdateCrop(OMX_CONFIG_RECTTYPE *in, OMX_CONFIG_RECTTYPE *out);
        bool isSameRect(OMX_CONFIG_RECTTYPE *Current, OMX_CONFIG_RECTTYPE *New);
        BasePort *GetPort(int index);
        int GetNumOfPort(void);
        int GetPortIndex(OMX_BUFFERHEADERTYPE *buf);

        OMX_BUFFERHEADERTYPE *GetPortBuffer(int index);

        char *GetComponentName(void) {
            return (char *)CompName.c_str();
        }
        OMX_HANDLETYPE GetHandle(void)
        {
            return pHandle;
        }
        int GetPortIndex()
        {
            return PortIndex++;
        }
        OMX_STATETYPE GetState() {
            return status;
        }
        bool isVdc(){
            if (strncmp(GetComponentName(),"OMX.SNI.VDC",sizeof("OMX.SNI.VDC")) == 0) {
                return true;
            } else {
                return false;
            }
        }
        
        // static
        static int omx_init_version(OMX_VERSIONTYPE  *lp_version);

        
};

#endif /* __BaseComponent_H__ */
