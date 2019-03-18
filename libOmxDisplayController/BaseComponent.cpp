
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>

#define LOG_TAG "BaseComponent"
#include "BaseComponent.h"

BaseComponent::BaseComponent()
{
    ATRACE_CALL();
    BaseComponent("");
}

BaseComponent::BaseComponent(std::string Name)
{
    ATRACE_CALL();
    CompName = Name;
    PortIndex = 0;
    pHandle = NULL;
    
    pthread_mutex_init( &lock, NULL );
    pthread_cond_init( &wait, NULL );
    ev = HWC_OMX_EVENT_NONE;
    
    status = OMX_StateInvalid;
}

BaseComponent::~BaseComponent()
{
    ATRACE_CALL();
    if (pHandle) {
        deinit();
    }
}

void BaseComponent::notify_event(BaseComponent *base, int event)
{
    ATRACE_CALL();
    pthread_mutex_lock( &base->lock );
    base->ev = event;
    pthread_cond_signal( &base->wait );
    pthread_mutex_unlock( &base->lock );
    
    return;
}

void BaseComponent::wait_event(BaseComponent *base, int event)
{
    ATRACE_CALL();
    pthread_mutex_lock( &base->lock );
    while( base->ev != event )
    {
        pthread_cond_wait( &base->wait, &base->lock );
    }
    base->ev = HWC_OMX_EVENT_NONE;
    pthread_mutex_unlock( &base->lock );
    
    return;
}

OMX_ERRORTYPE BaseComponent::default_event_handler(
    OMX_IN OMX_HANDLETYPE /* hComponent */,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1,
    OMX_IN OMX_U32 nData2,
    OMX_IN OMX_PTR /* pEventData */
)
{
    ATRACE_CALL();
//    ALOGV( "%s\n", __func__ );
    int event_id = HWC_OMX_CHANGE_STATE;

    BaseComponent *base = static_cast<BaseComponent *>(pAppData);

    if( eEvent == OMX_EventCmdComplete )
    {
        if( nData1 == OMX_CommandStateSet )
        {
            event_id = HWC_OMX_CHANGE_STATE;
            base->notify_event( base, event_id );
        }
        if( nData1 == OMX_CommandPortDisable )
        {
            ALOGI("default_event_handler : %s OMX_CommandPortDisable : port id = %d\n", base->GetComponentName(), nData2);
        }
    }

    return ( OMX_ErrorNone );
}

OMX_ERRORTYPE BaseComponent::default_empty_buffer_done(
    OMX_IN OMX_HANDLETYPE /* hComponent */,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer
)
{
    ATRACE_CALL();
    
    // ALOGV( "%s\n", __func__ );
    int index;
    BaseComponent *base = static_cast<BaseComponent *>(pAppData);
    index = pBuffer->nInputPortIndex;

    if (pBuffer->pOutputPortPrivate != NULL) {
        free(pBuffer->pOutputPortPrivate);
        pBuffer->pOutputPortPrivate = NULL;
    }

    Omx_BufferHeader *OMXBuff = base->Omx_BufferHeaderList.Get(pBuffer);
    base->Omx_BufferHeaderList.del(OMXBuff);
    delete OMXBuff;

    if (index < 0) {
        ALOGE("default_empty_buffer_done : %s invalid index = %d pBuffer = %p", base->GetComponentName(), index, pBuffer);
        return OMX_ErrorNone;
    }

    if (base->GetPort(index)->GetBlockedFlag()) {
        base->GetPort(index)->notify_event(HWC_OMX_EMPTY_BUFFER_DONE);
    } else {
        base->GetPort(index)->Used.del(pBuffer);
    }
    // Error check trap
    if (base->GetPort(index)->Used.GetSize() > 10) {
        ALOGE("error 1 %d : %d", __LINE__, base->GetPort(index)->Used.GetSize());
    }
    if (base->Omx_BufferHeaderList.GetSize() > 10) {
        ALOGE("error 2 %d : %d", __LINE__, base->Omx_BufferHeaderList.GetSize());
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE BaseComponent::default_fill_buffer_done(
    OMX_OUT OMX_HANDLETYPE /* hComponent */,
    OMX_OUT OMX_PTR pAppData,
    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer
)
{
    ATRACE_CALL();
    int index;

    // ALOGV( "%s\n", __func__ );
    BaseComponent *base = static_cast<BaseComponent *>(pAppData);
    index = base->GetPortIndex(pBuffer);

    Omx_BufferHeader *OMXBuff = base->Omx_BufferHeaderList.Get(pBuffer);
    base->Omx_BufferHeaderList.del(OMXBuff);
    delete OMXBuff;

    if (base->GetPort(index)->GetBlockedFlag()) {
        base->GetPort(index)->notify_event(HWC_OMX_FILL_BUFFER_DONE);
    } else {
        base->GetPort(index)->Used.del(pBuffer);
    }
    return OMX_ErrorNone;
}

// static
OMX_CALLBACKTYPE BaseComponent::DefaultCallbacks = {
    &default_event_handler, &default_empty_buffer_done, &default_fill_buffer_done
};

int BaseComponent::init(OMX_CALLBACKTYPE *cb)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret;
    
    if (pHandle != NULL) {
        return 0;
    }
    
    app_data = (OMX_PTR)this;
    ret = OMX_GetHandle( &pHandle, (OMX_STRING)GetComponentName(), app_data, cb );
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_GetHandle for %s failed : ret = 0x%x.\n", GetComponentName(), (unsigned int)ret );
        return -1;
    }
    status = OMX_StateLoaded;
    return 0;
}

