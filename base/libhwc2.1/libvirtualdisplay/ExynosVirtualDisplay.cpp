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

#undef LOG_TAG
#define LOG_TAG "virtualdisplay"
#include "ExynosVirtualDisplay.h"
#include "../libdevice/ExynosDevice.h"
#include "../libdevice/ExynosLayer.h"

#include "ExynosHWCHelper.h"
#ifdef GRALLOC_VERSION1
#include "gralloc1_priv.h"
#else
#include "gralloc_priv.h"
#endif

extern struct exynos_hwc_control exynosHWCControl;

ExynosVirtualDisplay::ExynosVirtualDisplay(uint32_t index, ExynosDevice *device)
    : ExynosDisplay(index, device)
{
    /* Initialization */
    mType = HWC_DISPLAY_VIRTUAL;
    mIndex = index;
    mDisplayId = getDisplayId(mType, mIndex);

    mDisplayControl.earlyStartMPP = false;

    mOutputBufferAcquireFenceFd = -1;
    mOutputBufferReleaseFenceFd = -1;

    mIsWFDState = 0;
    mIsSecureVDSState = false;
    mIsSkipFrame = false;
    mPresentationMode = false;

    // TODO : Hard coded currently
    mNumMaxPriorityAllowed = 1;

    mDisplayWidth = 0;
    mDisplayHeight = 0;
    mOutputBuffer = NULL;
    mCompositionType = COMPOSITION_GLES;
    mGLESFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    mSinkUsage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER;
    mIsSecureDRM = false;
    mIsNormalDRM = false;
    mNeedReloadResourceForHWFC = false;
    mMinTargetLuminance = 0;
    mMaxTargetLuminance = 100;
    mSinkDeviceType = 0;

    mUseDpu = false;
    mDisplayControl.enableExynosCompositionOptimization = false;
    mDisplayControl.enableClientCompositionOptimization = false;
    mDisplayControl.handleLowFpsLayers = false;
    mMaxWindowNum = 0;
}

ExynosVirtualDisplay::~ExynosVirtualDisplay()
{

}

void ExynosVirtualDisplay::createVirtualDisplay(uint32_t width, uint32_t height, int32_t* format)
{
    ALOGI("Virtual display is added. width(%d), height(%d), format(%d)", width, height, *format);

    initDisplay();

    // Virtual Display don't use skip static layer.
    mClientCompositionInfo.mEnableSkipStatic = false;

    mPlugState = true;
    mDisplayWidth = width;
    mDisplayHeight = height;
    mXres = width;
    mYres = height;
    mGLESFormat = *format;
}

void ExynosVirtualDisplay::destroyVirtualDisplay()
{
    ALOGI("Virtual display is deleted");

    mPlugState = false;
    mDisplayWidth = 0;
    mDisplayHeight = 0;
    mXres = 0;
    mYres = 0;
    mMinTargetLuminance = 0;
    mMaxTargetLuminance = 100;
    mSinkDeviceType = 0;
    mCompositionType = COMPOSITION_GLES;
    mGLESFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    mResourceManager->reloadResourceForHWFC();
    mResourceManager->setTargetDisplayLuminance(mMinTargetLuminance, mMaxTargetLuminance);
    mResourceManager->setTargetDisplayDevice(mSinkDeviceType);
    mNeedReloadResourceForHWFC = false;
}

int ExynosVirtualDisplay::setWFDMode(unsigned int mode)
{
    if ((mode == GOOGLEWFD_TO_LLWFD || mode == LLWFD_TO_GOOGLEWFD))
        mNeedReloadResourceForHWFC = true;
    mIsWFDState = mode;
    return HWC2_ERROR_NONE;
}

int ExynosVirtualDisplay::getWFDMode()
{
    return mIsWFDState;
}

int ExynosVirtualDisplay::getWFDInfo(int32_t* state, int32_t* compositionType, int32_t* format,
    int64_t* usage, int32_t* width, int32_t* height)
{
    *state = mIsWFDState;
    *compositionType = mCompositionType;
    if (mIsSkipFrame)
        *format = (int32_t)0xFFFFFFFF;
    else if (mIsSecureDRM && !mIsSecureVDSState)
        *format = (int32_t)HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN;
    else
        *format = (int32_t)mGLESFormat;
    *usage = mSinkUsage;
    *width = mDisplayWidth;
    *height = mDisplayHeight;

    return HWC2_ERROR_NONE;
}

