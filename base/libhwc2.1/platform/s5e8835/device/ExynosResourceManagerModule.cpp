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
#include "ExynosPrimaryDisplayModule.h"
#include "ExynosLayer.h"

#include <algorithm>
#include <utility>
#include <vector>

ExynosResourceManagerModule::ExynosResourceManagerModule()
        : ExynosResourceManager()
{
}

ExynosResourceManagerModule::~ExynosResourceManagerModule()
{
}

bool ExynosResourceManagerModule::isHWResourceAvailable(ExynosDisplay *display,
                                                       ExynosMPP *currentMPP,
                                                       ExynosMPPSource *mppSrc)
{
    if (!display || !currentMPP || !mppSrc)
        return false;

    if (static_cast<uint64_t>(display->mXres) * display->mYres < 2560 * 1600)
        return true;

    return !checkBandwidth(display) && !checkOverlapLimitation(display, mppSrc);
}

bool ExynosResourceManagerModule::checkBandwidth(ExynosDisplay *display)
{
    const float screenArea = static_cast<float>(display->mXres * display->mYres);
    float area = display->mClientCompositionInfo.mHasCompositionLayer ? screenArea : 0.0f;

    for (auto layer : display->mLayers) {
        if (!layer->mOtfMPP)
            continue;

        const auto &src = layer->mSrcImg;
        const auto &dst = layer->mDstImg;
        const bool rotated = dst.transform & HAL_TRANSFORM_ROT_90;
        const float scaleX = std::max(1.0f, static_cast<float>(src.w) /
                                              (rotated ? dst.h : dst.w));
        const float scaleY = std::max(1.0f, static_cast<float>(src.h) /
                                              (rotated ? dst.w : dst.h));
        float layerArea = static_cast<float>(dst.w) * dst.h;
        layerArea *= scaleX;
        layerArea *= scaleY;
        area += layerArea;
    }

    return area > 4.0f * screenArea;
}

bool ExynosResourceManagerModule::checkOverlapLimitation(ExynosDisplay *display,
                                                        ExynosMPPSource *mppSrc)
{
    const auto &dst = mppSrc->mDstImg;
    std::vector<std::pair<uint32_t, int>> edges;
    auto addRegion = [&](uint32_t top, uint32_t bottom) {
        top = std::max(top, dst.y);
        bottom = std::min(bottom, dst.y + dst.h);
        if (top < bottom) {
            edges.emplace_back(top, 1);
            edges.emplace_back(bottom, -1);
        }
    };

    if (display->mClientCompositionInfo.mHasCompositionLayer) {
        const auto &clientDst = display->mClientCompositionInfo.mDstImg;
        addRegion(clientDst.y, clientDst.y + clientDst.h);
    }
    for (auto layer : display->mLayers) {
        if (!layer->mOtfMPP)
            continue;
        const auto &layerDst = layer->mDstImg;
        addRegion(layerDst.y > 19 ? layerDst.y - 20 : 0, layerDst.y + layerDst.h);
    }

    std::sort(edges.begin(), edges.end());
    int overlap = 0;
    for (const auto &edge : edges) {
        overlap += edge.second;
        if (overlap >= 3)
            return true;
    }
    return false;
}

void ExynosResourceManagerModule::preAssignWindows()
{
    ExynosPrimaryDisplayModule *primaryDisplay =
        (ExynosPrimaryDisplayModule *)getDisplay(getDisplayId(HWC_DISPLAY_PRIMARY, 0));
    primaryDisplay->mBaseWindowIndex = 0;
    primaryDisplay->mMaxWindowNum =
        primaryDisplay->mDisplayInterface->getMaxWindowNum();

    bool virtualConnected = false;
    bool externalConnected = false;
    for (auto display: mDisplays) {
        if ((display == nullptr) ||
            (display->mType == HWC_DISPLAY_PRIMARY) ||
            (display->mUseDpu == false))
            continue;
        if (display->mPlugState == true) {
            if (display->mType == HWC_DISPLAY_VIRTUAL)
                virtualConnected = true;
            else if (display->mType == HWC_DISPLAY_EXTERNAL)
                externalConnected = true;
        }
    }
    if (virtualConnected) {
        primaryDisplay->mBaseWindowIndex += PRIMARY_MAIN_VIRTUAL_WINCNT;
        primaryDisplay->mMaxWindowNum -= PRIMARY_MAIN_VIRTUAL_WINCNT;
    } else if (externalConnected) {
        primaryDisplay->mBaseWindowIndex +=
            EXTERNAL_WINDOW_COUNT[mDeviceInfo.displayMode];
        primaryDisplay->mMaxWindowNum -= EXTERNAL_WINDOW_COUNT[mDeviceInfo.displayMode];
    }
}