int BaseComponent::deinit(void)
{
    ATRACE_CALL();
    int ret;

    ret = OMX_FreeHandle( pHandle );
    ALOGE_IF(ret != 0, "%s: component not found to be freed(ret=%d)", __FUNCTION__, ret);
    for(auto port = PortList.begin(); port != PortList.end(); ++port) {
        delete *port;
        PortList.erase(port);
    }
    pHandle = NULL;
    return 0;
}

int BaseComponent::SetState(OMX_STATETYPE state, bool blocked)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret;

    /* Transit from IDLE to LOADED */
    ret = OMX_SendCommand( pHandle, OMX_CommandStateSet, state, NULL );
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_SendCommand(OMX_StateLoaded) for %s failed : ret = 0x%x.\n", GetComponentName(), (unsigned int)ret );
        return ( -1 );
    }
    if (blocked) {
        wait_event(this, HWC_OMX_CHANGE_STATE);
    }
    status = state;
    return 0;
}

BasePort *BaseComponent::AddPort(int w, int h, int fr, unsigned int CFormat, OMX_DIRTYPE dir)
{
    ATRACE_CALL();
    int index = GetPortIndex();
    int ret = 0;
    BasePort *port = new BasePort(this, w, h, fr, CFormat, dir, index);
    if (port == NULL) {
        ALOGE("Can't Create port !!");
        return port;
    }
    ret = SetParamPortDefinition(port);
    if (ret < 0) {
        delete port;
        return (BasePort *)NULL;
    }
    PortList.push_back(port);
    return port;
}

BasePort *BaseComponent::AddPort(int w, int h, unsigned int CFormat, OMX_DIRTYPE dir)
{
    ATRACE_CALL();
    int index = GetPortIndex();
    int ret = 0;
    BasePort *port = new BasePort(this, w, h, CFormat, dir, index);
    if (port == NULL) {
        ALOGE("Can't Create port !!");
        return port;
    }
    ret = SetParamPortDefinition(port);
    if (ret < 0) {
        delete port;
        return (BasePort *)NULL;
    }
    PortList.push_back(port);
    return port;
}

int BaseComponent::GetPortIndex(OMX_BUFFERHEADERTYPE *buf)
{
    ATRACE_CALL();
    for(auto port = PortList.begin(); port != PortList.end(); ++port) {
        if (((BasePort *)*port)->Used.isPortBuffer(buf)) {
            return ((BasePort *)*port)->GetIndex();
        }
    }
    return -1;
}