int ExynosVirtualDisplay::sendWFDCommand(int32_t cmd, int32_t ext1, int32_t ext2)
{
    ALOGI("sendWFDCommand: cmd(%d), ext1(%d), ext2(%d)", cmd, ext1, ext2);

    int ret = 0;

    switch (cmd) {
        case SET_WFD_MODE:
            /* ext1: mode, ext2: unused */
            ret = setWFDMode(ext1);
            break;
        case SET_TARGET_DISPLAY_LUMINANCE:
            /* ext1: min, ext2: max */
            mMinTargetLuminance = (uint16_t)ext1;
            mMaxTargetLuminance = (uint16_t)ext2;
            mResourceManager->setTargetDisplayLuminance(mMinTargetLuminance, mMaxTargetLuminance);
            break;
        case SET_TARGET_DISPLAY_DEVICE:
            /* ext1: type, ext2: unused */
            mSinkDeviceType = ext1;
            mResourceManager->setTargetDisplayDevice(mSinkDeviceType);
            break;
        default:
            ALOGE("invalid cmd(%d)", cmd);
            break;
    }

    return ret;
}

int ExynosVirtualDisplay::setSecureVDSMode(unsigned int mode)
{
    mIsWFDState = mode;
    mIsSecureVDSState = !!mode;
    return HWC2_ERROR_NONE;
}

int ExynosVirtualDisplay::setWFDOutputResolution(
    unsigned int width, unsigned int height)
{
    ALOGI("setWFDOutputResolution width(%d), height(%d)", width, height);
    mDisplayWidth = width;
    mDisplayHeight = height;
    mXres = width;
    mYres = height;
    return HWC2_ERROR_NONE;
}

void ExynosVirtualDisplay::getWFDOutputResolution(
    unsigned int *width, unsigned int *height)
{
    *width = mDisplayWidth;
    *height = mDisplayHeight;
}

void ExynosVirtualDisplay::setPresentationMode(bool use)
{
    mPresentationMode = use;
}

int ExynosVirtualDisplay::getPresentationMode(void)
{
    return mPresentationMode;
}

int ExynosVirtualDisplay::setVDSGlesFormat(int format)
{
    DISPLAY_LOGD(eDebugVirtualDisplay, "setVDSGlesFormat: 0x%x", format);
    mGLESFormat = format;
    return HWC2_ERROR_NONE;
}

bool ExynosVirtualDisplay::is2StepBlendingRequired(exynos_image &src, private_handle_t *outbuf)
{
    return false;
}

int32_t ExynosVirtualDisplay::setOutputBuffer(
    buffer_handle_t buffer, int32_t releaseFence) {
    mOutputBuffer = buffer;
    mOutputBufferAcquireFenceFd = hwc_dup(releaseFence,
            this, FENCE_TYPE_DST_RELEASE, FENCE_IP_OUTBUF);

    if (mExynosCompositionInfo.mM2mMPP != NULL) {
        mExynosCompositionInfo.mM2mMPP->setOutBuf(mOutputBuffer, mOutputBufferAcquireFenceFd, this);
        mOutputBufferAcquireFenceFd = -1;
    }

    DISPLAY_LOGD(eDebugVirtualDisplay, "setOutputBuffer(), mOutputBufferAcquireFenceFd %d", mOutputBufferAcquireFenceFd);
    return HWC2_ERROR_NONE;
}

int ExynosVirtualDisplay::clearDisplay(bool readback) {
    return 0;
}

