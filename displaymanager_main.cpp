// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2016 Socionext Inc.
 *
 * based on frameworks/native/services/surfaceflinger/main_surfaceflinger.cpp
 */

#include <sys/resource.h>

#include <cutils/sched_policy.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include "DisplayManager.h"

using namespace android;

int main(int, char**) {

#ifdef USE_VNDSERVICE
    ProcessState::initWithDriver("/dev/vndbinder");
#endif /* USE_VNDSERVICE */
    // When SF is launched in its own process, limit the number of
    // binder threads to 4.
    ProcessState::self()->setThreadPoolMaxThreadCount(4);

    // start the thread pool
    sp<ProcessState> ps(ProcessState::self());
    ps->startThreadPool();

    // Get prop;
    

    // instantiate DisplayManager
    sp<DisplayManager> DM = new DisplayManager();

    setpriority(PRIO_PROCESS, 0, PRIORITY_URGENT_DISPLAY);

    set_sched_policy(0, SP_FOREGROUND);

    // initialize before clients can connect
    DM->init();

    // publish DisplayManager
    sp<IServiceManager> sm(defaultServiceManager());
    sm->addService(String16(DisplayManager::getServiceName()), DM, false);

    // run in this thread
    DM->run();

    return 0;
}
