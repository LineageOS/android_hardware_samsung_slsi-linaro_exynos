/*
**
** Copyright 2014, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera1SecParameters"
#include <cutils/log.h>

#include "ExynosCamera1Parameters.h"
#include "SecCameraUtil.h"

namespace android {

void ExynosCamera1Parameters::vendorSpecificConstructor(__unused int cameraId)
{
#ifdef SAMSUNG_OIS
    if (cameraId == CAMERA_ID_BACK) {
        mDebugInfo.debugSize[APP_MARKER_4] = sizeof(struct camera2_udm) + sizeof(struct ois_exif_data);
    } else {
        mDebugInfo.debugSize[APP_MARKER_4] = sizeof(struct camera2_udm);
    }
#else
    mDebugInfo.debugSize[APP_MARKER_4] = sizeof(struct camera2_udm);
#endif

    // CAUTION!! : Initial values must be prior to setDefaultParameter() function.
    // Initial Values : START
#if defined(SAMSUNG_COMPANION) || defined(SAMSUNG_EEPROM)
    m_romReadThreadDone = false;
    m_use_companion = isCompanion(cameraId);
#endif
    m_IsThumbnailCallbackOn = false;
    m_fastFpsMode = 0;
    m_previewRunning = false;
    m_previewSizeChanged = false;
    m_pictureRunning = false;
    m_recordingRunning = false;
    m_flagRestartPreviewChecked = false;
    m_flagRestartPreview = false;
    m_reallocBuffer = false;
    m_setFocusmodeSetting = false;
    m_flagMeteringRegionChanged = false;
    m_flagCheckDualMode = false;
    m_flagHWVDisMode = false;
    m_flagVideoStabilization = false;
    m_flag3dnrMode = false;
#ifdef LLS_CAPTURE
    m_flagLLSOn = false;
    m_LLSPreviewOn = false;
    m_LLSCaptureOn = false;
#endif
#ifdef SR_CAPTURE
    m_flagSRSOn = false;
#endif
#ifdef OIS_CAPTURE
    m_flagOISCaptureOn = false;
#endif

    m_flagCheckRecordingHint = false;
    m_zoomWithScaler = false;
#ifdef SAMSUNG_HRM
    m_flagSensorHRM_Hint = false;
#endif
#ifdef SAMSUNG_LIGHT_IR
    m_flagSensorLight_IR_Hint = false;
#endif
#ifdef SAMSUNG_GYRO
    m_flagSensorGyroHint = false;
#endif
#ifdef SAMSUNG_LLV
    m_isLLVOn = true;
#endif
#ifdef SAMSUNG_OT
    m_startObjectTracking = false;
    m_objectTrackingAreaChanged = false;
    m_objectTrackingGet = false;
    int maxNumFocusAreas = getMaxNumFocusAreas();
    m_objectTrackingArea = new ExynosRect2[maxNumFocusAreas];
    m_objectTrackingWeight = new int[maxNumFocusAreas];
#endif
#ifdef SAMSUNG_LBP
    m_useBestPic = USE_LIVE_BEST_PIC;
#endif
    m_enabledMsgType = 0;

    m_previewBufferCount = NUM_PREVIEW_BUFFERS;

    m_dvfsLock = false;

#ifdef SAMSUNG_DOF
    m_curLensStep = 0;
    m_curLensCount = 0;
#endif
#ifdef USE_BINNING_MODE
    m_binningProperty = checkProperty(false);
#endif
#ifdef USE_PREVIEW_CROP_FOR_ROATAION
    m_rotationProperty = checkRotationProperty();
#endif
#ifdef SAMSUNG_OIS
    m_oisNode = NULL;
    m_setOISmodeSetting = false;
#ifdef OIS_CAPTURE
    m_llsValue = 0;
#endif
#endif
    m_zoom_activated = false;
#ifdef FORCE_CAL_RELOAD
    m_calValid = true;
#endif
#ifdef USE_FADE_IN_ENTRANCE
    m_flagFirstEntrance = false;
#endif
#if defined(BURST_CAPTURE) && defined(VARIABLE_BURST_FPS)
    m_burstShotDuration = NORMAL_BURST_DURATION;
#endif
    m_isFactoryMode = false;
}

status_t ExynosCamera1Parameters::setParameters(const CameraParameters& params)
{
    status_t ret = NO_ERROR;

#ifdef TEST_GED_HIGH_SPEED_RECORDING
    int minFpsRange = 0, maxFpsRange = 0;
    int frameRate = 0;

    params.getPreviewFpsRange(&minFpsRange, &maxFpsRange);
    frameRate = params.getPreviewFrameRate();
    CLOGD("DEBUG(%s[%d]):getFastFpsMode=%d, maxFpsRange=%d, frameRate=%d",
        __FUNCTION__, __LINE__, getFastFpsMode(), maxFpsRange, frameRate);
    if (frameRate == 60) {
        setFastFpsMode(1);
    } else if (frameRate == 120) {
        setFastFpsMode(2);
    } else if (frameRate == 240) {
        setFastFpsMode(3);
    } else {
        setFastFpsMode(0);
    }

    CLOGD("DEBUG(%s[%d]):getFastFpsMode=%d", __FUNCTION__, __LINE__, getFastFpsMode());
#endif
#ifdef USE_FADE_IN_ENTRANCE
    if (checkFirstEntrance(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkFirstEntrance faild", __FUNCTION__, __LINE__);
    }
#endif
    /* Return OK means that the vision mode is enabled */
    if (checkVisionMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkVisionMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (getVisionMode() == true) {
        CLOGD("DEBUG(%s[%d]): Vision mode enabled", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    if (checkRecordingHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkRecordingHint fail", __FUNCTION__, __LINE__);

    if (checkDualMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkDualMode fail", __FUNCTION__, __LINE__);

    if (checkDualRecordingHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkDualRecordingHint fail", __FUNCTION__, __LINE__);

    if (checkEffectHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkEffectHint fail", __FUNCTION__, __LINE__);

    if (checkEffectRecordingHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkEffectRecordingHint fail", __FUNCTION__, __LINE__);

    if (checkPreviewFps(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPreviewFps fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (getRecordingRunning() == false) {
        if (checkVideoSize(params) != NO_ERROR)
            CLOGE("ERR(%s[%d]): checkVideoSize fail", __FUNCTION__, __LINE__);
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        if (checkFastFpsMode(params) != NO_ERROR)
            CLOGE("ERR(%s[%d]): checkFastFpsMode fail", __FUNCTION__, __LINE__);
    }

    if (checkVideoStabilization(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkVideoStabilization fail", __FUNCTION__, __LINE__);

    if (checkSWVdisMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSWVdisMode fail", __FUNCTION__, __LINE__);

    bool swVdisUIMode = false;
#ifdef SUPPORT_SW_VDIS
    swVdisUIMode = getVideoStabilization();
#endif /*SUPPORT_SW_VDIS*/
    m_setSWVdisUIMode(swVdisUIMode);

    if (checkVtMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkVtMode fail", __FUNCTION__, __LINE__);

    if (checkHWVdisMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkHWVdisMode fail", __FUNCTION__, __LINE__);

    if (check3dnrMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): check3dnrMode fail", __FUNCTION__, __LINE__);

    if (checkDrcMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkDrcMode fail", __FUNCTION__, __LINE__);

    if (checkOdcMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkOdcMode fail", __FUNCTION__, __LINE__);

    if (checkPreviewSize(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPreviewSize fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkPreviewFormat(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkPreviewFormat fail", __FUNCTION__, __LINE__);

    if (checkPictureSize(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPictureSize fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkPictureFormat(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPictureFormat fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkJpegQuality(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkJpegQuality fail", __FUNCTION__, __LINE__);

    if (checkThumbnailSize(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkThumbnailSize fail", __FUNCTION__, __LINE__);

    if (checkThumbnailQuality(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkThumbnailQuality fail", __FUNCTION__, __LINE__);

    if (checkZoomLevel(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkZoomLevel fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkRotation(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkRotation fail", __FUNCTION__, __LINE__);

    if (checkAutoExposureLock(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAutoExposureLock fail", __FUNCTION__, __LINE__);

    ret = checkExposureCompensation(params);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkExposureCompensation fail", __FUNCTION__, __LINE__);
        return ret;
    }

    if (checkMeteringMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkMeteringMode fail", __FUNCTION__, __LINE__);

    if (checkMeteringAreas(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkMeteringAreas fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkAntibanding(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAntibanding fail", __FUNCTION__, __LINE__);

    if (checkSceneMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkSceneMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkFocusMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkFocusMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkFlashMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkFlashMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkWhiteBalanceMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkWhiteBalanceMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkAutoWhiteBalanceLock(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAutoWhiteBalanceLock fail", __FUNCTION__, __LINE__);

#ifdef SAMSUNG_FOOD_MODE
    if (checkWbLevel(params) != NO_ERROR) {
        ALOGE("ERR(%s[%d]): checkWbLevel fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
#endif

    if (checkFocusAreas(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkFocusAreas fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

#ifdef SAMSUNG_MANUAL_FOCUS
    if (checkFocusDistance(params) != NO_ERROR) {
        ALOGE("ERR(%s[%d]): checkFocusDistance fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
#endif

    if (checkColorEffectMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkColorEffectMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkGpsAltitude(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsAltitude fail", __FUNCTION__, __LINE__);

    if (checkGpsLatitude(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsLatitude fail", __FUNCTION__, __LINE__);

    if (checkGpsLongitude(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsLongitude fail", __FUNCTION__, __LINE__);

    if (checkGpsProcessingMethod(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsProcessingMethod fail", __FUNCTION__, __LINE__);

    if (checkGpsTimeStamp(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsTimeStamp fail", __FUNCTION__, __LINE__);

#if 0
    if (checkCityId(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkCityId fail", __FUNCTION__, __LINE__);

    if (checkWeatherId(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkWeatherId fail", __FUNCTION__, __LINE__);
#endif

    if (checkBrightness(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkBrightness fail", __FUNCTION__, __LINE__);

    if (checkSaturation(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSaturation fail", __FUNCTION__, __LINE__);

    if (checkSharpness(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSharpness fail", __FUNCTION__, __LINE__);

    if (checkHue(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkHue fail", __FUNCTION__, __LINE__);

#ifdef SAMSUNG_COMPANION
    if (getSceneMode() != SCENE_MODE_HDR) {
        if (checkRTDrc(params) != NO_ERROR)
            CLOGE("ERR(%s[%d]): checkRTDrc fail", __FUNCTION__, __LINE__);
    }

    if (checkPaf(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkPaf fail", __FUNCTION__, __LINE__);

    if (getSceneMode() != SCENE_MODE_HDR) {
        if (checkRTHdr(params) != NO_ERROR)
            CLOGE("ERR(%s[%d]): checkRTHdr fail", __FUNCTION__, __LINE__);
    }
#endif

    if (checkIso(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkIso fail", __FUNCTION__, __LINE__);

    if (checkContrast(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkContrast fail", __FUNCTION__, __LINE__);

    if (checkHdrMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkHdrMode fail", __FUNCTION__, __LINE__);

    if (checkWdrMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkWdrMode fail", __FUNCTION__, __LINE__);

    if (checkShotMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkShotMode fail", __FUNCTION__, __LINE__);

    if (checkAntiShake(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAntiShake fail", __FUNCTION__, __LINE__);

    if (checkVRMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkVRMode fail", __FUNCTION__, __LINE__);

    if (checkGamma(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGamma fail", __FUNCTION__, __LINE__);

    if (checkSlowAe(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSlowAe fail", __FUNCTION__, __LINE__);

    if (checkScalableSensorMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkScalableSensorMode fail", __FUNCTION__, __LINE__);
#if 0 // bringup
    if (checkImageUniqueId(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkImageUniqueId fail", __FUNCTION__, __LINE__);
#endif
    if (checkSeriesShotMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSeriesShotMode fail", __FUNCTION__, __LINE__);

#ifdef BURST_CAPTURE
    if (checkSeriesShotFilePath(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSeriesShotFilePath fail", __FUNCTION__, __LINE__);
#endif

#ifdef SAMSUNG_LLV
    if (checkLLV(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkLLV fail", __FUNCTION__, __LINE__);
#endif

#ifdef SAMSUNG_DOF
    if (checkMoveLens(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkMoveLens fail", __FUNCTION__, __LINE__);
#endif

#ifdef SAMSUNG_OIS
    if (checkOIS(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkOIS fail", __FUNCTION__, __LINE__);
#endif

    if (m_getRestartPreviewChecked() == true) {
        CLOGD("DEBUG(%s[%d]):Need restart preview", __FUNCTION__, __LINE__);
        m_setRestartPreview(m_flagRestartPreviewChecked);
    }

    if (checkSetfileYuvRange() != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSetfileYuvRange fail", __FUNCTION__, __LINE__);

#ifdef SAMSUNG_QUICKSHOT
    if (checkQuickShot(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkQuichShot fail", __FUNCTION__, __LINE__);
#endif

    checkHorizontalViewAngle();

    if (checkFactoryMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkFactoryMode fail", __FUNCTION__, __LINE__);

    return ret;
}

void ExynosCamera1Parameters::setDefaultParameter(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;
    CameraParameters p;
    String8 tempStr;
    char strBuf[256];

    m_cameraInfo.autoFocusMacroPosition = ExynosCameraActivityAutofocus::AUTOFOCUS_MACRO_POSITION_BASE;

    /* Preview Size */
    getMaxPreviewSize(&m_cameraInfo.previewW, &m_cameraInfo.previewH);
    m_setHwPreviewSize(m_cameraInfo.previewW, m_cameraInfo.previewH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.previewW, &m_cameraInfo.previewH, MODE_PREVIEW, m_cameraId) != NO_ERROR) {
        CLOGE("ERR(%s):getResolutionList(MODE_PREVIEW) fail", __FUNCTION__);

        m_cameraInfo.previewW = 640;
        m_cameraInfo.previewH = 480;
        tempStr = String8::format("%dx%d", m_cameraInfo.previewW, m_cameraInfo.previewH);
    }

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, tempStr.c_str());
    CLOGD("DEBUG(%s): Default preview size is %dx%d", __FUNCTION__, m_cameraInfo.previewW, m_cameraInfo.previewH);
    p.setPreviewSize(m_cameraInfo.previewW, m_cameraInfo.previewH);

    /* Preview Format */
    tempStr.setTo("");
    tempStr = String8::format("%s,%s", CameraParameters::PIXEL_FORMAT_YUV420SP, CameraParameters::PIXEL_FORMAT_YUV420P);
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, tempStr);
    p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);


    /* Video Size */
    getMaxVideoSize(&m_cameraInfo.maxVideoW, &m_cameraInfo.maxVideoH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.maxVideoW, &m_cameraInfo.maxVideoH, MODE_VIDEO, m_cameraId) != NO_ERROR) {
        CLOGE("ERR(%s):getResolutionList(MODE_VIDEO) fail", __FUNCTION__);

        m_cameraInfo.videoW = 640;
        m_cameraInfo.videoH = 480;
        tempStr = String8::format("%dx%d", m_cameraInfo.maxVideoW, m_cameraInfo.maxVideoH);
    }
#ifdef CAMERA_GED_FEATURE
    else {
#ifdef USE_WQHD_RECORDING
        if (m_addHiddenResolutionList(tempStr, m_staticInfo, 2560, 1440, MODE_VIDEO, m_cameraId) != NO_ERROR) {
            CLOGW("WARN(%s):getResolutionList(MODE_VIDEO) fail", __FUNCTION__);
        }
#endif
#ifdef USE_UHD_RECORDING
        if (m_addHiddenResolutionList(tempStr, m_staticInfo, 3840, 2160, MODE_VIDEO, m_cameraId) != NO_ERROR) {
            CLOGW("WARN(%s):getResolutionList(MODE_VIDEO) fail", __FUNCTION__);
        }
#endif
    }
#endif

    CLOGD("DEBUG(%s): KEY_SUPPORTED_VIDEO_SIZES %s", __FUNCTION__, tempStr.c_str());

    p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, tempStr.c_str());

    CLOGD("DEBUG(%s): Max video size is %dx%d", __FUNCTION__, m_cameraInfo.maxVideoW, m_cameraInfo.maxVideoH);
    CLOGD("DEBUG(%s): Default video size is %dx%d", __FUNCTION__, m_cameraInfo.videoW, m_cameraInfo.videoH);
    p.setVideoSize(m_cameraInfo.videoW, m_cameraInfo.videoH);

    /* Video Format */
    if (getAdaptiveCSCRecording() == true) {
        CLOGI("INFO(%s[%d]):video_frame_foramt == YUV420SP_NV21", __FUNCTION__, __LINE__);
        p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP_NV21);
    } else {
        CLOGI("INFO(%s[%d]):video_frame_foramt == YUV420SP", __FUNCTION__, __LINE__);
        p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
    }

    /* Preferred preview size for Video */
    tempStr.setTo("");
    tempStr = String8::format("%dx%d", m_cameraInfo.previewW, m_cameraInfo.previewH);
    p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, tempStr.c_str());

    /* Picture Size */
    getMaxPictureSize(&m_cameraInfo.pictureW, &m_cameraInfo.pictureH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.pictureW, &m_cameraInfo.pictureH, MODE_PICTURE, m_cameraId) != NO_ERROR) {
        CLOGE("ERR(%s):m_getResolutionList(MODE_PICTURE) fail", __FUNCTION__);

        m_cameraInfo.pictureW = 640;
        m_cameraInfo.pictureW = 480;
        tempStr = String8::format("%dx%d", m_cameraInfo.pictureW, m_cameraInfo.pictureH);
    }

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, tempStr.c_str());
    CLOGD("DEBUG(%s): Default picture size is %dx%d", __FUNCTION__, m_cameraInfo.pictureW, m_cameraInfo.pictureH);
    p.setPictureSize(m_cameraInfo.pictureW, m_cameraInfo.pictureH);

    /* Picture Format */
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);
    p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);

    /* Jpeg Quality */
    p.set(CameraParameters::KEY_JPEG_QUALITY, "96"); /* maximum quality */

    /* Thumbnail Size */
    getMaxThumbnailSize(&m_cameraInfo.thumbnailW, &m_cameraInfo.thumbnailH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.thumbnailW, &m_cameraInfo.thumbnailH, MODE_THUMBNAIL, m_cameraId) != NO_ERROR) {
        tempStr = String8::format("%dx%d", m_cameraInfo.thumbnailW, m_cameraInfo.thumbnailH);
    }
    /* 0x0 is no thumbnail mode */
    tempStr.append(",0x0");
    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, tempStr.c_str());
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,  m_cameraInfo.thumbnailW);
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, m_cameraInfo.thumbnailH);

    /* Thumbnail Quality */
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "100");

    /* Exposure */
    p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, getMinExposureCompensation());
    p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, getMaxExposureCompensation());
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, 0);
    p.setFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, getExposureCompensationStep());

#ifdef USE_EV_SETTING_FOR_VIDEOCALL
    char propertyValue[PROPERTY_VALUE_MAX];
    property_get("sys.hangouts.fps", propertyValue, "0");
    if (strcmp(propertyValue, "15000") == 0) {
        p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, 3);
    }
#endif

    /* Auto Exposure Lock supported */
    if (getAutoExposureLockSupported() == true)
        p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "false");

    /* Face Detection */
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, getMaxNumDetectedFaces());
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, 0);

    /* Video Sanptshot Supported */
    if (getVideoSnapshotSupported() == true)
        p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "false");

    /* Video Stabilization Supported */
    if (getVideoStabilizationSupported() == true)
        p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, "false");

    /* Focus Mode */
    int focusMode = getSupportedFocusModes();
    tempStr.setTo("");
    if (focusMode & FOCUS_MODE_AUTO) {
        tempStr.append(CameraParameters::FOCUS_MODE_AUTO);
    } else if (focusMode & FOCUS_MODE_FIXED){
        tempStr.append(CameraParameters::FOCUS_MODE_FIXED);
    }
    if (focusMode & FOCUS_MODE_INFINITY) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_INFINITY);
    }
    if (focusMode & FOCUS_MODE_MACRO) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_MACRO);
    }
    if (focusMode & FOCUS_MODE_EDOF) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_EDOF);
    }
    if (focusMode & FOCUS_MODE_CONTINUOUS_VIDEO) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);
    }
    if (focusMode & FOCUS_MODE_CONTINUOUS_PICTURE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
    }
    if (focusMode & FOCUS_MODE_CONTINUOUS_PICTURE_MACRO) {
        tempStr.append(",");
        tempStr.append("continuous-picture-macro");
    }

    p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
          tempStr.c_str());

    if (focusMode & FOCUS_MODE_AUTO)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_AUTO);
    else if (focusMode & FOCUS_MODE_CONTINUOUS_PICTURE)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
    else if (focusMode & FOCUS_MODE_CONTINUOUS_VIDEO)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);
    else if (focusMode & FOCUS_MODE_FIXED)
        p.set(CameraParameters::KEY_FOCUS_MODE,
          CameraParameters::FOCUS_MODE_FIXED);
    else
        p.set(CameraParameters::KEY_FOCUS_MODE,
          CameraParameters::FOCUS_MODE_INFINITY);

/*TODO: This values will be changed */
#define BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR       "0.10,1.20,Infinity"
#define FRONT_CAMERA_FOCUS_DISTANCES_STR           "0.20,0.25,Infinity"

#define BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR      "0.10,0.20,Infinity"
#define BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR   "0.10,1.20,Infinity"

    /* Focus Distances */
    if (getCameraId() == CAMERA_ID_BACK)
        p.set(CameraParameters::KEY_FOCUS_DISTANCES,
              BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
    else
        p.set(CameraParameters::KEY_FOCUS_DISTANCES,
              FRONT_CAMERA_FOCUS_DISTANCES_STR);

    p.set(CameraParameters::FOCUS_DISTANCE_INFINITY, "Infinity");

    /* Max number of Focus Areas */
    p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, 0);
    if (focusMode & FOCUS_MODE_TOUCH) {
        p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, 1);
        p.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
    }

    /* Flash */
    int flashMode = getSupportedFlashModes();
    if (flashMode != FLASH_MODE_OFF) {
        tempStr.setTo("");
        if (flashMode & FLASH_MODE_OFF) {
            tempStr.append(CameraParameters::FLASH_MODE_OFF);
        }
        if (flashMode & FLASH_MODE_AUTO) {
            tempStr.append(",");
            tempStr.append(CameraParameters::FLASH_MODE_AUTO);
        }
        if (flashMode & FLASH_MODE_ON) {
            tempStr.append(",");
            tempStr.append(CameraParameters::FLASH_MODE_ON);
        }
        if (flashMode & FLASH_MODE_RED_EYE) {
            tempStr.append(",");
            tempStr.append(CameraParameters::FLASH_MODE_RED_EYE);
        }
        if (flashMode & FLASH_MODE_TORCH) {
            tempStr.append(",");
            tempStr.append(CameraParameters::FLASH_MODE_TORCH);
        }

        p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, tempStr.c_str());
        p.set(CameraParameters::KEY_FLASH_MODE, CameraParameters::FLASH_MODE_OFF);
    }

    /* scene mode */
    int sceneMode = getSupportedSceneModes();
    tempStr.setTo("");
    if (sceneMode & SCENE_MODE_AUTO) {
        tempStr.append(CameraParameters::SCENE_MODE_AUTO);
    }
    if (sceneMode & SCENE_MODE_ACTION) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_ACTION);
    }
    if (sceneMode & SCENE_MODE_PORTRAIT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_PORTRAIT);
    }
    if (sceneMode & SCENE_MODE_LANDSCAPE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_LANDSCAPE);
    }
    if (sceneMode & SCENE_MODE_NIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_NIGHT);
    }
    if (sceneMode & SCENE_MODE_NIGHT_PORTRAIT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_NIGHT_PORTRAIT);
    }
    if (sceneMode & SCENE_MODE_THEATRE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_THEATRE);
    }
    if (sceneMode & SCENE_MODE_BEACH) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_BEACH);
    }
    if (sceneMode & SCENE_MODE_SNOW) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_SNOW);
    }
    if (sceneMode & SCENE_MODE_SUNSET) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_SUNSET);
    }
    if (sceneMode & SCENE_MODE_STEADYPHOTO) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_STEADYPHOTO);
    }
    if (sceneMode & SCENE_MODE_FIREWORKS) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_FIREWORKS);
    }
    if (sceneMode & SCENE_MODE_SPORTS) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_SPORTS);
    }
    if (sceneMode & SCENE_MODE_PARTY) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_PARTY);
    }
    if (sceneMode & SCENE_MODE_CANDLELIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_CANDLELIGHT);
    }
#ifdef SAMSUNG_COMPANION
    if (sceneMode & SCENE_MODE_HDR) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_HDR);
    }
#endif

    p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,
          tempStr.c_str());
    p.set(CameraParameters::KEY_SCENE_MODE,
          CameraParameters::SCENE_MODE_AUTO);

    if (getHalVersion() != IS_HAL_VER_3_2) {
        /* effect */
        int effect = getSupportedColorEffects();
        tempStr.setTo("");
        if (effect & EFFECT_NONE) {
            tempStr.append(CameraParameters::EFFECT_NONE);
        }
        if (effect & EFFECT_MONO) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_MONO);
        }
        if (effect & EFFECT_NEGATIVE) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_NEGATIVE);
        }
        if (effect & EFFECT_SOLARIZE) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_SOLARIZE);
        }
        if (effect & EFFECT_SEPIA) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_SEPIA);
        }
        if (effect & EFFECT_POSTERIZE) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_POSTERIZE);
        }
        if (effect & EFFECT_WHITEBOARD) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_WHITEBOARD);
        }
        if (effect & EFFECT_BLACKBOARD) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_BLACKBOARD);
        }
        if (effect & EFFECT_AQUA) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_AQUA);
        }

        p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, tempStr.c_str());
        p.set(CameraParameters::KEY_EFFECT, CameraParameters::EFFECT_NONE);
    }

    /* white balance */
    int whiteBalance = getSupportedWhiteBalance();
    tempStr.setTo("");
    if (whiteBalance & WHITE_BALANCE_AUTO) {
        tempStr.append(CameraParameters::WHITE_BALANCE_AUTO);
    }
    if (whiteBalance & WHITE_BALANCE_INCANDESCENT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_INCANDESCENT);
    }
    if (whiteBalance & WHITE_BALANCE_FLUORESCENT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_FLUORESCENT);
    }
    if (whiteBalance & WHITE_BALANCE_WARM_FLUORESCENT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT);
    }
    if (whiteBalance & WHITE_BALANCE_DAYLIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_DAYLIGHT);
    }
    if (whiteBalance & WHITE_BALANCE_CLOUDY_DAYLIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT);
    }
    if (whiteBalance & WHITE_BALANCE_TWILIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_TWILIGHT);
    }
    if (whiteBalance & WHITE_BALANCE_SHADE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_SHADE);
    }

    p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE,
          tempStr.c_str());
    p.set(CameraParameters::KEY_WHITE_BALANCE, CameraParameters::WHITE_BALANCE_AUTO);

    /* Auto Whitebalance Lock supported */
    if (getAutoWhiteBalanceLockSupported() == true)
        p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "false");

    /*  anti banding */
    tempStr.setTo("");
    int antiBanding = getSupportedAntibanding();
#ifdef USE_CSC_FEATURE
    /* The Supported List always includes the AUTO Mode - Google */
    if (antiBanding & ANTIBANDING_AUTO) {
        tempStr.append(CameraParameters::ANTIBANDING_AUTO);
    }

    m_chooseAntiBandingFrequency();
    /* 50Hz or 60Hz */
    tempStr.append(",");
    tempStr.append(m_antiBanding);
#else

    if (antiBanding & ANTIBANDING_AUTO) {
        tempStr.append(CameraParameters::ANTIBANDING_AUTO);
    }
    if (antiBanding & ANTIBANDING_50HZ) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ANTIBANDING_50HZ);
    }
    if (antiBanding & ANTIBANDING_60HZ) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ANTIBANDING_60HZ);
    }
    if (antiBanding & ANTIBANDING_OFF) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ANTIBANDING_OFF);
    }
#endif

    p.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING,
          tempStr.c_str());

