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

#include "ExynosDrmFramebufferManagerModule.h"

ExynosDrmFramebufferManagerModule::ExynosDrmFramebufferManagerModule() {
}

ExynosDrmFramebufferManagerModule::~ExynosDrmFramebufferManagerModule () {
}

uint64_t ExynosDrmFramebufferManagerModule::getSBWCModifierBits(const format_description &format_desc) {
    uint32_t sbwcType = format_desc.type & FORMAT_SBWC_MASK;
    if (sbwcType == 0) {
        return 0;
    }

    uint32_t bitType = format_desc.type & BIT_MASK;
    if (bitType == BIT10) {
        switch (sbwcType) {
        case SBWC_LOSSY_40:
            return DRM_FORMAT_MOD_SAMSUNG_SBWC(SBWC_FORMAT_MOD_BLK_BYTENUM_32x2, SBWC_FORMAT_MOD_LOSSY);
        case SBWC_LOSSY_60:
            return DRM_FORMAT_MOD_SAMSUNG_SBWC(SBWC_FORMAT_MOD_BLK_BYTENUM_32x3, SBWC_FORMAT_MOD_LOSSY);
        case SBWC_LOSSY_80:
            return DRM_FORMAT_MOD_SAMSUNG_SBWC(SBWC_FORMAT_MOD_BLK_BYTENUM_32x4, SBWC_FORMAT_MOD_LOSSY);
        case SBWC_LOSSLESS:
            return DRM_FORMAT_MOD_SAMSUNG_SBWC(SBWC_FORMAT_MOD_BLK_BYTENUM_32x5, SBWC_FORMAT_MOD_LOSSLESS);
        default:
            return 0;
        }
    } else if (bitType == BIT8) {
        switch (sbwcType) {
        case SBWC_LOSSY_50:
            return DRM_FORMAT_MOD_SAMSUNG_SBWC(SBWC_FORMAT_MOD_BLK_BYTENUM_32x2, SBWC_FORMAT_MOD_LOSSY);
        case SBWC_LOSSY_75:
            return DRM_FORMAT_MOD_SAMSUNG_SBWC(SBWC_FORMAT_MOD_BLK_BYTENUM_32x3, SBWC_FORMAT_MOD_LOSSY);
        case SBWC_LOSSLESS:
            return DRM_FORMAT_MOD_SAMSUNG_SBWC(SBWC_FORMAT_MOD_BLK_BYTENUM_32x4, SBWC_FORMAT_MOD_LOSSLESS);
        default:
            return 0;
        }
    }

    return 0;
}
