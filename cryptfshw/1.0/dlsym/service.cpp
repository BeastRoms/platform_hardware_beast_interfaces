/*
 * Copyright (C) 2019 The LineageOS Project
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

#include <chrono>
#include <thread>

#include <dlfcn.h>

#define LOG_TAG "vendor.qti.hardware.cryptfshw@1.0-service-dlsym-qti"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <binder/ProcessState.h>
#include <hidl/HidlTransportSupport.h>

#include "CryptfsHw.h"

using android::OK;
using android::sp;
using android::status_t;
using android::base::GetBoolProperty;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

using ::vendor::qti::hardware::cryptfshw::V1_0::ICryptfsHw;
using ::vendor::qti::hardware::cryptfshw::V1_0::dlsym_qti::CryptfsHw;

int main() {
    void* libHandle = nullptr;

    sp<CryptfsHw> cryptfsHw;

    status_t status = OK;

    LOG(INFO) << "CryptfsHw HAL service is starting.";

#ifndef SKIP_WAITING_FOR_QSEE
    for (int i = 0; i < CRYPTFS_HW_UP_CHECK_COUNT; i++) {
        if (GetBoolProperty("sys.keymaster.loaded", false)) goto start;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG(ERROR) << "Timed out waiting for QSEECom";
    goto shutdown;
start:
#endif
    libHandle = dlopen("libQSEEComAPI.so", RTLD_NOW);
    if (libHandle == nullptr) {
        LOG(ERROR) << "Can not get libQSEEComAPI.so (" << dlerror() << ")";
        goto shutdown;
    }

    cryptfsHw = new CryptfsHw(libHandle);
    if (cryptfsHw == nullptr) {
        LOG(ERROR) << "Can not create an instance of CryptfsHw HAL CryptfsHw Iface, exiting.";
        goto shutdown;
    }

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    status = cryptfsHw->registerAsService();
    if (status != OK) {
        LOG(ERROR) << "Could not register service for CryptfsHw HAL CryptfsHw Iface (" << status
                   << ")";
        goto shutdown;
    }

    LOG(INFO) << "CryptfsHw HAL service is ready.";
    joinRpcThreadpool();
    // Should not pass this line

shutdown:
    if (libHandle != nullptr) dlclose(libHandle);

    // In normal operation, we don't expect the thread pool to shutdown
    LOG(ERROR) << "CryptfsHw HAL service is shutting down.";
    return 1;
}