#ifdef USE_CSC_FEATURE
    p.set(CameraParameters::KEY_ANTIBANDING, m_antiBanding);
#else
    p.set(CameraParameters::KEY_ANTIBANDING, CameraParameters::ANTIBANDING_AUTO);
#endif

    /* rotation */
    p.set(CameraParameters::KEY_ROTATION, 0);

    /* view angle */
    setHorizontalViewAngle(640, 480);
    p.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, getHorizontalViewAngle());
    p.setFloat(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, getVerticalViewAngle());

    /* metering */
    p.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS, getMaxNumMeteringAreas());
    p.set(CameraParameters::KEY_METERING_AREAS, "");

    /* zoom */
    if (getZoomSupported() == true) {
        int maxZoom = getMaxZoomLevel();
        ALOGI("INFO(%s):getMaxZoomLevel(%d)", __FUNCTION__, maxZoom);

        if (0 < maxZoom) {
            p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");

            if (getSmoothZoomSupported() == true)
                p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "true");
            else
                p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");

            p.set(CameraParameters::KEY_MAX_ZOOM, maxZoom - 1);
            p.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);

            int max_zoom_ratio = (int)getMaxZoomRatio();
            tempStr.setTo("");
            if (getZoomRatioList(tempStr, maxZoom, max_zoom_ratio, m_staticInfo->zoomRatioList) == NO_ERROR)
                p.set(CameraParameters::KEY_ZOOM_RATIOS, tempStr.c_str());
            else
                p.set(CameraParameters::KEY_ZOOM_RATIOS, "100");

            p.set("constant-growth-rate-zoom-supported", "true");

            ALOGV("INFO(%s):zoomRatioList=%s", "setDefaultParameter", tempStr.c_str());
        } else {
            p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
            p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
            p.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
            p.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
        }
    } else {
        p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
        p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
        p.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
        p.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
    }

    /* fps */
    uint32_t minFpsRange = 15;
    uint32_t maxFpsRange = 30;

    getPreviewFpsRange(&minFpsRange, &maxFpsRange);
#ifdef TEST_GED_HIGH_SPEED_RECORDING
    maxFpsRange = 120;
#endif
    CLOGI("INFO(%s[%d]):minFpsRange=%d, maxFpsRange=%d", "getPreviewFpsRange", __LINE__, (int)minFpsRange, (int)maxFpsRange);
    int minFps = (minFpsRange == 0) ? 0 : (int)minFpsRange;
    int maxFps = (maxFpsRange == 0) ? 0 : (int)maxFpsRange;

    tempStr.setTo("");
    snprintf(strBuf, 256, "%d", minFps);
    tempStr.append(strBuf);

    for (int i = minFps + 1; i <= maxFps; i++) {
        snprintf(strBuf, 256, ",%d", i);
        tempStr.append(strBuf);
    }
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,  tempStr.c_str());

    minFpsRange = minFpsRange * 1000;
    maxFpsRange = maxFpsRange * 1000;

    tempStr.setTo("");
    getSupportedFpsList(tempStr, minFpsRange, maxFpsRange, m_cameraId, m_staticInfo);
    CLOGI("INFO(%s):supportedFpsList=%s", "setDefaultParameter", tempStr.c_str());
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, tempStr.c_str());
    /* p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(15000,30000),(30000,30000)"); */

    /* limit 30 fps on default setting. */
    if (30 < maxFps)
        maxFps = 30;
    p.setPreviewFrameRate(maxFps);

    if (30000 < maxFpsRange)
        maxFpsRange = 30000;
    snprintf(strBuf, 256, "%d,%d", maxFpsRange/2, maxFpsRange);
    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, strBuf);

    /* focal length */
    int num = 0;
    int den = 0;
    int precision = 0;
    getFocalLength(&num, &den);

    switch (den) {
    default:
    case 1000:
        precision = 3;
        break;
    case 100:
        precision = 2;
        break;
    case 10:
        precision = 1;
        break;
    case 1:
        precision = 0;
        break;
    }
    snprintf(strBuf, 256, "%.*f", precision, ((float)num / (float)den));
    p.set(CameraParameters::KEY_FOCAL_LENGTH, strBuf);

    /* Additional params. */
    p.set("contrast", "auto");
    p.set("iso", "auto");

    // Set supported ISO values
    int isoValues = getSupportedISO();
    tempStr.setTo("");
    if (isoValues & ISO_AUTO) {
        tempStr.append(CameraParameters::ISO_AUTO);
    }
    if (isoValues & ISO_100) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_100);
    }
    if (isoValues & ISO_200) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_200);
    }
    if (isoValues & ISO_400) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_400);
    }
    if (isoValues & ISO_800) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_800);
    }

    p.set("iso-values",
          tempStr.c_str());

    p.set("wdr", 0);
    p.set("hdr-mode", 0);
    p.set("metering", "center");    /* For Samsung SDK */

    // Set Supported Metering Mode
    int meteringModes = getSupportedMeteringMode();
    tempStr.setTo("");
    if (meteringModes & METERING_MODE_MATRIX) {
        tempStr.append(CameraParameters::METERING_MATRIX);
    }
    if (meteringModes & METERING_MODE_CENTER) {
        tempStr.append(",");
        tempStr.append(CameraParameters::METERING_CENTER);
    }
    if (meteringModes & METERING_MODE_SPOT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::METERING_SPOT);
    }
#ifdef SAMSUNG_TN_FEATURE
    /* For Samsung SDK */
    p.set(CameraParameters::KEY_SUPPORTED_METERING_MODE,
        tempStr.c_str());
#endif

    p.set("brightness", 0);
    p.set("brightness-max", 2);
    p.set("brightness-min", -2);

    p.set("saturation", 0);
    p.set("saturation-max", 2);
    p.set("saturation-min", -2);

    p.set("sharpness", 0);
    p.set("sharpness-max", 2);
    p.set("sharpness-min", -2);

    p.set("hue", 0);
    p.set("hue-max", 2);
    p.set("hue-min", -2);

    /* For Series shot */
    p.set("burst-capture", 0);
    p.set("best-capture", 0);

    /* fnumber */
    getFnumber(&num, &den);
    p.set("fnumber-value-numerator", num);
    p.set("fnumber-value-denominator", den);

    /* max aperture value */
    getApertureValue(&num, &den);
    p.set("maxaperture-value-numerator", num);
    p.set("maxaperture-value-denominator", den);

    /* focal length */
    getFocalLength(&num, &den);
    p.set("focallength-value-numerator", num);
    p.set("focallength-value-denominator", den);

    /* focal length in 35mm film */
    int focalLengthIn35mmFilm = 0;
    focalLengthIn35mmFilm = getFocalLengthIn35mmFilm();
    p.set("focallength-35mm-value", focalLengthIn35mmFilm);

#if defined(USE_3RD_BLACKBOX) /* KOR ONLY */
    /* scale mode */
    bool supportedScalableMode = getSupportedScalableSensor();
    if (supportedScalableMode == true)
        p.set("scale_mode", -1);
#endif

#if defined(TEST_APP_HIGH_SPEED_RECORDING)
    p.set("fast-fps-mode", 0);
#endif

#ifdef SAMSUNG_LLV
    /* LLV */
    p.set("llv_mode", 0);
#endif

#ifdef SAMSUNG_COMPANION
    p.set(CameraParameters::KEY_DYNAMIC_RANGE_CONTROL, "off");

    /* PHASE_AF */
    p.set(CameraParameters::KEY_SUPPORTED_PHASE_AF, "off,on");  /* For Samsung SDK */
    p.set(CameraParameters::KEY_PHASE_AF, "off");

    /* RT_HDR */
    p.set(CameraParameters::KEY_SUPPORTED_RT_HDR, "off,on"); /* For Samsung SDK */
    p.set(CameraParameters::KEY_RT_HDR, "off");
#elif defined(SAMSUNG_TN_FEATURE)
    p.set(CameraParameters::KEY_DYNAMIC_RANGE_CONTROL, "off");

    /* PHASE_AF */
    p.set(CameraParameters::KEY_SUPPORTED_PHASE_AF, "off"); /* For Samsung SDK */
    p.set(CameraParameters::KEY_PHASE_AF, "off");

    /* RT_HDR */
    p.set(CameraParameters::KEY_SUPPORTED_RT_HDR, "off"); /* For Samsung SDK */
    p.set(CameraParameters::KEY_RT_HDR, "off");
#else
    p.set(CameraParameters::KEY_DYNAMIC_RANGE_CONTROL, "off");
#endif
    p.set("imageuniqueid-value", 0);

#if defined(SAMSUNG_OIS)
    p.set("ois-supported", "true");
    p.set("ois", "still");
#else
    //p.set("ois-supported", "false");
#endif

#ifdef SAMSUNG_VRMODE
    p.set("vrmode-supported", "true");
#endif

#if defined(BURST_CAPTURE) && defined(VARIABLE_BURST_FPS)
    tempStr.setTo("");
    tempStr = String8::format("(%d,%d)", BURSTSHOT_MIN_FPS, BURSTSHOT_MAX_FPS);
    p.set("burstshot-fps-values", tempStr.c_str());
#else
    p.set("burstshot-fps-values", "(0,0)");
#endif

    p.set("drc", "false");
    p.set("3dnr", "false");
    p.set("odc", "false");
#ifdef USE_FADE_IN_ENTRANCE
    p.set("entrance", "false");
#endif

#ifdef SAMSUNG_MANUAL_FOCUS
    p.set("focus-distance", -1);
#endif
#ifdef SAMSUNG_QUICKSHOT
    p.set("quick-shot", 0);
#endif
    p.set("effectrecording-hint", 0);

    m_params = p;

    /* make sure m_secCamera has all the settings we do.  applications
     * aren't required to call setParameters themselves (only if they
     * want to change something.
     */
    ret = setParameters(p);
    if (ret < 0)
        CLOGE("ERR(%s[%d]):setParameters is fail", __FUNCTION__, __LINE__);
}

status_t ExynosCamera1Parameters::checkVisionMode(const CameraParameters& params)
{
    status_t ret;

    /* Check vision mode */
    int intelligent_mode = params.getInt("intelligent-mode");
    CLOGD("DEBUG(%s):intelligent_mode : %d", "setParameters", intelligent_mode);

    ret = m_setIntelligentMode(intelligent_mode);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s): Inavalid Intelligent mode", "setParameters");
        return ret;
    }
    m_params.set("intelligent-mode", intelligent_mode);

    CLOGD("DEBUG(%s):intelligent_mode(%d) getVisionMode(%d)", "setParameters", intelligent_mode, getVisionMode());

    /* Smart stay need to skip more frames */
    int skipCompensation = m_frameSkipCounter.getCompensation();
    if (intelligent_mode == 1) {
        m_frameSkipCounter.setCompensation(skipCompensation + SMART_STAY_SKIP_COMPENSATION);
    } else {
        m_frameSkipCounter.setCompensation(skipCompensation);
    }

    if (getVisionMode() == true) {
        /* preset for each vision mode */
        switch (intelligent_mode) {
        case 2:
            m_setVisionModeFps(10);
            break;
        case 3:
            m_setVisionModeFps(5);
            break;
        default:
            m_setVisionModeFps(10);
            break;
        }

/* Vision mode custom frame rate will be enabled when application ready */
#if 0
        /* If user wants to set custom fps, vision mode set max fps to frame rate */
        int minFps = -1;
        int maxFps = -1;
        params.getPreviewFpsRange(&minFps, &maxFps);

        if (minFps > 0 && maxFps > 0) {
            CLOGD("DEBUG(%s): set user frame rate (%d)", __FUNCTION__, maxFps / 1000);
            m_setVisionModeFps(maxFps / 1000);
        }
#endif

        /* smart-screen-exposure */
        int newVisionAeTarget = params.getInt("smart-screen-exposure");
        if (0 < newVisionAeTarget) {
            CLOGD("DEBUG(%s):newVisionAeTarget : %d", "setParameters", newVisionAeTarget);
            m_setVisionModeAeTarget(newVisionAeTarget);
            m_params.set("smart-screen-exposure", newVisionAeTarget);
        }

        return OK;
    } else {
        return NO_ERROR;
    }
}

status_t ExynosCamera1Parameters::m_setIntelligentMode(int intelligentMode)
{
    status_t ret = NO_ERROR;
    bool visionMode = false;

    m_cameraInfo.intelligentMode = intelligentMode;

    if (intelligentMode > 1) {
        if (m_staticInfo->visionModeSupport == true) {
            visionMode = true;
        } else {
            CLOGE("ERR(%s): tried to set vision mode(not supported)", "setParameters");
            ret = BAD_VALUE;
        }
    } else if (getVisionMode()) {
        CLOGE("ERR(%s[%d]):vision mode can not change before stoppreview", __FUNCTION__, __LINE__);
        visionMode = true;
    }

    m_setVisionMode(visionMode);

    return ret;
 }

int ExynosCamera1Parameters::getIntelligentMode(void)
{
    return m_cameraInfo.intelligentMode;
}

void ExynosCamera1Parameters::m_setVisionMode(bool vision)
{
    m_cameraInfo.visionMode = vision;
}

bool ExynosCamera1Parameters::getVisionMode(void)
{
    return m_cameraInfo.visionMode;
}

void ExynosCamera1Parameters::m_setVisionModeFps(int fps)
{
    m_cameraInfo.visionModeFps = fps;
}

int ExynosCamera1Parameters::getVisionModeFps(void)
{
    return m_cameraInfo.visionModeFps;
}

void ExynosCamera1Parameters::m_setVisionModeAeTarget(int ae)
{
    m_cameraInfo.visionModeAeTarget = ae;
}

int ExynosCamera1Parameters::getVisionModeAeTarget(void)
{
    return m_cameraInfo.visionModeAeTarget;
}

status_t ExynosCamera1Parameters::checkEffectRecordingHint(const CameraParameters& params)
{
    /* Effect recording hint */
    bool flagEffectRecordingHint = false;
    int newEffectRecordingHint = params.getInt("effectrecording-hint");
    int curEffectRecordingHint = m_params.getInt("effectrecording-hint");

    if (newEffectRecordingHint < 0) {
        CLOGV("DEBUG(%s):Invalid newEffectRecordingHint", "setParameters");
        return NO_ERROR;
    }

    if (newEffectRecordingHint != curEffectRecordingHint) {
        CLOGD("DEBUG(%s):newEffectRecordingHint : %d", "setParameters", newEffectRecordingHint);
        if (newEffectRecordingHint == 1)
            flagEffectRecordingHint = true;
        m_setEffectRecordingHint(flagEffectRecordingHint);
        m_params.set("effectrecording-hint", newEffectRecordingHint);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setEffectRecordingHint(bool hint)
{
    ExynosCameraActivityAutofocus *autoFocusMgr = m_activityControl->getAutoFocusMgr();
    ExynosCameraActivityFlash *flashMgr = m_activityControl->getFlashMgr();

    m_cameraInfo.effectRecordingHint = hint;

    if (hint) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_ON);
    } else if (!hint && !getRecordingHint() && !getDualRecordingHint()) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_OFF);
    }
    autoFocusMgr->setRecordingHint(hint);
    flashMgr->setRecordingHint(hint);
}

#if defined(SAMSUNG_COMPANION) || defined(SAMSUNG_EEPROM)
void ExynosCamera1Parameters::setRomReadThreadDone(bool enable)
{
    Mutex::Autolock lock(m_parameterLock);

    m_romReadThreadDone = enable;
}
#endif

status_t ExynosCamera1Parameters::checkSWVdisMode(const CameraParameters& params)
{
    const char *newSwVdis = params.get("sw-vdis");
    bool currSwVdis = getSWVdisMode();
    if (newSwVdis != NULL) {
        CLOGD("DEBUG(%s):newSwVdis %s", "setParameters", newSwVdis);
        bool swVdisMode = true;

        if (!strcmp(newSwVdis, "off"))
            swVdisMode = false;

        m_setSWVdisMode(swVdisMode);
        m_params.set("sw-vdis", newSwVdis);
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::checkHWVdisMode(__unused const CameraParameters& params)
{
    status_t ret = NO_ERROR;

    bool hwVdisMode = this->getHWVdisMode();

    CLOGD("DEBUG(%s):newHwVdis %d", "setParameters", hwVdisMode);

    ret = setDisEnable(hwVdisMode);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setDisEnable(%d) fail", __FUNCTION__, __LINE__, hwVdisMode);
    } else {
        if (m_flagHWVDisMode != hwVdisMode) {
            m_flagHWVDisMode = hwVdisMode;
        }
    }

    return ret;
}

bool ExynosCamera1Parameters::isSWVdisMode(void)
{
    bool swVDIS_mode = false;
    bool use3DNR_dmaout = false;

    int nPreviewW, nPreviewH;
    getPreviewSize(&nPreviewW, &nPreviewH);

    if ((getRecordingHint() == true) &&
#ifndef SUPPORT_SW_VDIS_FRONTCAM
        (getCameraId() == CAMERA_ID_BACK) &&
#endif /*SUPPORT_SW_VDIS_FRONTCAM*/
        (getHighSpeedRecording() == false) &&
        (use3DNR_dmaout == false) &&
        (getSWVdisUIMode() == true) &&
        ((nPreviewW == 1920 && nPreviewH == 1080) || (nPreviewW == 1280 && nPreviewH == 720)))
    {
        swVDIS_mode = true;
    }

    return swVDIS_mode;
}

bool ExynosCamera1Parameters::isSWVdisModeWithParam(int nPreviewW, int nPreviewH)
{
    bool swVDIS_mode = false;
    bool use3DNR_dmaout = false;

    if ((getRecordingHint() == true) &&
#ifndef SUPPORT_SW_VDIS_FRONTCAM
        (getCameraId() == CAMERA_ID_BACK) &&
#endif /*SUPPORT_SW_VDIS_FRONTCAM*/
        (getHighSpeedRecording() == false) &&
        (use3DNR_dmaout == false) &&
        (getSWVdisUIMode() == true) &&
        ((nPreviewW == 1920 && nPreviewH == 1080) || (nPreviewW == 1280 && nPreviewH == 720)))
    {
        swVDIS_mode = true;
    }

    return swVDIS_mode;
}

bool ExynosCamera1Parameters::getHWVdisMode(void)
{
    bool ret = this->getVideoStabilization();

    /*
     * Only true case,
     * we will test whether support or not.
     */
    if (ret == true) {
        switch (getCameraId()) {
        case CAMERA_ID_BACK:
#ifdef SUPPORT_BACK_HW_VDIS
            ret = SUPPORT_BACK_HW_VDIS;
#else
            ret = false;
#endif
            break;
        case CAMERA_ID_FRONT:
#ifdef SUPPORT_FRONT_HW_VDIS
            ret = SUPPORT_FRONT_HW_VDIS;
#else
            ret = false;
#endif
            break;
        default:
            ret = false;
            break;
        }
    }

#ifdef SUPPORT_SW_VDIS
    if (ret == true &&
        this->isSWVdisMode() == true) {
        ret = false;
    }
#endif /*SUPPORT_SW_VDIS*/

    return ret;
}

int ExynosCamera1Parameters::getHWVdisFormat(void)
{
    return V4L2_PIX_FMT_YUYV;
}

void ExynosCamera1Parameters::m_setSWVdisMode(bool swVdis)
{
    m_cameraInfo.swVdisMode = swVdis;
}

bool ExynosCamera1Parameters::getSWVdisMode(void)
{
    return m_cameraInfo.swVdisMode;
}

void ExynosCamera1Parameters::m_setSWVdisUIMode(bool swVdisUI)
{
    m_cameraInfo.swVdisUIMode = swVdisUI;
}

bool ExynosCamera1Parameters::getSWVdisUIMode(void)
{
    return m_cameraInfo.swVdisUIMode;
}

status_t ExynosCamera1Parameters::m_adjustPreviewSize(__unused int previewW, __unused int previewH,
                                                     int *newPreviewW, int *newPreviewH,
                                                     int *newCalHwPreviewW, int *newCalHwPreviewH)
{
    /* hack : when app give 1446, we calibrate to 1440 */
    if (*newPreviewW == 1446 && *newPreviewH == 1080) {
        CLOGW("WARN(%s):Invalid previewSize(%d/%d). so, calibrate to (1440/%d)", __FUNCTION__, *newPreviewW, *newPreviewH, *newPreviewH);
        *newPreviewW = 1440;
    }

    /* calibrate H/W aligned size*/
    if (getRecordingHint() == true) {
        int videoW = 0, videoH = 0;
        ExynosRect bdsRect;

        getVideoSize(&videoW, &videoH);

        if ((videoW <= *newPreviewW) && (videoH <= *newPreviewH)) {
#ifdef SUPPORT_SW_VDIS
            if (isSWVdisModeWithParam(*newPreviewW, *newPreviewH) == true) {
                m_getSWVdisPreviewSize(*newPreviewW, *newPreviewH, newCalHwPreviewW, newCalHwPreviewH);
            } else
#endif /*SUPPORT_SW_VDIS*/
            {
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING)
                if ((videoW <= 1920 || videoH <= 1080) &&
                    (1920 < *newPreviewW || 1080 < *newPreviewH)) {

                    float videoRatio = ROUND_OFF(((float)videoW / (float)videoH), 2);

                    if (videoRatio == 1.33f) { /* 4:3 */
                        *newCalHwPreviewW = 1440;
                        *newCalHwPreviewH = 1080;
                    } else if (videoRatio == 1.77f) { /* 16:9 */
                        *newCalHwPreviewW = 1920;
                        *newCalHwPreviewH = 1080;
                    } else if (videoRatio == 1.00f) { /* 1:1 */
                        *newCalHwPreviewW = 1088;
                        *newCalHwPreviewH = 1088;
                    } else {
                        *newCalHwPreviewW = *newPreviewW;
                        *newCalHwPreviewH = *newPreviewH;
                    }

                    if (*newCalHwPreviewW != *newPreviewW  ||
                        *newCalHwPreviewH != *newPreviewH) {
                        CLOGW("WARN(%s[%d]):Limit hw preview size until %d x %d when videoSize(%d x %d)",
                            __FUNCTION__, __LINE__, *newCalHwPreviewW, *newCalHwPreviewH, videoW, videoH);
                    }
                } else
#endif
                {
                    *newCalHwPreviewW = *newPreviewW;
                    *newCalHwPreviewH = *newPreviewH;
                }
            }

            if (getHighSpeedRecording() == true) {
                int sizeList[SIZE_LUT_INDEX_END];

                if (m_getPreviewSizeList(sizeList) == NO_ERROR) {
                    /* On high-speed recording, scaling-up by SCC/SCP occurs the IS-ISP performance degradation.
                                    The scaling-up might be done by GSC for recording */
                    if ((sizeList[BDS_W] < *newPreviewW) && (sizeList[BDS_H] < *newPreviewH)) {
                        *newCalHwPreviewW = sizeList[BDS_W];
                        *newCalHwPreviewH = sizeList[BDS_H];
                    }
                } else {
                    ALOGE("ERR(%s):m_getPreviewSizeList() fail", __FUNCTION__);
                }
            }
        } else {
#ifdef SUPPORT_SW_VDIS
            if (isSWVdisModeWithParam(videoW, videoH) == true) {
                m_getSWVdisPreviewSize(videoW, videoH, newCalHwPreviewW, newCalHwPreviewH);
            } else
#endif /*SUPPORT_SW_VDIS*/
            /* video size > preview size : Use BDS size for SCP output size */
            {
                ALOGV("DEBUG(%s[%d]):preview(%dx%d) is smaller than video(%dx%d)",
                        __FUNCTION__, __LINE__, *newPreviewW, *newPreviewH, videoW, videoH);

                /* If the video ratio is differ with preview ratio,
                   the default ratio is set into preview ratio */
                if (SIZE_RATIO(*newPreviewW, *newPreviewH) != SIZE_RATIO(videoW, videoH))
                    ALOGW("WARN(%s): preview ratio(%dx%d) is not matched with video ratio(%dx%d)", __FUNCTION__,
                            *newPreviewW, *newPreviewH, videoW, videoH);

                if (m_isSupportedPreviewSize(*newPreviewW, *newPreviewH) == false) {
                    ALOGE("ERR(%s): new preview size is invalid(%dx%d)",
                            __FUNCTION__, *newPreviewW, *newPreviewH);
                    return BAD_VALUE;
                }

                /*
                 * This call is to get real preview size.
                 * so, HW dis size must not be added.
                 */
                m_getPreviewBdsSize(&bdsRect);

                if ((bdsRect.w <= videoW) && (bdsRect.h <= videoH)) {
                    *newCalHwPreviewW = bdsRect.w;
                    *newCalHwPreviewH = bdsRect.h;
                } else {
                    *newCalHwPreviewW = videoW;
                    *newCalHwPreviewH = videoH;

                    if (SIZE_RATIO(*newPreviewW, *newPreviewH) != SIZE_RATIO(videoW, videoH)) {
                        int  cropX, cropY, cropW, cropH;

                        getCropRectAlign(videoW, videoH,
                                         *newPreviewW, *newPreviewH,
                                         &cropX, &cropY,
                                         &cropW, &cropH,
                                         CAMERA_MAGIC_ALIGN, 2,
                                         0, 1);

                        *newCalHwPreviewW = cropW;
                        *newCalHwPreviewH = cropH;
                    }
                }
            }
        }
    } else if (getHighResolutionCallbackMode() == true) {
        if(CAMERA_LCD_SIZE == LCD_SIZE_1280_720) {
            *newCalHwPreviewW = 1280;
            *newCalHwPreviewH = 720;
        } else {
            *newCalHwPreviewW = 1920;
            *newCalHwPreviewH = 1080;
        }
    } else {
        *newCalHwPreviewW = *newPreviewW;
        *newCalHwPreviewH = *newPreviewH;
    }

#if defined(SCALER_MAX_SCALE_DOWN_RATIO)
    {
        ExynosRect bdsRect;

        if (getPreviewBdsSize(&bdsRect) == NO_ERROR) {
            int minW = bdsRect.w / SCALER_MAX_SCALE_DOWN_RATIO;
            int minH = bdsRect.h / SCALER_MAX_SCALE_DOWN_RATIO;
            int adjustW = 1;
            int adjustH = 1;

            if ((*newCalHwPreviewW < minW) || (*newCalHwPreviewH < minH)) {
                if (*newCalHwPreviewW < minW) {
                    adjustW = ROUND_UP(minW / *newCalHwPreviewW, 2);
                    CLOGW("WARN(%s): newCalHwPreviewW=%d,  minW=%d, adjustW=%d", __FUNCTION__,
                        *newCalHwPreviewW, minW, adjustW);
                }
                if (*newCalHwPreviewH < minH) {
                    adjustH = ROUND_UP(minH / *newCalHwPreviewH, 2);
                    CLOGW("WARN(%s): newCalHwPreviewH=%d,  minH=%d, adjustH=%d", __FUNCTION__,
                        *newCalHwPreviewH, minH, adjustH);
                }
                adjustW = (adjustW > adjustH) ? adjustW : adjustH;
                *newCalHwPreviewW *= adjustW;
                *newCalHwPreviewH *= adjustW;
            }
        }
    }
#endif

    if (getHalVersion() == IS_HAL_VER_3_2) {
#if defined(ENABLE_FULL_FRAME)
        ExynosRect bdsRect;
        getPreviewBdsSize(&bdsRect);
        *newCalHwPreviewW = bdsRect.w;
        *newCalHwPreviewH = bdsRect.h;
#else
        /* 1. try to get exact ratio */
        if (m_isSupportedPreviewSize(*newPreviewW, *newPreviewH) == false) {
            ALOGE("ERR(%s): new preview size is invalid(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        }

        /* 2. get bds size to set size to scp node due to internal scp buffer */
        int sizeList[SIZE_LUT_INDEX_END];
        if (m_getPreviewSizeList(sizeList) == NO_ERROR) {
            *newCalHwPreviewW = sizeList[BDS_W];
            *newCalHwPreviewH = sizeList[BDS_H];
        } else {
            ExynosRect bdsRect;
            getPreviewBdsSize(&bdsRect);
            *newCalHwPreviewW = bdsRect.w;
            *newCalHwPreviewH = bdsRect.h;
        }
#endif
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::setCropRegion(int x, int y, int w, int h)
{
    status_t ret = NO_ERROR;

    ret = setMetaCtlCropRegion(&m_metadata, x, y, w, h);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Failed to setMetaCtlCropRegion(%d, %d, %d, %d)",
                __FUNCTION__, __LINE__, x, y, w, h);
    }

    return ret;
}

void ExynosCamera1Parameters::m_adjustAeMode(enum aa_aemode curAeMode, enum aa_aemode *newAeMode)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        int curMeteringMode = getMeteringMode();
        if (curAeMode == AA_AEMODE_OFF) {
            switch(curMeteringMode) {
            case METERING_MODE_AVERAGE:
                *newAeMode = AA_AEMODE_AVERAGE;
                break;
            case METERING_MODE_CENTER:
                *newAeMode = AA_AEMODE_CENTER;
                break;
            case METERING_MODE_MATRIX:
                *newAeMode = AA_AEMODE_MATRIX;
                break;
            case METERING_MODE_SPOT:
                *newAeMode = AA_AEMODE_SPOT;
                break;
            default:
                *newAeMode = curAeMode;
                break;
            }
        }
    }
}

