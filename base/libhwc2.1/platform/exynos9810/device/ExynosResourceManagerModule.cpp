/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include "ExynosResourceManagerModule.h"
#include "ExynosMPPModule.h"
#include "ExynosPrimaryDisplayModule.h"

ExynosResourceManagerModule::ExynosResourceManagerModule()
        : ExynosResourceManager()
{
}

ExynosResourceManagerModule::~ExynosResourceManagerModule()
{
}

int32_t ExynosResourceManagerModule::assignCompositionTarget(
        ExynosDisplay *display, uint32_t targetType)
{
    if (display->mType != HWC_DISPLAY_EXTERNAL ||
            targetType != COMPOSITION_CLIENT || !display->mUseDpu)
        return ExynosResourceManager::assignCompositionTarget(display, targetType);

    auto &compositionInfo = display->mClientCompositionInfo;
    const compressionInfo_t compressionInfo = compositionInfo.mCompressionInfo;

    /* The new client target arrives after resource validation. */
    /* Reserve a channel that also supports an AFBC replacement. */
    compositionInfo.mCompressionInfo.type = COMP_TYPE_AFBC;
    int32_t ret = ExynosResourceManager::assignCompositionTarget(display, targetType);
    compositionInfo.mCompressionInfo = compressionInfo;
    return ret;
}

void ExynosResourceManagerModule::preAssignWindows()
{
    ExynosPrimaryDisplayModule *primaryDisplay =
        (ExynosPrimaryDisplayModule *)getDisplay(getDisplayId(HWC_DISPLAY_PRIMARY, 0));
    primaryDisplay->mBaseWindowIndex = 0;
    primaryDisplay->mMaxWindowNum =
        primaryDisplay->mDisplayInterface->getMaxWindowNum();

    bool extraDisplayConnected = false;
    for (auto display: mDisplays) {
        if ((display == nullptr) ||
                (display->mType == HWC_DISPLAY_PRIMARY) ||
                (display->mUseDpu == false))
            continue;
        if (display->mPlugState == true) {
            extraDisplayConnected = true;
        }
    }
    if (extraDisplayConnected) {
        primaryDisplay->mBaseWindowIndex =
            PRIMARY_DISP_BASE_WIN[mDeviceInfo.displayMode];
        primaryDisplay->mMaxWindowNum -= primaryDisplay->mBaseWindowIndex;
    }
}