int BaseComponent::GetNumOfPort(void)
{
    return PortList.size();
}

int BaseComponent::SetParamPortDefinition(BasePort *port)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret;
    ret = OMX_SetParameter( pHandle, OMX_IndexParamPortDefinition, port->GetPortDifinitionType() );
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_SetParameter(OMX_IndexParamPortDefinition) for %s failed  : ret = 0x%x.\n", GetComponentName(), (unsigned int)ret );
        return ( -1 );
    }
    return 0;
}

int BaseComponent::UpdateCrop(int InOut, OMX_CONFIG_RECTTYPE *rect)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_INDEXTYPE index;
    BasePort *port;
    OMX_CONFIG_RECTTYPE *current;

    port = GetPort(rect->nPortIndex);
    if (InOut == 0) { // Input
        index = OMX_IndexConfigCommonInputCrop;
        current = &port->in_crop;
    } else {
        index = OMX_IndexConfigCommonOutputCrop;
        current = &port->out_crop;
    }

    if (!isSameRect(current, rect)) {
        ret = OMX_SetConfig( GetHandle(), index, rect );
        if( ret != OMX_ErrorNone )
        {
            ALOGE( "OMX_SetConfig(OMX_IndexConfigCommonInpurCrop) for %s failed : ret = 0x%x.\n", GetComponentName(), (unsigned int)ret );
            return ( ret );
        }
        *current = *rect; 
    } else {
        // ignore : same crop
    }
    return 0;
}

int BaseComponent::UpdateCrop(OMX_CONFIG_RECTTYPE *in, OMX_CONFIG_RECTTYPE *out)
{
    ATRACE_CALL();
    int ret = 0;
    
    /* Input Crop */
    ret = UpdateCrop(0, in);
    if (ret < 0) {
        return ret;
    }
    
    /* Output Crop */
    ret = UpdateCrop(1, out);
    if (ret < 0) {
        return ret;
    }
    return ret;
}

int BaseComponent::SetRect(OMX_CONFIG_RECTTYPE *rect, BasePort *port, int x, int y, int w, int h)
{
    ATRACE_CALL();
    rect->nSize      = sizeof( *rect );
    omx_init_version( &rect->nVersion );
    if (port != NULL) {
        rect->nPortIndex = port->GetIndex();
    } else {
        ALOGE("%s:%d port is null", __func__, __LINE__);
        return -1;
    }
    rect->nLeft      = x;
    rect->nTop       = y;
    rect->nWidth     = w;
    rect->nHeight    = h;
    return 0;
}

bool BaseComponent::isSameRect(OMX_CONFIG_RECTTYPE *Current, OMX_CONFIG_RECTTYPE *New)
{
    ATRACE_CALL();
    if ( (Current->nLeft == New->nLeft)
         && (Current->nTop  == New->nTop)
         && (Current->nWidth  == New->nWidth)
         && (Current->nHeight == New->nHeight)) {
        // Same
        return true;
    }
    return false;
}

BasePort *BaseComponent::GetPort(int index)
{
    ATRACE_CALL();
    for(auto port = PortList.begin(); port != PortList.end(); ++port) {
        if (((BasePort *)*port)->GetIndex() == index) {
            return *port;
        }
    }
    ALOGW("%s:%d Can't find Port from index : %d", __func__, __LINE__, index);
    return NULL;
}


int BaseComponent::omx_init_version(
    OMX_VERSIONTYPE  *lp_version
)
{
    ATRACE_CALL();
    if( lp_version == NULL )
    {
        return ( -1 );
    }
    
    lp_version->s.nVersionMajor = 1;
    lp_version->s.nVersionMinor = 1;
    lp_version->s.nRevision     = 0;
    lp_version->s.nStep         = 0;
    
    return ( 0 );
}