/* TODO: Who explane this offset value? */
#define FW_CUSTOM_OFFSET (1)
/* F/W's middle value is 5, and step is -4, -3, -2, -1, 0, 1, 2, 3, 4 */
void ExynosCamera1Parameters::m_setExposureCompensation(int32_t value)
{
    setMetaCtlExposureCompensation(&m_metadata, value);
#if defined(USE_SUBDIVIDED_EV)
    setMetaCtlExposureCompensationStep(&m_metadata, m_staticInfo->exposureCompensationStep);
#endif
}

int32_t ExynosCamera1Parameters::getExposureCompensation(void)
{
    int32_t expCompensation;
    getMetaCtlExposureCompensation(&m_metadata, &expCompensation);

    return expCompensation;
}

void ExynosCamera1Parameters::m_setMeteringAreas(uint32_t num, ExynosRect2 *rect2s, int *weights)
{
    uint32_t maxNumMeteringAreas = getMaxNumMeteringAreas();

    if(getSamsungCamera()) {
        maxNumMeteringAreas = 1;
    }

    if (maxNumMeteringAreas == 0) {
        CLOGV("DEBUG(%s):maxNumMeteringAreas is 0. so, ignored", __FUNCTION__);
        return;
    }

    if (maxNumMeteringAreas < num)
        num = maxNumMeteringAreas;

    if (getAutoExposureLock() == true) {
        CLOGD("DEBUG(%s):autoExposure is Locked", __FUNCTION__);
        return;
    }

    if (num == 1) {
#ifdef CAMERA_GED_FEATURE
        int meteringMode = getMeteringMode();

        if (isRectNull(&rect2s[0]) == true) {
            switch (meteringMode) {
                case METERING_MODE_SPOT:
                    /*
                     * Even if SPOT metering mode, area must set valid values,
                     * but areas was invalid values, we change mode to CENTER.
                     */
                    m_setMeteringMode(METERING_MODE_CENTER);
                    m_cameraInfo.isTouchMetering = false;
                    break;
                case METERING_MODE_AVERAGE:
                case METERING_MODE_CENTER:
                case METERING_MODE_MATRIX:
                default:
                    /* adjust metering setting */
                    break;
            }
        } else {
            switch (meteringMode) {
                case METERING_MODE_CENTER:
                    /*
                     * SPOT metering mode in GED camera App was not set METERING_MODE_SPOT,
                     * but set metering areas only.
                     */
                    m_setMeteringMode(METERING_MODE_SPOT);
                    m_cameraInfo.isTouchMetering = true;
                    break;
                case METERING_MODE_AVERAGE:
                case METERING_MODE_MATRIX:
                case METERING_MODE_SPOT:
                default:
                    /* adjust metering setting */
                    break;
            }
        }
#endif
    } else {
        if (num > 1 && isRectEqual(&rect2s[0], &rect2s[1]) == false) {
            /* if MATRIX mode support, mode set METERING_MODE_MATRIX */
            m_setMeteringMode(METERING_MODE_AVERAGE);
            m_cameraInfo.isTouchMetering = false;
        } else {
            m_setMeteringMode(METERING_MODE_AVERAGE);
            m_cameraInfo.isTouchMetering = false;
        }
    }

    ExynosRect cropRegionRect;
    ExynosRect2 newRect2;

    getHwBayerCropRegion(&cropRegionRect.w, &cropRegionRect.h, &cropRegionRect.x, &cropRegionRect.y);

    for (uint32_t i = 0; i < num; i++) {
        bool isChangeMeteringArea = false;
#ifdef CAMERA_GED_FEATURE
        if (isRectNull(&rect2s[i]) == false)
            isChangeMeteringArea = true;
        else
            isChangeMeteringArea = false;
#else
        if ((isRectNull(&rect2s[i]) == false) ||((isRectNull(&rect2s[i]) == true) && (getMeteringMode() == METERING_MODE_SPOT)))
            isChangeMeteringArea = true;
#ifdef TOUCH_AE
        else if((getMeteringMode() == METERING_MODE_SPOT_TOUCH) || (getMeteringMode() == METERING_MODE_MATRIX_TOUCH)
            || (getMeteringMode() == METERING_MODE_CENTER_TOUCH) || (getMeteringMode() == METERING_MODE_AVERAGE_TOUCH))
            isChangeMeteringArea = true;
#endif
        else
            isChangeMeteringArea = false;
#endif
        if (isChangeMeteringArea == true) {
            CLOGD("DEBUG(%s) (%d %d %d %d) %d", __FUNCTION__, rect2s->x1, rect2s->y1, rect2s->x2, rect2s->y2, getMeteringMode());
            newRect2 = convertingAndroidArea2HWAreaBcropOut(&rect2s[i], &cropRegionRect);
            setMetaCtlAeRegion(&m_metadata, newRect2.x1, newRect2.y1,
                                newRect2.x2, newRect2.y2, weights[i]);
        }
    }
}

status_t ExynosCamera1Parameters::checkPictureFormat(const CameraParameters& params)
{
    int curPictureFormat = 0;
    int newPictureFormat = 0;
    int newHwPictureFormat = 0;
    const char *strNewPictureFormat = params.getPictureFormat();
    const char *strCurPictureFormat = m_params.getPictureFormat();

    if (strNewPictureFormat == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newPictureFormat %s", "setParameters", strNewPictureFormat);

    if (!strcmp(strNewPictureFormat, CameraParameters::PIXEL_FORMAT_JPEG)) {
        newPictureFormat = V4L2_PIX_FMT_JPEG;
        newHwPictureFormat = SCC_OUTPUT_COLOR_FMT;
    } else if (!strcmp(strNewPictureFormat, CameraParameters::PIXEL_FORMAT_YUV420SP_NV21)) {
        newPictureFormat = V4L2_PIX_FMT_NV21;
        newHwPictureFormat = SCC_OUTPUT_COLOR_FMT;
    } else {
        CLOGE("ERR(%s[%d]): Picture format(%s) is not supported!",
                __FUNCTION__, __LINE__, strNewPictureFormat);
        return BAD_VALUE;
    }

    curPictureFormat = getPictureFormat();

    if (newPictureFormat != curPictureFormat) {
        CLOGI("INFO(%s[%d]): Picture format changed, cur(%s) -> new(%s)",
                "Parameters", __LINE__, strCurPictureFormat, strNewPictureFormat);
        m_setPictureFormat(newPictureFormat);
        m_setHwPictureFormat(newHwPictureFormat);
        m_params.setPictureFormat(strNewPictureFormat);
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::checkMeteringMode(const CameraParameters& params)
{
    const char *strNewMeteringMode = params.get("metering");
    int newMeteringMode = -1;
    int curMeteringMode = -1;
#ifdef SAMSUNG_COMPANION
    const char *strNewRTHdr = params.get("rt-hdr");
#endif
    if (strNewMeteringMode == NULL) {
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s):strNewMeteringMode %s", "setParameters", strNewMeteringMode);

#ifdef SAMSUNG_COMPANION
    if ((strNewRTHdr != NULL && !strcmp(strNewRTHdr, "on"))
#ifdef TOUCH_AE
        && strcmp(strNewMeteringMode, "weighted-matrix")
#endif
        ) {
        newMeteringMode = METERING_MODE_MATRIX;
        ALOGD("DEBUG(%s):newMeteringMode %d in RT-HDR", "setParameters", newMeteringMode);
    } else
#endif
    {
        if (!strcmp(strNewMeteringMode, "average"))
            newMeteringMode = METERING_MODE_AVERAGE;
        else if (!strcmp(strNewMeteringMode, "center"))
            newMeteringMode = METERING_MODE_CENTER;
        else if (!strcmp(strNewMeteringMode, "matrix"))
            newMeteringMode = METERING_MODE_MATRIX;
        else if (!strcmp(strNewMeteringMode, "spot"))
            newMeteringMode = METERING_MODE_SPOT;
#ifdef TOUCH_AE
        else if (!strcmp(strNewMeteringMode, "weighted-center"))
            newMeteringMode = METERING_MODE_CENTER_TOUCH;
        else if (!strcmp(strNewMeteringMode, "weighted-matrix"))
            newMeteringMode = METERING_MODE_MATRIX_TOUCH;
        else if (!strcmp(strNewMeteringMode, "weighted-spot"))
            newMeteringMode = METERING_MODE_SPOT_TOUCH;
        else if (!strcmp(strNewMeteringMode, "weighted-average"))
            newMeteringMode = METERING_MODE_AVERAGE_TOUCH;
#endif
        else {
            ALOGE("ERR(%s):Invalid metering newMetering(%s)", __FUNCTION__, strNewMeteringMode);
            return UNKNOWN_ERROR;
        }
    }

    curMeteringMode = getMeteringMode();

    m_setMeteringMode(newMeteringMode);
#ifdef SAMSUNG_COMPANION
    if ((strNewRTHdr != NULL && strcmp(strNewRTHdr, "on"))
#ifdef TOUCH_AE
        || !strcmp(strNewMeteringMode, "weighted-matrix")
#endif
        ) {
        m_params.set("metering", strNewMeteringMode);
    }
#else
    m_params.set("metering", strNewMeteringMode);
#endif

    if (curMeteringMode != newMeteringMode) {
        ALOGI("INFO(%s): Metering Area is changed (%d -> %d)", __FUNCTION__, curMeteringMode, newMeteringMode);
        m_flagMeteringRegionChanged = true;
    }

    return NO_ERROR;
}

#ifdef USE_CSC_FEATURE
void ExynosCamera1Parameters::m_getAntiBandingFromLatinMCC(char *temp_str)
{
    char value[PROPERTY_VALUE_MAX];
    char country_value[10];

    memset(value, 0x00, sizeof(value));
    memset(country_value, 0x00, sizeof(country_value));
    if (!property_get("gsm.operator.numeric", value,"")) {
        strcpy(temp_str, CameraParameters::ANTIBANDING_60HZ);
        return ;
    }
    memcpy(country_value, value, 3);

    /** MCC Info. Jamaica : 338 / Argentina : 722 / Chile : 730 / Paraguay : 744 / Uruguay : 748  **/
    if (strstr(country_value,"338") || strstr(country_value,"722") || strstr(country_value,"730") || strstr(country_value,"744") || strstr(country_value,"748"))
        strcpy(temp_str, CameraParameters::ANTIBANDING_50HZ);
    else
        strcpy(temp_str, CameraParameters::ANTIBANDING_60HZ);
}

int ExynosCamera1Parameters::m_IsLatinOpenCSC()
{
    char sales_code[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.csc.sales_code", sales_code, "");
    if (strstr(sales_code,"TFG") || strstr(sales_code,"TPA") || strstr(sales_code,"TTT") || strstr(sales_code,"JDI") || strstr(sales_code,"PCI") )
        return 1;
    else
        return 0;
}

void ExynosCamera1Parameters::m_chooseAntiBandingFrequency()
{
    status_t ret = NO_ERROR;
    int LatinOpenCSClength = 5;
    char *LatinOpenCSCstr = NULL;
    char *CSCstr = NULL;
    const char *defaultStr = "50hz";

    if (m_IsLatinOpenCSC()) {
        LatinOpenCSCstr = (char *)malloc(LatinOpenCSClength);
        if (LatinOpenCSCstr == NULL) {
            CLOGE("LatinOpenCSCstr is NULL");
            CSCstr = (char *)defaultStr;
            memset(m_antiBanding, 0, sizeof(m_antiBanding));
            strcpy(m_antiBanding, CSCstr);
            return;
        }
        memset(LatinOpenCSCstr, 0, LatinOpenCSClength);

        m_getAntiBandingFromLatinMCC(LatinOpenCSCstr);
        CSCstr = LatinOpenCSCstr;
    } else {
        CSCstr = (char *)SecNativeFeature::getInstance()->getString("CscFeature_Camera_CameraFlicker");
    }

    if (CSCstr == NULL || strlen(CSCstr) == 0) {
        CSCstr = (char *)defaultStr;
    }

    memset(m_antiBanding, 0, sizeof(m_antiBanding));
    strcpy(m_antiBanding, CSCstr);
    CLOGD("m_antiBanding = %s",m_antiBanding);

    if (LatinOpenCSCstr != NULL) {
        free(LatinOpenCSCstr);
        LatinOpenCSCstr = NULL;
    }
}
#endif

status_t ExynosCamera1Parameters::checkSceneMode(const CameraParameters& params)
{
    int  newSceneMode = -1;
    int  curSceneMode = -1;
    const char *strNewSceneMode = params.get(CameraParameters::KEY_SCENE_MODE);

    if (strNewSceneMode == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewSceneMode %s", "setParameters", strNewSceneMode);

    if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_AUTO)) {
        newSceneMode = SCENE_MODE_AUTO;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_ACTION)) {
        newSceneMode = SCENE_MODE_ACTION;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_PORTRAIT)) {
        newSceneMode = SCENE_MODE_PORTRAIT;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_LANDSCAPE)) {
        newSceneMode = SCENE_MODE_LANDSCAPE;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_NIGHT)) {
        newSceneMode = SCENE_MODE_NIGHT;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_NIGHT_PORTRAIT)) {
        newSceneMode = SCENE_MODE_NIGHT_PORTRAIT;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_THEATRE)) {
        newSceneMode = SCENE_MODE_THEATRE;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_BEACH)) {
        newSceneMode = SCENE_MODE_BEACH;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SNOW)) {
        newSceneMode = SCENE_MODE_SNOW;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SUNSET)) {
        newSceneMode = SCENE_MODE_SUNSET;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_STEADYPHOTO)) {
        newSceneMode = SCENE_MODE_STEADYPHOTO;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_FIREWORKS)) {
        newSceneMode = SCENE_MODE_FIREWORKS;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SPORTS)) {
        newSceneMode = SCENE_MODE_SPORTS;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_PARTY)) {
        newSceneMode = SCENE_MODE_PARTY;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_CANDLELIGHT)) {
        newSceneMode = SCENE_MODE_CANDLELIGHT;
#ifdef SAMSUNG_COMPANION
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_HDR)) {
        newSceneMode = SCENE_MODE_HDR;
#endif
#ifdef SAMSUNG_FOOD_MODE
    } else if (!strcmp(strNewSceneMode, "food")) {
        enum aa_aemode aeMode = AA_AEMODE_MATRIX;
        newSceneMode = SCENE_MODE_FOOD;

        m_setExposureCompensation(2);
        setMetaCtlAeMode(&m_metadata, aeMode);
#endif
    } else if (!strcmp(strNewSceneMode, "aqua_scn")) {
        newSceneMode = SCENE_MODE_AQUA;
    } else {
        CLOGE("ERR(%s):unmatched scene_mode(%s)", "Parameters", strNewSceneMode);
        return BAD_VALUE;
    }

    curSceneMode = getSceneMode();

    if (curSceneMode != newSceneMode) {
        m_setSceneMode(newSceneMode);
        m_params.set(CameraParameters::KEY_SCENE_MODE, strNewSceneMode);
#ifdef SAMSUNG_COMPANION
        if(getUseCompanion() == true) {
            if (newSceneMode == SCENE_MODE_HDR)
                checkSceneRTHdr(true);
            else
                checkSceneRTHdr(false);
        }

        if ((newSceneMode == SCENE_MODE_HDR) || (curSceneMode == SCENE_MODE_HDR))
            m_setRestartPreview(true);
#endif
        updatePreviewFpsRange();
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setSceneMode(int value)
{
    enum aa_mode mode = AA_CONTROL_AUTO;
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;

    switch (value) {
    case SCENE_MODE_PORTRAIT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_NIGHT;
        break;
    case SCENE_MODE_BEACH:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_BEACH;
        break;
    case SCENE_MODE_SNOW:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SNOW;
        break;
    case SCENE_MODE_SUNSET:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SUNSET;
        break;
    case SCENE_MODE_FIREWORKS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_FIREWORKS;
        break;
    case SCENE_MODE_SPORTS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SPORTS;
        break;
    case SCENE_MODE_PARTY:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PARTY;
        break;
    case SCENE_MODE_CANDLELIGHT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_CANDLELIGHT;
        break;
    case SCENE_MODE_STEADYPHOTO:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_STEADYPHOTO;
        break;
    case SCENE_MODE_ACTION:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_ACTION;
        break;
    case SCENE_MODE_NIGHT_PORTRAIT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_NIGHT_PORTRAIT;
        break;
    case SCENE_MODE_THEATRE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_THEATRE;
        break;
#ifdef SAMSUNG_FOOD_MODE
    case SCENE_MODE_FOOD:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_FOOD;
        break;
#endif
    case SCENE_MODE_AQUA:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_AQUA;
        break;
    case SCENE_MODE_AUTO:
    default:
        mode = AA_CONTROL_AUTO;
        sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        break;
    }

    m_cameraInfo.sceneMode = value;
    setMetaCtlSceneMode(&m_metadata, mode, sceneMode);
    m_cameraInfo.whiteBalanceMode = m_convertMetaCtlAwbMode(&m_metadata);
}

status_t ExynosCamera1Parameters::checkFocusMode(const CameraParameters& params)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return NO_ERROR;
    } else {
        int  newFocusMode = -1;
        int  curFocusMode = -1;
        const char *strFocusMode = params.get(CameraParameters::KEY_FOCUS_MODE);
        const char *strNewFocusMode = m_adjustFocusMode(strFocusMode);

        if (strNewFocusMode == NULL) {
            return NO_ERROR;
        }

        CLOGD("DEBUG(%s):strNewFocusMode %s", "setParameters", strNewFocusMode);

        if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_AUTO)) {
            newFocusMode = FOCUS_MODE_AUTO;
            m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                    BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_INFINITY)) {
            newFocusMode = FOCUS_MODE_INFINITY;
            m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                    BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR);
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_MACRO)) {
            newFocusMode = FOCUS_MODE_MACRO;
            m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                    BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR);
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_FIXED)) {
            newFocusMode = FOCUS_MODE_FIXED;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_EDOF)) {
            newFocusMode = FOCUS_MODE_EDOF;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO)) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_VIDEO;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_PICTURE;
        } else if (!strcmp(strNewFocusMode, "face-priority")) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_PICTURE;
        } else if (!strcmp(strNewFocusMode, "continuous-picture-macro")) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_PICTURE_MACRO;
#ifdef SAMSUNG_OT
        } else if (!strcmp(strNewFocusMode, "object-tracking-picture")) {
            newFocusMode = FOCUS_MODE_OBJECT_TRACKING_PICTURE;
        } else if (!strcmp(strNewFocusMode, "object-tracking-video")) {
            newFocusMode = FOCUS_MODE_OBJECT_TRACKING_VIDEO;
#endif
#ifdef SAMSUNG_MANUAL_FOCUS
        } else if (!strcmp(strNewFocusMode, "manual")) {
            newFocusMode = FOCUS_MODE_MANUAL;
#endif
        } else {
            CLOGE("ERR(%s):unmatched focus_mode(%s)", __FUNCTION__, strNewFocusMode);
            return BAD_VALUE;
        }

        if (!(newFocusMode & getSupportedFocusModes())){
            CLOGE("ERR(%s[%d]): Focus mode(%s) is not supported!", __FUNCTION__, __LINE__, strNewFocusMode);
            return BAD_VALUE;
        }

#ifdef SAMSUNG_MANUAL_FOCUS
        /* Set focus distance to -1 if focus mode is changed from MANUAL to others */
        if ((FOCUS_MODE_MANUAL == getFocusMode()) && (FOCUS_MODE_MANUAL != newFocusMode)) {
            m_setFocusDistance(-1);
        }
#endif

#ifdef USE_FACTORY_FAST_AF
        if (getPreviewRunning() == true) {
            if (m_isFactoryMode == true && getRecordingHint() == true
                && m_cameraInfo.focusMode == FOCUS_MODE_CONTINUOUS_PICTURE
                && newFocusMode == FOCUS_MODE_CONTINUOUS_VIDEO) {
                CLOGD("DEBUG(%s:%d):need restart to set video setfile", __FUNCTION__, __LINE__);
                m_setRestartPreviewChecked(true);
            }
        }
#endif
        m_setFocusMode(newFocusMode);
        m_params.set(CameraParameters::KEY_FOCUS_MODE, strNewFocusMode);

        return NO_ERROR;
    }
}

const char *ExynosCamera1Parameters::m_adjustFocusMode(const char *focusMode)
{
    int sceneMode = getSceneMode();
    const char *newFocusMode = NULL;

    /* TODO: vendor specific adjust */

    newFocusMode = focusMode;

    return newFocusMode;
}

void ExynosCamera1Parameters::m_setFocusMode(int focusMode)
{
    m_cameraInfo.focusMode = focusMode;

    if(getZoomActiveOn()) {
        ALOGD("DEBUG(%s):zoom moving..", "setParameters");
        return;
    }

    /* TODO: Notify auto focus activity */
    if(getPreviewRunning() == true) {
        ALOGD("set Focus Mode(%s[%d]) !!!!", __FUNCTION__, __LINE__);
        m_activityControl->setAutoFocusMode(focusMode);
    } else {
        m_setFocusmodeSetting = true;
    }

#ifdef SAMSUNG_OT
    if(m_cameraInfo.focusMode == FOCUS_MODE_OBJECT_TRACKING_VIDEO
       || m_cameraInfo.focusMode == FOCUS_MODE_OBJECT_TRACKING_PICTURE) {
        m_startObjectTracking = true;
    }
    else
        m_startObjectTracking = false;
#endif
}

#ifdef SAMSUNG_MANUAL_FOCUS
status_t ExynosCamera1Parameters::checkFocusDistance(const CameraParameters& params)
{
    int newFocusDistance = params.getInt("focus-distance");
    int curFocusDistance = -1;

    if (newFocusDistance < 0) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newFocusDistance %d", "setParameters", newFocusDistance);

    curFocusDistance = getFocusDistance();

    if (curFocusDistance != newFocusDistance) {
        if (FOCUS_MODE_MANUAL != getFocusMode()) {
            newFocusDistance = -1;
        }

        m_setFocusDistance(newFocusDistance);
        m_params.set("focus-distance", newFocusDistance);
    }

    return NO_ERROR;
}

/**
 * m_setFocusDistance: set focus distance.
 *
 * distance should be -1, 0 or value greater than 0.
 */
void ExynosCamera1Parameters::m_setFocusDistance(int32_t distance)
{
    float metaDistance;

    if (distance < -1) {
        CLOGE("ERR(%s):Invalid new focus distance (%d)", __FUNCTION__, distance);
        return;
    }

    if (!distance) {
        metaDistance = 0;
    } else if (-1 == distance) {
        metaDistance = -1;
    } else {
        /* distance variable is in milimeters.
         * And if distance variable is converted into meter unit(called as A),
         * meta distance value is as followings:
         *     metaDistance = 1 / A     */
        metaDistance = 1000 / (float)distance;
    }

    setMetaCtlFocusDistance(&m_metadata, metaDistance);
}

int32_t ExynosCamera1Parameters::getFocusDistance(void)
{
    float metaDistance = 0.0f;

    getMetaCtlFocusDistance(&m_metadata, &metaDistance);

    /* Focus distance 0 means infinite */
    if (0.0f == metaDistance) {
        return -2;
    } else if (-1.0f == metaDistance) {
        return -1;
    }

    return (int32_t)(1000 / metaDistance);
}
#endif /* SAMSUNG_MANUAL_FOCUS */

#ifdef SAMSUNG_FOOD_MODE
status_t ExynosCamera1Parameters::checkWbLevel(const CameraParameters& params)
{
    int32_t newWbLevel = params.getInt("wb-level");
    int32_t curWbLevel = getWbLevel();
    int minWbLevel = -4, maxWbLevel = 4;

    if ((newWbLevel < minWbLevel) || (newWbLevel > maxWbLevel)) {
        ALOGE("ERR(%s): Invalide WbLevel (Min: %d, Max: %d, Value: %d)", __FUNCTION__,
            minWbLevel, maxWbLevel, newWbLevel);
        return BAD_VALUE;
    }

    ALOGD("DEBUG(%s):newWbLevel %d", "setParameters", newWbLevel);

    if (curWbLevel != newWbLevel) {
        m_setWbLevel(newWbLevel);
        m_params.set("wb-level", newWbLevel);
    }

    return NO_ERROR;
}

#define IS_WBLEVEL_DEFAULT (4)

void ExynosCamera1Parameters::m_setWbLevel(int32_t value)
{
    setMetaCtlWbLevel(&m_metadata, value + IS_WBLEVEL_DEFAULT + FW_CUSTOM_OFFSET);
}

int32_t ExynosCamera1Parameters::getWbLevel(void)
{
    int32_t wbLevel;

    getMetaCtlWbLevel(&m_metadata, &wbLevel);

    return wbLevel - IS_WBLEVEL_DEFAULT - FW_CUSTOM_OFFSET;
}

#endif

