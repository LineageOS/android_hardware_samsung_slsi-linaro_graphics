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

#ifndef EXYNOS_DRM_FRAMEBUFFER_MANAGER_MODULE_H
#define EXYNOS_DRM_FRAMEBUFFER_MANAGER_MODULE_H

#include "ExynosDrmFramebufferManager.h"
#include <drm_fourcc.h>

#define SBWC_IDENTIFIER (1 << 4)
#define SBWC_FORMAT_MOD_LOSSY (1 << 5)
#define SBWC_FORMAT_MOD_LOSSLESS (0 << 5)
#define SBWC_FORMAT_MOD_BLK_BYTE_MASK (0x7ULL << 6)
#define SBWC_BLK_BYTE_SET(blk_byte) ((blk_byte << 6) & SBWC_FORMAT_MOD_BLK_BYTE_MASK)
#define SBWC_BLK_BYTE_GET(modifier) (((modifier) & SBWC_FORMAT_MOD_BLK_BYTE_MASK) >> 6)
#define SBWC_FORMAT_MOD_BLK_BYTENUM_32x2 (2ULL)
#define SBWC_FORMAT_MOD_BLK_BYTENUM_32x3 (3ULL)
#define SBWC_FORMAT_MOD_BLK_BYTENUM_32x4 (4ULL)
#define SBWC_FORMAT_MOD_BLK_BYTENUM_32x5 (5ULL)
#define SBWC_FORMAT_MOD_BLK_BYTENUM_32x6 (6ULL)
#define DRM_FORMAT_MOD_SAMSUNG_SBWC(blk_byte, lossy) fourcc_mod_code(SAMSUNG, (SBWC_BLK_BYTE_SET(blk_byte) | lossy | SBWC_IDENTIFIER))

class ExynosDrmFramebufferManagerModule : public FramebufferManager {
public:
    ExynosDrmFramebufferManagerModule();
    ~ExynosDrmFramebufferManagerModule();

    virtual uint64_t getSBWCModifierBits(const format_description &format_desc);
};

#endif