int BaseComponent::EmptyBuffer(BasePort *port, char* buff, int buff_len, OMX_SNIVS_OUTPORT_PRIVATE_t *option, bool blocked, uint32_t flag)
{
    ATRACE_CALL();
    android::Mutex::Autolock lock(mEmptyBuffer_lock);
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    Omx_BufferHeader *OMXBuff = new Omx_BufferHeader();
    OMX_BUFFERHEADERTYPE *omx_buffer = OMXBuff->omx_buffer;
    int err;
    OMX_SNIVS_OUTPORT_PRIVATE_t *oPrivate = NULL;

    omx_buffer->pBuffer              = (OMX_U8 *)buff;
    omx_buffer->nAllocLen            = buff_len;
    omx_buffer->nFilledLen           = buff_len;
    omx_buffer->nInputPortIndex      = port->GetIndex();
    omx_buffer->nFlags               = flag;
    omx_buffer->pOutputPortPrivate   = NULL;
    if (option != NULL) {
        oPrivate = (OMX_SNIVS_OUTPORT_PRIVATE_t *)malloc(sizeof(OMX_SNIVS_OUTPORT_PRIVATE_t));
        memcpy(oPrivate, option, sizeof(OMX_SNIVS_OUTPORT_PRIVATE_t));
        omx_buffer->pOutputPortPrivate = oPrivate;
        omx_buffer->nTimeStamp = option->nTimeStamp;
        if ( ((OMX_PSC_COLOR_FORMATTYPE)port->GetPortDifinitionType()->format.video.eColorFormat != OMX_COLOR_FormatYUV10_TC_422PK)
            && ((OMX_PSC_COLOR_FORMATTYPE)port->GetPortDifinitionType()->format.video.eColorFormat != OMX_COLOR_FormatYUV20_422PK)
            && (option->nLumaBusAddress == 0)) {
            ALOGE("EmptyBuffer : nLumaBusAddress is 0: flag : %08x OMX_COLOR_FormatYUV10_TC_422PK(%08x) != %08x", flag,
                OMX_COLOR_FormatYUV10_TC_422PK,
                port->GetPortDifinitionType()->format.video.eColorFormat);
            omx_buffer->nFilledLen = 0;
            free(oPrivate);
            delete OMXBuff;
            return (-1);
        }
    }

    Omx_BufferHeaderList.Add(OMXBuff);
    port->Used.Add(omx_buffer);
    port->SetBlockedFlag(blocked);
    if (option != NULL) {
        if ((OMX_PSC_COLOR_FORMATTYPE)port->GetPortDifinitionType()->format.video.eColorFormat == OMX_COLOR_FormatYUV16_422PK) {
            omx_buffer->pBuffer              = (OMX_U8 *)(oPrivate->nLumaBusAddress);
            //ALOGI("%s(%d) CALL OMX_EmptyThisBuffer(0x%x, 0x%x)", __FUNCTION__, __LINE__, pHandle, omx_buffer->pBuffer);
        }
    }
    //ALOGI("CALL OMX_EmptyThisBuffer(0x%x, 0x%x)", pHandle, buff);
    ret = OMX_EmptyThisBuffer( pHandle, omx_buffer );
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_EmptyThisBuffer for %s failed : ret = 0x%x.\n", GetComponentName(), (unsigned int)ret );
        Omx_BufferHeaderList.del(OMXBuff);
        if (oPrivate != NULL) {
            free(oPrivate);
        }
        delete OMXBuff;
        return (-1);
    }
    port->AddEmptyBufferCounter();

    if (blocked) {
        err = ETIMEDOUT;
        while (err == ETIMEDOUT) {
            err = port->wait_event(HWC_OMX_EMPTY_BUFFER_DONE);
        }
        port->Used.del(omx_buffer);
    }

    return 0;
}

OMX_BUFFERHEADERTYPE *BaseComponent::GetPortBuffer(int index)
{
    OMX_BUFFERHEADERTYPE* buf = NULL;
    BasePort *port = NULL;
    port = GetPort(index);
    if (port == NULL) {
        ALOGE("%s:%d I can't find %s port !!!", __func__, __LINE__, GetComponentName());
        return NULL;
    }
    buf = port->GetPortBuffer();
    if (buf == NULL) {
        ALOGE("%s:%d I can't find %s port buffer !!!", __func__, __LINE__, GetComponentName());
        return NULL;
    }
    return buf;
}