void ExynosVirtualDisplay::doPreProcessing() {
    ExynosDisplay::doPreProcessing();

    /*
     * If there is layer that has priority higher than ePriorityMid
     * exynos composition handles only one layer that has the highest priority.
     * If there are more than one layer with same priority
     * exynos composition handles top layer.
     */
    int32_t maxPriorityIndex = -1;
    for (size_t i = 0; i < mLayers.size(); i++) {
        if (mLayers[i]->mOverlayPriority >= ePriorityHigh)
        {
            DISPLAY_LOGD(eDebugResourceManager, "\t[%zu] layer has high priority(%d)",
                    i, mLayers[i]->mOverlayPriority);
            if (maxPriorityIndex < 0)
            {
                maxPriorityIndex = i;
            } else {
                if (mLayers[i]->mOverlayPriority >= mLayers[maxPriorityIndex]->mOverlayPriority)
                    maxPriorityIndex = i;
            }
        }
    }
    if (maxPriorityIndex >= 0) {
        DISPLAY_LOGD(eDebugResourceManager, "\texynos composition will be assgined for only [%d] layer", maxPriorityIndex);
    }

    for (size_t i = 0; i < mLayers.size(); i++) {
        if ((maxPriorityIndex >= 0) &&
            (i != maxPriorityIndex)) {
            mLayers[i]->mLayerFlag |= EXYNOS_HWC_FORCE_CLIENT;
        } else {
            mLayers[i]->mLayerFlag &= ~(EXYNOS_HWC_FORCE_CLIENT);
        }
    }
}

int32_t ExynosVirtualDisplay::validateDisplay(
    uint32_t* outNumTypes, uint32_t* outNumRequests)
{
    DISPLAY_LOGD(eDebugVirtualDisplay, "validateDisplay");
    int32_t ret = HWC2_ERROR_NONE;

    initPerFrameData();

    mClientCompositionInfo.setCompressed(false);

    if (mNeedReloadResourceForHWFC) {
        ALOGI("validateDisplay() mIsWFDState %d", mIsWFDState);
        mResourceManager->reloadResourceForHWFC();
        mResourceManager->setTargetDisplayLuminance(mMinTargetLuminance, mMaxTargetLuminance);
        mResourceManager->setTargetDisplayDevice(mSinkDeviceType);
        mNeedReloadResourceForHWFC = false;
    }

    /* validateDisplay should be called for preAssignResource */
    ret = ExynosDisplay::validateDisplay(outNumTypes, outNumRequests);

    if (checkSkipFrame()) {
        handleSkipFrame();
    } else {
        setDrmMode();
        setSinkBufferUsage();
        setCompositionType();
    }

    return ret;
}

int32_t ExynosVirtualDisplay::canSkipValidate() {
    if (checkSkipFrame())
        return SKIP_ERR_FORCE_VALIDATE;

    return ExynosDisplay::canSkipValidate();
}

int32_t ExynosVirtualDisplay::presentDisplay(
    int32_t* outRetireFence)
{
    DISPLAY_LOGD(eDebugVirtualDisplay, "presentDisplay, mCompositionType %d",
        mCompositionType);

    int32_t ret = HWC2_ERROR_NONE;

    if ((ret = handleSkipPresent(outRetireFence)) >= 0)
        return ret;
    else
        ret = HWC2_ERROR_NONE;

    if (mIsSkipFrame) {
        if ((exynosHWCControl.skipValidate == true) &&
            ((mRenderingState == RENDERING_STATE_PRESENTED) ||
             (mRenderingState == RENDERING_STATE_NONE) ||
             /* validated by first validate */
             (mRenderingState == RENDERING_STATE_VALIDATED))) {

            if (mDevice->canSkipValidate() == false) {
                mRenderingState = RENDERING_STATE_NONE;
                return HWC2_ERROR_NOT_VALIDATED;
            } else {
                DISPLAY_LOGD(eDebugSkipValidate, "validate is skipped");
            }
        }

        handleAcquireFence();
        /* this frame is not presented, but mRenderingState is updated to RENDERING_STATE_PRESENTED */
        mRenderingState = RENDERING_STATE_PRESENTED;
        setPresentAndClearRenderingStatesFlags();

        initPerFrameData();
        return ret;
    }

    ret = ExynosDisplay::presentDisplay(outRetireFence);

    if (ret == HWC2_ERROR_NOT_VALIDATED) {
        DISPLAY_LOGD(eDebugVirtualDisplay, "need validate");
        return ret;
    }

    if (*outRetireFence == -1 && mOutputBufferReleaseFenceFd >= 0) {
        *outRetireFence = mOutputBufferReleaseFenceFd;
        mOutputBufferReleaseFenceFd = -1;
    }

    DISPLAY_LOGD(eDebugVirtualDisplay, "presentDisplay(), outRetireFence %d", *outRetireFence);

    return ret;
}

int ExynosVirtualDisplay::setWinConfigData()
{
    return NO_ERROR;
}

