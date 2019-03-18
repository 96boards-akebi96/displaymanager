/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "display_init"

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <asm/ion.h>
#include <asm/ion_uniphier.h>

#if !defined(MN2WS0310) && !defined(MN2WS0320)
#if __ANDROID_API__ > 19
#include <hardware/gralloc.h>
#include "mali_gralloc_module.h"
#endif /* __ANDROID_API__ > 19 */
#endif /* !defined(MN2WS0310) && !defined(MN2WS0320) */
#include "gralloc_priv.h"

#include <sys/resource.h>

#include <cutils/sched_policy.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "IDisplayManager.h"
#include "DisplayManager.h"
#include <hardware/hwcomposer.h>


using namespace android;

#define ION_DEVNAME "/dev/ion"
static void get_physical_address(private_handle_t **hnd)
{
    uint64_t phy_addr = 0;
    void* vir_addr    = MAP_FAILED;
    int fd_ion        = -1;
    int ret_ioctl     = -1;
    int opened        = 0;
    int alloced       = 0;
    int mapped        = 0;
    int w = 1920;
    int h = 1080;

    // open ion driver
    fd_ion = open(ION_DEVNAME, O_RDWR);
    if (fd_ion < 0) {
        printf("[DM] Failed to open(ion).\n");
        goto close_ion;
    }
    opened = 1;

    // allocate memory
    struct ion_allocation_data alloc_buf;
    memset(&alloc_buf, 0, sizeof(alloc_buf));
    alloc_buf.len          = w*h*4; // ARGB8888
    alloc_buf.align        = 0x1000;
    alloc_buf.heap_id_mask = ION_HEAP_ID_FB_MASK;
    alloc_buf.flags        = 0;
    ret_ioctl = ioctl(fd_ion, ION_IOC_ALLOC, &alloc_buf);
    if (ret_ioctl != 0) {
        printf("[DM] Failed to ion ioctl(alloc), ret=%d.\n", ret_ioctl);
        goto close_ion;
    }
    alloced = 1;

    // get a file descriptor to mmap
    struct ion_fd_data share_buf;
    memset(&share_buf, 0, sizeof(share_buf));
    share_buf.handle = alloc_buf.handle;
    ret_ioctl = ioctl(fd_ion, ION_IOC_MAP, &share_buf);
    if (ret_ioctl != 0) {
        printf("[DM] Failed to ion ioctl(map).\n");
        goto close_ion;
    }

    // mmap vitual address
    vir_addr = mmap(NULL, alloc_buf.len, (PROT_READ | PROT_WRITE), MAP_SHARED, share_buf.fd, 0);
    if (vir_addr == MAP_FAILED) {
        printf("[DM] Failed to mmap the allocated memory.\n");
        goto close_ion;
    }
    mapped = 1;

    // convert from virtual address to physical address
    struct ion_custom_data                ioctl_buf;
    struct ion_uniphier_virt_to_phys_data v2p_buf;
    memset(&ioctl_buf, 0, sizeof(ioctl_buf));
    ioctl_buf.cmd = ION_UNIP_IOC_VIRT_TO_PHYS;
    ioctl_buf.arg = (unsigned long)&v2p_buf;
    memset(&v2p_buf, 0, sizeof(v2p_buf));
    v2p_buf.handle = alloc_buf.handle;
    v2p_buf.virt   = (uint64_t)(uintptr_t)vir_addr;
    v2p_buf.len    = alloc_buf.len;
    ret_ioctl = ioctl(fd_ion, ION_IOC_CUSTOM, &ioctl_buf);
    if (ret_ioctl != 0) {
        printf("[DM] Failed to ion ioctl(custom, virt_to_phys).\n");
        goto close_ion;
    }

    phy_addr = v2p_buf.phys;
    ALOGD("[DM] Success physical addrees:%lx, size:0x%zx.\n", phy_addr, alloc_buf.len);

#if defined(API_IS_28)
    *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER,
               alloc_buf.len, vir_addr, GRALLOC_USAGE_HW_FB, 0, dup(fd_ion), 0, w, w, h, HAL_PIXEL_FORMAT_RGBA_8888);
#else
#if defined(API_IS_26)
    *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER,
               alloc_buf.len, vir_addr, GRALLOC_USAGE_HW_FB, 0, dup(fd_ion), 0);
#else
#if defined(API_IS_20)
    *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER,
               alloc_buf.len, vir_addr, GRALLOC_USAGE_HW_FB, 0, private_handle_t::LOCK_STATE_MAPPED, dup(fd_ion), 0);
#else
    *hnd = new private_handle_t(private_handle_t::PRIV_FLAGS_FRAMEBUFFER,
                GRALLOC_USAGE_HW_FB, alloc_buf.len, vir_addr, 0, dup(fd_ion), 0, 0);
#endif
#endif
#endif

    (*hnd)->pbase = (void *)(uintptr_t)phy_addr;
    (*hnd)->size  = alloc_buf.len;

close_ion:
    if (mapped == 1) {
        munmap(vir_addr, alloc_buf.len);
    }
    if (alloced == 1) {
        ioctl(fd_ion, ION_IOC_FREE, &alloc_buf);
    }
    if (opened == 1) {
        close(fd_ion);
        fd_ion = -1;
    }
    return;
}


int main(int, char**) {
    private_handle_t *hnd = NULL;
    // instantiate DisplayManager
    sp<DisplayManager> DM = new DisplayManager();

    get_physical_address(&hnd);
    DM->init();
    DM->run(false);
#ifdef USE_HWC_V15
    DM->SetPowerMode(HWC_POWER_MODE_NORMAL);
#else
    DM->SetPowerMode(0);
#endif
    DM->EmptyBuffer(IDisplayManager::TYPE_OSD, hnd);
    delete hnd;
    return 0;
}