// BasePort
BasePort::BasePort(BaseComponent * /*comp*/, int w, int h, int fr, unsigned int CFormat, OMX_DIRTYPE dir, int index)
{
    ATRACE_CALL();
//    ALOGI("----------- port index : %d %d %08x (%d:%d)", index, dir, CFormat, w, h);
    int err;

    pthread_mutex_init( &lock, NULL );
    pthread_cond_init( &wait, NULL );
    ev = BaseComponent::HWC_OMX_EVENT_NONE;
    blocked = false;

    omx_init_vportdef(&def, dir, index);
    def.format.video.xFramerate = fr;
    err = UpdateInfo(w,h,CFormat);
    if (err < 0) {
        ALOGE("%s:%d Error : %d", __func__, __LINE__, err);
    }
    in_crop.nLeft    = -1;
    in_crop.nTop     = -1;
    in_crop.nWidth   = -1;
    in_crop.nHeight  = -1;
    out_crop.nLeft   = -1;
    out_crop.nTop    = -1;
    out_crop.nWidth  = -1;
    out_crop.nHeight = -1;
    EmptyBufferCounter = 0;
}

BasePort::BasePort(BaseComponent * /*comp*/, int w, int h, unsigned int CFormat, OMX_DIRTYPE dir, int index)
{
    ATRACE_CALL();
//    ALOGI("----------- port index : %d %d %08x (%d:%d)", index, dir, CFormat, w, h);
    int err;
    
    pthread_mutex_init( &lock, NULL );
    pthread_cond_init( &wait, NULL );
    ev = BaseComponent::HWC_OMX_EVENT_NONE;
    blocked = false;
    
    omx_init_iportdef(&def, dir, index);
    err = UpdateInfo(w,h,CFormat);
    if (err < 0) {
        ALOGE("%s:%d Error : %d", __func__, __LINE__, err);
    }
    in_crop.nLeft    = -1;
    in_crop.nTop     = -1;
    in_crop.nWidth   = -1;
    in_crop.nHeight  = -1;
    out_crop.nLeft   = -1;
    out_crop.nTop    = -1;
    out_crop.nWidth  = -1;
    out_crop.nHeight = -1;
    EmptyBufferCounter = 0;
}

int BasePort::UpdateInfo(int w, int h, int fr, unsigned int CFormat)
{
    int ret;
    def.format.video.xFramerate = fr;
    ret = UpdateInfo(w,h,CFormat);
    return ret;
}


int BasePort::UpdateInfo(int w, int h, unsigned int CFormat)
{
    ATRACE_CALL();
    int BytePerPixel = 4;
    int nFrameHeight, nSliceHeight, nStride;
    
    switch (CFormat) {
        case OMX_COLOR_Format32bitARGB8888:       // from VCC(OSD)
            BytePerPixel = 4;
            nFrameHeight = h;
            nSliceHeight = h;
            break;
        default:
            ALOGE("%s:%d : UnKnown ColorFormat Error : %d", __func__, __LINE__, CFormat);
            return (-1);
    }
    if (BytePerPixel != -1) {
        nStride = w * BytePerPixel;
    }
    
    switch (def.eDomain) {
        case OMX_PortDomainVideo:
            def.format.video.nFrameWidth = w;
            def.format.video.nFrameHeight = nFrameHeight;
            def.format.video.nSliceHeight = nSliceHeight;
            def.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)CFormat;
            def.format.video.nStride = nStride;
            break;
        case OMX_PortDomainImage:
            def.format.image.nFrameWidth = w;
            def.format.image.nFrameHeight = nFrameHeight;
            def.format.image.nSliceHeight = nSliceHeight;
            def.format.image.eColorFormat = (OMX_COLOR_FORMATTYPE)CFormat;
            def.format.image.nStride = nStride;
            break;
        default:
            ALOGE("Unknown Domain : %d", def.eDomain);
            break;
    } 
    
    return 0;
}