int ExynosVirtualDisplay::setDisplayWinConfigData()
{
    return NO_ERROR;
}

int32_t ExynosVirtualDisplay::validateWinConfigData()
{
    return NO_ERROR;
}

int ExynosVirtualDisplay::deliverWinConfigData()
{
    mDpuData.retire_fence = -1;
    return 0;
}

int ExynosVirtualDisplay::setReleaseFences()
{
    DISPLAY_LOGD(eDebugVirtualDisplay, "setReleaseFences(), mCompositionType %d", mCompositionType);

    int ret = 0;

    if (mClientCompositionInfo.mHasCompositionLayer) {
        int fence;
        uint32_t framebufferTargetIndex;
        framebufferTargetIndex = mExynosCompositionInfo.mM2mMPP->getAssignedSourceNum() - 1;
        fence = mExynosCompositionInfo.mM2mMPP->getSrcReleaseFence(framebufferTargetIndex);
        if (fence > 0)
            fence_close(fence, this, FENCE_TYPE_SRC_ACQUIRE, FENCE_IP_FB);
    }

    ret = ExynosDisplay::setReleaseFences();

    mOutputBufferReleaseFenceFd = hwcCheckFenceDebug(this, FENCE_TYPE_RETIRE, FENCE_IP_G2D, mExynosCompositionInfo.mAcquireFence);
    setFenceInfo(mExynosCompositionInfo.mAcquireFence, this, FENCE_TYPE_RETIRE, FENCE_IP_G2D, FENCE_TO);
    mExynosCompositionInfo.mAcquireFence = -1;
    /* mClientCompositionInfo.mAcquireFence is delivered to G2D */
    mClientCompositionInfo.mAcquireFence = -1;

    return ret;
}

bool ExynosVirtualDisplay::checkFrameValidation()
{
    if (mOutputBuffer == NULL) {
        handleAcquireFence();
        return false;
    }

    private_handle_t *outbufHandle = private_handle_t::dynamicCast(mOutputBuffer);
    if (outbufHandle == NULL) {
        handleAcquireFence();
        return false;
    }

    if (mCompositionType != COMPOSITION_HWC) {
        if (mClientCompositionInfo.mTargetBuffer == NULL) {
            handleAcquireFence();
            return false;
        }
    }

    return true;

}

void ExynosVirtualDisplay::setSinkBufferUsage()
{
    mSinkUsage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER;
    if (mIsSecureDRM) {
        mSinkUsage |= GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_PROTECTED;
    } else if (mIsNormalDRM)
        mSinkUsage |= GRALLOC_USAGE_PRIVATE_NONSECURE;
}

void ExynosVirtualDisplay::setCompositionType()
{
    size_t compositionClientLayerCount = 0;
    size_t CompositionDeviceLayerCount = 0;;
    for (size_t i = 0; i < mLayers.size(); i++) {
        ExynosLayer *layer = mLayers[i];
        if (layer->mValidateCompositionType == HWC2_COMPOSITION_CLIENT ||
            layer->mValidateCompositionType == HWC2_COMPOSITION_INVALID) {
            compositionClientLayerCount++;
        }
        if (layer->mValidateCompositionType == HWC2_COMPOSITION_DEVICE ||
            layer->mValidateCompositionType == HWC2_COMPOSITION_EXYNOS) {
            CompositionDeviceLayerCount++;
        }
    }
    if (compositionClientLayerCount > 0 && CompositionDeviceLayerCount > 0) {
        mCompositionType = COMPOSITION_MIXED;
    } else if (CompositionDeviceLayerCount > 0) {
        mCompositionType = COMPOSITION_HWC;
    } else {
        mCompositionType = COMPOSITION_GLES;
    }

    if (mCompositionType == COMPOSITION_GLES)
        mCompositionType = COMPOSITION_MIXED;

    DISPLAY_LOGD(eDebugVirtualDisplay, "setCompositionType(), compositionClientLayerCount %zu, CompositionDeviceLayerCount %zu, mCompositionType %d",
        compositionClientLayerCount, CompositionDeviceLayerCount, mCompositionType);
}

void ExynosVirtualDisplay::initPerFrameData()
{
    mIsSkipFrame = false;
    mIsSecureDRM = false;
    mIsNormalDRM = false;
    mCompositionType = COMPOSITION_HWC;
    mSinkUsage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER;
}