status_t ExynosCamera1Parameters::checkFocusAreas(const CameraParameters& params)
{
    int ret = NO_ERROR;
    const char *newFocusAreas = params.get(CameraParameters::KEY_FOCUS_AREAS);
    int curFocusMode = getFocusMode();
    uint32_t maxNumFocusAreas = getMaxNumFocusAreas();

    if (newFocusAreas == NULL) {
        ExynosRect2 nullRect2[1];
        nullRect2[0].x1 = 0;
        nullRect2[0].y1 = 0;
        nullRect2[0].x2 = 0;
        nullRect2[0].y2 = 0;

        if (m_cameraInfo.numValidFocusArea != 0)
            m_setFocusAreas(0, nullRect2, NULL);

        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newFocusAreas %s", "setParameters", newFocusAreas);

    /* In CameraParameters.h */
    /*
     * Focus area only has effect if the cur focus mode is FOCUS_MODE_AUTO,
     * FOCUS_MODE_MACRO, FOCUS_MODE_CONTINUOUS_VIDEO, or
     * FOCUS_MODE_CONTINUOUS_PICTURE.
     */
    if (curFocusMode & FOCUS_MODE_AUTO
         || curFocusMode & FOCUS_MODE_MACRO
         || curFocusMode & FOCUS_MODE_CONTINUOUS_VIDEO
         || curFocusMode & FOCUS_MODE_CONTINUOUS_PICTURE
         || curFocusMode & FOCUS_MODE_CONTINUOUS_PICTURE_MACRO
#ifdef SAMSUNG_OT
         || curFocusMode & FOCUS_MODE_OBJECT_TRACKING_VIDEO
         || curFocusMode & FOCUS_MODE_OBJECT_TRACKING_PICTURE
#endif
    ) {

        /* ex : (-10,-10,0,0,300),(0,0,10,10,700) */
        ExynosRect2 *rect2s = new ExynosRect2[maxNumFocusAreas];
        int         *weights = new int[maxNumFocusAreas];

        uint32_t validFocusedAreas = bracketsStr2Ints((char *)newFocusAreas, maxNumFocusAreas, rect2s, weights, 1);

        /* Check duplicate area */
        if (validFocusedAreas > 1) {
            for (uint32_t k = 0; k < validFocusedAreas; k++) {
                if (rect2s[k].x1 == rect2s[k+1].x1 &&
                        rect2s[k].y1 == rect2s[k+1].y1 &&
                        rect2s[k].x2 == rect2s[k+1].x2 &&
                        rect2s[k].y2 == rect2s[k+1].y2)
                    validFocusedAreas = 0;
            }
        }

        if (0 < validFocusedAreas) {
            /* CameraParameters.h */
            /*
              * A special case of single focus area (0,0,0,0,0) means driver to decide
              * the focus area. For example, the driver may use more signals to decide
              * focus areas and change them dynamically. Apps can set (0,0,0,0,0) if they
              * want the driver to decide focus areas.
              */
            m_setFocusAreas(validFocusedAreas, rect2s, weights);
            m_params.set(CameraParameters::KEY_FOCUS_AREAS, newFocusAreas);
        } else {
            CLOGE("ERR(%s):FocusAreas value is invalid", __FUNCTION__);
            ret = UNKNOWN_ERROR;
        }

        delete [] rect2s;
        delete [] weights;
    }

    return ret;
}

void ExynosCamera1Parameters::m_setFocusAreas(uint32_t numValid, ExynosRect2 *rect2s, int *weights)
{
#ifdef SAMSUNG_OT
    int curFocusMode = getFocusMode();
#endif
    uint32_t maxNumFocusAreas = getMaxNumFocusAreas();
    if (maxNumFocusAreas < numValid)
        numValid = maxNumFocusAreas;

    if ((numValid == 1 || numValid == 0) && (isRectNull(&rect2s[0]) == true)) {
        /* m_setFocusMode(FOCUS_MODE_AUTO); */
        ExynosRect2 newRect2(0,0,0,0);
        m_activityControl->setAutoFcousArea(newRect2, 1000);

        m_activityControl->touchAFMode = false;
        m_activityControl->touchAFModeForFlash = false;
    } else {
#ifdef SAMSUNG_OT
        if(curFocusMode & FOCUS_MODE_OBJECT_TRACKING_VIDEO
            || curFocusMode & FOCUS_MODE_OBJECT_TRACKING_PICTURE
            || m_objectTrackingGet == true) {
            for (uint32_t i = 0; i < numValid; i++) {
                m_objectTrackingArea[i] = rect2s[i];
                m_objectTrackingWeight[i] = weights[i];
                m_objectTrackingAreaChanged = true;

                m_activityControl->touchAFMode = false;
                m_activityControl->touchAFModeForFlash = false;
            }

            m_cameraInfo.numValidFocusArea = numValid;

            return;
        }
#endif
        ExynosRect cropRegionRect;
        ExynosRect2 newRect2;

        getHwBayerCropRegion(&cropRegionRect.w, &cropRegionRect.h, &cropRegionRect.x, &cropRegionRect.y);

        for (uint32_t i = 0; i < numValid; i++) {
            newRect2 = convertingAndroidArea2HWAreaBcropOut(&rect2s[i], &cropRegionRect);
            /*setMetaCtlAfRegion(&m_metadata, rect2s[i].x1, rect2s[i].y1,
                                rect2s[i].x2, rect2s[i].y2, weights[i]);*/
            m_activityControl->setAutoFcousArea(newRect2, weights[i]);
        }
        m_activityControl->touchAFMode = true;
        m_activityControl->touchAFModeForFlash = true;
    }

    m_cameraInfo.numValidFocusArea = numValid;
}

void ExynosCamera1Parameters::m_setExifChangedAttribute(exif_attribute_t *exifInfo,
                                                       ExynosRect       *pictureRect,
                                                       ExynosRect       *thumbnailRect,
                                                       __unused camera2_dm       *dm,
                                                       camera2_udm      *udm)
{
    /* 2 0th IFD TIFF Tags */
    /* 3 Width */
    exifInfo->width = pictureRect->w;
    /* 3 Height */
    exifInfo->height = pictureRect->h;

    /* 3 Orientation */
    switch (m_cameraInfo.rotation) {
    case 90:
        exifInfo->orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        exifInfo->orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        exifInfo->orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        exifInfo->orientation = EXIF_ORIENTATION_UP;
        break;
    }

    /* 3 Maker note */
    /* back-up udm info for exif's maker note */
#ifdef SAMSUNG_OIS
    if (getCameraId() == CAMERA_ID_BACK) {
        memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)udm, sizeof(struct camera2_udm));
        getOisEXIFFromFile(m_staticInfo, (int)m_cameraInfo.oisMode);

        /* Copy ois data to debugData*/
        memcpy((void *)(mDebugInfo.debugData[APP_MARKER_4] + sizeof(struct camera2_udm)),
            (void *)&m_staticInfo->ois_exif_info, sizeof(m_staticInfo->ois_exif_info));
    } else {
        memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)udm, mDebugInfo.debugSize[APP_MARKER_4]);
    }
#else
    memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)udm, mDebugInfo.debugSize[APP_MARKER_4]);
#endif
    exifInfo->maker_note_size = 0;

    /* 3 Date time */
    struct timeval rawtime;
    struct tm timeinfo;
    gettimeofday(&rawtime, NULL);
    localtime_r((time_t *)&rawtime.tv_sec, &timeinfo);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", &timeinfo);
    snprintf((char *)exifInfo->sec_time, 5, "%04d", (int)(rawtime.tv_usec/1000));

#ifdef SAMSUNG_UTC_TS
    gmtime_r((time_t *)&rawtime.tv_sec, &timeinfo);
    struct utc_ts m_utc;
    strftime((char *)&m_utc.utc_ts_data[4], 20, "%Y:%m:%d %H:%M:%S", &timeinfo);
    /* UTC Time Info Tag : 0x1001 */
    m_utc.utc_ts_data[0] = 0x10;
    m_utc.utc_ts_data[1] = 0x01;
    m_utc.utc_ts_data[2] = 0x00;
    m_utc.utc_ts_data[3] = 0x16;
    memcpy(mDebugInfo.debugData[5], m_utc.utc_ts_data, sizeof(struct utc_ts));
#endif

    /* 2 0th IFD Exif Private Tags */
    bool flagSLSIAlgorithm = true;
    /*
     * vendorSpecific2[100]      : exposure
     * vendorSpecific2[101]      : iso(gain)
     * vendorSpecific2[102] /256 : Bv
     * vendorSpecific2[103]      : Tv
     */

    /* 3 ISO Speed Rating */
    exifInfo->iso_speed_rating = udm->internal.vendorSpecific2[101];

    /* 3 Exposure Time */
    exifInfo->exposure_time.num = 1;

    if (udm->ae.vendorSpecific[0] == 0xAEAEAEAE)
        exifInfo->exposure_time.den = (uint32_t)udm->ae.vendorSpecific[64];
    else
        exifInfo->exposure_time.den = (uint32_t)udm->internal.vendorSpecific2[100];

    /* 3 Shutter Speed */
    exifInfo->shutter_speed.num = (uint32_t)(ROUND_OFF_HALF(((double)(udm->internal.vendorSpecific2[103] / 256.f) * EXIF_DEF_APEX_DEN), 0));
    exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        /* 3 Aperture */
        exifInfo->aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
        exifInfo->aperture.den = COMMON_DENOMINATOR;

        /* 3 Max Aperture */
        exifInfo->max_aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
        exifInfo->max_aperture.den = COMMON_DENOMINATOR;
    } else {
        /* 3 Aperture */
        exifInfo->aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * m_staticInfo->apertureDen;
        exifInfo->aperture.den = m_staticInfo->apertureDen;

        /* 3 Max Aperture */
        exifInfo->max_aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * m_staticInfo->apertureDen;
        exifInfo->max_aperture.den = m_staticInfo->apertureDen;
    }

    /* 3 Brightness */
    int temp = udm->internal.vendorSpecific2[102];
    if ((int)udm->ae.vendorSpecific[102] < 0)
        temp = -temp;

    exifInfo->brightness.num = (int32_t)(ROUND_OFF_HALF((double)((temp * EXIF_DEF_APEX_DEN) / 256.f), 0));
    if ((int)udm->ae.vendorSpecific[102] < 0)
        exifInfo->brightness.num = -exifInfo->brightness.num;

    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;

    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[100](%d)", __FUNCTION__, udm->internal.vendorSpecific2[100]);
    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[101](%d)", __FUNCTION__, udm->internal.vendorSpecific2[101]);
    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[102](%d)", __FUNCTION__, udm->internal.vendorSpecific2[102]);
    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[103](%d)", __FUNCTION__, udm->internal.vendorSpecific2[103]);

    CLOGD("DEBUG(%s):iso_speed_rating(%d)", __FUNCTION__, exifInfo->iso_speed_rating);
    CLOGD("DEBUG(%s):exposure_time(%d/%d)", __FUNCTION__, exifInfo->exposure_time.num, exifInfo->exposure_time.den);
    CLOGD("DEBUG(%s):shutter_speed(%d/%d)", __FUNCTION__, exifInfo->shutter_speed.num, exifInfo->shutter_speed.den);
    CLOGD("DEBUG(%s):aperture     (%d/%d)", __FUNCTION__, exifInfo->aperture.num, exifInfo->aperture.den);
    CLOGD("DEBUG(%s):brightness   (%d/%d)", __FUNCTION__, exifInfo->brightness.num, exifInfo->brightness.den);

    /* 3 Exposure Bias */
    exifInfo->exposure_bias.num = (int32_t)getExposureCompensation() * (m_staticInfo->exposureCompensationStep * 10);
    exifInfo->exposure_bias.den = 10;

    /* 3 Metering Mode */
#ifdef SAMSUNG_COMPANION
    enum companion_wdr_mode wdr_mode;
    wdr_mode = getRTHdr();
    if (wdr_mode == COMPANION_WDR_ON) {
        exifInfo->metering_mode = EXIF_METERING_PATTERN;
    } else
#endif
    {
        switch (m_cameraInfo.meteringMode) {
        case METERING_MODE_CENTER:
            exifInfo->metering_mode = EXIF_METERING_CENTER;
            break;
        case METERING_MODE_MATRIX:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        case METERING_MODE_SPOT:
#ifdef TOUCH_AE
        case METERING_MODE_CENTER_TOUCH:
        case METERING_MODE_SPOT_TOUCH:
        case METERING_MODE_AVERAGE_TOUCH:
        case METERING_MODE_MATRIX_TOUCH:
#endif
            exifInfo->metering_mode = EXIF_METERING_SPOT;
            break;
        case METERING_MODE_AVERAGE:
        default:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        }

#ifdef SAMSUNG_FOOD_MODE
        if(getSceneMode() == SCENE_MODE_FOOD) {
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
        }
#endif
    }

    /* 3 Flash */
    if (m_cameraInfo.flashMode == FLASH_MODE_OFF) {
        exifInfo->flash = 0;
    } else if (m_cameraInfo.flashMode == FLASH_MODE_TORCH) {
        exifInfo->flash = 1;
    } else {
        exifInfo->flash = getMarkingOfExifFlash();
    }

    /* 3 White Balance */
    if (m_cameraInfo.whiteBalanceMode == WHITE_BALANCE_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

    /* 3 Focal Length in 35mm length */
    exifInfo->focal_length_in_35mm_length = m_staticInfo->focalLengthIn35mmLength;

    /* 3 Scene Capture Type */
    switch (m_cameraInfo.sceneMode) {
    case SCENE_MODE_PORTRAIT:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHT:
        exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    /* 3 Image Unique ID */
#if defined(SAMSUNG_TN_FEATURE) && defined(SENSOR_FW_GET_FROM_FILE)
    char *front_fw = NULL;
    char *savePtr;

    if (getCameraId() == CAMERA_ID_BACK){
        memset(exifInfo->unique_id, 0, sizeof(exifInfo->unique_id));
        strncpy((char *)exifInfo->unique_id,
                getSensorFWFromFile(m_staticInfo, m_cameraId), sizeof(exifInfo->unique_id) - 1);
    } else if (getCameraId() == CAMERA_ID_FRONT) {
        front_fw = strtok_r((char *)getSensorFWFromFile(m_staticInfo, m_cameraId), " ", &savePtr);
        strcpy((char *)exifInfo->unique_id, front_fw);
    }
#endif

    /* 2 0th IFD GPS Info Tags */
    if (m_cameraInfo.gpsLatitude != 0 && m_cameraInfo.gpsLongitude != 0) {
        if (m_cameraInfo.gpsLatitude > 0)
            strncpy((char *)exifInfo->gps_latitude_ref, "N", 2);
        else
            strncpy((char *)exifInfo->gps_latitude_ref, "S", 2);

        if (m_cameraInfo.gpsLongitude > 0)
            strncpy((char *)exifInfo->gps_longitude_ref, "E", 2);
        else
            strncpy((char *)exifInfo->gps_longitude_ref, "W", 2);

        if (m_cameraInfo.gpsAltitude > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        double latitude = fabs(m_cameraInfo.gpsLatitude);
        double longitude = fabs(m_cameraInfo.gpsLongitude);
        double altitude = fabs(m_cameraInfo.gpsAltitude);

        exifInfo->gps_latitude[0].num = (uint32_t)latitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((latitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)(round((((latitude - exifInfo->gps_latitude[0].num) * 60)
                                        - exifInfo->gps_latitude[1].num) * 60));
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)longitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((longitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)(round((((longitude - exifInfo->gps_longitude[0].num) * 60)
                                        - exifInfo->gps_longitude[1].num) * 60));
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)altitude;
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        gmtime_r(&m_cameraInfo.gpsTimeStamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    /* 2 1th IFD TIFF Tags */
    exifInfo->widthThumb = thumbnailRect->w;
    exifInfo->heightThumb = thumbnailRect->h;

    setMarkingOfExifFlash(0);
}

/* for CAMERA2_API_SUPPORT */
void ExynosCamera1Parameters::m_setExifChangedAttribute(exif_attribute_t *exifInfo,
                                                       ExynosRect       *pictureRect,
                                                       ExynosRect       *thumbnailRect,
                                                       camera2_shot_t   *shot)
{
    /* JPEG Picture Size */
    exifInfo->width = pictureRect->w;
    exifInfo->height = pictureRect->h;

    /* Orientation */
    switch (shot->ctl.jpeg.orientation) {
    case 90:
        exifInfo->orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        exifInfo->orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        exifInfo->orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        exifInfo->orientation = EXIF_ORIENTATION_UP;
        break;
    }

    /* Maker Note Size */
    /* back-up udm info for exif's maker note */
#ifdef SAMSUNG_OIS
    if (getCameraId() == CAMERA_ID_BACK) {
        memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)&shot->udm, sizeof(struct camera2_udm));
        getOisEXIFFromFile(m_staticInfo, (int)m_cameraInfo.oisMode);

        /* Copy ois data to debugData*/
        memcpy((void *)(mDebugInfo.debugData[APP_MARKER_4] + sizeof(struct camera2_udm)),
                (void *)&m_staticInfo->ois_exif_info, sizeof(m_staticInfo->ois_exif_info));
    } else {
        memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)&shot->udm, mDebugInfo.debugSize[APP_MARKER_4]);
    }
#else
    memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)&shot->udm, mDebugInfo.debugSize[APP_MARKER_4]);
#endif

    /* TODO */