int BasePort::omx_init_vportdef(OMX_PARAM_PORTDEFINITIONTYPE *lp_def, int dir, int index)
{
    ATRACE_CALL();
    if( lp_def == NULL )
    {
        return ( -1 );
    }
    
    lp_def->nSize                              = sizeof( *lp_def );
    BaseComponent::omx_init_version( &( lp_def->nVersion ) );
    lp_def->nPortIndex                         = index;
    lp_def->eDir                               = (OMX_DIRTYPE)dir;
    lp_def->nBufferCountActual                 = 0;
    lp_def->nBufferCountMin                    = 0;
    lp_def->nBufferSize                        = 0;
    lp_def->bEnabled                           = OMX_TRUE;
    lp_def->bPopulated                         = OMX_FALSE;
    lp_def->eDomain                            = OMX_PortDomainVideo;
    lp_def->format.video.cMIMEType             = NULL;
    lp_def->format.video.pNativeRender         = NULL;
    lp_def->format.video.nFrameWidth           = 0;
    lp_def->format.video.nFrameHeight          = 0;
    lp_def->format.video.nStride               = 0;
    lp_def->format.video.nSliceHeight          = 0;
    lp_def->format.video.nBitrate              = 0x0;
    lp_def->format.video.xFramerate            = 0;
    lp_def->format.video.bFlagErrorConcealment = OMX_FALSE;
    lp_def->format.video.eCompressionFormat    = OMX_VIDEO_CodingUnused;
    lp_def->format.video.eColorFormat          = (OMX_COLOR_FORMATTYPE)OMX_COLOR_Format32bitARGB8888;
    lp_def->format.video.pNativeWindow         = 0;
    lp_def->bBuffersContiguous                 = OMX_TRUE;
    lp_def->nBufferAlignment                   = 4;
    
    return ( 0 );
}

int BasePort::omx_init_iportdef(OMX_PARAM_PORTDEFINITIONTYPE *lp_def, int dir, int index)
{
    ATRACE_CALL();
    if( lp_def == NULL )
    {
        return ( -1 );
    }
    
    lp_def->nSize                              = sizeof( *lp_def );
    BaseComponent::omx_init_version( &( lp_def->nVersion ) );
    lp_def->nPortIndex                         = index;
    lp_def->eDir                               = (OMX_DIRTYPE)dir;
    lp_def->nBufferCountActual                 = 0;
    lp_def->nBufferCountMin                    = 0;
    lp_def->nBufferSize                        = 0;
    lp_def->bEnabled                           = OMX_TRUE;
    lp_def->bPopulated                         = OMX_FALSE;
    lp_def->eDomain                            = OMX_PortDomainImage;
    lp_def->format.image.cMIMEType             = NULL;
    lp_def->format.image.pNativeRender         = NULL;
    lp_def->format.image.nFrameWidth           = 0;
    lp_def->format.image.nFrameHeight          = 0;
    lp_def->format.image.nStride               = 0;
    lp_def->format.image.nSliceHeight          = 0;
    lp_def->format.image.bFlagErrorConcealment = OMX_FALSE;
    lp_def->format.image.eCompressionFormat    = OMX_IMAGE_CodingUnused;
    lp_def->format.image.eColorFormat          = (OMX_COLOR_FORMATTYPE)OMX_COLOR_Format32bitARGB8888;
    lp_def->format.image.pNativeWindow         = 0;
    lp_def->bBuffersContiguous                 = OMX_TRUE;
    lp_def->nBufferAlignment                   = 4;
    
    return ( 0 );
}

void BasePort::notify_event(int event)
{
    ATRACE_CALL();
    pthread_mutex_lock( &lock );
    ev = event;
    pthread_cond_signal( &wait );
    pthread_mutex_unlock( &lock );
    
    return;
}