bool ExynosVirtualDisplay::checkSkipFrame()
{
    if (mLayers.size() == 0) {
        DISPLAY_LOGD(eDebugVirtualDisplay, "checkSkipFrame(), mLayers.size() %zu", mLayers.size());
        return true;
    }

    if (mIsWFDState == 0) {
        DISPLAY_LOGD(eDebugVirtualDisplay, "checkSkipFrame(), mIsWFDState %d", mIsWFDState);
        return true;
    }

    if (mIsWFDState == GOOGLEWFD_TO_LLWFD || mIsWFDState == LLWFD_TO_GOOGLEWFD) {
        DISPLAY_LOGD(eDebugVirtualDisplay, "checkSkipFrame(), mIsWFDState %d", mIsWFDState);
        return true;
    }

    return false;
}

void ExynosVirtualDisplay::setDrmMode()
{
    mIsSecureDRM = false;
    for (size_t i = 0; i < mLayers.size(); i++) {
        ExynosLayer *layer = mLayers[i];
        if ((layer->mValidateCompositionType == HWC2_COMPOSITION_DEVICE ||
            layer->mValidateCompositionType == HWC2_COMPOSITION_EXYNOS) &&
            layer->mLayerBuffer && getDrmMode(layer->mLayerBuffer) == SECURE_DRM) {
            mIsSecureDRM = true;
            DISPLAY_LOGD(eDebugVirtualDisplay, "include secure drm layer");
        }
        if ((layer->mValidateCompositionType == HWC2_COMPOSITION_DEVICE ||
            layer->mValidateCompositionType == HWC2_COMPOSITION_EXYNOS) &&
            layer->mLayerBuffer && getDrmMode(layer->mLayerBuffer) == NORMAL_DRM) {
            mIsNormalDRM = true;
            DISPLAY_LOGD(eDebugVirtualDisplay, "include normal drm layer");
        }
    }
    DISPLAY_LOGD(eDebugVirtualDisplay, "setDrmMode(), mIsSecureDRM %d", mIsSecureDRM);
}

void ExynosVirtualDisplay::handleSkipFrame()
{
    mIsSkipFrame = true;
    for (size_t i = 0; i < mLayers.size(); i++) {
        ExynosLayer *layer = mLayers[i];
        layer->mValidateCompositionType = HWC2_COMPOSITION_EXYNOS;
    }
    mIsSecureDRM = false;
    mIsNormalDRM = false;
    mCompositionType = COMPOSITION_HWC;
    mSinkUsage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_VIDEO_ENCODER;
    DISPLAY_LOGD(eDebugVirtualDisplay, "handleSkipFrame()");
}

void ExynosVirtualDisplay::handleAcquireFence()
{
    /* handle fence of DEVICE or EXYNOS composition layers */
    for (size_t i = 0; i < mLayers.size(); i++) {
        ExynosLayer *layer = mLayers[i];
        if (layer->mValidateCompositionType == HWC2_COMPOSITION_DEVICE ||
            layer->mValidateCompositionType == HWC2_COMPOSITION_EXYNOS)
            layer->mAcquireFence = fence_close(layer->mAcquireFence,
                    this, FENCE_TYPE_SRC_ACQUIRE, FENCE_IP_ALL);
    }

    mClientCompositionInfo.mAcquireFence = fence_close(mClientCompositionInfo.mAcquireFence,
            this, FENCE_TYPE_SRC_ACQUIRE, FENCE_IP_FB);
    mOutputBufferAcquireFenceFd = fence_close(mOutputBufferAcquireFenceFd,
            this, FENCE_TYPE_DST_ACQUIRE, FENCE_IP_ALL);
    DISPLAY_LOGD(eDebugVirtualDisplay, "handleAcquireFence()");
}

int32_t ExynosVirtualDisplay::getHdrCapabilities(uint32_t* outNumTypes,
        int32_t* outTypes, float* outMaxLuminance,
        float* outMaxAverageLuminance, float* outMinLuminance)
{
    if (outTypes == NULL) {
        *outNumTypes = 1;
        return 0;
    }
    outTypes[0] = HAL_HDR_HDR10;
    return 0;
}