#if 0
    if (getSeriesShotCount() && getShotMode() != SHOT_MODE_BEST_PHOTO) {
        unsigned char l_makernote[98] = { 0x07, 0x00, 0x01, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
            0x30, 0x31, 0x30, 0x30, 0x02, 0x00, 0x04, 0x00, 0x01, 0x00,
            0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x40, 0x00, 0x04, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00,
            0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x10, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5A, 0x00,
            0x00, 0x00, 0x50, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        long long int mCityId = getCityId();
        l_makernote[46] = getWeatherId();
        memcpy(l_makernote + 90, &mCityId, 8);
        exifInfo->maker_note_size = 98;
        memcpy(exifInfo->maker_note, l_makernote, sizeof(l_makernote));
    } else {
        exifInfo->maker_note_size = 0;
    }
#else
    exifInfo->maker_note_size = 0;
#endif

    /* Date Time */
    struct timeval rawtime;
    struct tm timeinfo;
    gettimeofday(&rawtime, NULL);
    localtime_r((time_t *)&rawtime.tv_sec, &timeinfo);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", &timeinfo);
    sprintf((char *)exifInfo->sec_time, "%d", (int)(rawtime.tv_usec/1000));

    /* Exif Private Tags */
    bool flagSLSIAlgorithm = true;
    /*
     * vendorSpecific2[0] : info
     * vendorSpecific2[100] : 0:sirc 1:cml
     * vendorSpecific2[101] : cml exposure
     * vendorSpecific2[102] : cml iso(gain)
     * vendorSpecific2[103] : cml Bv
     */

    /* ISO Speed Rating */
    exifInfo->iso_speed_rating = shot->dm.aa.vendor_isoValue;

    /* Exposure Time */
    exifInfo->exposure_time.num = 1;
    /* HACK : Sometimes, F/W does NOT send the exposureTime */
    if (shot->dm.sensor.exposureTime != 0)
        exifInfo->exposure_time.den = (uint32_t) 1e9 / shot->dm.sensor.exposureTime;
    else
        exifInfo->exposure_time.num = 0;

    /* Shutter Speed */
    exifInfo->shutter_speed.num = (uint32_t) (ROUND_OFF_HALF(((double) (shot->udm.internal.vendorSpecific2[104] / 256.f) * EXIF_DEF_APEX_DEN), 0));
    exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;

    /* Aperture */
    exifInfo->aperture.num = APEX_FNUM_TO_APERTURE((double) (exifInfo->fnumber.num) / (double) (exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
    exifInfo->aperture.den = COMMON_DENOMINATOR;

    /* Max Aperture */
    exifInfo->max_aperture.num = APEX_FNUM_TO_APERTURE((double) (exifInfo->fnumber.num) / (double) (exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
    exifInfo->max_aperture.den = COMMON_DENOMINATOR;

    /* Brightness */
    int temp = shot->udm.internal.vendorSpecific2[103];
    if ((int) shot->udm.ae.vendorSpecific[103] < 0)
        temp = -temp;
    exifInfo->brightness.num = (int32_t) (ROUND_OFF_HALF((double)((temp * EXIF_DEF_APEX_DEN)/256.f), 0));
    if ((int) shot->udm.ae.vendorSpecific[103] < 0)
        exifInfo->brightness.num = -exifInfo->brightness.num;
    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;

    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[101](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[101]);
    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[102](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[102]);
    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[103](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[103]);
    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[104](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[104]);

    ALOGD("DEBUG(%s):iso_speed_rating(%d)", __FUNCTION__, exifInfo->iso_speed_rating);
    ALOGD("DEBUG(%s):exposure_time(%d/%d)", __FUNCTION__, exifInfo->exposure_time.num, exifInfo->exposure_time.den);
    ALOGD("DEBUG(%s):shutter_speed(%d/%d)", __FUNCTION__, exifInfo->shutter_speed.num, exifInfo->shutter_speed.den);
    ALOGD("DEBUG(%s):aperture     (%d/%d)", __FUNCTION__, exifInfo->aperture.num, exifInfo->aperture.den);
    ALOGD("DEBUG(%s):brightness   (%d/%d)", __FUNCTION__, exifInfo->brightness.num, exifInfo->brightness.den);

    /* Exposure Bias */
#if defined(USE_SUBDIVIDED_EV)
    exifInfo->exposure_bias.num = shot->ctl.aa.aeExpCompensation * (m_staticInfo->exposureCompensationStep * 10);
#else
    exifInfo->exposure_bias.num =
        (shot->ctl.aa.aeExpCompensation) * (m_staticInfo->exposureCompensationStep * 10);
#endif
    exifInfo->exposure_bias.den = 10;

    /* Metering Mode */
#ifdef SAMSUNG_COMPANION
    enum companion_wdr_mode wdr_mode;
    wdr_mode = shot->uctl.companionUd.wdr_mode;
    if (wdr_mode == COMPANION_WDR_ON) {
        exifInfo->metering_mode = EXIF_METERING_PATTERN;
    } else
#endif
    {
        switch (shot->ctl.aa.aeMode) {
        case AA_AEMODE_CENTER:
            exifInfo->metering_mode = EXIF_METERING_CENTER;
            break;
        case AA_AEMODE_MATRIX:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        case AA_AEMODE_SPOT:
            exifInfo->metering_mode = EXIF_METERING_SPOT;
            break;
        default:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        }

#ifdef SAMSUNG_FOOD_MODE
        if(getSceneMode() == SCENE_MODE_FOOD) {
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
        }
#endif
    }

    /* Flash Mode */
    if (shot->ctl.flash.flashMode == CAM2_FLASH_MODE_OFF) {
        exifInfo->flash = 0;
    } else if (shot->ctl.flash.flashMode == CAM2_FLASH_MODE_TORCH) {
        exifInfo->flash = 1;
    } else {
        exifInfo->flash = getMarkingOfExifFlash();
    }

    /* White Balance */
    if (shot->ctl.aa.awbMode == AA_AWBMODE_WB_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

    /* Focal Length in 35mm length */
    exifInfo->focal_length_in_35mm_length = getFocalLengthIn35mmFilm();

    /* Scene Capture Type */
    switch (shot->ctl.aa.sceneMode) {
    case AA_SCENE_MODE_PORTRAIT:
    case AA_SCENE_MODE_FACE_PRIORITY:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case AA_SCENE_MODE_LANDSCAPE:
        exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case AA_SCENE_MODE_NIGHT:
        exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    /* Image Unique ID */
#if defined(SAMSUNG_TN_FEATURE) && defined(SENSOR_FW_GET_FROM_FILE)
    char *front_fw = NULL;
    char *savePtr;

    if (getCameraId() == CAMERA_ID_BACK){
        memset(exifInfo->unique_id, 0, sizeof(exifInfo->unique_id));
        strncpy((char *)exifInfo->unique_id,
                getSensorFWFromFile(m_staticInfo, m_cameraId), sizeof(exifInfo->unique_id) - 1);
    } else if (getCameraId() == CAMERA_ID_FRONT) {
        front_fw = strtok_r((char *)getSensorFWFromFile(m_staticInfo, m_cameraId), " ", &savePtr);
        strcpy((char *)exifInfo->unique_id, front_fw);
    }
#endif

    /* GPS Coordinates */
    double gpsLatitude = shot->ctl.jpeg.gpsCoordinates[0];
    double gpsLongitude = shot->ctl.jpeg.gpsCoordinates[1];
    double gpsAltitude = shot->ctl.jpeg.gpsCoordinates[2];
    if (gpsLatitude != 0 && gpsLongitude != 0) {
        if (gpsLatitude > 0)
            strncpy((char *) exifInfo->gps_latitude_ref, "N", 2);
        else
            strncpy((char *) exifInfo->gps_latitude_ref, "s", 2);

        if (gpsLongitude > 0)
            strncpy((char *) exifInfo->gps_longitude_ref, "E", 2);
        else
            strncpy((char *) exifInfo->gps_longitude_ref, "W", 2);

        if (gpsAltitude > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        gpsLatitude = fabs(gpsLatitude);
        gpsLongitude = fabs(gpsLongitude);
        gpsAltitude = fabs(gpsAltitude);

        exifInfo->gps_latitude[0].num = (uint32_t) gpsLatitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((gpsLatitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)(round((((gpsLatitude - exifInfo->gps_latitude[0].num) * 60)
                        - exifInfo->gps_latitude[1].num) * 60));
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)gpsLongitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((gpsLongitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)(round((((gpsLongitude - exifInfo->gps_longitude[0].num) * 60)
                        - exifInfo->gps_longitude[1].num) * 60));
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)gpsAltitude;
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        long gpsTimestamp = (long) shot->ctl.jpeg.gpsTimestamp;
        gmtime_r(&gpsTimestamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    /* Thumbnail Size */
    exifInfo->widthThumb = thumbnailRect->w;
    exifInfo->heightThumb = thumbnailRect->h;

    setMarkingOfExifFlash(0);
}

/* TODO: Do not used yet */
#if 0
status_t ExynosCamera1Parameters::checkCityId(const CameraParameters& params)
{
    long long int newCityId = params.getInt64(CameraParameters::KEY_CITYID);
    long long int curCityId = -1;

    if (newCityId < 0)
        newCityId = 0;

    curCityId = getCityId();

    if (curCityId != newCityId) {
        m_setCityId(newCityId);
        m_params.set(CameraParameters::KEY_CITYID, newCityId);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setCityId(long long int cityId)
{
    m_cameraInfo.cityId = cityId;
}

long long int ExynosCamera1Parameters::getCityId(void)
{
    return m_cameraInfo.cityId;
}

status_t ExynosCamera1Parameters::checkWeatherId(const CameraParameters& params)
{
    int newWeatherId = params.getInt(CameraParameters::KEY_WEATHER);
    int curWeatherId = -1;

    if (newWeatherId < 0 || newWeatherId > 5) {
        return BAD_VALUE;
    }

    curWeatherId = (int)getWeatherId();

    if (curWeatherId != newWeatherId) {
        m_setWeatherId((unsigned char)newWeatherId);
        m_params.set(CameraParameters::KEY_WEATHER, newWeatherId);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setWeatherId(unsigned char weatherId)
{
    m_cameraInfo.weatherId = weatherId;
}

unsigned char ExynosCamera1Parameters::getWeatherId(void)
{
    return m_cameraInfo.weatherId;
}
#endif

#ifdef SAMSUNG_QUICKSHOT
status_t ExynosCamera1Parameters::checkQuickShot(const CameraParameters& params)
{
    int newQshot = params.getInt("quick-shot");
    int curQshot= -1;

    CLOGD("DEBUG(%s):newQshot %d", "setParameters", newQshot);

    if (newQshot < 0) {
        CLOGE("ERR(%s): Invalid quick shot mode", __FUNCTION__, newQshot);
        return BAD_VALUE;
    }

    curQshot = getQuickShot();

    if (curQshot != newQshot) {
        m_params.set("quick-shot", newQshot);
    }

    return NO_ERROR;
}

int ExynosCamera1Parameters::getQuickShot(void)
{
    int32_t qshot = 0;

    qshot = m_params.getInt("quick-shot");
    return qshot;
}
#endif

#ifdef SAMSUNG_OIS
status_t ExynosCamera1Parameters::checkOIS(const CameraParameters& params)
{
    enum optical_stabilization_mode newOIS = OPTICAL_STABILIZATION_MODE_OFF;
    enum optical_stabilization_mode curOIS = OPTICAL_STABILIZATION_MODE_OFF;
    const char *strNewOIS = params.get("ois");
    int zoomLevel = getZoomLevel();
    int zoomRatio = (int)getZoomRatio(zoomLevel);

    if (strNewOIS == NULL) {
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s):strNewOIS %s", "setParameters", strNewOIS);

    if (!strcmp(strNewOIS, "off")) {
        newOIS = OPTICAL_STABILIZATION_MODE_OFF;
    } else if (!strcmp(strNewOIS, "still")) {
        if (getRecordingHint() == true) {
            if (m_cameraInfo.videoSizeRatioId == SIZE_RATIO_4_3) {
                newOIS = OPTICAL_STABILIZATION_MODE_VIDEO_RATIO_4_3;
            } else {
                newOIS = OPTICAL_STABILIZATION_MODE_VIDEO;
            }
        } else {
            if(zoomRatio >= 4000) {
                newOIS = OPTICAL_STABILIZATION_MODE_STILL_ZOOM;
            } else {
                newOIS = OPTICAL_STABILIZATION_MODE_STILL;
            }
        }
    } else if (!strcmp(strNewOIS, "still_zoom")) {
        newOIS = OPTICAL_STABILIZATION_MODE_STILL_ZOOM;
    } else if (!strcmp(strNewOIS, "video")) {
        if (m_cameraInfo.videoSizeRatioId == SIZE_RATIO_4_3) {
            newOIS = OPTICAL_STABILIZATION_MODE_VIDEO_RATIO_4_3;
        } else {
            newOIS = OPTICAL_STABILIZATION_MODE_VIDEO;
        }
    } else if (!strcmp(strNewOIS, "sine_x")) {
        newOIS = OPTICAL_STABILIZATION_MODE_SINE_X;
    } else if (!strcmp(strNewOIS, "sine_y")) {
        newOIS = OPTICAL_STABILIZATION_MODE_SINE_Y;
    } else if (!strcmp(strNewOIS, "center")) {
        newOIS = OPTICAL_STABILIZATION_MODE_CENTERING;
    } else {
        ALOGE("ERR(%s):Invalid ois command(%s)", __FUNCTION__, strNewOIS);
        return BAD_VALUE;
    }

    curOIS = getOIS();

    if (curOIS != newOIS) {
#if defined(OIS_ACTUATOR_SKIP_FRAME)
        if(curOIS == OPTICAL_STABILIZATION_MODE_SINE_X && newOIS == OPTICAL_STABILIZATION_MODE_SINE_Y) {
            m_frameSkipCounter.setCompensation(OIS_ACTUATOR_SKIP_FRAME);
        }
#endif
        ALOGD("%s set OIS, new OIS Mode = %d", __FUNCTION__, newOIS);
        m_setOIS(newOIS);
        m_params.set("ois", strNewOIS);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setOIS(enum  optical_stabilization_mode ois)
{
    m_cameraInfo.oisMode = ois;

    if(getZoomActiveOn()) {
        ALOGD("DEBUG(%s):zoom moving..", "setParameters");
        return;
    }

#if 0 // Host controlled OIS Factory Mode
    if(m_oisNode) {
        setOISMode();
    } else {
        ALOGD("Ois node is not prepared yet(%s[%d]) !!!!", __FUNCTION__, __LINE__);
        m_setOISmodeSetting = true;
    }
#else
    setOISMode();
#endif
}

enum optical_stabilization_mode ExynosCamera1Parameters::getOIS(void)
{
    return m_cameraInfo.oisMode;
}

void ExynosCamera1Parameters::setOISNode(ExynosCameraNode *node)
{
    m_oisNode = node;
}

void ExynosCamera1Parameters::setOISModeSetting(bool enable)
{
    m_setOISmodeSetting = enable;
}

int ExynosCamera1Parameters::getOISModeSetting(void)
{
    return m_setOISmodeSetting;
}

void ExynosCamera1Parameters::setOISMode(void)
{
    int ret = 0;

    ALOGD("(%s[%d])set OIS Mode = %d", __FUNCTION__, __LINE__, m_cameraInfo.oisMode);

    setMetaCtlOIS(&m_metadata, m_cameraInfo.oisMode);

#if 0 // Host controlled OIS Factory Mode
    if (m_cameraInfo.oisMode == OPTICAL_STABILIZATION_MODE_SINE_X && m_oisNode != NULL) {
        ret = m_oisNode->setControl(V4L2_CID_CAMERA_OIS_SINE_MODE, OPTICAL_STABILIZATION_MODE_SINE_X);
        if (ret < 0) {
            ALOGE("ERR(%s[%d]):FLITE setControl fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
    } else if (m_cameraInfo.oisMode == OPTICAL_STABILIZATION_MODE_SINE_Y && m_oisNode != NULL) {
        ret = m_oisNode->setControl(V4L2_CID_CAMERA_OIS_SINE_MODE, OPTICAL_STABILIZATION_MODE_SINE_Y);
        if (ret < 0) {
            ALOGE("ERR(%s[%d]):FLITE setControl fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
    }
#endif
}

#endif

#ifdef SAMSUNG_COMPANION
status_t ExynosCamera1Parameters::checkRTDrc(const CameraParameters& params)
{
    enum companion_drc_mode newRTDrc = COMPANION_DRC_OFF;
    enum companion_drc_mode curRTDrc = COMPANION_DRC_OFF;
    const char *strNewRTDrc = params.get("dynamic-range-control");

    if (strNewRTDrc == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewRTDrc %s", "setParameters", strNewRTDrc);

    if (!strcmp(strNewRTDrc, "off")) {
        newRTDrc = COMPANION_DRC_OFF;
    } else if (!strcmp(strNewRTDrc, "on")) {
        newRTDrc = COMPANION_DRC_ON;
    } else {
        CLOGE("ERR(%s):Invalid rt drc(%s)", __FUNCTION__, strNewRTDrc);
        return BAD_VALUE;
    }

    curRTDrc = getRTDrc();

    if (curRTDrc != newRTDrc) {
        m_setRTDrc(newRTDrc);
        m_params.set("dynamic-range-control", strNewRTDrc);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setRTDrc(enum  companion_drc_mode rtdrc)
{
    setMetaCtlRTDrc(&m_metadata, rtdrc);
}

enum companion_drc_mode ExynosCamera1Parameters::getRTDrc(void)
{
    enum companion_drc_mode mode;

    getMetaCtlRTDrc(&m_metadata, &mode);

    return mode;
}

status_t ExynosCamera1Parameters::checkPaf(const CameraParameters& params)
{
    enum companion_paf_mode newPaf = COMPANION_PAF_OFF;
    enum companion_paf_mode curPaf = COMPANION_PAF_OFF;
    const char *strNewPaf = params.get("phase-af");

    if (strNewPaf == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewPaf %s", "setParameters", strNewPaf);

#ifdef RAWDUMP_CAPTURE
    newPaf = COMPANION_PAF_OFF;
#else
    if (!strcmp(strNewPaf, "off")) {
        newPaf = COMPANION_PAF_OFF;
    } else if (!strcmp(strNewPaf, "on")) {
        newPaf = COMPANION_PAF_ON;
    } else {
        CLOGE("ERR(%s):Invalid paf(%s)", __FUNCTION__, strNewPaf);
        return BAD_VALUE;
    }
#endif

    curPaf = getPaf();

    if (curPaf != newPaf) {
        m_setPaf(newPaf);
        m_params.set("phase-af", strNewPaf);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setPaf(enum companion_paf_mode paf)
{
    setMetaCtlPaf(&m_metadata, paf);
}

enum companion_paf_mode ExynosCamera1Parameters::getPaf(void)
{
    enum companion_paf_mode mode = COMPANION_PAF_OFF;

    getMetaCtlPaf(&m_metadata, &mode);

    return mode;
}

status_t ExynosCamera1Parameters::checkRTHdr(const CameraParameters& params)
{
    enum companion_wdr_mode newRTHdr = COMPANION_WDR_OFF;
    enum companion_wdr_mode curRTHdr = COMPANION_WDR_OFF;
    const char *strNewRTHdr = params.get("rt-hdr");

    if (strNewRTHdr == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewRTWdr %s", "setParameters", strNewRTHdr);

    if (!strcmp(strNewRTHdr, "off")) {
        newRTHdr = COMPANION_WDR_OFF;
    } else if (!strcmp(strNewRTHdr, "on")) {
        newRTHdr = COMPANION_WDR_ON;
    } else if (!strcmp(strNewRTHdr, "auto")) {
        newRTHdr = COMPANION_WDR_AUTO;
    } else {
        CLOGE("ERR(%s):Invalid rt wdr(%s)", __FUNCTION__, strNewRTHdr);
        return BAD_VALUE;
    }

    curRTHdr = getRTHdr();

    if (curRTHdr != newRTHdr) {
        m_setRTHdr(newRTHdr);
        m_params.set("rt-hdr", strNewRTHdr);
        /* For Samsung SDK */
        if (getPreviewRunning() == true) {
            ALOGD("DEBUG(%s[%d]):setRestartPreviewChecked true", __FUNCTION__, __LINE__);
            m_setRestartPreviewChecked(true);
        }
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::checkSceneRTHdr(bool onoff)
{
    if (onoff) {
        m_setRTHdr(COMPANION_WDR_ON);
        m_setRTDrc(COMPANION_DRC_ON);
    } else {
        m_setRTHdr(COMPANION_WDR_OFF);
        m_setRTDrc(COMPANION_DRC_OFF);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setRTHdr(enum companion_wdr_mode rthdr)
{
    setMetaCtlRTHdr(&m_metadata, rthdr);
}

enum companion_wdr_mode ExynosCamera1Parameters::getRTHdr(void)
{
    enum companion_wdr_mode mode = COMPANION_WDR_OFF;

    if (getUseCompanion() == true)
        getMetaCtlRTHdr(&m_metadata, &mode);

    return mode;
}
#endif

#ifdef SAMSUNG_TN_FEATURE
void ExynosCamera1Parameters::setParamExifInfo(camera2_udm *udm)
{
    uint32_t iso = udm->internal.vendorSpecific2[101];
    uint32_t exposure_time = 0;

    if (udm->ae.vendorSpecific[0] == 0xAEAEAEAE)
        exposure_time = udm->ae.vendorSpecific[64];
    else
        exposure_time = udm->internal.vendorSpecific2[100];

    ALOGI("DEBUG(%s):set exif_exptime %u, exif_iso %u", __FUNCTION__, exposure_time, iso);

    m_params.set("exif_exptime", exposure_time);
    m_params.set("exif_iso", iso);
}
#endif

status_t ExynosCamera1Parameters::checkWdrMode(const CameraParameters& params)
{
    int newWDR = params.getInt("wdr");
    bool curWDR = -1;
    bool toggle = false;

    if (newWDR < 0) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newWDR %d", "setParameters", newWDR);

    curWDR = getWdrMode();

    if (curWDR != (bool)newWDR) {
        m_setWdrMode((bool)newWDR);
        m_params.set("wdr", newWDR);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setWdrMode(bool wdr)
{
    m_cameraInfo.wdrMode = wdr;
}

bool ExynosCamera1Parameters::getWdrMode(void)
{
    return m_cameraInfo.wdrMode;
}

#ifdef SAMSUNG_LBP
int32_t ExynosCamera1Parameters::getHoldFrameCount(void)
{
    if(getCameraId() == CAMERA_ID_BACK)
        return MAX_LIVE_REAR_BEST_PICS;
    else
        return MAX_LIVE_FRONT_BEST_PICS;
}
#endif

#ifdef USE_BINNING_MODE
int ExynosCamera1Parameters::getBinningMode(void)
{
    char cameraModeProperty[PROPERTY_VALUE_MAX];
    int ret = 0;

    if (m_staticInfo->vtcallSizeLutMax == 0 || m_staticInfo->vtcallSizeLut == NULL) {
       ALOGV("(%s):vtCallSizeLut is NULL, can't support the binnig mode", __FUNCTION__);
       return ret;
    }

#ifdef SAMSUNG_COMPANION
    if ((getCameraId() == CAMERA_ID_FRONT) && getUseCompanion()) {
        ALOGV("(%s): Companion mode in front can't support the binning mode.(%d,%d)",
        __FUNCTION__, getCameraId(), getUseCompanion());
        return ret;
    }
#endif

    /* For VT Call with DualCamera Scenario */
    if (getDualMode()) {
        ALOGV("(%s):DualMode can't support the binnig mode.(%d,%d)", __FUNCTION__, getCameraId(), getDualMode());
        return ret;
    }

    if (getVtMode() != 3 && getVtMode() > 0 && getVtMode() < 5) {
        ret = 1;
    } else {
        if (m_binningProperty == true && getSamsungCamera() == false) {
            ret = 1;
        }
    }
    return ret;
}
#endif

#ifdef USE_PREVIEW_CROP_FOR_ROATAION
int ExynosCamera1Parameters::getRotationProperty(void)
{
    return m_rotationProperty;
}
#endif

status_t ExynosCamera1Parameters::checkShotMode(const CameraParameters& params)
{
    int newShotMode = params.getInt("shot-mode");
    int curShotMode = -1;

#ifdef USE_LIMITATION_FOR_THIRD_PARTY
    if(getSamsungCamera() == false) {
        int vtMode = params.getInt("vtmode");

        newShotMode = checkPropertyForShotMode(newShotMode, vtMode);
    }
#endif
    if (newShotMode < 0) {
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s):newShotMode %d", "setParameters", newShotMode);

    curShotMode = getShotMode();

    if ((getRecordingHint() == true)
#ifdef SAMSUNG_TN_FEATURE
        && (newShotMode != SHOT_MODE_SEQUENCE) && (newShotMode != SHOT_MODE_AQUA)
#endif
#ifdef USE_FASTMOTION_CROP
        && (newShotMode != SHOT_MODE_FASTMOTION)
#endif
        ) {
        m_setShotMode(SHOT_MODE_NORMAL);
        m_params.set("shot-mode", SHOT_MODE_NORMAL);
    } else if (curShotMode != newShotMode) {
        m_setShotMode(newShotMode);
        m_params.set("shot-mode", newShotMode);

        updatePreviewFpsRange();
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setShotMode(int shotMode)
{
    enum aa_mode mode = AA_CONTROL_AUTO;
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
    bool changeSceneMode = true;

    switch (shotMode) {
    case SHOT_MODE_DRAMA:
#ifdef SAMSUNG_DOF
    case SHOT_MODE_LIGHT_TRACE:
#endif
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_DRAMA;
        break;
#ifdef SAMSUNG_MAGICSHOT
    case SHOT_MODE_MAGIC:
        if (getCameraId() == CAMERA_ID_BACK) {
            mode = AA_CONTROL_USE_SCENE_MODE;
            sceneMode = AA_SCENE_MODE_DRAMA;
        } else {
            mode = AA_CONTROL_AUTO;
            sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        }
        break;
#endif
    case SHOT_MODE_3D_PANORAMA:
    case SHOT_MODE_PANORAMA:
    case SHOT_MODE_FRONT_PANORAMA:
    case SHOT_MODE_INTERACTIVE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PANORAMA;
        break;
    case SHOT_MODE_NIGHT:
    case SHOT_MODE_NIGHT_SCENE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_LLS;
        break;
    case SHOT_MODE_ANIMATED_SCENE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_ANIMATED;
        break;
    case SHOT_MODE_SPORTS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SPORTS;
        break;
    case SHOT_MODE_GOLF:
#ifdef SAMSUNG_TN_FEATURE
    case SHOT_MODE_SEQUENCE:
#endif
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_GOLF;
        break;
    case SHOT_MODE_NORMAL:
    case SHOT_MODE_AUTO:
    case SHOT_MODE_BEAUTY_FACE:
    case SHOT_MODE_BEST_PHOTO:
    case SHOT_MODE_BEST_FACE:
    case SHOT_MODE_ERASER:
    case SHOT_MODE_RICH_TONE:
    case SHOT_MODE_STORY:
    case SHOT_MODE_SELFIE_ALARM:
#ifdef SAMSUNG_DOF
    case SHOT_MODE_3DTOUR:
    case SHOT_MODE_OUTFOCUS:
#endif
    case SHOT_MODE_FASTMOTION:
    case SHOT_MODE_PRO_MODE:
        mode = AA_CONTROL_AUTO;
        sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        break;
    case SHOT_MODE_DUAL:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_DUAL;
        break;
    case SHOT_MODE_AQUA:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_AQUA;
        break;
    case SHOT_MODE_AUTO_PORTRAIT:
    case SHOT_MODE_PET:
#ifdef USE_LIMITATION_FOR_THIRD_PARTY
    case THIRD_PARTY_BLACKBOX_MODE:
    case THIRD_PARTY_VTCALL_MODE:
#endif
    default:
        changeSceneMode = false;
        break;
    }

#ifdef LLS_CAPTURE
    if(m_flagLLSOn) {
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_LLS;
    }
#ifdef SET_LLS_PREVIEW_SETFILE
    if (shotMode == SHOT_MODE_NIGHT) {
        setLLSPreviewOn(true);
    } else {
        setLLSPreviewOn(false);
    }
#endif
#endif

    m_cameraInfo.shotMode = shotMode;
    if (changeSceneMode == true)
        setMetaCtlSceneMode(&m_metadata, mode, sceneMode);
}

int ExynosCamera1Parameters::getShotMode(void)
{
    return m_cameraInfo.shotMode;
}

status_t ExynosCamera1Parameters::checkVtMode(const CameraParameters& params)
{
    int newVTMode = params.getInt("vtmode");
    int curVTMode = -1;

    CLOGD("DEBUG(%s):newVTMode %d", "setParameters", newVTMode);

    /*
     * VT mode
     *   1: 3G vtmode (176x144, Fixed 7fps)
     *   2: LTE or WIFI vtmode (640x480, Fixed 15fps)
     *   3: Reserved : Smart Stay
     *   4: CHINA vtmode (1280x720, Fixed 30fps)
     */
    if (newVTMode == 3 || (newVTMode < 0 || newVTMode > 4)) {
        newVTMode = 0;
    }

    curVTMode = getVtMode();

    if (curVTMode != newVTMode) {
        m_setVtMode(newVTMode);
        m_params.set("vtmode", newVTMode);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setVtMode(int vtMode)
{
    m_cameraInfo.vtMode = vtMode;

    setMetaVtMode(&m_metadata, (enum camera_vt_mode)vtMode);
}

int ExynosCamera1Parameters::getVtMode(void)
{
    return m_cameraInfo.vtMode;
}

status_t ExynosCamera1Parameters::checkVRMode(const CameraParameters& params)
{
    int newVRMode =  params.getInt("vrmode");
    int curVRMode = getVRMode();

    CLOGD("DEBUG(%s):newVRMode %d", "setParameters", newVRMode);

    if (curVRMode != newVRMode) {
        m_setVRMode(newVRMode);
        m_params.set("vrmode", newVRMode);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setVRMode(int vrMode)
{
    m_cameraInfo.vrMode = vrMode;
}

int ExynosCamera1Parameters::getVRMode(void)
{
    return m_cameraInfo.vrMode;
}

status_t ExynosCamera1Parameters::checkGamma(const CameraParameters& params)
{
    bool newGamma = false;
    bool curGamma = false;
    const char *strNewGamma = params.get("video_recording_gamma");

    if (strNewGamma == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewGamma %s", "setParameters", strNewGamma);

    if (!strcmp(strNewGamma, "off")) {
        newGamma = false;
    } else if (!strcmp(strNewGamma, "on")) {
        newGamma = true;
    } else {
        CLOGE("ERR(%s):unmatched gamma(%s)", __FUNCTION__, strNewGamma);
        return BAD_VALUE;
    }

    curGamma = getGamma();

    if (curGamma != newGamma) {
        m_setGamma(newGamma);
        m_params.set("video_recording_gamma", strNewGamma);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setGamma(bool gamma)
{
    m_cameraInfo.gamma = gamma;
}

bool ExynosCamera1Parameters::getGamma(void)
{
    return m_cameraInfo.gamma;
}

status_t ExynosCamera1Parameters::checkSlowAe(const CameraParameters& params)
{
    bool newSlowAe = false;
    bool curSlowAe = false;
    const char *strNewSlowAe = params.get("slow_ae");

    if (strNewSlowAe == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewSlowAe %s", "setParameters", strNewSlowAe);

    if (!strcmp(strNewSlowAe, "off"))
        newSlowAe = false;
    else if (!strcmp(strNewSlowAe, "on"))
        newSlowAe = true;
    else {
        CLOGE("ERR(%s):unmatched slow_ae(%s)", __FUNCTION__, strNewSlowAe);
        return BAD_VALUE;
    }

    curSlowAe = getSlowAe();

    if (curSlowAe != newSlowAe) {
        m_setSlowAe(newSlowAe);
        m_params.set("slow_ae", strNewSlowAe);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setSlowAe(bool slowAe)
{
    m_cameraInfo.slowAe = slowAe;
}

bool ExynosCamera1Parameters::getSlowAe(void)
{
    return m_cameraInfo.slowAe;
}

status_t ExynosCamera1Parameters::checkImageUniqueId(__unused const CameraParameters& params)
{
    const char *strCurImageUniqueId = m_params.get("imageuniqueid-value");
    const char *strNewImageUniqueId = NULL;

#if defined(SAMSUNG_COMPANION) || defined(SAMSUNG_EEPROM)
    if (m_romReadThreadDone != true) {
        ALOGD("DEBUG(%s): checkImageUniqueId : romReadThreadDone(%d)", "setParameters", m_romReadThreadDone);
        return NO_ERROR;
    }
#endif

    if(strCurImageUniqueId == NULL || strcmp(strCurImageUniqueId, "") == 0 || strcmp(strCurImageUniqueId, "0") == 0) {
        strNewImageUniqueId = getImageUniqueId();

        if (strNewImageUniqueId != NULL && strcmp(strNewImageUniqueId, "") != 0) {
            ALOGD("DEBUG(%s):newImageUniqueId %s ", "setParameters", strNewImageUniqueId );
            m_params.set("imageuniqueid-value", strNewImageUniqueId);
        }
    }
    return NO_ERROR;
}

status_t ExynosCamera1Parameters::m_setImageUniqueId(const char *uniqueId)
{
    int uniqueIdLength;

    if (uniqueId == NULL) {
        return BAD_VALUE;
    }

    memset(m_cameraInfo.imageUniqueId, 0, sizeof(m_cameraInfo.imageUniqueId));

    uniqueIdLength = strlen(uniqueId);
    memcpy(m_cameraInfo.imageUniqueId, uniqueId, uniqueIdLength);

    return NO_ERROR;
}

const char *ExynosCamera1Parameters::getImageUniqueId(void)
{
#if defined(SAMSUNG_TN_FEATURE) && defined(SENSOR_FW_GET_FROM_FILE)
    char *sensorfw = NULL;
    char *uniqueid = NULL;
    char *savePtr;
#ifdef FORCE_CAL_RELOAD
    char checkcal[50];

    memset(checkcal, 0, sizeof(checkcal));
#endif
    sensorfw = (char *)getSensorFWFromFile(m_staticInfo, m_cameraId);
#ifdef FORCE_CAL_RELOAD
    sprintf(checkcal, "%s", sensorfw);
    m_calValid = m_checkCalibrationDataValid(checkcal);
#endif

    if (getCameraId() == CAMERA_ID_BACK) {
        uniqueid = sensorfw;
    } else {
#ifdef SAMSUNG_EEPROM_FRONT
        if (SAMSUNG_EEPROM_FRONT) {
            uniqueid = sensorfw;
        } else
#endif
        {
            uniqueid = strtok_r(sensorfw, " ", &savePtr);
        }
    }
    setImageUniqueId(uniqueid);

    return (const char *)m_cameraInfo.imageUniqueId;
#else
    return m_cameraInfo.imageUniqueId;
#endif
}

#ifdef SAMSUNG_TN_FEATURE
void ExynosCamera1Parameters::setImageUniqueId(char *uniqueId)
{
    memcpy(m_cameraInfo.imageUniqueId, uniqueId, sizeof(m_cameraInfo.imageUniqueId));
}
#endif

#ifdef BURST_CAPTURE
status_t ExynosCamera1Parameters::checkSeriesShotFilePath(const CameraParameters& params)
{
    const char *seriesShotFilePath = params.get("capture-burst-filepath");

    if (seriesShotFilePath != NULL) {
        snprintf(m_seriesShotFilePath, sizeof(m_seriesShotFilePath), "%s", seriesShotFilePath);
        CLOGD("DEBUG(%s): seriesShotFilePath %s", "setParameters", seriesShotFilePath);
        m_params.set("capture-burst-filepath", seriesShotFilePath);
    } else {
        CLOGD("DEBUG(%s): seriesShotFilePath NULL", "setParameters");
        memset(m_seriesShotFilePath, 0, 100);
    }

    return NO_ERROR;
}

#ifdef VARIABLE_BURST_FPS
void ExynosCamera1Parameters::setBurstShotDuration(int fps)
{
    if (fps == BURSTSHOT_MIN_FPS)
        m_burstShotDuration = MIN_BURST_DURATION;
    else
        m_burstShotDuration = NORMAL_BURST_DURATION;
}

int ExynosCamera1Parameters::getBurstShotDuration(void)
{
    return m_burstShotDuration;
}
#endif /* VARIABLE_BURST_FPS */
#endif /* BURST_CAPTURE */

status_t ExynosCamera1Parameters::checkSeriesShotMode(const CameraParameters& params)
{
#ifndef SAMSUNG_TN_FEATURE
    int burstCount = params.getInt("burst-capture");
    int bestCount = params.getInt("best-capture");

    CLOGD("DEBUG(%s): burstCount(%d), bestCount(%d)", "setParameters", burstCount, bestCount);

    if (burstCount < 0 || bestCount < 0) {
        CLOGE("ERR(%s[%d]): Invalid burst-capture count(%d), best-capture count(%d)", __FUNCTION__, __LINE__, burstCount, bestCount);
        return BAD_VALUE;
    }

    /* TODO: select shot count */
    if (bestCount > burstCount) {
        m_setSeriesShotCount(bestCount);
        m_params.set("burst-capture", 0);
        m_params.set("best-capture", bestCount);
    } else {
        m_setSeriesShotCount(burstCount);
        m_params.set("burst-capture", burstCount);
        m_params.set("best-capture", 0);
    }
#endif
    return NO_ERROR;
}

#ifdef SAMSUNG_LLV
status_t ExynosCamera1Parameters::checkLLV(const CameraParameters& params)
{
    int mode = params.getInt("llv_mode");
    int prev_mode = m_params.getInt("llv_mode");

    if (mode !=  prev_mode) {
        CLOGW("[PARM_DBG] new LLV mode : %d ", mode);
        setLLV(mode);
        m_params.set("llv_mode", mode);
    } else {
        CLOGD("%s: No value change in LLV mode : %d", __FUNCTION__, mode);
    }

    return NO_ERROR;
}
#endif

#ifdef SAMSUNG_DOF
status_t ExynosCamera1Parameters::checkMoveLens(const CameraParameters& params)
{
    const char *newStrMoveLens = params.get("focus-bracketing-values");
    const char *curStrMoveLens = m_params.get("focus-bracketing-values");

    if(newStrMoveLens != NULL) {
        CLOGD("[DOF][%s][%d] string : %s", "checkMoveLens", __LINE__, newStrMoveLens);
    }
    else {
        memset(m_cameraInfo.lensPosTbl, 0, sizeof(m_cameraInfo.lensPosTbl));
        return NO_ERROR;
    }

    if(SHOT_MODE_OUTFOCUS != getShotMode())
        return NO_ERROR;

    CLOGD("[DOF][PARM_DBG] new Lens position : %s ", newStrMoveLens);
    char *start;
    char *end;
    char delim = ',';
    int N = 6; // max count is 5 (+ number of value)

    if (newStrMoveLens != NULL) {
        start = (char *)newStrMoveLens;
        for(int i = 1; i < N; i++) {
            m_cameraInfo.lensPosTbl[i] = (int) strtol(start, &end, 10);
            CLOGD("[DOF][%s][%d] lensPosTbl[%d] : %d",
                    "checkMoveLens", __LINE__, i, m_cameraInfo.lensPosTbl[i]);
            if(*end != delim) {
                m_cameraInfo.lensPosTbl[0] = i; // Total Count
                CLOGD("[DOF][%s][%d] lensPosTbl[0] : %d", "checkMoveLens", __LINE__, i);
                break;
            }
            start = end+1;
        }
    }

    m_params.set("focus-bracketing-values", newStrMoveLens);

    return NO_ERROR;
}

int ExynosCamera1Parameters::getMoveLensTotal(void)
{
    return m_cameraInfo.lensPosTbl[0];
}

void ExynosCamera1Parameters::setMoveLensTotal(int count)
{
    m_cameraInfo.lensPosTbl[0] = count;
}

void ExynosCamera1Parameters::setMoveLensCount(int count)
{
    m_curLensCount = count;
    CLOGD("[DOF][%s][%d] m_curLensCount : %d",
                "setMoveLensCount", __LINE__, m_curLensCount);
}

void ExynosCamera1Parameters::m_setLensPosition(int step)
{
    CLOGD("[DOF][%s][%d] step : %d",
                "m_setLensPosition", __LINE__, step);
    setMetaCtlLensPos(&m_metadata, m_cameraInfo.lensPosTbl[step]);
    m_curLensStep = m_cameraInfo.lensPosTbl[step];
}
#endif

status_t ExynosCamera1Parameters::checkFactoryMode(const CameraParameters& params)
{
    int factorymode = 0;
    factorymode = params.getInt("factorytest");

    m_isFactoryMode = (factorymode == 1) ? true : false;

    if (m_isFactoryMode == true) {
        CLOGD("DEBUG(%s): factory mode(%d)", "setParameters", m_isFactoryMode);
#ifdef FRONT_ONLY_CAMERA_USE_FACTORY_MIRROR_FLIP
        m_cameraInfo.flipHorizontal = 1;
#endif
    }

    return NO_ERROR;
}

#ifdef BURST_CAPTURE
int ExynosCamera1Parameters::getSeriesShotSaveLocation(void)
{
    int seriesShotSaveLocation = m_seriesShotSaveLocation;
    int shotMode = getShotMode();

    /* GED's series shot work as callback */
#ifdef CAMERA_GED_FEATURE
    seriesShotSaveLocation = BURST_SAVE_CALLBACK;
#else
    if (shotMode == SHOT_MODE_BEST_PHOTO) {
        seriesShotSaveLocation = BURST_SAVE_CALLBACK;
#ifdef ONE_SECOND_BURST_CAPTURE
    } else if (m_cameraInfo.seriesShotMode == SERIES_SHOT_MODE_ONE_SECOND_BURST) {
        seriesShotSaveLocation = BURST_SAVE_CALLBACK;
#endif
    } else {
        if (m_seriesShotSaveLocation == 0)
            seriesShotSaveLocation = BURST_SAVE_PHONE;
        else
            seriesShotSaveLocation = BURST_SAVE_SDCARD;
    }
#endif

    return seriesShotSaveLocation;
}

void ExynosCamera1Parameters::setSeriesShotSaveLocation(int ssaveLocation)
{
    m_seriesShotSaveLocation = ssaveLocation;
}

char *ExynosCamera1Parameters::getSeriesShotFilePath(void)
{
    return m_seriesShotFilePath;
}
#endif

int ExynosCamera1Parameters::getSeriesShotDuration(void)
{
    switch (m_cameraInfo.seriesShotMode) {
    case SERIES_SHOT_MODE_BURST:
#ifdef VARIABLE_BURST_FPS
        return getBurstShotDuration();
#else
        return NORMAL_BURST_DURATION;
#endif
    case SERIES_SHOT_MODE_BEST_FACE:
        return BEST_FACE_DURATION;
    case SERIES_SHOT_MODE_BEST_PHOTO:
        return BEST_PHOTO_DURATION;
    case SERIES_SHOT_MODE_ERASER:
        return ERASER_DURATION;
#ifdef SAMSUNG_MAGICSHOT
    case SERIES_SHOT_MODE_MAGIC:
        return MAGIC_SHOT_DURATION;
#endif
    case SERIES_SHOT_MODE_SELFIE_ALARM:
        return SELFIE_ALARM_DURATION;
#ifdef ONE_SECOND_BURST_CAPTURE
    case SERIES_SHOT_MODE_ONE_SECOND_BURST:
        return ONE_SECOND_BURST_CAPTURE_DURATION;
#endif
    default:
        return 0;
    }
    return 0;
}

int ExynosCamera1Parameters::getSeriesShotMode(void)
{
    return m_cameraInfo.seriesShotMode;
}

void ExynosCamera1Parameters::setSeriesShotMode(int sshotMode, int count)
{
    int sshotCount = 0;
    int shotMode = getShotMode();
    if (sshotMode == SERIES_SHOT_MODE_BURST) {
        if (shotMode == SHOT_MODE_BEST_PHOTO) {
            sshotMode = SERIES_SHOT_MODE_BEST_PHOTO;
            sshotCount = 8;
        } else if (shotMode == SHOT_MODE_BEST_FACE) {
            sshotMode = SERIES_SHOT_MODE_BEST_FACE;
            sshotCount = 5;
        } else if (shotMode == SHOT_MODE_ERASER) {
            sshotMode = SERIES_SHOT_MODE_ERASER;
            sshotCount = 5;
        }
#ifdef SAMSUNG_MAGICSHOT
        else if (shotMode == SHOT_MODE_MAGIC) {
            sshotMode = SERIES_SHOT_MODE_MAGIC;
            if ( m_cameraId == CAMERA_ID_FRONT)
                sshotCount = 16;
            else
                sshotCount = 32;
        }
#endif
        else if (shotMode == SHOT_MODE_SELFIE_ALARM) {
            sshotMode = SERIES_SHOT_MODE_SELFIE_ALARM;
            sshotCount = 3;
        } else {
            sshotMode = SERIES_SHOT_MODE_BURST;
            sshotCount = MAX_SERIES_SHOT_COUNT;
        }
    } else if (sshotMode == SERIES_SHOT_MODE_LLS ||
            sshotMode == SERIES_SHOT_MODE_SIS) {
            if(count > 0) {
                sshotCount = count;
            } else {
                sshotCount = 5;
            }
#ifdef ONE_SECOND_BURST_CAPTURE
    } else if (sshotMode == SERIES_SHOT_MODE_ONE_SECOND_BURST) {
        sshotCount = ONE_SECOND_BURST_CAPTURE_COUNT;
#endif
    }

    CLOGD("DEBUG(%s[%d]: set shotmode(%d), shotCount(%d)", __FUNCTION__, __LINE__, sshotMode, sshotCount);

    m_cameraInfo.seriesShotMode = sshotMode;
    m_setSeriesShotCount(sshotCount);
}

void ExynosCamera1Parameters::m_setSeriesShotCount(int seriesShotCount)
{
    m_cameraInfo.seriesShotCount = seriesShotCount;
}

int ExynosCamera1Parameters::getSeriesShotCount(void)
{
    return m_cameraInfo.seriesShotCount;
}

void ExynosCamera1Parameters::setSamsungCamera(bool value)
{
    String8 tempStr;
    ExynosCameraActivityAutofocus *autoFocusMgr = m_activityControl->getAutoFocusMgr();

    m_cameraInfo.samsungCamera = value;
    autoFocusMgr->setSamsungCamera(value);

    /* zoom */
    if (getZoomSupported() == true) {
        int maxZoom = getMaxZoomLevel();
        ALOGI("INFO(%s):change MaxZoomLevel and ZoomRatio List.(%d)", __FUNCTION__, maxZoom);

        if (0 < maxZoom) {
            m_params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");

            if (getSmoothZoomSupported() == true)
                m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "true");
            else
                m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");

            m_params.set(CameraParameters::KEY_MAX_ZOOM, maxZoom - 1);
            m_params.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);

            int max_zoom_ratio = (int)getMaxZoomRatio();
            tempStr.setTo("");
            if (getZoomRatioList(tempStr, maxZoom, max_zoom_ratio, m_staticInfo->zoomRatioList) == NO_ERROR)
                m_params.set(CameraParameters::KEY_ZOOM_RATIOS, tempStr.c_str());
            else
                m_params.set(CameraParameters::KEY_ZOOM_RATIOS, "100");

            m_params.set("constant-growth-rate-zoom-supported", "true");

            ALOGV("INFO(%s):zoomRatioList=%s", "setDefaultParameter", tempStr.c_str());
        } else {
            m_params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
            m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
            m_params.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
            m_params.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
        }
    } else {
        m_params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
        m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
        m_params.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
        m_params.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
    }

    /* Picture Format */
    tempStr.setTo("");
    tempStr = String8::format("%s,%s", CameraParameters::PIXEL_FORMAT_JPEG, CameraParameters::PIXEL_FORMAT_YUV420SP_NV21);
    m_params.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, tempStr);
    m_params.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
}

bool ExynosCamera1Parameters::getSamsungCamera(void)
{
    return m_cameraInfo.samsungCamera;
}

void ExynosCamera1Parameters::m_initMetadata(void)
{
    memset(&m_metadata, 0x00, sizeof(struct camera2_shot_ext));
    struct camera2_shot *shot = &m_metadata.shot;

    // 1. ctl
    // request
    shot->ctl.request.id = 0;
    shot->ctl.request.metadataMode = METADATA_MODE_FULL;
    shot->ctl.request.frameCount = 0;

    // lens
    shot->ctl.lens.focusDistance = -1.0f;
    if (getHalVersion() == IS_HAL_VER_3_2) {
        shot->ctl.lens.aperture = m_staticInfo->aperture;
        shot->ctl.lens.focalLength = m_staticInfo->focalLength;
    } else {
        shot->ctl.lens.aperture = (float)m_staticInfo->apertureNum / (float)m_staticInfo->apertureDen;
        shot->ctl.lens.focalLength = (float)m_staticInfo->focalLengthNum / (float)m_staticInfo->focalLengthDen;
    }
    shot->ctl.lens.filterDensity = 0.0f;
    shot->ctl.lens.opticalStabilizationMode = ::OPTICAL_STABILIZATION_MODE_OFF;

    int minFps = (m_staticInfo->minFps == 0) ? 0 : (m_staticInfo->maxFps / 2);
    int maxFps = (m_staticInfo->maxFps == 0) ? 0 : m_staticInfo->maxFps;

    /* The min fps can not be '0'. Therefore it is set up default value '15'. */
    if (minFps == 0) {
        CLOGW("WRN(%s): Invalid min fps value(%d)", __FUNCTION__, minFps);
        minFps = 15;
    }

    /*  The initial fps can not be '0' and bigger than '30'. Therefore it is set up default value '30'. */
    if (maxFps == 0 || 30 < maxFps) {
        CLOGW("WRN(%s): Invalid max fps value(%d)", __FUNCTION__, maxFps);
        maxFps = 30;
    }

    /* sensor */
    shot->ctl.sensor.exposureTime = 0;
    shot->ctl.sensor.frameDuration = (1000 * 1000 * 1000) / maxFps;
    shot->ctl.sensor.sensitivity = 0;

    /* flash */
    shot->ctl.flash.flashMode = ::CAM2_FLASH_MODE_OFF;
    shot->ctl.flash.firingPower = 0;
    shot->ctl.flash.firingTime = 0;

    /* hotpixel */
    shot->ctl.hotpixel.mode = (enum processing_mode)0;

    /* demosaic */
    shot->ctl.demosaic.mode = (enum demosaic_processing_mode)0;

    /* noise */
#ifdef USE_NEW_NOISE_REDUCTION_ALGORITHM
    shot->ctl.noise.mode = ::PROCESSING_MODE_FAST;
#else
    shot->ctl.noise.mode = ::PROCESSING_MODE_OFF;
#endif
    shot->ctl.noise.strength = 5;

    /* shading */
    shot->ctl.shading.mode = (enum processing_mode)0;

    /* color */
    shot->ctl.color.mode = ::COLORCORRECTION_MODE_FAST;
    static const float colorTransform_hal3[9] = {
        1.0f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };
    static const struct rational colorTransform[9] = {
        {1, 0}, {0, 0}, {0, 0},
        {0, 0}, {1, 0}, {0, 0},
        {0, 0}, {0, 0}, {1, 0}
    };

    if (getHalVersion() == IS_HAL_VER_3_2) {
        for (size_t i = 0; i < sizeof(colorTransform)/sizeof(colorTransform[0]); i++) {
            shot->ctl.color.transform[i].num = colorTransform_hal3[i] * COMMON_DENOMINATOR;
            shot->ctl.color.transform[i].den = COMMON_DENOMINATOR;
        }
    } else {
        memcpy(shot->ctl.color.transform, colorTransform, sizeof(shot->ctl.color.transform));
    }

    /* tonemap */
    shot->ctl.tonemap.mode = ::TONEMAP_MODE_FAST;
    static const float tonemapCurve[4] = {
        0.f, 0.f,
        1.f, 1.f
    };

    int tonemapCurveSize = sizeof(tonemapCurve);
    int sizeOfCurve = sizeof(shot->ctl.tonemap.curveRed) / sizeof(shot->ctl.tonemap.curveRed[0]);

    for (int i = 0; i < sizeOfCurve; i ++) {
        memcpy(&(shot->ctl.tonemap.curveRed[i]),   tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveGreen[i]), tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveBlue[i]),  tonemapCurve, tonemapCurveSize);
    }

    /* edge */
#ifdef USE_NEW_NOISE_REDUCTION_ALGORITHM
    shot->ctl.edge.mode = ::PROCESSING_MODE_FAST;
#else
    shot->ctl.edge.mode = ::PROCESSING_MODE_OFF;
#endif
    shot->ctl.edge.strength = 5;

    /* scaler
     * Max Picture Size == Max Sensor Size - Sensor Margin
     */
    if (m_setParamCropRegion(0,
                             m_staticInfo->maxPictureW, m_staticInfo->maxPictureH,
                             m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH
                             ) != NO_ERROR) {
        ALOGE("ERR(%s):m_setZoom() fail", __FUNCTION__);
    }

    /* jpeg */
    shot->ctl.jpeg.quality = 96;
    shot->ctl.jpeg.thumbnailSize[0] = m_staticInfo->maxThumbnailW;
    shot->ctl.jpeg.thumbnailSize[1] = m_staticInfo->maxThumbnailH;
    shot->ctl.jpeg.thumbnailQuality = 100;
    shot->ctl.jpeg.gpsCoordinates[0] = 0;
    shot->ctl.jpeg.gpsCoordinates[1] = 0;
    shot->ctl.jpeg.gpsCoordinates[2] = 0;
    memset(&shot->ctl.jpeg.gpsProcessingMethod, 0x0,
        sizeof(shot->ctl.jpeg.gpsProcessingMethod));
    shot->ctl.jpeg.gpsTimestamp = 0L;
    shot->ctl.jpeg.orientation = 0L;

    /* stats */
    shot->ctl.stats.faceDetectMode = ::FACEDETECT_MODE_OFF;
    shot->ctl.stats.histogramMode = ::STATS_MODE_OFF;
    shot->ctl.stats.sharpnessMapMode = ::STATS_MODE_OFF;

    /* aa */
    shot->ctl.aa.captureIntent = ::AA_CAPTURE_INTENT_CUSTOM;
    shot->ctl.aa.mode = ::AA_CONTROL_AUTO;
    shot->ctl.aa.effectMode = ::AA_EFFECT_OFF;
    shot->ctl.aa.sceneMode = ::AA_SCENE_MODE_FACE_PRIORITY;
    shot->ctl.aa.videoStabilizationMode =
        (enum aa_videostabilization_mode) VIDEO_STABILIZATION_MODE_OFF ;

    /* default metering is center */
    shot->ctl.aa.aeMode = ::AA_AEMODE_CENTER;
    shot->ctl.aa.aeRegions[0] = 0;
    shot->ctl.aa.aeRegions[1] = 0;
    shot->ctl.aa.aeRegions[2] = 0;
    shot->ctl.aa.aeRegions[3] = 0;
    shot->ctl.aa.aeRegions[4] = 1000;
    shot->ctl.aa.aeLock = ::AA_AE_LOCK_OFF;
#if defined(USE_SUBDIVIDED_EV)
    shot->ctl.aa.aeExpCompensation = 0; /* 21 is middle */
#else
    shot->ctl.aa.aeExpCompensation = 5; /* 5 is middle */
#endif
    shot->ctl.aa.aeTargetFpsRange[0] = minFps;
    shot->ctl.aa.aeTargetFpsRange[1] = maxFps;

    shot->ctl.aa.aeAntibandingMode = ::AA_AE_ANTIBANDING_AUTO;
    shot->ctl.aa.vendor_aeflashMode = ::AA_FLASHMODE_OFF;
    shot->ctl.aa.awbMode = ::AA_AWBMODE_WB_AUTO;
    shot->ctl.aa.awbLock = ::AA_AWB_LOCK_OFF;
    shot->ctl.aa.afMode = ::AA_AFMODE_OFF;
    shot->ctl.aa.afRegions[0] = 0;
    shot->ctl.aa.afRegions[1] = 0;
    shot->ctl.aa.afRegions[2] = 0;
    shot->ctl.aa.afRegions[3] = 0;
    shot->ctl.aa.afRegions[4] = 1000;
    shot->ctl.aa.afTrigger = (enum aa_af_trigger) AA_AF_TRIGGER_IDLE;
    shot->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
    shot->ctl.aa.vendor_isoValue = 0;

    /* 2. dm */

    /* 3. utrl */
#ifdef SAMSUNG_COMPANION
    m_metadata.shot.uctl.companionUd.drc_mode = COMPANION_DRC_OFF;
    m_metadata.shot.uctl.companionUd.paf_mode = COMPANION_PAF_OFF;
    m_metadata.shot.uctl.companionUd.wdr_mode = COMPANION_WDR_OFF;
#endif
#ifdef USE_FW_ZOOMRATIO
    m_metadata.shot.uctl.zoomRatio = 1.00f;
#else
    m_metadata.shot.uctl.zoomRatio = 0;
#endif

    /* 4. udm */

    /* 5. magicNumber */
    shot->magicNumber = SHOT_MAGIC_NUMBER;

    setMetaSetfile(&m_metadata, 0x0);

    /* user request */
    m_metadata.drc_bypass = 1;
    m_metadata.dis_bypass = 1;
    m_metadata.dnr_bypass = 1;
    m_metadata.fd_bypass  = 1;
}

status_t ExynosCamera1Parameters::duplicateCtrlMetadata(void *buf)
{
    if (buf == NULL) {
        CLOGE("ERR: buf is NULL");
        return BAD_VALUE;
    }

    struct camera2_shot_ext *meta_shot_ext = (struct camera2_shot_ext *)buf;
    memcpy(&meta_shot_ext->shot.ctl, &m_metadata.shot.ctl, sizeof(struct camera2_ctl));
#ifdef SAMSUNG_COMPANION
    meta_shot_ext->shot.uctl.companionUd.wdr_mode = getRTHdr();
    meta_shot_ext->shot.uctl.companionUd.paf_mode = getPaf();
#endif

#ifdef SAMSUNG_DOF
    if(getShotMode() == SHOT_MODE_OUTFOCUS && m_curLensCount > 0) {
        meta_shot_ext->shot.uctl.lensUd.pos = m_metadata.shot.uctl.lensUd.pos;
        meta_shot_ext->shot.uctl.lensUd.posSize = m_metadata.shot.uctl.lensUd.posSize;
        meta_shot_ext->shot.uctl.lensUd.direction = m_metadata.shot.uctl.lensUd.direction;
        meta_shot_ext->shot.uctl.lensUd.slewRate = m_metadata.shot.uctl.lensUd.slewRate;
    }
#endif

#ifdef SAMSUNG_HRM
    if((getCameraId() == CAMERA_ID_BACK) && (m_metadata.shot.uctl.aaUd.hrmInfo.ir_data != 0)
                    && m_flagSensorHRM_Hint) {
        meta_shot_ext->shot.uctl.aaUd.hrmInfo = m_metadata.shot.uctl.aaUd.hrmInfo;
    }
#endif
#ifdef SAMSUNG_LIGHT_IR
    if((getCameraId() == CAMERA_ID_FRONT) && (m_metadata.shot.uctl.aaUd.illuminationInfo.visible_exptime != 0)
                    && m_flagSensorLight_IR_Hint) {
        meta_shot_ext->shot.uctl.aaUd.illuminationInfo = m_metadata.shot.uctl.aaUd.illuminationInfo;
    }
#endif
#ifdef SAMSUNG_GYRO
    if((getCameraId() == CAMERA_ID_BACK) && m_flagSensorGyroHint) {
        meta_shot_ext->shot.uctl.aaUd.gyroInfo = m_metadata.shot.uctl.aaUd.gyroInfo;
    }
#endif
#ifdef USE_FW_ZOOMRATIO
    if(getCameraId() == CAMERA_ID_BACK) {
        meta_shot_ext->shot.uctl.zoomRatio = m_metadata.shot.uctl.zoomRatio;
    }
#endif

    meta_shot_ext->shot.uctl.vtMode = m_metadata.shot.uctl.vtMode;

    return NO_ERROR;
}

#ifdef LLS_CAPTURE
int ExynosCamera1Parameters::getLLS(struct camera2_shot_ext *shot)
{
#ifdef OIS_CAPTURE
    m_llsValue = shot->shot.dm.stats.vendor_LowLightMode;
#endif

#if defined(RAWDUMP_CAPTURE) && !defined(STK_PICTURE) && !defined(STK_PREVIEW)
    return LLS_LEVEL_ZSL;
#endif
#if defined(LLS_VALUE_VERSION_5_0)
    int ret = shot->shot.dm.stats.vendor_LowLightMode;

    ALOGV("DEBUG(%s[%d]):m_flagLLSOn(%d) getFlashMode(%d), LowLightMode(%d)",
        __FUNCTION__, __LINE__, m_flagLLSOn, getFlashMode(), shot->shot.dm.stats.vendor_LowLightMode);

    return ret;
#elif defined(LLS_VALUE_VERSION_3_0)

    int ret = shot->shot.dm.stats.vendor_LowLightMode;

    if (m_cameraId == CAMERA_ID_FRONT) {
        return shot->shot.dm.stats.vendor_LowLightMode;
    }

    if(m_flagLLSOn) {
        switch (getFlashMode()) {
        case FLASH_MODE_AUTO:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_OFF:
            if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_HIGH)
                ret = LLS_LEVEL_LOW;
            else
                ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_ON:
        case FLASH_MODE_TORCH:
        case FLASH_MODE_RED_EYE:
            ret = LLS_LEVEL_HIGH;
            break;
        default:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        }
    }else {
        switch (getFlashMode()) {
        case FLASH_MODE_AUTO:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_OFF:
            if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_HIGH)
                ret = LLS_LEVEL_ZSL_LIKE;
            else
                ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_ON:
        case FLASH_MODE_TORCH:
        case FLASH_MODE_RED_EYE:
            ret = LLS_LEVEL_HIGH;
            break;
        default:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        }
    }

    ALOGV("DEBUG(%s[%d]):m_flagLLSOn(%d) getFlashMode(%d), LowLightMode(%d)",
        __FUNCTION__, __LINE__, m_flagLLSOn, getFlashMode(), shot->shot.dm.stats.vendor_LowLightMode);

    return ret;

#elif defined(LLS_VALUE_VERSION_2_0)

    int ret = shot->shot.dm.stats.vendor_LowLightMode;

    if (m_cameraId == CAMERA_ID_FRONT) {
        return shot->shot.dm.stats.vendor_LowLightMode;
    }

    if(m_flagLLSOn) {
        switch (getFlashMode()) {
        case FLASH_MODE_AUTO:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_OFF:
            if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_FLASH)
                ret = LLS_LEVEL_LOW;
            else
                ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_ON:
        case FLASH_MODE_TORCH:
        case FLASH_MODE_RED_EYE:
            ret = LLS_LEVEL_HIGH;
            break;
        default:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        }
    }else {
        switch (getFlashMode()) {
        case FLASH_MODE_AUTO:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_OFF:
            if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_HIGH)
                ret = LLS_LEVEL_ZSL_LIKE;
            else
                ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        case FLASH_MODE_ON:
        case FLASH_MODE_TORCH:
        case FLASH_MODE_RED_EYE:
            ret = LLS_LEVEL_HIGH;
            break;
        default:
            ret = shot->shot.dm.stats.vendor_LowLightMode;
            break;
        }
    }

    ALOGV("DEBUG(%s[%d]):m_flagLLSOn(%d) getFlashMode(%d), LowLightMode(%d)",
        __FUNCTION__, __LINE__, m_flagLLSOn, getFlashMode(), shot->shot.dm.stats.vendor_LowLightMode);

    return ret;

#else /* not defined FLASHED_LLS_CAPTURE */

    int ret = LLS_LEVEL_ZSL;

    switch (getFlashMode()) {
    case FLASH_MODE_OFF:
        if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_LOW
            || shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_HIGH)
            ret = LLS_LEVEL_LOW;
        else if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_SIS)
            ret = LLS_LEVEL_SIS;
        break;
    case FLASH_MODE_AUTO:
        if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_LOW)
            ret = LLS_LEVEL_LOW;
        else if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_HIGH)
            ret = LLS_LEVEL_ZSL;
        else if (shot->shot.dm.stats.vendor_LowLightMode == STATE_LLS_LEVEL_SIS)
            ret = LLS_LEVEL_SIS;
        break;
    case FLASH_MODE_ON:
    case FLASH_MODE_TORCH:
    case FLASH_MODE_RED_EYE:
    default:
    ret = LLS_LEVEL_ZSL;
        break;
    }

#if defined(LLS_NOT_USE_SIS_FRONT)
    if ((getCameraId() == CAMERA_ID_FRONT) && (ret == LLS_LEVEL_SIS)) {
        ret = LLS_LEVEL_ZSL;
    }
#endif

    ALOGV("DEBUG(%s[%d]):m_flagLLSOn(%d) getFlashMode(%d), LowLightMode(%d), shotmode(%d)",
        __FUNCTION__, __LINE__, m_flagLLSOn, getFlashMode(), shot->shot.dm.stats.vendor_LowLightMode,getShotMode());

    return ret;
#endif
}


void ExynosCamera1Parameters::setLLSOn(uint32_t enable)
{
    m_flagLLSOn = enable;
}

bool ExynosCamera1Parameters::getLLSOn(void)
{
    return m_flagLLSOn;
}

void ExynosCamera1Parameters::m_setLLSShotMode()
{
    enum aa_mode mode = AA_CONTROL_USE_SCENE_MODE;
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_LLS;

    setMetaCtlSceneMode(&m_metadata, mode, sceneMode);
}

#ifdef SET_LLS_PREVIEW_SETFILE
void ExynosCamera1Parameters::setLLSPreviewOn(bool enable)
{
    m_LLSPreviewOn = enable;
}

int ExynosCamera1Parameters::getLLSPreviewOn()
{
    return m_LLSPreviewOn;
}
#endif

#ifdef SET_LLS_CAPTURE_SETFILE
void ExynosCamera1Parameters::setLLSCaptureOn(bool enable)
{
    m_LLSCaptureOn = enable;
}

int ExynosCamera1Parameters::getLLSCaptureOn()
{
    return m_LLSCaptureOn;
}
#endif
#endif

#ifdef LLS_REPROCESSING
void ExynosCamera1Parameters::setLLSCaptureCount(int count)
{
    m_LLSCaptureCount = count;
}

int ExynosCamera1Parameters::getLLSCaptureCount()
{
    return m_LLSCaptureCount;
}
#endif

#ifdef SR_CAPTURE
void ExynosCamera1Parameters::setSROn(uint32_t enable)
{
    m_flagSRSOn = (enable > 0) ? true : false;
}

bool ExynosCamera1Parameters::getSROn(void)
{
    return m_flagSRSOn;
}
#endif

#ifdef OIS_CAPTURE
void ExynosCamera1Parameters::checkOISCaptureMode(uint32_t lls)
{
    ExynosCameraActivityFlash *m_flashMgr = m_activityControl->getFlashMgr();

    if((getRecordingHint() == true) || (getShotMode() > SHOT_MODE_AUTO && getShotMode() != SHOT_MODE_NIGHT)) {
        if((getRecordingHint() == true)
            || (getShotMode() != SHOT_MODE_BEAUTY_FACE
                && getShotMode() != SHOT_MODE_OUTFOCUS
                && getShotMode() != SHOT_MODE_SELFIE_ALARM
                && getShotMode() != SHOT_MODE_PRO_MODE)) {
            setOISCaptureModeOn(false);
            ALOGD("DEBUG(%s[%d]: zsl-like capture mode off for special shot", __FUNCTION__, __LINE__);
            return;
        }
    }

#ifdef SR_CAPTURE
    if(getSROn() == true) {
        ALOGD("DEBUG(%s[%d]: zsl-like multi capture mode off (SR mode)", __FUNCTION__, __LINE__);
        setOISCaptureModeOn(false);
            return;
        }
#endif

    if(getSeriesShotMode() == SERIES_SHOT_MODE_NONE && m_flashMgr->getNeedCaptureFlash()) {
        setOISCaptureModeOn(false);
        ALOGD("DEBUG(%s[%d]: zsl-like capture mode off for flash single capture", __FUNCTION__, __LINE__);
        return;
    }

#ifdef FLASHED_LLS_CAPTURE
    if(m_flashMgr->getNeedCaptureFlash() == true && getSeriesShotMode()== SERIES_SHOT_MODE_LLS) {
        ALOGD("DEBUG(%s[%d]: zsl-like flash lls capture mode on, lls(%d)", __FUNCTION__, __LINE__, m_llsValue);
        setOISCaptureModeOn(true);
        return;
    }
#endif

    if(getSeriesShotCount() > 0 && m_llsValue > 0) {
        ALOGD("DEBUG(%s[%d]: zsl-like multi capture mode on, lls(%d)", __FUNCTION__, __LINE__, m_llsValue);
        setOISCaptureModeOn(true);
        return;
    }

    ALOGD("DEBUG(%s[%d]: Low Light value(%d), m_flagLLSOn(%d),getFlashMode(%d)", __FUNCTION__, __LINE__, lls, m_flagLLSOn, getFlashMode());
    if(m_flagLLSOn) {
        switch (getFlashMode()) {
        case FLASH_MODE_AUTO:
        case FLASH_MODE_OFF:
            if (lls == LLS_LEVEL_ZSL_LIKE || lls == LLS_LEVEL_LOW || lls == LLS_LEVEL_FLASH) {
                setOISCaptureModeOn(true);
            }
            break;
        case FLASH_MODE_ON:
        case FLASH_MODE_TORCH:
        case FLASH_MODE_RED_EYE:
            if (lls == LLS_LEVEL_FLASH) {
                setOISCaptureModeOn(true);
            }
            break;
        default:
            setOISCaptureModeOn(false);
            break;
        }
    } else {
        switch (getFlashMode()) {
        case FLASH_MODE_AUTO:
            if (lls == LLS_LEVEL_ZSL_LIKE && m_flashMgr->getNeedFlash() == false) {
                setOISCaptureModeOn(true);
            }
            break;
        case FLASH_MODE_OFF:
            if (lls == LLS_LEVEL_ZSL_LIKE) {
                setOISCaptureModeOn(true);
            }
            break;
        default:
            setOISCaptureModeOn(false);
            break;
        }
    }
}

void ExynosCamera1Parameters::setOISCaptureModeOn(bool enable)
{
    m_flagOISCaptureOn = enable;
}

bool ExynosCamera1Parameters::getOISCaptureModeOn(void)
{
    return m_flagOISCaptureOn;
}
#endif

void ExynosCamera1Parameters::m_getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange)
{
    uint32_t currentSetfile = 0;
    uint32_t stateReg = 0;
    int flagYUVRange = YUV_FULL_RANGE;

    unsigned int minFps = 0;
    unsigned int maxFps = 0;
    getPreviewFpsRange(&minFps, &maxFps);

#ifdef SAMSUNG_COMPANION
    if (getRTHdr() == COMPANION_WDR_ON) {
        stateReg |= STATE_REG_RTHDR_ON;
    } else if (getRTHdr() == COMPANION_WDR_AUTO) {
        stateReg |= STATE_REG_RTHDR_AUTO;
    }
#endif

    if (m_isUHDRecordingMode() == true)
        stateReg |= STATE_REG_UHD_RECORDING;

    if (getDualMode() == true) {
        stateReg |= STATE_REG_DUAL_MODE;
        if (getDualRecordingHint() == true)
            stateReg |= STATE_REG_DUAL_RECORDINGHINT;
    } else {
        if (getRecordingHint() == true)
            stateReg |= STATE_REG_RECORDINGHINT;
    }

    if (flagReprocessing == true)
        stateReg |= STATE_REG_FLAG_REPROCESSING;

#ifdef SET_LLS_PREVIEW_SETFILE
    if (getLLSPreviewOn())
        stateReg |= STATE_REG_NEED_LLS;
#endif
#ifdef SET_LLS_CAPTURE_SETFILE
    if (getLLSCaptureOn())
        stateReg |= STATE_REG_NEED_LLS;
#endif
#ifdef SR_CAPTURE
    int zoomLevel = getZoomLevel();
    float zoomRatio = getZoomRatio(zoomLevel) / 1000;

    if (getRecordingHint() == false && flagReprocessing == true
#ifdef SET_LLS_CAPTURE_SETFILE
        && !getLLSCaptureOn()
#endif
        ) {
        if (zoomRatio >= 3.0f && zoomRatio < 4.0f) {
            stateReg |= STATE_REG_ZOOM;
            ALOGV("(%s)[%d] : currentSetfile zoom",__func__, __LINE__);
        } else if (zoomRatio >= 4.0f) {
            if (getSROn()) {
                stateReg |= STATE_REG_ZOOM_OUTDOOR;
                ALOGV("(%s)[%d] : currentSetfile zoomoutdoor",__func__, __LINE__);
            } else {
                stateReg |= STATE_REG_ZOOM_INDOOR;
                ALOGV("(%s)[%d] : currentSetfile zoomindoor",__func__, __LINE__);
            }
        }
    }
#endif

    if ((stateReg & STATE_REG_RECORDINGHINT)||
        (stateReg & STATE_REG_UHD_RECORDING)||
        (stateReg & STATE_REG_DUAL_RECORDINGHINT)) {
        if (flagReprocessing == false) {
            flagYUVRange = YUV_LIMITED_RANGE;
        }
    }

    if (m_cameraId == CAMERA_ID_FRONT) {
        int vtMode = getVtMode();

        if (0 < vtMode) {
            switch (vtMode) {
            case 1:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT1;
                if(stateReg == STATE_STILL_CAPTURE)
                    currentSetfile = ISS_SUB_SCENARIO_FRONT_VT1_STILL_CAPTURE;
                break;
            case 2:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT2;
                break;
            case 4:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT4;
                break;
            default:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT2;
                break;
            }
        } else if (getIntelligentMode() == 1) {
            currentSetfile = ISS_SUB_SCENARIO_FRONT_SMART_STAY;
        } else if (getShotMode() == SHOT_MODE_FRONT_PANORAMA) {
            currentSetfile = ISS_SUB_SCENARIO_FRONT_PANORAMA;
        } else {
            switch(stateReg) {
                case STATE_STILL_PREVIEW:
#if FRONT_CAMERA_USE_SAMSUNG_COMPANION
                    if (!getUseCompanion())
                        currentSetfile = ISS_SUB_SCENARIO_FRONT_C2_OFF_STILL_PREVIEW;
                    else
#endif
#if defined(USE_BINNING_MODE) && defined(SET_PREVIEW_BINNING_SETFILE)
                    if (getBinningMode() == true)
                        currentSetfile = ISS_SUB_SCENARIO_FRONT_STILL_PREVIEW_BINNING;
                    else
#endif
                        currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                    break;

                case STATE_STILL_PREVIEW_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_ON;
                    break;

                case STATE_STILL_PREVIEW_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_AUTO;
                    break;

                case STATE_VIDEO:
#if FRONT_CAMERA_USE_SAMSUNG_COMPANION
                    if (!getUseCompanion())
                        currentSetfile = ISS_SUB_SCENARIO_FRONT_C2_OFF_VIDEO;
                    else
#endif
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                    break;

                case STATE_VIDEO_WDR_ON:
                case STATE_UHD_VIDEO_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_ON;
                    break;

                case STATE_VIDEO_WDR_AUTO:
                case STATE_UHD_VIDEO_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_AUTO;
                    break;

                case STATE_STILL_CAPTURE:
                case STATE_VIDEO_CAPTURE:
                case STATE_UHD_PREVIEW_CAPTURE:
                case STATE_UHD_VIDEO_CAPTURE:
#if FRONT_CAMERA_USE_SAMSUNG_COMPANION
                    if (!getUseCompanion())
                        currentSetfile = ISS_SUB_SCENARIO_FRONT_C2_OFF_STILL_CAPTURE;
                    else
#endif
                        currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                    break;

                case STATE_STILL_CAPTURE_WDR_ON:
                case STATE_VIDEO_CAPTURE_WDR_ON:
                case STATE_UHD_PREVIEW_CAPTURE_WDR_ON:
                case STATE_UHD_VIDEO_CAPTURE_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                    break;

                case STATE_STILL_CAPTURE_WDR_AUTO:
                case STATE_VIDEO_CAPTURE_WDR_AUTO:
                case STATE_UHD_PREVIEW_CAPTURE_WDR_AUTO:
                case STATE_UHD_VIDEO_CAPTURE_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO;
                    break;

                case STATE_DUAL_STILL_PREVIEW:
                case STATE_DUAL_STILL_CAPTURE:
                case STATE_DUAL_VIDEO_CAPTURE:
                    currentSetfile = ISS_SUB_SCENARIO_DUAL_STILL;
                    break;

                case STATE_DUAL_VIDEO:
                    currentSetfile = ISS_SUB_SCENARIO_DUAL_VIDEO;
                    break;

                case STATE_UHD_PREVIEW:
                case STATE_UHD_VIDEO:
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS;
                    break;

                default:
                    ALOGD("(%s)can't define senario of setfile.(0x%4x)",__func__, stateReg);
                    break;
            }
        }
    } else {
        switch(stateReg) {
            case STATE_STILL_PREVIEW:
#if defined(USE_BINNING_MODE) && defined(SET_PREVIEW_BINNING_SETFILE)
                if (getBinningMode() == true)
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_BINNING;
                else
#endif
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                break;

            case STATE_STILL_PREVIEW_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_ON;
                break;

            case STATE_STILL_PREVIEW_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_AUTO;
                break;

#ifdef SET_LLS_PREVIEW_SETFILE
            case STATE_STILL_PREVIEW_LLS:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LLS;
                break;
#endif
            case STATE_STILL_CAPTURE:
            case STATE_VIDEO_CAPTURE:
            case STATE_DUAL_STILL_CAPTURE:
            case STATE_DUAL_VIDEO_CAPTURE:
            case STATE_UHD_PREVIEW_CAPTURE:
            case STATE_UHD_VIDEO_CAPTURE:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                break;

            case STATE_STILL_CAPTURE_WDR_ON:
            case STATE_VIDEO_CAPTURE_WDR_ON:
            case STATE_UHD_PREVIEW_CAPTURE_WDR_ON:
            case STATE_UHD_VIDEO_CAPTURE_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                break;

            case STATE_STILL_CAPTURE_WDR_AUTO:
            case STATE_VIDEO_CAPTURE_WDR_AUTO:
            case STATE_UHD_PREVIEW_CAPTURE_WDR_AUTO:
            case STATE_UHD_VIDEO_CAPTURE_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO;
                break;

            case STATE_VIDEO:
                if (30 < minFps && 30 < maxFps) {
                    if (300 == minFps && 300 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_WVGA_300FPS;
                    } else if (60 == minFps && 60 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS;
                    } else if (240 == minFps && 240 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_240FPS;
                    } else {
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED;
                    }
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO;

#ifdef USE_FACTORY_FAST_AF
                    if (m_isFactoryMode) {
                        if (getFocusMode() == FOCUS_MODE_CONTINUOUS_PICTURE) {
                            currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                        } else {
                            currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                        }
                    }
#endif
                }
                break;

            case STATE_VIDEO_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_ON;
                break;

            case STATE_VIDEO_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_AUTO;
                break;

            case STATE_DUAL_VIDEO:
                currentSetfile = ISS_SUB_SCENARIO_DUAL_VIDEO;
                break;

            case STATE_DUAL_STILL_PREVIEW:
                currentSetfile = ISS_SUB_SCENARIO_DUAL_STILL;
                break;

            case STATE_UHD_PREVIEW:
            case STATE_UHD_VIDEO:
                currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS;
                break;

            case STATE_UHD_PREVIEW_WDR_ON:
            case STATE_UHD_VIDEO_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON;
                break;

#ifdef SR_CAPTURE
            case STATE_STILL_CAPTURE_ZOOM:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_ZOOM;
                break;
            case STATE_STILL_CAPTURE_ZOOM_OUTDOOR:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_ZOOM_OUTDOOR;
                break;
            case STATE_STILL_CAPTURE_ZOOM_INDOOR:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_ZOOM_INDOOR;
                break;
            case STATE_STILL_CAPTURE_WDR_ON_ZOOM:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON_ZOOM;
                break;
            case STATE_STILL_CAPTURE_WDR_ON_ZOOM_OUTDOOR:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON_ZOOM_OUTDOOR;
                break;
            case STATE_STILL_CAPTURE_WDR_ON_ZOOM_INDOOR:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON_ZOOM_INDOOR;
                break;
            case STATE_STILL_CAPTURE_WDR_AUTO_ZOOM:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO_ZOOM;
                break;
            case STATE_STILL_CAPTURE_WDR_AUTO_ZOOM_OUTDOOR:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO_ZOOM_OUTDOOR;
                break;
            case STATE_STILL_CAPTURE_WDR_AUTO_ZOOM_INDOOR:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO_ZOOM_INDOOR;
                break;
#endif
#ifdef SET_LLS_CAPTURE_SETFILE
            case STATE_STILL_CAPTURE_LLS:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LLS;
#ifdef LLS_REPROCESSING
                switch(getLLSCaptureCount()) {
                    case 1:
                        currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                        break;
                    default:
                        break;
                }
#endif
                break;
            case STATE_VIDEO_CAPTURE_WDR_ON_LLS:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON_LLS;
#ifdef LLS_REPROCESSING
                switch(getLLSCaptureCount()) {
                    case 1:
                        currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                        break;
                    default:
                        break;
                }
#endif
                break;
            case STATE_STILL_CAPTURE_WDR_AUTO_LLS:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_LLS;
#ifdef LLS_REPROCESSING
                switch(getLLSCaptureCount()) {
                    case 1:
                        currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO;
                        break;
                    default:
                        break;
                }
#endif
                break;
#endif
            default:
                ALOGD("(%s)can't define senario of setfile.(0x%4x)",__func__, stateReg);
                break;
        }
    }
#if 0
    ALOGD("(%s)[%d] : ===============================================================================",__func__, __LINE__);
    ALOGD("(%s)[%d] : CurrentState(0x%4x)",__func__, __LINE__, stateReg);
    ALOGD("(%s)[%d] : getRTHdr()(%d)",__func__, __LINE__, getRTHdr());
    ALOGD("(%s)[%d] : getRecordingHint()(%d)",__func__, __LINE__, getRecordingHint());
    ALOGD("(%s)[%d] : m_isUHDRecordingMode()(%d)",__func__, __LINE__, m_isUHDRecordingMode());
    ALOGD("(%s)[%d] : getDualMode()(%d)",__func__, __LINE__, getDualMode());
    ALOGD("(%s)[%d] : getDualRecordingHint()(%d)",__func__, __LINE__, getDualRecordingHint());
    ALOGD("(%s)[%d] : flagReprocessing(%d)",__func__, __LINE__, flagReprocessing);
    ALOGD("(%s)[%d] : ===============================================================================",__func__, __LINE__);
    ALOGD("(%s)[%d] : currentSetfile(%d)",__func__, __LINE__, currentSetfile);
    ALOGD("(%s)[%d] : flagYUVRange(%d)",__func__, __LINE__, flagYUVRange);
    ALOGD("(%s)[%d] : ===============================================================================",__func__, __LINE__);
#else
    ALOGD("(%s)[%d] : CurrentState (0x%4x), currentSetfile(%d)",__func__, __LINE__, stateReg, currentSetfile);
#endif

done:
    *setfile = currentSetfile;
    *yuvRange = flagYUVRange;
}

#ifdef SAMSUNG_LLV
void ExynosCamera1Parameters::setLLV(bool enable)
{
    m_isLLVOn = enable;
}

bool ExynosCamera1Parameters::getLLV(void)
{
    bool isSizeSupported = true;
    int videoW, videoH;

    getVideoSize(&videoW, &videoH);
    if(videoW > 1920 || videoH > 1080)
        isSizeSupported = false;
    else {
        uint32_t minFPS = 0;
        uint32_t maxFPS = 0;
        getPreviewFpsRange(&minFPS, &maxFPS);
        if(minFPS > 30 || maxFPS > 30)
            isSizeSupported = false;
    }

    return (m_isLLVOn && isSizeSupported);
}
#endif

int *ExynosCamera1Parameters::getHighSpeedSizeTable(int fpsMode) {
    int index = 0;
    int *sizeList = NULL;
    int videoW = 0, videoH = 0;
    int videoRatioEnum = SIZE_RATIO_16_9;
    float videoRatio = 0.00f;

    /*
        SIZE_RATIO_16_9 = 0,
        SIZE_RATIO_4_3,
        SIZE_RATIO_1_1,
        SIZE_RATIO_3_2,
        SIZE_RATIO_5_4,
        SIZE_RATIO_5_3,
        SIZE_RATIO_11_9,
      */
    getVideoSize(&videoW, &videoH);
    videoRatio = ROUND_OFF(((float)videoW / (float)videoH), 2);
    if (videoRatio == 1.33f) { /* 4:3 */
        videoRatioEnum = SIZE_RATIO_4_3;
    } else if (videoRatio == 1.77f) { /* 16:9 */
        videoRatioEnum = SIZE_RATIO_16_9;
    } else if (videoRatio == 1.00f) { /* 1:1 */
        videoRatioEnum = SIZE_RATIO_1_1;
    } else if (videoRatio == 1.50f) { /* 3:2 */
        videoRatioEnum = SIZE_RATIO_3_2;
    } else if (videoRatio == 1.25f) { /* 5:4 */
        videoRatioEnum = SIZE_RATIO_5_4;
    } else if (videoRatio == 1.66f) { /* 5:3*/
        videoRatioEnum = SIZE_RATIO_5_3;
    } else if (videoRatio == 1.22f) { /* 11:9 */
        videoRatioEnum = SIZE_RATIO_11_9;
    } else {
        videoRatioEnum = SIZE_RATIO_16_9;
    }

    switch(fpsMode) {
        case CONFIG_MODE::HIGHSPEED_60:
            if (m_staticInfo->videoSizeLutHighSpeed60Max == 0
                || m_staticInfo->videoSizeLutHighSpeed60 == NULL) {
                ALOGE("ERR(%s[%d]):videoSizeLutHighSpeed is NULL.fpsMode(%d)", __FUNCTION__, __LINE__, fpsMode);
                sizeList = NULL;
            } else {
                for (index = 0; index < m_staticInfo->videoSizeLutHighSpeed60Max; index++) {
                    if (m_staticInfo->videoSizeLutHighSpeed60[index][RATIO_ID] == videoRatioEnum) {
                        break;
                    }
                }

                if (index >= m_staticInfo->videoSizeLutHighSpeed60Max)
                    index = 0;

                sizeList = m_staticInfo->videoSizeLutHighSpeed60[index];
            }
            break;
        case CONFIG_MODE::HIGHSPEED_120:
            if (m_staticInfo->videoSizeLutHighSpeed120Max == 0
                || m_staticInfo->videoSizeLutHighSpeed120 == NULL) {
                ALOGE("ERR(%s[%d]):videoSizeLutHighSpeed is NULL.fpsMode(%d)", __FUNCTION__, __LINE__, fpsMode);
                sizeList = NULL;
            } else {
                for (index = 0; index < m_staticInfo->videoSizeLutHighSpeed120Max; index++) {
                    if (m_staticInfo->videoSizeLutHighSpeed120[index][RATIO_ID] == videoRatioEnum) {
                        break;
                    }
                }

                if (index >= m_staticInfo->videoSizeLutHighSpeed120Max)
                    index = 0;

                sizeList = m_staticInfo->videoSizeLutHighSpeed120[index];
            }
            break;
        default :
            ALOGE("ERR(%s[%d]):getConfigMode : fpsmode(%d) fail", __FUNCTION__, __LINE__, fpsMode);
            break;
    }

    return sizeList;
}

status_t ExynosCamera1Parameters::calcPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int previewW = 0, previewH = 0, previewFormat = 0;
    int hwPreviewW = 0, hwPreviewH = 0, hwPreviewFormat = 0;
    previewFormat = getPreviewFormat();
    hwPreviewFormat = getHwPreviewFormat();

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
#ifdef SUPPORT_SW_VDIS
    if(isSWVdisMode())
        m_swVDIS_AdjustPreviewSize(&hwPreviewW, &hwPreviewH);
#endif /*SUPPORT_SW_VDIS*/
    getPreviewSize(&previewW, &previewH);

    ret = getCropRectAlign(hwPreviewW, hwPreviewH,
                           previewW, previewH,
                           &cropX, &cropY,
                           &cropW, &cropH,
                           CAMERA_MAGIC_ALIGN, 2,
                           0, 1);

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = cropW;
    srcRect->fullH = cropH;
    srcRect->colorFormat = hwPreviewFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = previewW;
    dstRect->h = previewH;
    dstRect->fullW = previewW;
    dstRect->fullH = previewH;
    dstRect->colorFormat = previewFormat;

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcRecordingGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;

    int hwPreviewW = 0, hwPreviewH = 0, hwPreviewFormat = 0;
    int videoW = 0, videoH = 0, videoFormat = 0;
    float zoomRatio = getZoomRatio(0) / 1000;

    hwPreviewFormat = getHwPreviewFormat();
    videoFormat     = getVideoFormat();

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
#ifdef SUPPORT_SW_VDIS
    if(isSWVdisMode())
        m_swVDIS_AdjustPreviewSize(&hwPreviewW, &hwPreviewH);
#endif /*SUPPORT_SW_VDIS*/
    getVideoSize(&videoW, &videoH);

    if (SIZE_RATIO(hwPreviewW, hwPreviewH) == SIZE_RATIO(videoW, videoH)) {
        cropW = hwPreviewW;
        cropH = hwPreviewH;
    } else  {
        ret = getCropRectAlign(hwPreviewW, hwPreviewH,
                         videoW, videoH,
                         &cropX, &cropY,
                         &cropW, &cropH,
                         2, 2,
                         0, zoomRatio);
    }

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = hwPreviewW;
    srcRect->fullH = hwPreviewH;
    srcRect->colorFormat = hwPreviewFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = videoW;
    dstRect->h = videoH;
    dstRect->fullW = videoW;
    dstRect->fullH = videoH;
    dstRect->colorFormat = videoFormat;

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int pictureW = 0, pictureH = 0;
    int previewW = 0, previewH = 0;
    int hwSensorMarginW = 0, hwSensorMarginH = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
#if 0
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;
    int pictureFormat = 0, hwPictureFormat = 0;
#endif
    int zoomLevel = 0;
    int maxZoomRatio = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    float zoomRatio = getZoomRatio(0) / 1000;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
#if 0
    pictureFormat = getHwPictureFormat();
#endif
    zoomLevel = getZoomLevel();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);
    m_adjustSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    hwSensorW -= hwSensorMarginW;
    hwSensorH -= hwSensorMarginH;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
        int maxSensorW = 0, maxSensorH = 0;
        float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

        m_getCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);
        getMaxSensorSize(&maxSensorW, &maxSensorH);

        /* 1. Scale down the crop region to adjust with the bcrop input size */
        scaleRatioX = (float) hwSensorW / (float) maxSensorW;
        scaleRatioY = (float) hwSensorH / (float) maxSensorH;
        cropRegionX = (int) (cropRegionX * scaleRatioX);
        cropRegionY = (int) (cropRegionY * scaleRatioY);
        cropRegionW = (int) (cropRegionW * scaleRatioX);
        cropRegionH = (int) (cropRegionH * scaleRatioY);

        if (cropRegionW < 1 || cropRegionH < 1) {
            cropRegionW = hwSensorW;
            cropRegionH = hwSensorH;
        }

        /* 2. Calculate the real crop region with considering the target ratio */
        ret = getCropRectAlign(cropRegionW, cropRegionH,
                previewW, previewH,
                &cropX, &cropY,
                &cropW, &cropH,
                CAMERA_BCROP_ALIGN, 2,
                0, 0.0f);

        cropX = ALIGN_DOWN((cropRegionX + cropX), 2);
        cropY = ALIGN_DOWN((cropRegionY + cropY), 2);
    } else {
        ret = getCropRectAlign(hwSensorW, hwSensorH,
                previewW, previewH,
                &cropX, &cropY,
                &cropW, &cropH,
                CAMERA_BCROP_ALIGN, 2,
                zoomLevel, zoomRatio);

        cropX = ALIGN_DOWN(cropX, 2);
        cropY = ALIGN_DOWN(cropY, 2);
        cropW = ALIGN_UP(hwSensorW - (cropX * 2), CAMERA_BCROP_ALIGN);
        cropH = hwSensorH - (cropY * 2);
    }

    if (getUsePureBayerReprocessing() == false) {
        int pictureCropX = 0, pictureCropY = 0;
        int pictureCropW = 0, pictureCropH = 0;

        zoomLevel = 0;
        zoomRatio = getZoomRatio(zoomLevel) / 1000;

        ret = getCropRectAlign(cropW, cropH,
                pictureW, pictureH,
                &pictureCropX, &pictureCropY,
                &pictureCropW, &pictureCropH,
                CAMERA_BCROP_ALIGN, 2,
                zoomLevel, zoomRatio);

        pictureCropX = ALIGN_DOWN(pictureCropX, 2);
        pictureCropY = ALIGN_DOWN(pictureCropY, 2);
        pictureCropW = cropW - (pictureCropX * 2);
        pictureCropH = cropH - (pictureCropY * 2);

        if (pictureCropW < pictureW / maxZoomRatio || pictureCropH < pictureH / maxZoomRatio) {
            ALOGW("WRN(%s[%d]): zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", __FUNCTION__, __LINE__, maxZoomRatio, cropW, cropH, pictureW, pictureH);
            float src_ratio = 1.0f;
            float dst_ratio = 1.0f;
            /* ex : 1024 / 768 */
            src_ratio = ROUND_OFF_HALF(((float)cropW / (float)cropH), 2);
            /* ex : 352  / 288 */
            dst_ratio = ROUND_OFF_HALF(((float)pictureW / (float)pictureH), 2);

            if (dst_ratio <= src_ratio) {
                /* shrink w */
                cropX = ALIGN_DOWN(((int)(hwSensorW - ((pictureH / maxZoomRatio) * src_ratio)) >> 1), 2);
                cropY = ALIGN_DOWN(((hwSensorH - (pictureH / maxZoomRatio)) >> 1), 2);
            } else {
                /* shrink h */
                cropX = ALIGN_DOWN(((hwSensorW - (pictureW / maxZoomRatio)) >> 1), 2);
                cropY = ALIGN_DOWN(((int)(hwSensorH - ((pictureW / maxZoomRatio) / src_ratio)) >> 1), 2);
            }
            cropW = ALIGN_UP(hwSensorW - (cropX * 2), CAMERA_BCROP_ALIGN);
            cropH = hwSensorH - (cropY * 2);
        }
    }

#if 0
    ALOGD("DEBUG(%s):hwSensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, hwSensorW, hwSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = hwSensorW;
    srcRect->h = hwSensorH;
    srcRect->fullW = hwSensorW;
    srcRect->fullH = hwSensorH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = cropX;
    dstRect->y = cropY;
    dstRect->w = cropW;
    dstRect->h = cropH;
    dstRect->fullW = cropW;
    dstRect->fullH = cropH;
    dstRect->colorFormat = bayerFormat;

    m_setHwBayerCropRegion(dstRect->w, dstRect->h, dstRect->x, dstRect->y);
    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPreviewDzoomCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int previewW = 0, previewH = 0;
    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;

    int zoomLevel = 0;
    int maxZoomRatio = 0;
    float zoomRatio = getZoomRatio(0) / 1000;

    /* TODO: check state ready for start */
    zoomLevel = getZoomLevel();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getHwPreviewSize(&previewW, &previewH);
    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    ret = getCropRectAlign(srcRect->w, srcRect->h,
                     previewW, previewH,
                     &srcRect->x, &srcRect->y,
                     &srcRect->w, &srcRect->h,
                     2, 2,
                     zoomLevel, zoomRatio);
    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = previewW;
    dstRect->h = previewH;

    ALOGV("INFO(%s[%d]):SRC cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d ratio = %f", __FUNCTION__, __LINE__, srcRect->x, srcRect->y, srcRect->w, srcRect->h, zoomLevel, zoomRatio);
    ALOGV("INFO(%s[%d]):DST cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d ratio = %f", __FUNCTION__, __LINE__, dstRect->x, dstRect->y, dstRect->w, dstRect->h, zoomLevel, zoomRatio);

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::getPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int hwBnsW   = 0;
    int hwBnsH   = 0;
    int hwBcropW = 0;
    int hwBcropH = 0;
    int zoomLevel = 0;
    float zoomRatio = 1.00f;
    int hwSensorMarginW = 0;
    int hwSensorMarginH = 0;

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_staticInfo->pictureSizeLut == NULL
        || m_staticInfo->pictureSizeLutMax <= m_cameraInfo.pictureSizeRatioId
        || m_cameraInfo.pictureSizeRatioId != m_cameraInfo.previewSizeRatioId)
        return calcPictureBayerCropSize(srcRect, dstRect);

    /* use LUT */
    hwBnsW   = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BNS_W];
    hwBnsH   = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BNS_H];
    hwBcropW = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BCROP_W];
    hwBcropH = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BCROP_H];

    if (getHalVersion() != IS_HAL_VER_3_2) {
#ifdef SR_CAPTURE
        if(getSROn()) {
            zoomLevel = ZOOM_LEVEL_0;
        } else {
            zoomLevel = getZoomLevel();
        }
#else
        zoomLevel = getZoomLevel();
#endif
        zoomRatio = getZoomRatio(zoomLevel) / 1000;

#if defined(SCALER_MAX_SCALE_UP_RATIO)
        /*
         * After dividing float & casting int,
         * zoomed size can be smaller too much.
         * so, when zoom until max, ceil up about floating point.
         */
        if (ALIGN_UP((int)((float)hwBcropW / zoomRatio), CAMERA_BCROP_ALIGN) * SCALER_MAX_SCALE_UP_RATIO < hwBcropW ||
                ALIGN_UP((int)((float)hwBcropH / zoomRatio), 2) * SCALER_MAX_SCALE_UP_RATIO < hwBcropH) {
            hwBcropW = ALIGN_UP((int)ceil((float)hwBcropW / zoomRatio), CAMERA_BCROP_ALIGN);
            hwBcropH = ALIGN_UP((int)ceil((float)hwBcropH / zoomRatio), 2);
        } else
#endif
        {
            hwBcropW = ALIGN_UP((int)((float)hwBcropW / zoomRatio), CAMERA_BCROP_ALIGN);
            hwBcropH = ALIGN_UP((int)((float)hwBcropH / zoomRatio), 2);
        }
    }

    /* Re-calculate the BNS size for removing Sensor Margin.
       On Capture Stream(3AA_M2M_Input), the BNS is not used.
       So, the BNS ratio is not needed to be considered for sensor margin */
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);
    hwBnsW = hwBnsW - hwSensorMarginW;
    hwBnsH = hwBnsH - hwSensorMarginH;

    /* src */
    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = hwBnsW;
    srcRect->h = hwBnsH;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
        int maxSensorW = 0, maxSensorH = 0;
        float scaleRatioX = 0.0f, scaleRatioY = 0.0f;
        status_t ret = NO_ERROR;

        m_getCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);
        getMaxSensorSize(&maxSensorW, &maxSensorH);

        /* 1. Scale down the crop region to adjust with the bcrop input size */
        scaleRatioX = (float) hwBnsW / (float) maxSensorW;
        scaleRatioY = (float) hwBnsH / (float) maxSensorH;
        cropRegionX = (int) (cropRegionX * scaleRatioX);
        cropRegionY = (int) (cropRegionY * scaleRatioY);
        cropRegionW = (int) (cropRegionW * scaleRatioX);
        cropRegionH = (int) (cropRegionH * scaleRatioY);

        if (cropRegionW < 1 || cropRegionH < 1) {
            cropRegionW = hwBnsW;
            cropRegionH = hwBnsH;
        }

        /* 2. Calculate the real crop region with considering the target ratio */
        if ((cropRegionW > hwBcropW) && (cropRegionH > hwBcropH)) {
            dstRect->x = ALIGN_DOWN((cropRegionX + ((cropRegionW - hwBcropW) >> 1)), 2);
            dstRect->y = ALIGN_DOWN((cropRegionY + ((cropRegionH - hwBcropH) >> 1)), 2);
            dstRect->w = hwBcropW;
            dstRect->h = hwBcropH;
        } else {
            ret = getCropRectAlign(cropRegionW, cropRegionH,
                    hwBcropW, hwBcropH,
                    &(dstRect->x), &(dstRect->y),
                    &(dstRect->w), &(dstRect->h),
                    CAMERA_MAGIC_ALIGN, 2,
                    0, 0.0f);
            dstRect->x = ALIGN_DOWN((cropRegionX + dstRect->x), 2);
            dstRect->y = ALIGN_DOWN((cropRegionY + dstRect->y), 2);
        }
    } else {
        /* dst */
        if (hwBnsW > hwBcropW) {
            dstRect->x = ALIGN_UP(((hwBnsW - hwBcropW) >> 1), 2);
            dstRect->w = hwBcropW;
        } else {
            dstRect->x = 0;
            dstRect->w = hwBnsW;
        }

        if (hwBnsH > hwBcropH) {
            dstRect->y = ALIGN_UP(((hwBnsH - hwBcropH) >> 1), 2);
            dstRect->h = hwBcropH;
        } else {
            dstRect->y = 0;
            dstRect->h = hwBnsH;
        }
    }

#if DEBUG
    ALOGD("DEBUG(%s):zoomRatio=%f", __FUNCTION__, zoomRatio);
    ALOGD("DEBUG(%s):hwBnsSize (%dx%d), hwBcropSize (%d, %d)(%dx%d)",
            __FUNCTION__, srcRect->w, srcRect->h, dstRect->x, dstRect->y, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int maxSensorW = 0, maxSensorH = 0;
    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0, hwPictureFormat = 0;
    int hwSensorCropX = 0, hwSensorCropY = 0;
    int hwSensorCropW = 0, hwSensorCropH = 0;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    int previewW = 0, previewH = 0;
    int hwSensorMarginW = 0, hwSensorMarginH = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int zoomLevel = 0;
    float zoomRatio = 1.0f;
    int maxZoomRatio = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
    pictureFormat = getHwPictureFormat();
    zoomLevel = getZoomLevel();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getMaxSensorSize(&maxSensorW, &maxSensorH);
    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    hwSensorW -= hwSensorMarginW;
    hwSensorH -= hwSensorMarginH;

    if (getUsePureBayerReprocessing() == true) {
        if (getHalVersion() == IS_HAL_VER_3_2) {
            int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
            float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

            m_getCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);

            /* 1. Scale down the crop region to adjust with the bcrop input size */
            scaleRatioX = (float) hwSensorW / (float) maxSensorW;
            scaleRatioY = (float) hwSensorH / (float) maxSensorH;
            cropRegionX = (int) (cropRegionX * scaleRatioX);
            cropRegionY = (int) (cropRegionY * scaleRatioY);
            cropRegionW = (int) (cropRegionW * scaleRatioX);
            cropRegionH = (int) (cropRegionH * scaleRatioY);

            if (cropRegionW < 1 || cropRegionH < 1) {
                cropRegionW = hwSensorW;
                cropRegionH = hwSensorH;
            }

            ret = getCropRectAlign(cropRegionW, cropRegionH,
                    pictureW, pictureH,
                    &cropX, &cropY,
                    &cropW, &cropH,
                    CAMERA_MAGIC_ALIGN, 2,
                    0, 0.0f);

            cropX = ALIGN_DOWN((cropRegionX + cropX), 2);
            cropY = ALIGN_DOWN((cropRegionY + cropY), 2);
        } else {
            ret = getCropRectAlign(hwSensorW, hwSensorH,
                    pictureW, pictureH,
                    &cropX, &cropY,
                    &cropW, &cropH,
                    CAMERA_MAGIC_ALIGN, 2,
                    zoomLevel, zoomRatio);

            cropX = ALIGN_DOWN(cropX, 2);
            cropY = ALIGN_DOWN(cropY, 2);
            cropW = hwSensorW - (cropX * 2);
            cropH = hwSensorH - (cropY * 2);
        }

        if (cropW < pictureW / maxZoomRatio || cropH < pictureH / maxZoomRatio) {
            ALOGW("WRN(%s[%d]): zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", __FUNCTION__, __LINE__, maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorW - (cropX * 2);
            cropH = hwSensorH - (cropY * 2);
        }
    } else {
        zoomLevel = 0;
        if (getHalVersion() == IS_HAL_VER_3_2)
            zoomRatio = 0.0f;
        else
            zoomRatio = getZoomRatio(zoomLevel) / 1000;

        getHwBayerCropRegion(&hwSensorCropW, &hwSensorCropH, &hwSensorCropX, &hwSensorCropY);

        ret = getCropRectAlign(hwSensorCropW, hwSensorCropH,
                     pictureW, pictureH,
                     &cropX, &cropY,
                     &cropW, &cropH,
                     CAMERA_MAGIC_ALIGN, 2,
                     zoomLevel, zoomRatio);

        cropX = ALIGN_DOWN(cropX, 2);
        cropY = ALIGN_DOWN(cropY, 2);
        cropW = hwSensorCropW - (cropX * 2);
        cropH = hwSensorCropH - (cropY * 2);

        if (cropW < pictureW / maxZoomRatio || cropH < pictureH / maxZoomRatio) {
            ALOGW("WRN(%s[%d]): zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", __FUNCTION__, __LINE__, maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorCropW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorCropH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorCropW - (cropX * 2);
            cropH = hwSensorCropH - (cropY * 2);
        }
    }

#if 1
    ALOGD("DEBUG(%s):maxSensorSize (%dx%d), hwSensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, maxSensorW, maxSensorH, hwSensorW, hwSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = maxSensorW;
    srcRect->h = maxSensorH;
    srcRect->fullW = maxSensorW;
    srcRect->fullH = maxSensorH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = cropX;
    dstRect->y = cropY;
    dstRect->w = cropW;
    dstRect->h = cropH;
    dstRect->fullW = cropW;
    dstRect->fullH = cropH;
    dstRect->colorFormat = bayerFormat;
    return NO_ERROR;
}

status_t ExynosCamera1Parameters::m_getPreviewBdsSize(ExynosRect *dstRect)
{
    int hwBdsW = 0;
    int hwBdsH = 0;
    int sizeList[SIZE_LUT_INDEX_END];

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_staticInfo->previewSizeLut == NULL
        || m_staticInfo->previewSizeLutMax <= m_cameraInfo.previewSizeRatioId
        || m_getPreviewSizeList(sizeList) != NO_ERROR) {
        ExynosRect rect;
        return calcPreviewBDSSize(&rect, dstRect);
    }

    /* use LUT */
    hwBdsW = sizeList[BDS_W];
    hwBdsH = sizeList[BDS_H];

    if (getRecordingHint() == true) {
        int videoW = 0, videoH = 0;
        getVideoSize(&videoW, &videoH);

        if (m_cameraInfo.previewSizeRatioId != m_cameraInfo.videoSizeRatioId)
            ALOGV("WARN(%s[%d]):preview ratioId(%d) != videoRatioId(%d), use previewRatioId",
                    __FUNCTION__, __LINE__,
                    m_cameraInfo.previewSizeRatioId, m_cameraInfo.videoSizeRatioId);

        if ((videoW == 3840 && videoH == 2160) || (videoW == 2560 && videoH == 1440)) {
            hwBdsW = videoW;
            hwBdsH = videoH;
        }
    }
#ifdef USE_BDS_WIDE_SELFIE
    else if (getShotMode() == SHOT_MODE_FRONT_PANORAMA && !getRecordingHint()) {
        hwBdsW = WIDE_SELFIE_WIDTH;
        hwBdsH = WIDE_SELFIE_HEIGHT;
    }
#endif

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = hwBdsW;
    dstRect->h = hwBdsH;

#ifdef DEBUG_PERFRAME
    ALOGD("DEBUG(%s):hwBdsSize (%dx%d)", __FUNCTION__, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPreviewBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int pictureW = 0, pictureH = 0;
    int previewW = 0, previewH = 0;
    ExynosRect bnsSize;
    ExynosRect bayerCropSize;
#if 0
    int pictureFormat = 0, hwPictureFormat = 0;
#endif
    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int zoomLevel = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    float zoomRatio = 1.0f;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
#if 0
    pictureFormat = getHwPictureFormat();
#endif
    zoomLevel = getZoomLevel();
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);
    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);

    /* TODO: get crop size from ctlMetadata */
    ret = getCropRectAlign(hwSensorW, hwSensorH,
                     previewW, previewH,
                     &cropX, &cropY,
                     &cropW, &cropH,
                     CAMERA_MAGIC_ALIGN, 2,
                     zoomLevel, zoomRatio);

    zoomRatio = getZoomRatio(0) / 1000;

    ret = getCropRectAlign(cropW, cropH,
                     previewW, previewH,
                     &crop_crop_x, &crop_crop_y,
                     &crop_crop_w, &crop_crop_h,
                     2, 2,
                     0, zoomRatio);

    cropX = ALIGN_UP(cropX, 2);
    cropY = ALIGN_UP(cropY, 2);
    cropW = hwSensorW - (cropX * 2);
    cropH = hwSensorH - (cropY * 2);

//    ALIGN_UP(crop_crop_x, 2);
//    ALIGN_UP(crop_crop_y, 2);

#if 0
    ALOGD("DEBUG(%s):hwSensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, hwSensorW, hwSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = cropW;
    srcRect->fullH = cropH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;
    /* For Front Single Scenario, BDS should not be used */
    if (m_cameraId == CAMERA_ID_FRONT && getDualMode() == false) {
        getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
        dstRect->w = bayerCropSize.w;
        dstRect->h = bayerCropSize.h;
        dstRect->fullW = bayerCropSize.w;
        dstRect->fullH = bayerCropSize.h;
    } else {
        dstRect->w = previewW;
        dstRect->h = previewH;
        dstRect->fullW = previewW;
        dstRect->fullH = previewH;
    }

    if (dstRect->w > srcRect->w)
        dstRect->w = srcRect->w;
    if (dstRect->h > srcRect->h)
        dstRect->h = srcRect->h;

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPictureBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int maxSensorW = 0, maxSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int pictureW = 0, pictureH = 0;
    int previewW = 0, previewH = 0;
#if 0
    int pictureFormat = 0, hwPictureFormat = 0;
#endif
    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int zoomLevel = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    float zoomRatio = 1.0f;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
#if 0
    pictureFormat = getHwPictureFormat();
#endif
    zoomLevel = getZoomLevel();
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getMaxSensorSize(&maxSensorW, &maxSensorH);
    getPreviewSize(&previewW, &previewH);

    zoomRatio = getZoomRatio(zoomLevel) / 1000;
    /* TODO: get crop size from ctlMetadata */
    ret = getCropRectAlign(maxSensorW, maxSensorH,
                     pictureW, pictureH,
                     &cropX, &cropY,
                     &cropW, &cropH,
                     CAMERA_MAGIC_ALIGN, 2,
                     zoomLevel, zoomRatio);

    zoomRatio = getZoomRatio(0) / 1000;
    ret = getCropRectAlign(cropW, cropH,
                     pictureW, pictureH,
                     &crop_crop_x, &crop_crop_y,
                     &crop_crop_w, &crop_crop_h,
                     2, 2,
                     0, zoomRatio);

    cropX = ALIGN_UP(cropX, 2);
    cropY = ALIGN_UP(cropY, 2);
    cropW = maxSensorW - (cropX * 2);
    cropH = maxSensorH - (cropY * 2);

//    ALIGN_UP(crop_crop_x, 2);
//    ALIGN_UP(crop_crop_y, 2);

#if 0
    ALOGD("DEBUG(%s):SensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, maxSensorW, maxSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = cropW;
    srcRect->fullH = cropH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = pictureW;
    dstRect->h = pictureH;
    dstRect->fullW = pictureW;
    dstRect->fullH = pictureH;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;

    if (dstRect->w > srcRect->w)
        dstRect->w = srcRect->w;
    if (dstRect->h > srcRect->h)
        dstRect->h = srcRect->h;

    return NO_ERROR;
}

#ifdef SAMSUNG_LBP
bool ExynosCamera1Parameters::getUseBestPic(void)
{
    return m_useBestPic;
}
#endif

bool ExynosCamera1Parameters::doCscRecording(void)
{
    bool ret = true;
    int hwPreviewW = 0, hwPreviewH = 0;
    int videoW = 0, videoH = 0;

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
#ifdef SUPPORT_SW_VDIS
    if(isSWVdisMode())
        m_swVDIS_AdjustPreviewSize(&hwPreviewW, &hwPreviewH);
#endif /*SUPPORT_SW_VDIS*/

    getVideoSize(&videoW, &videoH);
    CLOGV("DEBUG(%s[%d]):hwPreviewSize = %d x %d",  __FUNCTION__, __LINE__, hwPreviewW, hwPreviewH);
    CLOGV("DEBUG(%s[%d]):videoSize     = %d x %d",  __FUNCTION__, __LINE__, videoW, videoH);

    if (((videoW == 3840 && videoH == 2160) || (videoW == 1920 && videoH == 1080) || (videoW == 2560 && videoH == 1440))
        && m_useAdaptiveCSCRecording == true
        && videoW == hwPreviewW
        && videoH == hwPreviewH) {
        ret = false;
    }

    return ret;
}

#ifdef DEBUG_RAWDUMP
bool ExynosCamera1Parameters::checkBayerDumpEnable(void)
{
#ifndef RAWDUMP_CAPTURE
    char enableRawDump[PROPERTY_VALUE_MAX];
    property_get("ro.debug.rawdump", enableRawDump, "0");

    if (strcmp(enableRawDump, "1") == 0) {
        /*CLOGD("checkBayerDumpEnable : 1");*/
        return true;
    } else {
        /*CLOGD("checkBayerDumpEnable : 0");*/
        return false;
    }
#endif
    return true;
}
#endif  /* DEBUG_RAWDUMP */

bool ExynosCamera1Parameters::increaseMaxBufferOfPreview(void)
{
    if((getShotMode() == SHOT_MODE_BEAUTY_FACE)||(getShotMode() == SHOT_MODE_FRONT_PANORAMA)
#ifdef LLS_CAPTURE
        || (getLLSOn() == true && getCameraId() == CAMERA_ID_FRONT)
#endif
        ) {
        return true;
    } else {
        return false;
    }
}

bool ExynosCamera1Parameters::getSupportedZoomPreviewWIthScaler(void)
{
    bool ret = false;
    int cameraId = getCameraId();
    bool flagDual = getDualMode();
    int fastFpsMode = getFastFpsMode();
    int vrMode = getVRMode();

    if (cameraId == CAMERA_ID_BACK) {
        if (fastFpsMode > CONFIG_MODE::HIGHSPEED_60 &&
            fastFpsMode < CONFIG_MODE::MAX &&
            vrMode != 1) {
            ret = true;
        }
    } else {
        if (flagDual == true) {
            ret = true;
        }
    }

    return ret;
}

#ifdef USE_FADE_IN_ENTRANCE
status_t ExynosCamera1Parameters::checkFirstEntrance(const CameraParameters& params)
{
    const char *strNewFirstEntrance = params.get("entrance");
    bool curFirstEntrance = getFirstEntrance();
    bool firstEntrance = false;

    CLOGD("DEBUG(%s):newFirstEntrance %s", "setParameters", strNewFirstEntrance);

    if (strNewFirstEntrance != NULL && !strcmp(strNewFirstEntrance, "true")) {
        firstEntrance = true;
    } else {
        firstEntrance = false;
    }

    if (curFirstEntrance != firstEntrance) {
        setFirstEntrance(firstEntrance);
        m_params.set("entrance", strNewFirstEntrance);
    }
    return NO_ERROR;
}

void ExynosCamera1Parameters::setFirstEntrance(bool value)
{
    m_flagFirstEntrance = value;
}

bool ExynosCamera1Parameters::getFirstEntrance(void)
{
    return m_flagFirstEntrance;
}
#endif

#ifdef SAMSUNG_OT
bool ExynosCamera1Parameters::getObjectTrackingEnable(void)
{
    return m_startObjectTracking;
}

bool ExynosCamera1Parameters::getObjectTrackingChanged(void)
{
    return m_objectTrackingAreaChanged;
}

void ExynosCamera1Parameters::setObjectTrackingChanged(bool newValue)
{
    m_objectTrackingAreaChanged = newValue;

    return;
}

int ExynosCamera1Parameters::getObjectTrackingAreas(int* validFocusArea, ExynosRect2* areas, int* weights)
{
    if(m_objectTrackingArea == NULL || m_objectTrackingWeight == NULL) {
        ALOGE("ERR(%s[%d]): NULL pointer!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    *validFocusArea = m_cameraInfo.numValidFocusArea;
    for (int i = 0; i < *validFocusArea; i++) {
        areas[i] = m_objectTrackingArea[i];
        weights[i] = m_objectTrackingWeight[i];
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::setObjectTrackingGet(bool newValue)
{
    m_objectTrackingGet = newValue;

    return;
}

bool ExynosCamera1Parameters::getObjectTrackingGet(void)
{
    return m_objectTrackingGet;
}
#endif
#ifdef SAMSUNG_LBP
void ExynosCamera1Parameters::setNormalBestFrameCount(uint32_t count)
{
    m_normal_best_frame_count = count;
}

uint32_t ExynosCamera1Parameters::getNormalBestFrameCount(void)
{
    return m_normal_best_frame_count;
}

void ExynosCamera1Parameters::resetNormalBestFrameCount(void)
{
    m_normal_best_frame_count = 0;
}

void ExynosCamera1Parameters::setSCPFrameCount(uint32_t count)
{
    m_scp_frame_count = count;
}

uint32_t ExynosCamera1Parameters::getSCPFrameCount(void)
{
    return m_scp_frame_count;
}

void ExynosCamera1Parameters::resetSCPFrameCount(void)
{
    m_scp_frame_count = 0;
}

void ExynosCamera1Parameters::setBayerFrameCount(uint32_t count)
{
    m_bayer_frame_count = count;
}

uint32_t ExynosCamera1Parameters::getBayerFrameCount(void)
{
    return m_bayer_frame_count;
}

void ExynosCamera1Parameters::resetBayerFrameCount(void)
{
    m_bayer_frame_count = 0;
}
#endif

#ifdef SAMSUNG_HRM
void ExynosCamera1Parameters::m_setHRM(int ir_data, int status)
{
    setMetaCtlHRM(&m_metadata, ir_data, status);
}
void ExynosCamera1Parameters::m_setHRM_Hint(bool flag)
{
    m_flagSensorHRM_Hint = flag;
}
#endif
#ifdef SAMSUNG_LIGHT_IR
void ExynosCamera1Parameters::m_setLight_IR(SensorListenerEvent_t data)
{
    setMetaCtlLight_IR(&m_metadata, data);
}
void ExynosCamera1Parameters::m_setLight_IR_Hint(bool flag)
{
    m_flagSensorLight_IR_Hint = flag;
}
#endif
#ifdef SAMSUNG_GYRO
void ExynosCamera1Parameters::m_setGyro(SensorListenerEvent_t data)
{
    setMetaCtlGyro(&m_metadata, data);
}
void ExynosCamera1Parameters::m_setGyroHint(bool flag)
{
    m_flagSensorGyroHint = flag;
}
#endif

#ifdef FORCE_CAL_RELOAD
bool ExynosCamera1Parameters::m_checkCalibrationDataValid(char *sensor_fw)
{
    bool ret = true;
    char *tok = NULL;
    char *calcheck = NULL;
    char *last;
    char readdata[50];
    FILE *fp = NULL;

    fp = fopen("/sys/class/camera/rear/rear_force_cal_load", "r");
    if (fp == NULL) {
        return ret;
    } else {
        if (fgets(readdata, sizeof(readdata), fp) == NULL) {
            CLOGE("ERR(%s[%d]):failed to read sysfs entry", __FUNCTION__, __LINE__);
        }
        CLOGD("DEBUG(%s[%d]):%s", __FUNCTION__, __LINE__, readdata);

        fclose(fp);
    }

    tok = strtok_r(sensor_fw, " ", &last);
    if (tok != NULL) {
        calcheck = strtok_r(NULL, " ", &last);
    }

    if (calcheck != NULL && strlen(calcheck) >= 2 && !strncmp(calcheck, "NG", 2)) {
        CLOGE("ERR(%s):Cal data has error", __FUNCTION__);
        ret = false;
    }

    return ret;
}
#endif

#ifdef SAMSUNG_COMPANION
void ExynosCamera1Parameters::setUseCompanion(bool use)
{
    m_use_companion = use;
}

bool ExynosCamera1Parameters::getUseCompanion()
{
    return m_use_companion;
}
#endif

void ExynosCamera1Parameters::setIsThumbnailCallbackOn(bool enable)
{
    m_IsThumbnailCallbackOn = enable;
}

bool ExynosCamera1Parameters::getIsThumbnailCallbackOn()
{
    return m_IsThumbnailCallbackOn;
}
}; /* namespace android */