static const char *color_format_std_name[] = {
    "OMX_COLOR_FormatUnused",
    "OMX_COLOR_FormatMonochrome",
    "OMX_COLOR_Format8bitRGB332",
    "OMX_COLOR_Format12bitRGB444",
    "OMX_COLOR_Format16bitARGB4444",
    "OMX_COLOR_Format16bitARGB1555",
    "OMX_COLOR_Format16bitRGB565",
    "OMX_COLOR_Format16bitBGR565",
    "OMX_COLOR_Format18bitRGB666",
    "OMX_COLOR_Format18bitARGB1665",
    "OMX_COLOR_Format19bitARGB1666",
    "OMX_COLOR_Format24bitRGB888",
    "OMX_COLOR_Format24bitBGR888",
    "OMX_COLOR_Format24bitARGB1887",
    "OMX_COLOR_Format25bitARGB1888",
    "OMX_COLOR_Format32bitBGRA8888",
    "OMX_COLOR_Format32bitARGB8888",
    "OMX_COLOR_FormatYUV411Planar",
    "OMX_COLOR_FormatYUV411PackedPlanar",
    "OMX_COLOR_FormatYUV420Planar",
    "OMX_COLOR_FormatYUV420PackedPlanar",
    "OMX_COLOR_FormatYUV420SemiPlanar",
    "OMX_COLOR_FormatYUV422Planar",
    "OMX_COLOR_FormatYUV422PackedPlanar",
    "OMX_COLOR_FormatYUV422SemiPlanar",
    "OMX_COLOR_FormatYCbYCr",
    "OMX_COLOR_FormatYCrYCb",
    "OMX_COLOR_FormatCbYCrY",
    "OMX_COLOR_FormatCrYCbY",
    "OMX_COLOR_FormatYUV444Interleaved",
    "OMX_COLOR_FormatRawBayer8bit",
    "OMX_COLOR_FormatRawBayer10bit",
    "OMX_COLOR_FormatRawBayer8bitcompressed",
    "OMX_COLOR_FormatL2",
    "OMX_COLOR_FormatL4",
    "OMX_COLOR_FormatL8",
    "OMX_COLOR_FormatL16",
    "OMX_COLOR_FormatL24",
    "OMX_COLOR_FormatL32",
    "OMX_COLOR_FormatYUV420PackedSemiPlanar",
    "OMX_COLOR_FormatYUV422PackedSemiPlanar",
    "OMX_COLOR_Format18BitBGR666",
    "OMX_COLOR_Format24BitARGB6666",
    "OMX_COLOR_Format24BitABGR6666",
    "",
};

const char *BasePort::getColorFormatName(OMX_COLOR_FORMATTYPE colorformat)
{
    if (colorformat < OMX_COLOR_Format24BitABGR6666) {
        return color_format_std_name[colorformat];
    } else {
        return (const char *)"";
    }
}

int BasePort::wait_event(int event, struct timespec *timeout)
{
    ATRACE_CALL();
    pthread_mutex_lock( &lock );
    struct timespec to;
    int ret = 0;

    if (timeout == NULL) {
        // default 1sec
        struct timeval tv;
        gettimeofday(&tv, NULL);
        to.tv_sec = tv.tv_sec + 1;
        to.tv_nsec = tv.tv_usec * 1000;
    } else {
        to = *timeout;
    }
    while( ev != event && ev != BaseComponent::HWC_OMX_TIMEOUT)
    {
        ret = pthread_cond_timedwait( &wait, &lock, &to);
        switch (ret) {
            case ETIMEDOUT:
                ev = BaseComponent::HWC_OMX_TIMEOUT;
                break;
            case EINVAL:
            case EINTR:
                ALOGW("interrupt : %d", ret);
                break;
            default:
                break;
        }
    }
    ev = BaseComponent::HWC_OMX_EVENT_NONE;
    pthread_mutex_unlock( &lock );
    
    return ret;
}

int32_t BasePort::dump(android::String8 &out)
{
    out.appendFormat("def.nSize : %d\n", def.nSize);
    out.appendFormat("def.nPortIndex : %d\n", def.nPortIndex);
    out.appendFormat("def.eDir : %d\n", def.eDir);
    out.appendFormat("def.nBufferCountActual : %d\n", def.nBufferCountActual);
    out.appendFormat("def.nBufferCountMin : %d\n", def.nBufferCountMin);
    out.appendFormat("def.nBufferSize : %d\n", def.nBufferSize);
    out.appendFormat("def.bEnabled : %d\n", def.bEnabled);
    out.appendFormat("def.bPopulated : %d\n", def.bPopulated);
    out.appendFormat("def.eDomain : %d\n", def.eDomain);
    switch (def.eDomain) {
        case OMX_PortDomainVideo:
            out.appendFormat("def.format.video.cMIMEType : %s\n", def.format.video.cMIMEType);
            out.appendFormat("def.format.video.pNativeRender : %p\n", def.format.video.pNativeRender);
            out.appendFormat("def.format.video.nFrameWidth : %d\n", def.format.video.nFrameWidth);
            out.appendFormat("def.format.video.nFrameHeight : %d\n", def.format.video.nFrameHeight);
            out.appendFormat("def.format.video.nStride : %d\n", def.format.video.nStride);
            out.appendFormat("def.format.video.nSliceHeight : %d\n", def.format.video.nSliceHeight);
            out.appendFormat("def.format.video.nBitrate : %d\n", def.format.video.nBitrate);
            out.appendFormat("def.format.video.xFramerate : %d %3.2fHz\n", def.format.video.xFramerate, (def.format.video.xFramerate/65536.0f));
            out.appendFormat("def.format.video.eCompressionFormat : %d\n", def.format.video.eCompressionFormat);
            out.appendFormat("def.format.video.eColorFormat : %d (%s)\n", def.format.video.eColorFormat, getColorFormatName(def.format.video.eColorFormat));
            break;
        case OMX_PortDomainImage:
            out.appendFormat("def.format.image.cMIMEType : %s\n", def.format.image.cMIMEType);
            out.appendFormat("def.format.image.pNativeRender : %p\n", def.format.image.pNativeRender);
            out.appendFormat("def.format.image.nFrameWidth : %d\n", def.format.image.nFrameWidth);
            out.appendFormat("def.format.image.nFrameHeight : %d\n", def.format.image.nFrameHeight);
            out.appendFormat("def.format.image.nStride : %d\n", def.format.image.nStride);
            out.appendFormat("def.format.image.nSliceHeight : %d\n", def.format.image.nSliceHeight);
            out.appendFormat("def.format.image.eCompressionFormat : %d\n", def.format.image.eCompressionFormat);
            out.appendFormat("def.format.image.eColorFormat : %d (%s)\n", def.format.image.eColorFormat, getColorFormatName(def.format.image.eColorFormat));
            break;
        default:
            break;
    }
    out.appendFormat("Crop : (%d,%d)-(%d,%d) -> (%d,%d)-(%d,%d)\n",
        in_crop.nLeft, in_crop.nTop, in_crop.nWidth, in_crop.nHeight,
        out_crop.nLeft, out_crop.nTop, out_crop.nWidth, out_crop.nHeight);
    out.appendFormat("EmptyBufferCounter : %d\n", EmptyBufferCounter);
    return 0;
}

int BasePort::Enable(BaseComponent *comp)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ALOGI( "OMX_SendCommand(OMX_CommandPortEnable) for %s %d\n",comp->GetComponentName(), GetIndex());
    ret = OMX_SendCommand(comp->GetHandle(), OMX_CommandPortEnable, GetIndex(), NULL);
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_SendCommand(OMX_CommandPortEnable) for %s failed : ret = 0x%x.\n",
                    comp->GetComponentName(), (unsigned int)ret );
        return (-1);
    }
    return 0;
}

int BasePort::Disable(BaseComponent *comp)
{
    ATRACE_CALL();
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ALOGI( "OMX_SendCommand(OMX_CommandPortDisable) for %s %d\n",comp->GetComponentName(), GetIndex());
    ret = OMX_SendCommand(comp->GetHandle(), OMX_CommandPortDisable, GetIndex(), NULL);
    if( ret != OMX_ErrorNone )
    {
        ALOGE( "OMX_SendCommand(OMX_CommandPortDisable) for %s failed : ret = 0x%x.\n",
                    comp->GetComponentName(), (unsigned int)ret );
        return (-1);
    }
    return 0;
}

