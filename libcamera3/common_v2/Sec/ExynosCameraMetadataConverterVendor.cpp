/*
 * Copyright (C) 2017, Samsung Electronics Co. LTD
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraMetadataConverterVendor"

#include "ExynosCameraMetadataConverter.h"
#include "ExynosCameraRequestManager.h"

#ifdef SAMSUNG_TN_FEATURE
#include "SecCameraVendorTags.h"
#endif

namespace android {

void ExynosCameraMetadataConverter::m_constructVendorDefaultRequestSettings(__unused int type, __unused CameraMetadata *settings)
{
#ifdef SAMSUNG_TN_FEATURE
    if (m_cameraId == CAMERA_ID_SECURE) {
        /* gain */
        settings->update(SAMSUNG_ANDROID_SENSOR_GAIN, &(m_sensorStaticInfo->gain), 1);

        /* current */
        settings->update(SAMSUNG_ANDROID_LED_CURRENT, &(m_sensorStaticInfo->ledCurrent), 1);

        /* pulseDelay */
        settings->update(SAMSUNG_ANDROID_LED_PULSE_DELAY, &(m_sensorStaticInfo->ledPulseDelay), 1);

        /* pulseWidth */
        settings->update(SAMSUNG_ANDROID_LED_PULSE_WIDTH, &(m_sensorStaticInfo->ledPulseWidth), 1);

        /* maxTime */
        settings->update(SAMSUNG_ANDROID_LED_MAX_TIME, &(m_sensorStaticInfo->ledMaxTime), 1);

        return;
    }
#endif

#ifdef SAMSUNG_TN_FEATURE
#ifdef USE_ALWAYS_FD_ON
    if (m_configurations->getMode(CONFIGURATION_PIP_MODE) == false) {
        m_configurations->setMode(CONFIGURATION_ALWAYS_FD_ON_MODE, true);
    }
#endif

#ifdef SAMSUNG_PAF
    /* PAF */
    int32_t pafmode = SAMSUNG_ANDROID_CONTROL_PAF_MODE_ON;
    settings->update(SAMSUNG_ANDROID_CONTROL_PAF_MODE, &pafmode, 1);
#endif

#ifdef SAMSUNG_RTHDR
    /* RT - HDR */
    int32_t hdrmode;
    if (m_cameraId == CAMERA_ID_FRONT && m_configurations->getSamsungCamera() == false)
        hdrmode = SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_AUTO;
    else
        hdrmode = SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_OFF;
    settings->update(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE, &hdrmode, 1);

     /* Hdr level */
    int32_t hdrlevel = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL, &hdrlevel, 1);
#endif

#ifdef SAMSUNG_OIS
    /* OIS */
    const uint8_t oismode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_ON;
    settings->update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE, &oismode, 1);

    int32_t vendoroismode = SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_PICTURE;
    switch (type) {
    case CAMERA3_TEMPLATE_PREVIEW:
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
    case CAMERA3_TEMPLATE_MANUAL:
        vendoroismode = SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_PICTURE;
        break;
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        vendoroismode = SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_VIDEO;
        break;
    default:
        CLOGD("Custom intent type is selected for setting control intent(%d)", type);
        break;
    }
    settings->update(SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE, &vendoroismode, 1);
#endif

#ifdef SAMSUNG_CONTROL_METERING
    /* Metering */
    int32_t meteringmode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_CENTER;
    switch (type) {
    case CAMERA3_TEMPLATE_PREVIEW:
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
    case CAMERA3_TEMPLATE_MANUAL:
        meteringmode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_CENTER;
        break;
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        meteringmode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_MATRIX;
        break;
    default:
        CLOGD("Custom intent type is selected for setting control intent(%d)", type);
        break;
    }
    settings->update(SAMSUNG_ANDROID_CONTROL_METERING_MODE, &meteringmode, 1);
#endif

    /* Shooting mode */
    int32_t shootingmode = SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SINGLE;
    settings->update(SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE, &shootingmode, 1);

    /* Light condition Enable Mode */
    int32_t lightConditionEnableMode = SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION_ENABLE_SIMPLE;
    settings->update(SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION_ENABLE_MODE, &lightConditionEnableMode, 1);

    /* Color Temperature */
    int32_t colortemp = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE, &colortemp, 1);

    /* WbLevel */
    int32_t wbLevel = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_WBLEVEL, &wbLevel, 1);

    /* Flip mode */
    int32_t flipmode = SAMSUNG_ANDROID_SCALER_FLIP_MODE_NONE;
    settings->update(SAMSUNG_ANDROID_SCALER_FLIP_MODE, &flipmode, 1);

    /* Capture hint */
    int32_t capturehint = SAMSUNG_ANDROID_CONTROL_CAPTURE_HINT_NONE;
    settings->update(SAMSUNG_ANDROID_CONTROL_CAPTURE_HINT, &capturehint, 1);

    /* lens pos */
    int32_t focusLensPos = 0;
    settings->update(SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS, &focusLensPos, 1);

#ifdef SAMSUNG_DOF
    /* lens pos stall */
    int32_t focusLensPosStall = 0;
    settings->update(SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS_STALL, &focusLensPosStall, 1);
#endif

#ifdef SUPPORT_MULTI_AF
    /* multiAfMode */
    uint8_t multiAfMode = (uint8_t)SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE_OFF;
    settings->update(SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE, &multiAfMode, 1);
#endif

    /* transientAction */
    int32_t transientAction = SAMSUNG_ANDROID_CONTROL_TRANSIENT_ACTION_NONE;
    settings->update(SAMSUNG_ANDROID_CONTROL_TRANSIENTACTION, &transientAction, 1);

#ifdef SAMSUNG_HYPERLAPSE
    /* recording motion speed mode*/
    int32_t motionSpeedMode = SAMSUNG_ANDROID_CONTROL_RECORDING_MOTION_SPEED_MODE_AUTO;
    settings->update(SAMSUNG_ANDROID_CONTROL_RECORDING_MOTION_SPEED_MODE, &motionSpeedMode, 1);
#endif

#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
    /* dual camera disable */
    uint8_t dualCameraDisable = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_DUAL_CAMERA_DISABLE, &dualCameraDisable, 1);
#endif

#ifdef SAMSUNG_DUAL_PORTRAIT_SOLUTION
    int32_t bokehBlurStrength = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_BOKEH_BLUR_STRENGTH, &bokehBlurStrength, 1);

    int32_t zoomInOutPhoto = SAMSUNG_ANDROID_CONTROL_ZOOM_IN_OUT_PHOTO_OFF;
    settings->update(SAMSUNG_ANDROID_CONTROL_ZOOM_IN_OUT_PHOTO, &zoomInOutPhoto, 1);
#endif

#endif /* SAMSUNG_TN_FEATURE*/

#ifdef SAMSUNG_SSM
    /* super slow state */
    int32_t state = SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_STATE_READY;
    settings->update(SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_STATE, &state, 1);
#endif

#if defined(SAMSUNG_VIDEO_BEAUTY) || defined(SAMSUNG_DUAL_PORTRAIT_BEAUTY)
    int32_t beautyFaceRetouchLevel = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_RETOUCH_LEVEL, &beautyFaceRetouchLevel, 1);
#endif

#ifdef SAMSUNG_DUAL_PORTRAIT_BEAUTY
    int32_t beautyFaceSkinColorLevel = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_SKIN_COLOR_LEVEL, &beautyFaceSkinColorLevel, 1);
#endif

#ifdef SAMSUNG_TN_FEATURE
    int32_t repeatingRequestHint = SAMSUNG_ANDROID_CONTROL_REPEATING_REQUEST_HINT_NONE;
    settings->update(SAMSUNG_ANDROID_CONTROL_REPEATING_REQUEST_HINT, &repeatingRequestHint, 1);

    int64_t latestPreviewTimestamp = 0;
    settings->update(SAMSUNG_ANDROID_CONTROL_LATEST_PREVIEW_TIMESTAMP, &latestPreviewTimestamp, 1);

    const int64_t sceneDetectInfo[7] = {0, 0, 0, 0, 0, 0, 0};
    settings->update(SAMSUNG_ANDROID_CONTROL_SCENE_DETECTION_INFO, sceneDetectInfo, ARRAY_LENGTH(sceneDetectInfo));
#endif

    return;
}

void ExynosCameraMetadataConverter::m_constructVendorStaticInfo(__unused struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                __unused CameraMetadata *info, __unused int cameraId)
{
    Vector<int32_t> i32Vector;
    Vector<uint8_t> ui8Vector;

#ifdef SAMSUNG_TN_FEATURE
    status_t ret = NO_ERROR;

    /* samsung.android.control.aeAvailableModes */
    ui8Vector.clear();
    if (m_createVendorControlAvailableAeModeConfigurations(sensorStaticInfo, &ui8Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_AE_AVAILABLE_MODES,
                            ui8Vector.array(), ui8Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_CONTROL_AE_AVAILABLE_MODES update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_AE_AVAILABLE_MODES is NULL");
    }

    /* samsung.android.control.awbAvailableModes */
    if (sensorStaticInfo->vendorAwbModes != NULL) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_AWB_AVAILABLE_MODES,
                sensorStaticInfo->vendorAwbModes,
                sensorStaticInfo->vendorAwbModesLength);
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_CONTROL_AWB_AVAILABLE_MODES update failed(%d)", ret);
    } else {
        CLOGD2("vendorAwbModes at sensorStaticInfo is NULL");
    }

    /* samsung.android.control.afAvailableModes */
    ui8Vector.clear();
    if (m_createVendorControlAvailableAfModeConfigurations(sensorStaticInfo, &ui8Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_AF_AVAILABLE_MODES,
                            ui8Vector.array(), ui8Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_CONTROL_AF_AVAILABLE_MODES update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_AF_AVAILABLE_MODES is NULL");
    }

    /* samsung.android.control.colorTemperature */
    ret = info->update(SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE_RANGE,
            sensorStaticInfo->vendorWbColorTempRange,
            ARRAY_LENGTH(sensorStaticInfo->vendorWbColorTempRange));
    if (ret < 0) {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE_RANGE update failed(%d)", ret);
    }

    /* samsung.android.control.wbLevelRange */
    ret = info->update(SAMSUNG_ANDROID_CONTROL_WBLEVEL_RANGE,
            sensorStaticInfo->vendorWbLevelRange,
            ARRAY_LENGTH(sensorStaticInfo->vendorWbLevelRange));
    if (ret < 0) {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_WBLEVEL_RANGE update failed(%d)", ret);
    }

#ifdef SAMSUNG_RTHDR
    /* samsung.android.control.liveHdrLevelRange */
    ret = info->update(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL_RANGE,
            sensorStaticInfo->vendorHdrRange,
            ARRAY_LENGTH(sensorStaticInfo->vendorHdrRange));
    if (ret < 0)
        CLOGD2("SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL_RANGE update failed(%d)", ret);

    /* samsung.android.control.liveHdrMode */
    ret = info->update(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_AVAILABLE_MODES,
            sensorStaticInfo->vendorHdrModes,
            sensorStaticInfo->vendorHdrModesLength);
    if (ret < 0)
        CLOGD2("SAMSUNG_ANDROID_CONTROL_LIVE_HDR_AVAILABLE_MODES update failed(%d)", ret);
#endif

#ifdef SAMSUNG_PAF
    /* samsung.android.control.pafAvailableMode */
    ret = info->update(SAMSUNG_ANDROID_CONTROL_PAF_AVAILABLE_MODE,
            &(sensorStaticInfo->vendorPafAvailable), 1);
    if (ret < 0)
        CLOGD2("SAMSUNG_ANDROID_CONTROL_PAF_AVAILABLE_MODE update failed(%d)", ret);
#endif

#ifdef SAMSUNG_CONTROL_METERING
    /* samsung.android.control.meteringAvailableMode */
    if (sensorStaticInfo->vendorMeteringModes != NULL) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_METERING_AVAILABLE_MODE,
                sensorStaticInfo->vendorMeteringModes,
                sensorStaticInfo->vendorMeteringModesLength);
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_CONTROL_METERING_AVAILABLE_MODE update failed(%d)", ret);
    } else {
        CLOGD2("vendorMeteringModes at sensorStaticInfo is NULL");
    }
#endif

#ifdef SAMSUNG_OIS
    /* samsung.android.lens.info.availableOpticalStabilizationOperationMode */
    if (sensorStaticInfo->vendorOISModes != NULL) {
        ret = info->update(SAMSUNG_ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION_OPERATION_MODE ,
                sensorStaticInfo->vendorOISModes,
                sensorStaticInfo->vendorOISModesLength);
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION_OPERATION_MODE",
                    "update failed(%d)", ret);
        }
    } else {
        CLOGD2("vendorOISModes at sensorStaticInfo is NULL");
    }
#endif

    /* samsung.android.lens.info.horizontalViewAngle */
    ret = info->update(SAMSUNG_ANDROID_LENS_INFO_HORIZONTAL_VIEW_ANGLES ,
            sensorStaticInfo->horizontalViewAngle,
            SIZE_RATIO_END);
    if (ret < 0) {
        CLOGD2("SAMSUNG_ANDROID_LENS_INFO_HORIZONTAL_VIEW_ANGLES",
            "update failed(%d)", ret);
    }

    /* samsung.android.lens.info.verticalViewAngle */
    ret = info->update(SAMSUNG_ANDROID_LENS_INFO_VERTICAL_VIEW_ANGLE ,
            &(sensorStaticInfo->verticalViewAngle),
            1);
    if (ret < 0) {
        CLOGD2("SAMSUNG_ANDROID_LENS_INFO_VERTICAL_VIEW_ANGLE",
            "update failed(%d)", ret);
    }

    /* samsung.android.control.flipAvailableMode */
    if (sensorStaticInfo->vendorFlipModes != NULL) {
        ret = info->update(SAMSUNG_ANDROID_SCALER_FLIP_AVAILABLE_MODES,
                sensorStaticInfo->vendorFlipModes,
                sensorStaticInfo->vendorFlipModesLength);
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_SCALER_FLIP_AVAILABLE_MODES update failed(%d)", ret);
        }
    } else {
        CLOGD2("vendorFlipModes at sensorStaticInfo is NULL");
    }

    /* samsung.android.control.burstShotFpsRange */
    ret = info->update(SAMSUNG_ANDROID_CONTROL_AVAILABLE_BURST_SHOT_FPS,
            sensorStaticInfo->availableBurstShotFps,
            sensorStaticInfo->availableBurstShotFpsLength);
    if (ret < 0) {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_AVAILABLE_BURST_SHOT_FPS update failed(%d)", ret);
    }

    /* samsung.android.scaler.availableVideoConfigurations */
    i32Vector.clear();
    if (m_createVendorControlAvailableVideoConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_SCALER_AVAILABLE_VIDEO_CONFIGURATIONS,
                            i32Vector.array(), i32Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_VIDEO_CONFIGURATIONS update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_VIDEO_CONFIGURATIONS is NULL");
    }

    /* samsung.android.scaler.availablePreviewStreamConfigurations */
    i32Vector.clear();
    if (m_createVendorControlAvailablePreviewConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_SCALER_AVAILABLE_PREVIEW_STREAM_CONFIGURATIONS,
                            i32Vector.array(), i32Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_PREVIEW_STREAM_CONFIGURATIONS update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_PREVIEW_STREAM_CONFIGURATIONS is NULL");
    }

    /* samsung.android.scaler.availablePictureStreamConfigurations */
    i32Vector.clear();
    if (m_createVendorControlAvailablePictureConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_SCALER_AVAILABLE_PICTURE_STREAM_CONFIGURATIONS,
                            i32Vector.array(), i32Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_PICTURE_STREAM_CONFIGURATIONS update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_PICTURE_STREAM_CONFIGURATIONS is NULL");
    }

    /* samsung.android.scaler.availableThumbnailStreamConfigurations */
    if (sensorStaticInfo->availableThumbnailCallbackSizeList != NULL) {
        i32Vector.clear();
        if (m_createVendorScalerAvailableThumbnailConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR) {
            ret = info->update(SAMSUNG_ANDROID_SCALER_AVAILABLE_THUMBNAIL_STREAM_CONFIGURATIONS,
                                i32Vector.array(), i32Vector.size());
            if (ret < 0) {
                CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_THUMBNAIL_STREAM_CONFIGURATIONS update failed(%d)",
                        ret);
            }
        } else {
            CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_THUMBNAIL_STREAM_CONFIGURATIONS update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_THUMBNAIL_STREAM_CONFIGURATIONS is NULL");
    }

    /* samsung.android.scaler.availableHighSpeedVideoConfiguration */
    i32Vector.clear();
    if (m_createVendorControlAvailableHighSpeedVideoConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_SCALER_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS,
                            i32Vector.array(), i32Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS is NULL");
    }

    /* samsung.android.scaler.availableIrisStreamConfigurations */
    if (sensorStaticInfo->availableIrisSizeList != NULL
        && sensorStaticInfo->availableIrisFormatList != NULL) {
        i32Vector.clear();
        if (m_createVendorScalerAvailableIrisConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR) {
            ret = info->update(SAMSUNG_ANDROID_SCALER_AVAILABLE_IRIS_STREAM_CONFIGURATIONS,
                                i32Vector.array(), i32Vector.size());
            if (ret < 0) {
                CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_IRIS_STREAM_CONFIGURATIONS update failed(%d)",
                        ret);
            }
        } else {
            CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_IRIS_STREAM_CONFIGURATIONS update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_IRIS_STREAM_CONFIGURATIONS is NULL");
    }

    /* samsung.android.lens.info.availableMaxDigitalZoom */
    const float maxZoom = (sensorStaticInfo->maxZoomRatioVendor / 1000);
    ret = info->update(SAMSUNG_ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &maxZoom, 1);
    if (ret < 0) {
        CLOGD2("SAMSUNG_ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM", "update failed(%d)", ret);
    }

#ifdef SUPPORT_DEPTH_MAP
    /* samsung.android.depth.availableDepthStreamConfigurations */
    if (sensorStaticInfo->availableDepthSizeList != NULL
        && sensorStaticInfo->availableDepthFormatList != NULL) {
        i32Vector.clear();
        if (m_createVendorDepthAvailableDepthConfigurations(sensorStaticInfo, &i32Vector) == NO_ERROR) {
            ret = info->update(SAMSUNG_ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS,
                                i32Vector.array(), i32Vector.size());
            if (ret < 0) {
                CLOGD2("SAMSUNG_ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS update failed(%d)", ret);
            }
        } else {
            CLOGD2("SAMSUNG_ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS is NULL");
    }
#endif

#ifdef SUPPORT_MULTI_AF
    /* samsung.android.control.multiAfAvailableModes */
    if (sensorStaticInfo->vendorMultiAfAvailable != NULL) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_MULTI_AF_AVAILABLE_MODES ,
                sensorStaticInfo->vendorMultiAfAvailable,
                sensorStaticInfo->vendorMultiAfAvailableLength);
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_CONTROL_MULTI_AF_AVAILABLE_MODES update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_MULTI_AF_AVAILABLE_MODES is NULL");
    }
#endif

    /* samsung.android.control.availableEffects */
    ui8Vector.clear();
    if (m_createVendorControlAvailableEffectModesConfigurations(sensorStaticInfo, &ui8Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_EFFECT_AVAILABLE_MODES,
                            ui8Vector.array(), ui8Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_CONTROL_EFFECT_AVAILABLE_MODES update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_EFFECT_AVAILABLE_MODES is NULL");
    }

    /* samsung.android.control.availableFeatures */
    i32Vector.clear();
    if (m_createVendorControlAvailableFeatures(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_AVAILABLE_FEATURES,
                            i32Vector.array(), i32Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_CONTROL_AVAILABLE_FEATURES update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_AVAILABLE_FEATURES is NULL");
    }

    if (cameraId == CAMERA_ID_SECURE) {
        /* samsung.android.sensor.info.gainRange */
        ret = info->update(SAMSUNG_ANDROID_SENSOR_INFO_GAIN_RANGE,
                sensorStaticInfo->gainRange,
                ARRAY_LENGTH(sensorStaticInfo->gainRange));
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_SENSOR_INFO_GAIN_RANGE update failed(%d)", ret);

        /* samsung.android.led.currentRangee */
        ret = info->update(SAMSUNG_ANDROID_LED_CURRENT_RANGE,
                sensorStaticInfo->ledCurrentRange,
                ARRAY_LENGTH(sensorStaticInfo->ledCurrentRange));
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_LED_CURRENT_RANGE update failed(%d)", ret);

        /* samsung.android.led.pulseDelayRange */
        ret = info->update(SAMSUNG_ANDROID_LED_PULSE_DELAY_RANGE,
                sensorStaticInfo->ledPulseDelayRange,
                ARRAY_LENGTH(sensorStaticInfo->ledPulseDelayRange));
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_LED_PULSE_DELAY_RANGE update failed(%d)", ret);

        /* samsung.android.led.pulseWidthRange */
        ret = info->update(SAMSUNG_ANDROID_LED_PULSE_WIDTH_RANGE,
                sensorStaticInfo->ledPulseWidthRange,
                ARRAY_LENGTH(sensorStaticInfo->ledPulseWidthRange));
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_LED_PULSE_WIDTH_RANGE update failed(%d)", ret);

        /* samsung.android.led.maxTimeRange */
        ret = info->update(SAMSUNG_ANDROID_LED_LED_MAX_TIME_RANGE,
                sensorStaticInfo->ledMaxTimeRange,
                ARRAY_LENGTH(sensorStaticInfo->ledMaxTimeRange));
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_LED_LED_MAX_TIME_RANGE update failed(%d)", ret);
    }

    /* samsung.android.control.effectAeAvailableTargetFpsRanges */
    i32Vector.clear();
    if (m_createVendorEffectAeAvailableFpsRanges(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_EFFECT_AE_AVAILABLE_TARGET_FPS_RANGE,
                            i32Vector.array(), i32Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_CONTROL_EFFECT_AE_AVAILABLE_TARGET_FPS_RANGE update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_EFFECT_AE_AVAILABLE_TARGET_FPS_RANGE is NULL");
    }

    /* samsung.android.jpeg.availableThumbnailSizes */
    i32Vector.clear();
    if (m_createVendorAvailableThumbnailSizes(sensorStaticInfo, &i32Vector) == NO_ERROR) {
        ret = info->update(SAMSUNG_ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
                            i32Vector.array(), i32Vector.size());
        if (ret < 0) {
            CLOGD2("SAMSUNG_ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES update failed(%d)", ret);
        }
    } else {
        CLOGD2("SAMSUNG_ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES update failed(%d)", ret);
    }
#endif /* SAMSUNG_TN_FEATURE */
    return;
}

status_t ExynosCameraMetadataConverter::initShotVendorData(struct camera2_shot *shot)
{
    /* utrl */
#ifdef SAMSUNG_PAF
    shot->uctl.isModeUd.paf_mode = CAMERA_PAF_ON;
#else
    shot->uctl.isModeUd.paf_mode = CAMERA_PAF_OFF;
#endif
    shot->uctl.isModeUd.wdr_mode = CAMERA_WDR_OFF;
#ifdef SUPPORT_DEPTH_MAP
    if (m_configurations->getMode(CONFIGURATION_DEPTH_MAP_MODE)) {
        shot->uctl.isModeUd.disparity_mode = CAMERA_DISPARITY_CENSUS_CENTER;
    } else
#endif
    {
        shot->uctl.isModeUd.disparity_mode = CAMERA_DISPARITY_OFF;
    }

#ifdef SAMSUNG_TN_FEATURE
    if (m_configurations->getMode(CONFIGURATION_FACTORY_TEST_MODE) == true)
        shot->uctl.opMode = CAMERA_OP_MODE_HAL3_FAC;
    else if (m_configurations->getSamsungCamera() == true)
        shot->uctl.opMode = CAMERA_OP_MODE_HAL3_TW;
    else
#endif
        shot->uctl.opMode = CAMERA_OP_MODE_HAL3_GED;

    shot->uctl.vtMode = (enum camera_vt_mode)m_configurations->getModeValue(CONFIGURATION_VT_MODE);

#ifdef SAMSUNG_TN_FEATURE
    shot->uctl.aaUd.temperatureInfo.usb_thermal = 0xFFFF;

    shot->uctl.productColorInfo = m_configurations->getProductColorInfo();
#endif

    return OK;
}

status_t ExynosCameraMetadataConverter::translateControlControlData(CameraMetadata *settings,
                                                                     struct camera2_shot_ext *dst_ext,
                                                                     struct CameraMetaParameters *metaParameters)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;
    uint32_t vendorAeMode = 0;
    uint32_t vendorAfMode = 0;
    ExynosCameraActivityControl *activityControl;
    ExynosCameraActivityFlash *flashMgr;

#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
    if (m_subPrameters != NULL
        && (m_configurations->getModeValue(CONFIGURATION_DUAL_DISP_CAM_TYPE)
            == UNI_PLUGIN_CAMERA_TYPE_TELE)) {
        activityControl = m_subPrameters->getActivityControl();
    } else
#endif
    {
        activityControl = m_parameters->getActivityControl();
    }

    flashMgr = activityControl->getFlashMgr();
    if (flashMgr == NULL) {
        CLOGE("FlashMgr is NULL!!");
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    translatePreVendorControlControlData(settings, dst_ext);

    /* ANDROID_CONTROL_AE_ANTIBANDING_MODE */
    entry = settings->find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        if (dst->ctl.aa.aeAntibandingMode != AA_AE_ANTIBANDING_OFF) {
            dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode) m_defaultAntibanding;
        }

        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_COLOR_AE_ANTIBANDING_MODE(%d), m_defaultAntibanding(%d)",
                entry.data.u8[0], m_defaultAntibanding);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION */
    entry = settings->find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
    if (entry.count > 0) {
        dst->ctl.aa.aeExpCompensation = (int32_t) (entry.data.i32[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
        if (prev_entry.count > 0) {
            prev_value = (int32_t) (prev_entry.data.i32[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.aeExpCompensation) {
            CLOGD("ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION(%d)",
                dst->ctl.aa.aeExpCompensation);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AE_MODE */
    entry = settings->find(ANDROID_CONTROL_AE_MODE);
    if (entry.count > 0) {
        enum aa_aemode aeMode = AA_AEMODE_OFF;
        enum ExynosCameraActivityFlash::FLASH_REQ flashReq = ExynosCameraActivityFlash::FLASH_REQ_OFF;

        vendorAeMode = entry.data.u8[0];
        aeMode = (enum aa_aemode) FIMC_IS_METADATA(entry.data.u8[0]);

        dst->ctl.aa.aeMode = aeMode;
        dst->uctl.flashMode = CAMERA_FLASH_MODE_OFF;

        switch (aeMode) {
        case AA_AEMODE_ON_AUTO_FLASH_REDEYE:
            metaParameters->m_flashMode = FLASH_MODE_RED_EYE;
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_AUTO;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_RED_EYE;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON_AUTO_FLASH:
            metaParameters->m_flashMode = FLASH_MODE_AUTO;
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_AUTO;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_AUTO;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON_ALWAYS_FLASH:
            metaParameters->m_flashMode = FLASH_MODE_ON;
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_ON;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_ON;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON:
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
        case AA_AEMODE_OFF:
            metaParameters->m_flashMode = FLASH_MODE_OFF;
            m_overrideFlashControl = false;
            break;
        default:
#ifdef SAMSUNG_TN_FEATURE
            if (vendorAeMode > SAMSUNG_ANDROID_CONTROL_AE_MODE_START) {
                translateVendorAeControlControlData(dst_ext, vendorAeMode, &aeMode, &flashReq, metaParameters);
            } else
#endif
            {
                metaParameters->m_flashMode = FLASH_MODE_OFF;
                m_overrideFlashControl = false;
            }
            break;
        }

        CLOGV("flashReq(%d)", flashReq);
        flashMgr->setFlashExposure(aeMode);
        flashMgr->setFlashReq(flashReq, m_overrideFlashControl);

        /* ANDROID_CONTROL_AE_MODE */
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || (uint32_t)prev_value != vendorAeMode) {
            CLOGD("ANDROID_CONTROL_AE_MODE(%d)", vendorAeMode);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AE_LOCK */
    entry = settings->find(ANDROID_CONTROL_AE_LOCK);
    if (entry.count > 0) {
        dst->ctl.aa.aeLock = (enum aa_ae_lock) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_LOCK);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AE_LOCK(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

#ifndef SAMSUNG_CONTROL_METERING // Will be processed in vendor control
    /* ANDROID_CONTROL_AE_REGIONS */
    entry = settings->find(ANDROID_CONTROL_AE_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 aeRegion;

        aeRegion.x1 = entry.data.i32[0];
        aeRegion.y1 = entry.data.i32[1];
        aeRegion.x2 = entry.data.i32[2];
        aeRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.aeRegions[4] = entry.data.i32[4];

        m_convertActiveArrayTo3AARegion(&aeRegion);

        dst->ctl.aa.aeRegions[0] = aeRegion.x1;
        dst->ctl.aa.aeRegions[1] = aeRegion.y1;
        dst->ctl.aa.aeRegions[2] = aeRegion.x2;
        dst->ctl.aa.aeRegions[3] = aeRegion.y2;
        CLOGV("ANDROID_CONTROL_AE_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);

        // If AE region has meaningful value, AE region can be applied to the output image
        if (entry.data.i32[0] && entry.data.i32[1] && entry.data.i32[2] && entry.data.i32[3]) {
            if (dst->ctl.aa.aeMode == AA_AEMODE_CENTER) {
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_CENTER_TOUCH;
            } else if (dst->ctl.aa.aeMode == AA_AEMODE_MATRIX) {
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_MATRIX_TOUCH;
            } else if (dst->ctl.aa.aeMode == AA_AEMODE_SPOT) {
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_SPOT_TOUCH;
            }
            CLOGV("update AA_AEMODE(%d)", dst->ctl.aa.aeMode);
        }
    }
#endif

    /* ANDROID_CONTROL_AWB_REGIONS */
    /* AWB region value would not be used at the f/w,
    because AWB is not related with a specific region */
    entry = settings->find(ANDROID_CONTROL_AWB_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 awbRegion;

        awbRegion.x1 = entry.data.i32[0];
        awbRegion.y1 = entry.data.i32[1];
        awbRegion.x2 = entry.data.i32[2];
        awbRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.awbRegions[4] = entry.data.i32[4];
        m_convertActiveArrayTo3AARegion(&awbRegion);

        dst->ctl.aa.awbRegions[0] = awbRegion.x1;
        dst->ctl.aa.awbRegions[1] = awbRegion.y1;
        dst->ctl.aa.awbRegions[2] = awbRegion.x2;
        dst->ctl.aa.awbRegions[3] = awbRegion.y2;
        CLOGV("ANDROID_CONTROL_AWB_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }

    /* ANDROID_CONTROL_AF_REGIONS */
    entry = settings->find(ANDROID_CONTROL_AF_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 afRegion;

        afRegion.x1 = entry.data.i32[0];
        afRegion.y1 = entry.data.i32[1];
        afRegion.x2 = entry.data.i32[2];
        afRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.afRegions[4] = entry.data.i32[4];
        m_convertActiveArrayTo3AARegion(&afRegion);

        dst->ctl.aa.afRegions[0] = afRegion.x1;
        dst->ctl.aa.afRegions[1] = afRegion.y1;
        dst->ctl.aa.afRegions[2] = afRegion.x2;
        dst->ctl.aa.afRegions[3] = afRegion.y2;
        CLOGV("ANDROID_CONTROL_AF_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }

    /* ANDROID_CONTROL_AE_TARGET_FPS_RANGE */
    entry = settings->find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
    if (entry.count > 0) {
        int32_t prev_fps[2] = {0, };
        uint32_t max_fps, min_fps;

        m_configurations->checkPreviewFpsRange(entry.data.i32[0], entry.data.i32[1]);
        m_configurations->getPreviewFpsRange(&min_fps, &max_fps);
        dst->ctl.aa.aeTargetFpsRange[0] = min_fps;
        dst->ctl.aa.aeTargetFpsRange[1] = max_fps;
        m_maxFps = dst->ctl.aa.aeTargetFpsRange[1];

        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
        if (prev_entry.count > 0) {
            prev_fps[0] = prev_entry.data.i32[0];
            prev_fps[1] = prev_entry.data.i32[1];
            isMetaExist = true;
        }

        if (!isMetaExist || (uint32_t)prev_fps[0] != dst->ctl.aa.aeTargetFpsRange[0] ||
            (uint32_t)prev_fps[1] != dst->ctl.aa.aeTargetFpsRange[1]) {
            CLOGD("ANDROID_CONTROL_AE_TARGET_FPS_RANGE(%d-%d)",
                dst->ctl.aa.aeTargetFpsRange[0], dst->ctl.aa.aeTargetFpsRange[1]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER */
    entry = settings->find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
    if (entry.count > 0) {
        dst->ctl.aa.aePrecaptureTrigger = (enum aa_ae_precapture_trigger) entry.data.u8[0];
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_ae_precapture_trigger) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.aePrecaptureTrigger) {
            CLOGD("ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER(%d)",
                dst->ctl.aa.aePrecaptureTrigger);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AF_MODE */
    entry = settings->find(ANDROID_CONTROL_AF_MODE);
    if (entry.count > 0) {
        int faceDetectionMode = FACEDETECT_MODE_OFF;
        camera_metadata_entry_t fd_entry;

        fd_entry = settings->find(ANDROID_STATISTICS_FACE_DETECT_MODE);
        if (fd_entry.count > 0) {
            faceDetectionMode = (enum facedetect_mode) FIMC_IS_METADATA(fd_entry.data.u8[0]);
        }

        vendorAfMode = entry.data.u8[0];
        dst->ctl.aa.afMode = (enum aa_afmode) FIMC_IS_METADATA(entry.data.u8[0]);

        switch (dst->ctl.aa.afMode) {
        case AA_AFMODE_AUTO:
            if (faceDetectionMode > FACEDETECT_MODE_OFF) {
                dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_FACE);
            } else {
                dst->ctl.aa.vendor_afmode_option = 0x00;
            }
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            break;
        case AA_AFMODE_MACRO:
            dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_MACRO);
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            break;
        case AA_AFMODE_CONTINUOUS_VIDEO:
            dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_VIDEO);
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            /* The afRegion value should be (0,0,0,0) at the Continuous Video mode */
            dst->ctl.aa.afRegions[0] = 0;
            dst->ctl.aa.afRegions[1] = 0;
            dst->ctl.aa.afRegions[2] = 0;
            dst->ctl.aa.afRegions[3] = 0;
            break;
        case AA_AFMODE_CONTINUOUS_PICTURE:
            if (faceDetectionMode > FACEDETECT_MODE_OFF) {
                dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_FACE);
            } else {
                dst->ctl.aa.vendor_afmode_option = 0x00;
            }
            dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            /* The afRegion value should be (0,0,0,0) at the Continuous Picture mode */
            dst->ctl.aa.afRegions[0] = 0;
            dst->ctl.aa.afRegions[1] = 0;
            dst->ctl.aa.afRegions[2] = 0;
            dst->ctl.aa.afRegions[3] = 0;
            break;
        case AA_AFMODE_OFF:
        default:
#ifdef SAMSUNG_TN_FEATURE
            if (vendorAfMode > SAMSUNG_ANDROID_CONTROL_AF_MODE_START) {
                translateVendorAfControlControlData(dst_ext, vendorAfMode);
            } else
#endif
            {
                dst->ctl.aa.vendor_afmode_option = 0x00;
                dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
            }
            break;
        }

        m_preAfMode = m_afMode;
        m_afMode = dst->ctl.aa.afMode;

#ifdef SAMSUNG_DOF
        if (m_configurations->getModeValue(CONFIGURATION_SHOT_MODE) == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELECTIVE_FOCUS) {
            dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_OUT_FOCUSING);
        }
#endif

#ifdef SUPPORT_MULTI_AF
        if (m_flagMultiAf) {
            dst->ctl.aa.vendor_afmode_option |= SET_BIT(AA_AFMODE_OPTION_BIT_MULTI_AF);
        }
#endif

#ifdef SAMSUNG_OT
        if ((dst->ctl.aa.vendor_afmode_option & SET_BIT(AA_AFMODE_OPTION_BIT_OBJECT_TRACKING)) != 0) {
            metaParameters->m_startObjectTracking = true;
        } else {
            metaParameters->m_startObjectTracking = false;
        }
#endif

        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AF_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AF_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AF_TRIGGER */
    entry = settings->find(ANDROID_CONTROL_AF_TRIGGER);
    if (entry.count > 0) {
        dst->ctl.aa.afTrigger = (enum aa_af_trigger)entry.data.u8[0];
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AF_TRIGGER);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AF_TRIGGER(%d)", entry.data.u8[0]);
#ifdef USE_DUAL_CAMERA
            if (entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_START) {
                m_configurations->setDualOperationModeLockCount(10);
            }
#endif
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AWB_LOCK */
    entry = settings->find(ANDROID_CONTROL_AWB_LOCK);
    if (entry.count > 0) {
        dst->ctl.aa.awbLock = (enum aa_awb_lock) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AWB_LOCK);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AWB_LOCK(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AWB_MODE */
    entry = settings->find(ANDROID_CONTROL_AWB_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.awbMode = (enum aa_awbmode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AWB_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_AWB_MODE(%d)", (int32_t)entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_EFFECT_MODE */
    entry = settings->find(ANDROID_CONTROL_EFFECT_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.effectMode = (enum aa_effect_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_EFFECT_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_EFFECT_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_MODE */
    entry = settings->find(ANDROID_CONTROL_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.mode = (enum aa_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != entry.data.u8[0]) {
            CLOGD("ANDROID_CONTROL_MODE(%d)", (int32_t)entry.data.u8[0]);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_MODE check must be prior to ANDROID_CONTROL_SCENE_MODE check */
    /* ANDROID_CONTROL_SCENE_MODE */
    entry = settings->find(ANDROID_CONTROL_SCENE_MODE);
    if (entry.count > 0) {
        uint8_t scene_mode;

        scene_mode = (uint8_t)entry.data.u8[0];
        m_sceneMode = scene_mode;
        setSceneMode(scene_mode, dst_ext);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_SCENE_MODE);
        if (prev_entry.count > 0) {
            prev_value = (uint8_t)prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (m_configurations->getRestartStream() == false) {
            if (isMetaExist && prev_value != scene_mode) {
                if (prev_value == ANDROID_CONTROL_SCENE_MODE_HDR
                    || scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {
                    m_configurations->setRestartStream(true);
                    CLOGD("setRestartStream(SCENE_MODE_HDR)");
                }
            }
        }

        if (!isMetaExist || prev_value != scene_mode) {
            CLOGD("ANDROID_CONTROL_SCENE_MODE(%d)", scene_mode);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_VIDEO_STABILIZATION_MODE */
    entry = settings->find(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.videoStabilizationMode = (enum aa_videostabilization_mode) entry.data.u8[0];
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_videostabilization_mode) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.videoStabilizationMode) {
            CLOGD("ANDROID_CONTROL_VIDEO_STABILIZATION_MODE(%d)",
                dst->ctl.aa.videoStabilizationMode);
        }
        isMetaExist = false;
    }

    enum ExynosCameraActivityFlash::FLASH_STEP flashStep = ExynosCameraActivityFlash::FLASH_STEP_OFF;
    enum ExynosCameraActivityFlash::FLASH_STEP curFlashStep = ExynosCameraActivityFlash::FLASH_STEP_END;
    bool isFlashStepChanged = false;

    flashMgr->getFlashStep(&curFlashStep);

    /* Check Precapture Trigger to turn on the pre-flash */
    switch (dst->ctl.aa.aePrecaptureTrigger) {
    case AA_AE_PRECAPTURE_TRIGGER_START:
        if (flashMgr->getNeedCaptureFlash() == true
            && (flashMgr->getFlashStatus() == AA_FLASHMODE_OFF || curFlashStep == ExynosCameraActivityFlash::FLASH_STEP_OFF)) {
#ifdef SAMSUNG_TN_FEATURE
            if (vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_AUTO_SCREEN_FLASH ||
                vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_ALWAYS_SCREEN_FLASH) {
                flashStep = ExynosCameraActivityFlash::FLASH_STEP_PRE_LCD_ON;
            } else
#endif
            {
                flashStep = ExynosCameraActivityFlash::FLASH_STEP_PRE_START;
            }
            flashMgr->setCaptureStatus(true);
            isFlashStepChanged = true;
#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
            m_configurations->setModeValue(CONFIGURATION_DUAL_HOLD_FALLBACK_STATE, 1);
#endif
        }
        break;
    case AA_AE_PRECAPTURE_TRIGGER_CANCEL:
        if (flashMgr->getNeedCaptureFlash() == true
            && (flashMgr->getFlashStatus() != AA_FLASHMODE_OFF
                    || curFlashStep != ExynosCameraActivityFlash::FLASH_STEP_OFF)
            && (flashMgr->getFlashStatus() != AA_FLASHMODE_CANCEL
                    || curFlashStep != ExynosCameraActivityFlash::FLASH_STEP_CANCEL)) {
            flashStep = ExynosCameraActivityFlash::FLASH_STEP_CANCEL;
            flashMgr->setCaptureStatus(false);
            isFlashStepChanged = true;
#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
            m_configurations->setModeValue(CONFIGURATION_DUAL_HOLD_FALLBACK_STATE, 0);
#endif
        }
        break;
    case AA_AE_PRECAPTURE_TRIGGER_IDLE:
    default:
        break;
    }
    /* Check Capture Intent to turn on the main-flash */

    /* ANDROID_CONTROL_CAPTURE_INTENT */
    entry = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
    if (entry.count > 0) {
        dst->ctl.aa.captureIntent = (enum aa_capture_intent) entry.data.u8[0];
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_CAPTURE_INTENT);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_capture_intent) prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.captureIntent) {
            CLOGD("ANDROID_CONTROL_CAPTURE_INTENT(%d)",
                dst->ctl.aa.captureIntent);
        }
        isMetaExist = false;
    }

    switch (dst->ctl.aa.captureIntent) {
    case AA_CAPTURE_INTENT_STILL_CAPTURE:
        if (flashMgr->getNeedCaptureFlash() == true) {
            flashMgr->setNeedFlashMainStart(false);
            flashMgr->getFlashStep(&curFlashStep);
            if (curFlashStep == ExynosCameraActivityFlash::FLASH_STEP_OFF) {
                CLOGD("curFlashStep(%d) : flash off", curFlashStep);
                flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_OFF);
                flashMgr->setFlashReq(ExynosCameraActivityFlash::FLASH_REQ_OFF);
                m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 0);
            } else {
                isFlashStepChanged = true;
                m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 1);

#ifdef SAMSUNG_TN_FEATURE
                if (vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_AUTO_SCREEN_FLASH ||
                    vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_ALWAYS_SCREEN_FLASH) {
                    flashStep = ExynosCameraActivityFlash::FLASH_STEP_LCD_ON;
                } else
#endif
                {
                    flashStep = ExynosCameraActivityFlash::FLASH_STEP_MAIN_START;
                    flashMgr->setNeedFlashMainStart(true);
                    isFlashStepChanged = false;
                }
            }
        } else {
            m_configurations->setModeValue(CONFIGURATION_MARKING_EXIF_FLASH, 0);
        }
        break;
    case AA_CAPTURE_INTENT_VIDEO_RECORD:
    case AA_CAPTURE_INTENT_CUSTOM:
    case AA_CAPTURE_INTENT_PREVIEW:
    case AA_CAPTURE_INTENT_VIDEO_SNAPSHOT:
    case AA_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
    case AA_CAPTURE_INTENT_MANUAL:
    default:
        break;
    }

    if (isFlashStepChanged == true && flashMgr != NULL)
        flashMgr->setFlashStep(flashStep);

    if (m_configurations->getMode(CONFIGURATION_RECORDING_MODE)
#ifdef SAMSUNG_FACTORY_SSM_TEST
        || m_configurations->getModeValue(CONFIGURATION_OPERATION_MODE) == OPERATION_MODE_SSM_TEST
#endif
    ) {
        dst->ctl.aa.vendor_videoMode = AA_VIDEOMODE_ON;
    }

    /* If aeMode or Mode is NOT Off, Manual AE control can NOT be operated */
    if (dst->ctl.aa.aeMode == AA_AEMODE_OFF
        || dst->ctl.aa.mode == AA_CONTROL_OFF) {
        m_isManualAeControl = true;
        CLOGV("Operate Manual AE Control, aeMode(%d), Mode(%d)",
                dst->ctl.aa.aeMode, dst->ctl.aa.mode);
    } else {
        m_isManualAeControl = false;
    }
    m_configurations->setMode(CONFIGURATION_MANUAL_AE_CONTROL_MODE, m_isManualAeControl);

    translateVendorControlControlData(settings, dst_ext);

    return OK;
}

void ExynosCameraMetadataConverter::translatePreVendorControlControlData(__unused CameraMetadata *settings,
                                                                          struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;

    /* SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE);
    if (entry.count > 0) {
        int32_t shooting_mode = (int32_t)entry.data.i32[0];

        /* HACK : ignore shooting mode for secure camera */
        if (m_cameraId == CAMERA_ID_SECURE)
            m_configurations->setModeValue(CONFIGURATION_SHOT_MODE, SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SINGLE);
        else
            m_configurations->setModeValue(CONFIGURATION_SHOT_MODE, shooting_mode);

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE);
        if (prev_entry.count > 0) {
            prev_value = (int32_t)prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != shooting_mode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE(%d)", shooting_mode);
        }
        isMetaExist = false;

#ifdef USE_ALWAYS_FD_ON
        if (m_configurations->getSamsungCamera() == true
            && m_configurations->getMode(CONFIGURATION_PIP_MODE) == false) {
            switch (shooting_mode) {
            case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SLOW_MOTION:
            case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_HYPER_MOTION:
            case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SUPER_SLOW_MOTION:
                m_configurations->setMode(CONFIGURATION_ALWAYS_FD_ON_MODE, false);
                break;
            default:
                m_configurations->setMode(CONFIGURATION_ALWAYS_FD_ON_MODE, true);
                break;
            }
        }
#endif
    }

#ifdef SUPPORT_MULTI_AF
    /* SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE);
    if (entry.count > 0) {
        uint8_t multiAfMode = (uint8_t) entry.data.u8[0];

        if (multiAfMode) {
            m_flagMultiAf = true;
        } else {
            m_flagMultiAf = false;
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE);
        if (prev_entry.count > 0) {
            prev_value = (uint8_t) prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != multiAfMode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_MULTI_AF_MODE(%d)", multiAfMode);
        }
        isMetaExist = false;
    }
#endif

#ifdef SAMSUNG_HYPERLAPSE
    if (m_configurations->getMode(CONFIGURATION_HYPERLAPSE_MODE) == true) {
        entry = settings->find(SAMSUNG_ANDROID_CONTROL_RECORDING_MOTION_SPEED_MODE);
        if (entry.count > 0) {
            int32_t motionSpeedMode = (int32_t)entry.data.i32[0];

            m_configurations->setModeValue(CONFIGURATION_HYPERLAPSE_SPEED, motionSpeedMode);

            prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_RECORDING_MOTION_SPEED_MODE);
            if (prev_entry.count > 0) {
                prev_value = (int32_t)prev_entry.data.i32[0];
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != motionSpeedMode) {
                CLOGD("SAMSUNG_ANDROID_CONTROL_RECORDING_MOTION_SPEED_MODE(%d)", motionSpeedMode);
            }
            isMetaExist = false;
        }
    }
#endif

    /* SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_RETOUCH_LEVEL */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_RETOUCH_LEVEL);
    if (entry.count > 0) {
        int32_t beautyFaceRetouchLevel = entry.data.i32[0];

#if defined(SAMSUNG_VIDEO_BEAUTY) || defined(SAMSUNG_DUAL_PORTRAIT_BEAUTY)
        m_configurations->setModeValue(CONFIGURATION_BEAUTY_RETOUCH_LEVEL, beautyFaceRetouchLevel);
#endif

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_RETOUCH_LEVEL);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != beautyFaceRetouchLevel) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_RETOUCH_LEVEL(%d)", beautyFaceRetouchLevel);
        }
        isMetaExist = false;
    }

#ifdef SAMSUNG_DUAL_PORTRAIT_BEAUTY
    /* SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_SKIN_COLOR_LEVEL */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_SKIN_COLOR_LEVEL);
    if (entry.count > 0) {
        int32_t beautyFaceSkinColorLevel = entry.data.i32[0];

        m_configurations->setModeValue(CONFIGURATION_BEAUTY_FACE_SKIN_COLOR_LEVEL, beautyFaceSkinColorLevel);

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_SKIN_COLOR_LEVEL);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != beautyFaceSkinColorLevel) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_BEAUTY_FACE_SKIN_COLOR_LEVEL(%d)", beautyFaceSkinColorLevel);
        }
        isMetaExist = false;
    }
#endif /* SAMSUNG_DUAL_PORTRAIT_BEAUTY */
#endif /* SAMSUNG_TN_FEATURE */

    return;
}

void ExynosCameraMetadataConverter::translateVendorControlControlData(__unused CameraMetadata *settings,
							                                         struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

#ifdef SAMSUNG_CONTROL_METERING
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    int32_t prev_value;
    bool isMetaExist = false;

    if (dst->ctl.aa.aeMode != AA_AEMODE_OFF) {
        /* SAMSUNG_ANDROID_CONTROL_METERING_MODE */
        entry = settings->find(SAMSUNG_ANDROID_CONTROL_METERING_MODE);
        if (entry.count > 0) {
            int32_t metering_mode = entry.data.i32[0];

            switch (entry.data.i32[0]) {
            case SAMSUNG_ANDROID_CONTROL_METERING_MODE_CENTER:
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_CENTER;
                break;
            case SAMSUNG_ANDROID_CONTROL_METERING_MODE_SPOT:
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_SPOT;
                break;
            case SAMSUNG_ANDROID_CONTROL_METERING_MODE_MATRIX:
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_MATRIX;
                break;
            case SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_CENTER:
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_CENTER_TOUCH;
                break;
            case SAMSUNG_ANDROID_CONTROL_METERING_MODE_MANUAL:
            case SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_SPOT:
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_SPOT_TOUCH;
                break;
            case SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_MATRIX:
                dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_MATRIX_TOUCH;
                break;
            default:
                break;
            }

            prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_METERING_MODE);
            if (prev_entry.count > 0) {
                prev_value = prev_entry.data.i32[0];
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != metering_mode) {
                CLOGD("SAMSUNG_ANDROID_CONTROL_METERING_MODE(%d, aeMode = %d)",
                    entry.data.i32[0], dst->ctl.aa.aeMode);
            }
            isMetaExist = false;
        }
    }
#endif

#ifdef SAMSUNG_PAF
    /* SAMSUNG_ANDROID_CONTROL_PAF_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_PAF_MODE);
    if (entry.count > 0) {
        int32_t paf_mode = entry.data.i32[0];

        dst->uctl.isModeUd.paf_mode = (enum camera2_paf_mode) FIMC_IS_METADATA(entry.data.i32[0]);
        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_PAF_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != paf_mode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_PAF_MODE(%d)", paf_mode);
        }
        isMetaExist = false;
    }
#endif

#ifdef SAMSUNG_RTHDR
    /* Warning!! Don't change the order of HDR control routines */
    /* HDR control priority : HDR_SCENE (high) > HDR_MODE > HDR_LEVEL (low) */
    /* SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL);
    if (entry.count > 0) {
        int32_t hdr_level = entry.data.i32[0];

        dst->uctl.isModeUd.wdr_mode = (enum camera2_wdr_mode) FIMC_IS_METADATA(entry.data.i32[0]);
        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (m_configurations->getRestartStream() == false) {
            if (isMetaExist && prev_value != hdr_level) {
                m_configurations->setRestartStream(true);
                CLOGD("setRestartStream(LIVE_HDR_LEVEL)");
            }
        }

        if (!isMetaExist || prev_value != hdr_level) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL(%d)", hdr_level);
        }
        isMetaExist = false;
    }

    /* SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE);
    if (entry.count > 0) {
        int32_t hdr_mode = entry.data.i32[0];

        if (entry.data.i32[0] != SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_OFF) {
            dst->uctl.isModeUd.wdr_mode = (enum camera2_wdr_mode) FIMC_IS_METADATA(entry.data.i32[0]);
        }
        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (m_configurations->getSamsungCamera() == false
            && m_configurations->getRestartStream() == false) {
            if (isMetaExist && prev_value != hdr_mode) {
                m_configurations->setRestartStream(true);
                CLOGD("setRestartStream(LIVE_HDR_MODE)");
            }
        }

        if (!isMetaExist || prev_value != hdr_mode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE(%d)", hdr_mode);
        }
        isMetaExist = false;
    }

    /* check scene mode for RT-HDR */
    if (m_sceneMode == ANDROID_CONTROL_SCENE_MODE_HDR && dst->ctl.aa.mode == AA_CONTROL_USE_SCENE_MODE) {
        dst->uctl.isModeUd.wdr_mode = CAMERA_WDR_ON;
    }

#ifdef SUPPORT_WDR_AUTO_LIKE
    if (dst->uctl.isModeUd.wdr_mode == CAMERA_WDR_OFF
        && (m_configurations->getSamsungCamera() == true || m_configurations->getModeValue(CONFIGURATION_VT_MODE) > 0)) {
        if (m_cameraId == CAMERA_ID_BACK) {
            if (m_configurations->getModeValue(CONFIGURATION_SHOT_MODE) != SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_PRO) {
                dst->uctl.isModeUd.wdr_mode = CAMERA_WDR_AUTO_LIKE;
            }
        } else if (m_cameraId == CAMERA_ID_BACK_1) {
#ifdef SAMSUNG_DUAL_PORTRAIT_SOLUTION
            if (m_configurations->getModeValue(CONFIGURATION_SHOT_MODE) == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_LIVE_FOCUS) {
                dst->uctl.isModeUd.wdr_mode = CAMERA_WDR_AUTO_LIKE;
            }
#endif
        }
    }
#endif

    m_parameters->setRTHdr(dst->uctl.isModeUd.wdr_mode);
#ifdef USE_DUAL_CAMERA
    if (m_subPrameters != NULL) {
        m_subPrameters->setRTHdr(dst->uctl.isModeUd.wdr_mode);
    }
#endif
#endif

#ifdef SAMSUNG_CONTROL_METERING
    /* Forcely set metering mode to matrix in HDR ON as requested by IQ team */
    if (dst->uctl.isModeUd.wdr_mode == CAMERA_WDR_ON
        && dst->ctl.aa.aeMode != (enum aa_aemode)AA_AEMODE_MATRIX_TOUCH) {
        dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_MATRIX;
    }
#endif

#ifdef SAMSUNG_FOCUS_PEAKING
    if (m_configurations->getModeValue(CONFIGURATION_SHOT_MODE) == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_PRO) {
        dst->uctl.isModeUd.disparity_mode = CAMERA_DISPARITY_CENSUS_CENTER;
    }
#endif

#ifdef SAMSUNG_TN_FEATURE
    /* SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION_ENABLE_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION_ENABLE_MODE);
    if (entry.count > 0) {
        int32_t lls_mode = entry.data.i32[0];

#ifdef LLS_CAPTURE
        if (lls_mode == SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION_ENABLE_FULL
                && dst->ctl.aa.mode != AA_CONTROL_USE_SCENE_MODE) {
            // To Do : Should use another meta rather than sceneMode to enable lls mode, later.
            dst->ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
            dst->ctl.aa.sceneMode = AA_SCENE_MODE_LLS;
            m_configurations->setMode(CONFIGURATION_LIGHT_CONDITION_ENABLE_MODE, true);
        } else {
            m_configurations->setMode(CONFIGURATION_LIGHT_CONDITION_ENABLE_MODE, false);
        }
#endif

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION_ENABLE_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != lls_mode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION_ENABLE_MODE(%d)", lls_mode);
        }
        isMetaExist = false;
    }

    /* double-check for vendor awb mode */
    /* ANDROID_CONTROL_AWB_MODE */
    entry = settings->find(ANDROID_CONTROL_AWB_MODE);
    if (entry.count > 0) {
        uint8_t cur_value = entry.data.u8[0];

        if (entry.data.u8[0] == SAMSUNG_ANDROID_CONTROL_AWB_MODE_CUSTOM_K) {
            dst->ctl.aa.awbMode = AA_AWBMODE_WB_CUSTOM_K;
        }

        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AWB_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != cur_value) {
            CLOGD("ANDROID_CONTROL_AWB_MODE(%d)", cur_value);
        }
        isMetaExist = false;
    }

    /* SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE);
    if (entry.count > 0) {
        int32_t kelvin = entry.data.i32[0];

        if ((dst->ctl.aa.awbMode == AA_AWBMODE_WB_CUSTOM_K)
            && (checkRangeOfValid(SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE_RANGE, kelvin) == NO_ERROR)) {
            dst->ctl.aa.vendor_awbValue = entry.data.i32[0];
            CLOGV("SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE(%d)", entry.data.i32[0]);
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != kelvin) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE(%d)", kelvin);
        }
        isMetaExist = false;
    }

    /* SAMSUNG_ANDROID_CONTROL_WBLEVEL */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_WBLEVEL);
    if (entry.count > 0) {
        int32_t wbLevel = entry.data.i32[0];

        if ((dst->ctl.aa.awbMode != AA_AWBMODE_WB_CUSTOM_K)
            && (checkRangeOfValid(SAMSUNG_ANDROID_CONTROL_WBLEVEL_RANGE, wbLevel) == NO_ERROR)) {
            dst->ctl.aa.vendor_awbValue = wbLevel + IS_WBLEVEL_DEFAULT + FW_CUSTOM_OFFSET;
            CLOGV("SAMSUNG_ANDROID_CONTROL_WBLEVEL(%d)", entry.data.i32[0]);
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_WBLEVEL);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != wbLevel) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_WBLEVEL(%d)", wbLevel);
        }
        isMetaExist = false;
    }

    // Shooting modes should be checked before ANDROID_CONTROL_AE_REGIONS
    int32_t shooting_mode = m_configurations->getModeValue(CONFIGURATION_SHOT_MODE);
    setShootingMode(shooting_mode, dst_ext);

#ifdef SAMSUNG_CONTROL_METERING
    /* ANDROID_CONTROL_AE_REGIONS */
    entry = settings->find(ANDROID_CONTROL_AE_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 aeRegion;

        aeRegion.x1 = entry.data.i32[0];
        aeRegion.y1 = entry.data.i32[1];
        aeRegion.x2 = entry.data.i32[2];
        aeRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.aeRegions[4] = entry.data.i32[4];

        if (m_configurations->getSamsungCamera() == true) {
            if (dst->ctl.aa.aeMode == AA_AEMODE_SPOT) {
                int hwSensorW,hwSensorH;
                m_parameters->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);

                aeRegion.x1 = hwSensorW/2;
                aeRegion.y1 = hwSensorH/2;
                aeRegion.x2 = hwSensorW/2;
                aeRegion.y2 = hwSensorH/2;
            } else if (dst->ctl.aa.aeMode == AA_AEMODE_CENTER || dst->ctl.aa.aeMode == AA_AEMODE_MATRIX) {
                aeRegion.x1 = 0;
                aeRegion.y1 = 0;
                aeRegion.x2 = 0;
                aeRegion.y2 = 0;
            }
        } else {
            if (entry.data.i32[0] && entry.data.i32[1] && entry.data.i32[2] && entry.data.i32[3]) {
                if (dst->ctl.aa.aeMode == AA_AEMODE_CENTER) {
                    dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_CENTER_TOUCH;
                } else if (dst->ctl.aa.aeMode == AA_AEMODE_MATRIX) {
                    dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_MATRIX_TOUCH;
                } else if (dst->ctl.aa.aeMode == AA_AEMODE_SPOT) {
                    dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_SPOT_TOUCH;
                }
            }
        }
        m_convertActiveArrayTo3AARegion(&aeRegion);

        dst->ctl.aa.aeRegions[0] = aeRegion.x1;
        dst->ctl.aa.aeRegions[1] = aeRegion.y1;
        dst->ctl.aa.aeRegions[2] = aeRegion.x2;
        dst->ctl.aa.aeRegions[3] = aeRegion.y2;
        CLOGV("ANDROID_CONTROL_AE_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    } else {
        if (dst->ctl.aa.aeMode == AA_AEMODE_SPOT_TOUCH)
            dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_CENTER; //default ae
    }
#endif

    /* SAMSUNG_ANDROID_CONTROL_TRANSIENTACTION */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_TRANSIENTACTION);
    if (entry.count > 0) {
        int32_t transientAction = entry.data.i32[0];

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_TRANSIENTACTION);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != transientAction) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_TRANSIENTACTION(%d)", transientAction);
            m_configurations->setModeValue(CONFIGURATION_TRANSIENT_ACTION_MODE, transientAction);
        }
        isMetaExist = false;
    }

#ifdef SAMSUNG_SSM
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_MODE);
    if (entry.count > 0) {
        int32_t mode = entry.data.i32[0];

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }
        if (!isMetaExist || prev_value != mode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_MODE(%d)", mode);
            m_configurations->setModeValue(CONFIGURATION_SSM_MODE_VALUE, mode);
        }
        isMetaExist = false;
    }

    entry = settings->find(SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_TRIGGER);
    if (entry.count > 0) {
        int32_t trigger = 0;

        trigger = entry.data.i32[0];
        if (trigger > 0) {
            m_configurations->setModeValue(CONFIGURATION_SSM_TRIGGER, (int)true);
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_TRIGGER);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }
        if (!isMetaExist || prev_value != trigger) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_TRIGGER(%d)", trigger);
        }
        isMetaExist = false;
    }

    entry = settings->find(SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_AUTO_DETECT_REGIONS);
    if (entry.count > 0) {
        ExynosRect2 aeRegion;

        aeRegion.x1 = entry.data.i32[0];
        aeRegion.y1 = entry.data.i32[1];
        aeRegion.x2 = entry.data.i32[2];
        aeRegion.y2 = entry.data.i32[3];

        m_convertActiveArrayToSSMRegion(&aeRegion);
        m_configurations->setSSMRegion(aeRegion);

        CLOGV("SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_AUTO_DETECT_REGIONS(%d,%d,%d,%d,%d)",
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }
#endif

#ifdef SAMSUNG_DUAL_PORTRAIT_SOLUTION
    /* SAMSUNG_ANDROID_CONTROL_BOKEH_BLUR_STRENGTH */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_BOKEH_BLUR_STRENGTH);
    if (entry.count > 0) {
        int32_t bokehBlurStrength = entry.data.i32[0];

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_BOKEH_BLUR_STRENGTH);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != bokehBlurStrength) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_BOKEH_BLUR_STRENGTH(%d)", bokehBlurStrength);
            m_configurations->setBokehBlurStrength(bokehBlurStrength);
        }
        isMetaExist = false;
    }

    /* SAMSUNG_ANDROID_CONTROL_ZOOM_IN_OUT_PHOTO */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_ZOOM_IN_OUT_PHOTO);
    if (entry.count > 0) {
        int32_t zoomInOutPhoto = entry.data.i32[0];

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_ZOOM_IN_OUT_PHOTO);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != zoomInOutPhoto) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_ZOOM_IN_OUT_PHOTO(%d)", zoomInOutPhoto);
            m_configurations->setZoomInOutPhoto(zoomInOutPhoto);
        }
        isMetaExist = false;
    }
#endif /* SAMSUNG_DUAL_PORTRAIT_SOLUTION */

#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
    /* SAMSUNG_ANDROID_CONTROL_DUAL_CAMERA_DISABLE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_DUAL_CAMERA_DISABLE);
    if (entry.count > 0) {
        uint8_t dualCameraDisable = entry.data.u8[0];

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_DUAL_CAMERA_DISABLE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dualCameraDisable) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_DUAL_CAMERA_DISABLE(%d)", dualCameraDisable);
            m_configurations->setModeValue(CONFIGURATION_DUAL_CAMERA_DISABLE, dualCameraDisable);
        }
        isMetaExist = false;
    }
#endif

    /* SAMSUNG_ANDROID_CONTROL_REPEATING_REQUEST_HINT */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_REPEATING_REQUEST_HINT);
    if (entry.count > 0) {
        int32_t repeatingRequestHint = entry.data.i32[0];

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_REPEATING_REQUEST_HINT);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != repeatingRequestHint) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_REPEATING_REQUEST_HINT(%d)", repeatingRequestHint);
            m_configurations->setRepeatingRequestHint(repeatingRequestHint);
        }
        isMetaExist = false;
    }
#endif /* SAMSUNG_TN_FEATURE */

#ifdef SAMSUNG_TN_FEATURE
    /* SAMSUNG_ANDROID_CONTROL_SCENE_DETECTION_INFO */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_SCENE_DETECTION_INFO);
    if (entry.count > 0) {
        ExynosRect2 sceneObjectRegion;

        dst->uctl.sceneDetectInfoUd.timeStamp = (uint64_t)(entry.data.i64[0]);
        dst->uctl.sceneDetectInfoUd.scene_index = (enum camera2_scene_index)(entry.data.i64[1]);
        dst->uctl.sceneDetectInfoUd.confidence_score = (uint32_t)(entry.data.i64[2]);

        /* entry.data.i32[3] : left */
        /* entry.data.i32[4] : top */
        /* entry.data.i32[5] : width */
        /* entry.data.i32[6] : height */
        if (entry.data.i32[5] > 0 && entry.data.i32[6] > 0) {
            sceneObjectRegion.x1 = (uint32_t)(entry.data.i64[3]);      /* left */
            sceneObjectRegion.y1 = (uint32_t)(entry.data.i64[4]);      /* top */
            sceneObjectRegion.x2 = (uint32_t)(sceneObjectRegion.x1 + entry.data.i32[5] - 1);       /* right */
            sceneObjectRegion.y2 = (uint32_t)(sceneObjectRegion.y1 + entry.data.i32[6] - 1);       /* bottom */

            m_convertActiveArrayTo3AARegion(&sceneObjectRegion);
            dst->uctl.sceneDetectInfoUd.object_roi[0] = sceneObjectRegion.x1;
            dst->uctl.sceneDetectInfoUd.object_roi[1] = sceneObjectRegion.y1;
            dst->uctl.sceneDetectInfoUd.object_roi[2] = sceneObjectRegion.x2;
            dst->uctl.sceneDetectInfoUd.object_roi[3] = sceneObjectRegion.y2;
        } else {
            dst->uctl.sceneDetectInfoUd.object_roi[0] = 0;   /* left */
            dst->uctl.sceneDetectInfoUd.object_roi[1] = 0;   /* top */
            dst->uctl.sceneDetectInfoUd.object_roi[2] = 0;   /* right */
            dst->uctl.sceneDetectInfoUd.object_roi[3] = 0;   /* bottom */
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_SCENE_DETECTION_INFO);
        if (prev_entry.count > 0) {
            if (prev_entry.data.i64[1] != dst->uctl.sceneDetectInfoUd.scene_index) {
                    CLOGD("timestamp(%lld->%lld),scene_index(%lld->%lld),confidence_score(%lld->%lld),\
                                    object_roi(%lld,%lld,%lld,%lld->%lld,%lld,%lld,%lld)",
                        prev_entry.data.i64[0],
                        entry.data.i64[0],
                        prev_entry.data.i64[1],
                        entry.data.i64[1],
                        prev_entry.data.i64[2],
                        entry.data.i64[2],
                        prev_entry.data.i64[3], prev_entry.data.i64[4], prev_entry.data.i64[5], prev_entry.data.i64[6],
                        entry.data.i64[3], entry.data.i64[4], entry.data.i64[5], entry.data.i64[6]);
                }
        }
    }
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorAeControlControlData(struct camera2_shot_ext *dst_ext,
                                                                    __unused uint32_t vendorAeMode,
                                                                    __unused aa_aemode *aeMode,
                                                                    __unused ExynosCameraActivityFlash::FLASH_REQ *flashReq,
                                                                    __unused struct CameraMetaParameters *metaParameters)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    switch(vendorAeMode) {
        case SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_AUTO_SCREEN_FLASH:
            *aeMode = AA_AEMODE_ON_AUTO_FLASH;
            metaParameters->m_flashMode = FLASH_MODE_AUTO;
            *flashReq = ExynosCameraActivityFlash::FLASH_REQ_AUTO;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_AUTO;
            m_overrideFlashControl = true;
            break;
        case SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_ALWAYS_SCREEN_FLASH:
            *aeMode = AA_AEMODE_ON_ALWAYS_FLASH;
            metaParameters->m_flashMode = FLASH_MODE_ON;
            *flashReq = ExynosCameraActivityFlash::FLASH_REQ_ON;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_ON;
            m_overrideFlashControl = true;
            break;
        case SAMSUNG_ANDROID_CONTROL_AE_MODE_OFF_ALWAYS_FLASH:
            *aeMode = AA_AEMODE_ON_ALWAYS_FLASH;
            metaParameters->m_flashMode = FLASH_MODE_ON;
            *flashReq = ExynosCameraActivityFlash::FLASH_REQ_ON;
            dst->ctl.aa.aeMode = AA_AEMODE_OFF;
            dst->uctl.flashMode = CAMERA_FLASH_MODE_ON;
            m_overrideFlashControl = true;
            break;
        default:
            break;
    }
#endif
}

void ExynosCameraMetadataConverter::translateVendorAfControlControlData(struct camera2_shot_ext *dst_ext,
							                                         uint32_t vendorAfMode)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

    switch(vendorAfMode) {
#ifdef SAMSUNG_OT
    case SAMSUNG_ANDROID_CONTROL_AF_MODE_OBJECT_TRACKING_PICTURE:
        dst->ctl.aa.afMode = ::AA_AFMODE_CONTINUOUS_PICTURE;
        dst->ctl.aa.vendor_afmode_option = 0x00
                | SET_BIT(AA_AFMODE_OPTION_BIT_OBJECT_TRACKING);
        dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
        break;
    case SAMSUNG_ANDROID_CONTROL_AF_MODE_OBJECT_TRACKING_VIDEO:
        dst->ctl.aa.afMode = ::AA_AFMODE_CONTINUOUS_VIDEO;
        dst->ctl.aa.vendor_afmode_option = 0x00
                | SET_BIT(AA_AFMODE_OPTION_BIT_VIDEO)
                | SET_BIT(AA_AFMODE_OPTION_BIT_OBJECT_TRACKING);
        dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_OFF;
        break;
#endif
#ifdef SAMSUNG_FIXED_FACE_FOCUS
    case SAMSUNG_ANDROID_CONTROL_AF_MODE_FIXED_FACE:
        dst->ctl.aa.afMode = ::AA_AFMODE_OFF;
        dst->ctl.aa.vendor_afmode_option = 0x00;
        dst->ctl.aa.vendor_afmode_ext = AA_AFMODE_EXT_FIXED_FACE;
        break;
#endif
    default:
        break;
    }

#ifdef SAMSUNG_OT
    if(vendorAfMode == SAMSUNG_ANDROID_CONTROL_AF_MODE_OBJECT_TRACKING_PICTURE
        || vendorAfMode == SAMSUNG_ANDROID_CONTROL_AF_MODE_OBJECT_TRACKING_VIDEO) {
        ExynosRect2 afRegion, newRect;
        ExynosRect cropRegionRect;
        int weight;

        afRegion.x1 = dst->ctl.aa.afRegions[0];
        afRegion.y1 = dst->ctl.aa.afRegions[1];
        afRegion.x2 = dst->ctl.aa.afRegions[2];
        afRegion.y2 = dst->ctl.aa.afRegions[3];
        weight = dst->ctl.aa.afRegions[4];

        m_parameters->getHwBayerCropRegion(&cropRegionRect.w, &cropRegionRect.h,
                                          &cropRegionRect.x, &cropRegionRect.y);
        newRect = convertingHWArea2AndroidArea(&afRegion, &cropRegionRect);

        m_configurations->setObjectTrackingAreas(0, newRect, weight);
    }
#endif
}

void ExynosCameraMetadataConverter::translateVendorLensControlData(__unused CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext,
                                                                    __unused struct CameraMetaParameters *metaParameters)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t prev_entry;
    __unused int32_t prev_value;
    __unused bool isMetaExist = false;
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

#ifdef SAMSUNG_OIS
    /* SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE */
    entry = settings->find(SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE);
    if (entry.count > 0) {
        int zoomRatio = (int)(metaParameters->m_zoomRatio * 1000);
        int vdisMode = dst->ctl.aa.videoStabilizationMode;
        int32_t oismode = (int32_t)entry.data.i32[0];

        if (dst->ctl.lens.opticalStabilizationMode == OPTICAL_STABILIZATION_MODE_STILL) {
            switch (entry.data.i32[0]) {
            case SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_VIDEO:
                if (vdisMode == VIDEO_STABILIZATION_MODE_ON) {
#ifdef SAMSUNG_SW_VDIS_USE_OIS
                    dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_VDIS;
#else
                    dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
#endif
                } else {
#ifdef SAMSUNG_SW_VDIS
                    if (m_configurations->getMode(CONFIGURATION_SWVDIS_MODE) == true) {
#ifdef SAMSUNG_SW_VDIS_USE_OIS
                        dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_VDIS;
#else
                        dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
#endif
                    } else
#endif
#ifdef SAMSUNG_HYPERLAPSE
                    if (m_configurations->getMode(CONFIGURATION_HYPERLAPSE_MODE) == true) {
                        dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
                    } else
#endif
                    {
                        dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_VIDEO;
                    }
                }
                break;
            case SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_PICTURE:
                if (zoomRatio >= 4000) {
                    dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_STILL_ZOOM;
                } else {
                    dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_STILL;
                }
                break;
            case SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_SINE_X:
                dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_SINE_X;
                break;
            case SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE_SINE_Y:
                dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_SINE_Y;
                break;
            }
        } else {
            dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE);
        if (prev_entry.count > 0) {
            prev_value = (int32_t)prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != oismode) {
            CLOGD("SAMSUNG_ANDROID_LENS_OPTICAL_STABILIZATION_OPERATION_MODE(%d)", oismode);
        }
        isMetaExist = false;
    }
#endif

#ifdef SAMSUNG_TN_FEATURE
    entry = settings->find(SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS);
    if (entry.count > 0) {
        int32_t focus_pos = 0;
        focus_pos = (int32_t)entry.data.i32[0];

        if (focus_pos > 0) {
            dst->uctl.lensUd.pos = focus_pos;
            dst->uctl.lensUd.posSize = 10;
            dst->uctl.lensUd.direction = 0;
            dst->uctl.lensUd.slewRate = 0;
        } else {
            dst->uctl.lensUd.pos = 0;
            dst->uctl.lensUd.posSize = 0;
            dst->uctl.lensUd.direction = 0;
            dst->uctl.lensUd.slewRate = 0;
        }

        CLOGV("Focus lens pos : %d", focus_pos);

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS);
        if (prev_entry.count > 0) {
            prev_value = entry.data.i32[0];
            isMetaExist = true;
        }
        if (!isMetaExist || prev_value != focus_pos) {
            CLOGD("SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS(%d)", focus_pos);
        }
        isMetaExist = false;
    }
#endif

#ifdef SAMSUNG_DOF
    entry = settings->find(SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS_STALL);
    if (entry.count > 0) {
        int32_t focus_pos = 0;

        focus_pos = (int32_t)entry.data.i32[0];
        if (focus_pos > 0) {
            m_configurations->setModeValue(CONFIGURATION_FOCUS_LENS_POS, focus_pos);
            CLOGD("Focus lens pos : %d", focus_pos);
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS_STALL);
        if (prev_entry.count > 0) {
            prev_value = entry.data.i32[0];
            isMetaExist = true;
        }
        if (!isMetaExist || prev_value != focus_pos) {
            CLOGD("SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS_STALL(%d)", focus_pos);
        }
        isMetaExist = false;
    }
#endif

#ifdef SAMSUNG_TN_FEATURE
    if (m_isManualAeControl == false) {
        /* ANDROID_LENS_APERTURE */
        entry = settings->find(ANDROID_LENS_APERTURE);
        if (entry.count > 0 ) {
#ifdef SAMSUNG_DUAL_PORTRAIT_APERTURE_VALUE
            int32_t shooting_mode = m_configurations->getModeValue(CONFIGURATION_SHOT_MODE);
            if (shooting_mode == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_LIVE_FOCUS &&
                m_configurations->getMode(CONFIGURATION_FACTORY_TEST_MODE) == false) {
                dst->ctl.lens.aperture = SAMSUNG_DUAL_PORTRAIT_APERTURE_VALUE;
            } else
#endif
            {
                dst->ctl.lens.aperture = (int32_t)(entry.data.f[0] * 100);
            }

            prev_entry = m_prevMeta->find(ANDROID_LENS_APERTURE);
            if (prev_entry.count > 0) {
                prev_value = (int32_t)(prev_entry.data.f[0] * 100);
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != (int32_t)(entry.data.f[0] * 100)) {
                CLOGD("ANDROID_LENS_APERTURE(%d)", (int32_t)(entry.data.f[0] * 100));
            }
            isMetaExist = false;
        }
    }
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorSensorControlData(__unused CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t prev_entry;
    struct camera2_shot *dst = NULL;
    __unused int32_t prev_value;
    __unused bool isMetaExist = false;

    dst = &dst_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    if (m_configurations->getSamsungCamera() == true && m_isManualAeControl == false) {
        /* ANDROID_SENSOR_SENSITIVITY */
        entry = settings->find(ANDROID_SENSOR_SENSITIVITY);
        if (entry.count > 0 ) {
            if ((uint32_t) entry.data.i32[0] != 0) {
                dst->ctl.aa.vendor_isoMode = AA_ISOMODE_MANUAL;
                dst->ctl.sensor.sensitivity = (uint32_t) entry.data.i32[0];
                dst->ctl.aa.vendor_isoValue = (uint32_t) entry.data.i32[0];
            } else {
                dst->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
                dst->ctl.sensor.sensitivity = 0;
                dst->ctl.aa.vendor_isoValue = 0;
            }

            prev_entry = m_prevMeta->find(ANDROID_SENSOR_SENSITIVITY);
            if (prev_entry.count > 0) {
                prev_value = (uint32_t) (prev_entry.data.i32[0]);
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != dst->ctl.sensor.sensitivity) {
                CLOGD("ANDROID_SENSOR_SENSITIVITY(%d)",
                    dst->ctl.sensor.sensitivity);
            }
            isMetaExist = false;
        }
    }

    if ((m_configurations->getSamsungCamera() == true)
        && settings->exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
        /* Capture exposure time (us) */
        dst->ctl.aa.vendor_captureExposureTime = (uint32_t)(dst->ctl.sensor.exposureTime / 1000);

        if (dst->ctl.sensor.exposureTime > (uint64_t)(CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT)*1000) {
            dst->ctl.sensor.exposureTime = (uint64_t)(CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT)*1000;
            dst->ctl.sensor.frameDuration = dst->ctl.sensor.exposureTime;
        }

        m_configurations->setCaptureExposureTime(dst->ctl.aa.vendor_captureExposureTime);

        CLOGV("CaptureExposureTime(%d) PreviewExposureTime(%lld)",
            dst->ctl.aa.vendor_captureExposureTime,
            dst->ctl.sensor.exposureTime);
    }

    if (m_cameraId == CAMERA_ID_SECURE) {
        if (m_isManualAeControl == true) {
            m_configurations->setExposureTime((int64_t)dst->ctl.sensor.exposureTime);
        }

        entry = settings->find(SAMSUNG_ANDROID_SENSOR_GAIN);
        if (entry.count > 0) {
            int32_t gain = entry.data.i32[0];
            int32_t prev_value = 0L;
            prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_SENSOR_GAIN);
            if (prev_entry.count > 0) {
                prev_value = prev_entry.data.i32[0];
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != gain) {
                CLOGD("SAMSUNG_ANDROID_SENSOR_GAIN(%d)", gain);
                m_configurations->setGain(gain);
            }
            isMetaExist = false;
        }
    }
#endif /* SAMSUNG_TN_FEATURE */

    return;
}

void ExynosCameraMetadataConverter::translateVendorLedControlData(__unused CameraMetadata *settings,
                                                                  struct camera2_shot_ext *dst_ext)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t prev_entry;
    __unused int32_t prev_value;
    __unused bool isMetaExist = false;

    struct camera2_shot *dst = NULL;
    dst = &dst_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    if (m_cameraId == CAMERA_ID_SECURE) {
        entry = settings->find(SAMSUNG_ANDROID_LED_CURRENT);
        if (entry.count > 0) {
            int32_t current = entry.data.i32[0];
            int32_t prev_value = 0L;
            prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_LED_CURRENT);
            if (prev_entry.count > 0) {
                prev_value = prev_entry.data.i32[0];
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != current) {
                CLOGD("SAMSUNG_ANDROID_LED_CURRENT(%d)", current);
                m_configurations->setLedCurrent(current);
            }
            isMetaExist = false;
        }

        entry = settings->find(SAMSUNG_ANDROID_LED_PULSE_DELAY);
        if (entry.count > 0) {
            int64_t pulseDelay = entry.data.i64[0];
            int64_t prev_value = 0L;
            prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_LED_PULSE_DELAY);
            if (prev_entry.count > 0) {
                prev_value = prev_entry.data.i64[0];
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != pulseDelay) {
                CLOGD("SAMSUNG_ANDROID_LED_PULSE_DELAY(%lld)", pulseDelay);
                m_configurations->setLedPulseDelay(pulseDelay);
            }
            isMetaExist = false;
        }

        entry = settings->find(SAMSUNG_ANDROID_LED_PULSE_WIDTH);
        if (entry.count > 0) {
            int64_t pulseWidth = entry.data.i64[0];
            int64_t prev_value = 0L;
            prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_LED_PULSE_WIDTH);
            if (prev_entry.count > 0) {
                prev_value = prev_entry.data.i64[0];
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != pulseWidth) {
                CLOGD("SAMSUNG_ANDROID_LED_PULSE_WIDTH(%lld)", pulseWidth);
                m_configurations->setLedPulseWidth(pulseWidth);
            }
            isMetaExist = false;
        }

        entry = settings->find(SAMSUNG_ANDROID_LED_MAX_TIME);
        if (entry.count > 0) {
            int32_t maxTime = entry.data.i32[0];
            int32_t prev_value = 0L;
            prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_LED_MAX_TIME);
            if (prev_entry.count > 0) {
                prev_value = prev_entry.data.i32[0];
                isMetaExist = true;
            }

            if (!isMetaExist || prev_value != maxTime) {
                CLOGD("SAMSUNG_ANDROID_LED_MAX_TIME(%d)", maxTime);
                m_configurations->setLedMaxTime(maxTime);
            }
            isMetaExist = false;
        }
    }
#endif /* SAMSUNG_TN_FEATURE */

    return;
}

void ExynosCameraMetadataConverter::translateVendorControlMetaData(__unused CameraMetadata *settings,
                                                                   struct camera2_shot_ext *src_ext,
                                                                   __unused ExynosCameraRequestSP_sprt_t request)
{
    struct camera2_shot *src = NULL;
    __unused camera_metadata_entry_t entry;

    src = &src_ext->shot;

#ifdef SAMSUNG_CONTROL_METERING
    int32_t vendorAeMode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_CENTER;
    switch (src->dm.aa.aeMode) {
    case AA_AEMODE_CENTER:
        vendorAeMode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_CENTER;
        break;
    case AA_AEMODE_SPOT:
        vendorAeMode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_SPOT;
        break;
    case AA_AEMODE_MATRIX:
        vendorAeMode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_MATRIX;
        break;
    case AA_AEMODE_CENTER_TOUCH:
        vendorAeMode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_CENTER;
        break;
    case AA_AEMODE_SPOT_TOUCH:
        entry = settings->find(SAMSUNG_ANDROID_CONTROL_METERING_MODE);
        if (entry.count > 0) {
            vendorAeMode = entry.data.i32[0];
        } else {
            vendorAeMode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_SPOT;
        }
        break;
    case AA_AEMODE_MATRIX_TOUCH:
        vendorAeMode = SAMSUNG_ANDROID_CONTROL_METERING_MODE_WEIGHTED_MATRIX;
        break;
    default:
        break;
    }
    settings->update(SAMSUNG_ANDROID_CONTROL_METERING_MODE, &vendorAeMode, 1);
    CLOGV("vendorAeMode(%d)", vendorAeMode);
#endif

#ifdef SAMSUNG_RTHDR
    /* SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE);
    if (entry.count > 0) {
        int32_t vendorHdrState = (int32_t) CAMERA_METADATA(src->dm.stats.vendor_wdrAutoState);
        settings->update(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_STATE, &vendorHdrState, 1);
        CLOGV("dm.stats.vendor_wdrAutoState(%d)", src->dm.stats.vendor_wdrAutoState);
    }
#endif

#ifdef SAMSUNG_TN_FEATURE
    int32_t light_condition = m_configurations->getLightCondition();
    settings->update(SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION, &light_condition, 1);
    CLOGV(" light_condition(%d)", light_condition);

    int32_t burstShotFps = m_configurations->getModeValue(CONFIGURATION_BURSTSHOT_FPS);
    int32_t captureHint = SAMSUNG_ANDROID_CONTROL_CAPTURE_HINT_NONE;
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_CAPTURE_HINT);
    if (entry.count > 0) {
        captureHint = entry.data.i32[0];
    }
    /* stop reporting dynamic burstshot fps while burstshot is in progress */
    if (captureHint != SAMSUNG_ANDROID_CONTROL_CAPTURE_HINT_BURST
        && captureHint != SAMSUNG_ANDROID_CONTROL_CAPTURE_HINT_AGIF) {
        settings->update(SAMSUNG_ANDROID_CONTROL_BURST_SHOT_FPS, &burstShotFps, 1);
        CLOGV(" burstShotFps(%d)", burstShotFps);
    }

    int32_t exposure_value = (int32_t) src->dm.aa.vendor_exposureValue;
    settings->update(SAMSUNG_ANDROID_CONTROL_EV_COMPENSATION_VALUE, &exposure_value, 1);
    CLOGV(" exposure_value(%d)", exposure_value);

    int32_t beauty_scene_index = (int32_t) src->udm.ae.vendorSpecific[395];
    settings->update(SAMSUNG_ANDROID_CONTROL_BEAUTY_SCENE_INDEX, &beauty_scene_index, 1);
    CLOGV(" beauty_scene_index(%d)", beauty_scene_index);

#ifdef SAMSUNG_IDDQD
    uint8_t lens_dirty_detect = (uint8_t) m_configurations->getIDDQDresult();
    settings->update(SAMSUNG_ANDROID_CONTROL_LENS_DIRTY_DETECT, &lens_dirty_detect, 1);
    CLOGV(" lens_dirty_detect(%d)", lens_dirty_detect);
#else
    uint8_t lens_dirty_detect = (uint8_t) false;
    settings->update(SAMSUNG_ANDROID_CONTROL_LENS_DIRTY_DETECT, &lens_dirty_detect, 1);
    CLOGV(" lens_dirty_detect(%d)", lens_dirty_detect);
#endif

#ifdef SAMSUNG_FACTORY_DRAM_TEST
    if (m_configurations->getModeValue(CONFIGURATION_OPERATION_MODE) == OPERATION_MODE_DRAM_TEST) {
        uint8_t facotry_dram_test_state = (uint8_t) m_configurations->getModeValue(CONFIGURATION_FACTORY_DRAM_TEST_STATE);
        settings->update(SAMSUNG_ANDROID_CONTROL_FACTORY_DRAM_TEST_STATE, &facotry_dram_test_state, 1);
        CLOGV(" facotry_dram_test_state(%d)", facotry_dram_test_state);
    }
#endif

    int32_t brightness_value = (int32_t) src->udm.internal.vendorSpecific[2];
    settings->update(SAMSUNG_ANDROID_CONTROL_BRIGHTNESS_VALUE, &brightness_value, 1);
    CLOGV(" brightness_value(%d)", brightness_value);

#ifdef SAMSUNG_SSM
    int32_t ssm_state = m_configurations->getModeValue(CONFIGURATION_SSM_STATE);
    settings->update(SAMSUNG_ANDROID_CONTROL_SUPER_SLOW_MOTION_STATE, &ssm_state, 1);
    CLOGV("ssm_state(%d)", ssm_state);
#endif

#ifdef SAMSUNG_DUAL_PORTRAIT_SOLUTION
    if (m_configurations->getScenario() == SCENARIO_DUAL_REAR_PORTRAIT
        || m_configurations->getScenario() == SCENARIO_DUAL_FRONT_PORTRAIT) {
        int32_t captureIntent = 0;
        int32_t bokehState = 0, bokehCaptureState = 0;

        entry = settings->find(ANDROID_CONTROL_CAPTURE_INTENT);
        if (entry.count > 0) {
            captureIntent = (enum aa_capture_intent) entry.data.u8[0];
        }
        if (captureIntent == AA_CAPTURE_INTENT_STILL_CAPTURE) {
            bokehState = (int32_t) m_configurations->getBokehCaptureState();
            if (bokehState == CONTROL_BOKEH_STATE_CAPTURE_FAIL) {
                bokehCaptureState = SAMSUNG_ANDROID_CONTROL_BOKEH_STATE_CAPTURE_PROCESSING_FAIL;
            } else {
                bokehCaptureState = SAMSUNG_ANDROID_CONTROL_BOKEH_STATE_CAPTURE_PROCESSING_SUCCESS;
            }
            settings->update(SAMSUNG_ANDROID_CONTROL_BOKEHSTATE, &bokehCaptureState, 1);
            CLOGD("BokehCapture bokehState(%d) bokehCaptureState(%d)",
                bokehState, bokehCaptureState);
        } else if (captureIntent == AA_CAPTURE_INTENT_PREVIEW) {
            bokehState = (int32_t) m_configurations->getBokehPreviewState();
            settings->update(SAMSUNG_ANDROID_CONTROL_BOKEHSTATE, &bokehState, 1);
            CLOGV("bokehState(%d)", bokehState);
        }
    }
#endif

    /* SAMSUNG_ANDROID_CONTROL_SCENE_DETECTION_INFO */
    int64_t sceneDetectionInfo[7] = {0, };

    sceneDetectionInfo[0] = (int64_t)src->uctl.sceneDetectInfoUd.timeStamp;
    sceneDetectionInfo[1] = (int64_t)src->udm.scene_index;  /* UDM */
    sceneDetectionInfo[2] = (int64_t)src->uctl.sceneDetectInfoUd.confidence_score;
    sceneDetectionInfo[3] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[0];
    sceneDetectionInfo[4] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[1];
    sceneDetectionInfo[5] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[2];
    sceneDetectionInfo[6] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[3];

    settings->update(SAMSUNG_ANDROID_CONTROL_SCENE_DETECTION_INFO,
                    sceneDetectionInfo,
                    sizeof(sceneDetectionInfo)/sizeof(int64_t));
    CLOGV("sceneDetectionInfo : scene index(%lld)", sceneDetectionInfo[1]);
#endif

#ifdef SAMSUNG_STR_BV_OFFSET
    int32_t strBvOffset = (int32_t) request->getBvOffset();
    settings->update(SAMSUNG_ANDROID_CONTROL_STR_BV_OFFSET, &strBvOffset, 1);
    CLOGV("strBvOffset(%d)", strBvOffset);
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorJpegMetaData(__unused CameraMetadata *settings)
{
#ifdef SAMSUNG_TN_FEATURE
    String8 uniqueIdStr;
    ExynosCameraParameters *parameters;

#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
    if (m_subPrameters != NULL
        && (m_configurations->getModeValue(CONFIGURATION_DUAL_DISP_CAM_TYPE)
            == UNI_PLUGIN_CAMERA_TYPE_TELE)) {
        parameters = m_subPrameters;
    } else
#endif
    {
        parameters = m_parameters;
    }

    uniqueIdStr = String8::format("%s", parameters->getImageUniqueId());

    settings->update(SAMSUNG_ANDROID_JPEG_IMAGE_UNIQUE_ID, uniqueIdStr);
    CLOGV("imageUniqueId is %s", uniqueIdStr.c_str());
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorLensMetaData(__unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    settings->update(SAMSUNG_ANDROID_LENS_INFO_FOCALLENGTH_IN_35MM_FILM,
        &(m_sensorStaticInfo->focalLengthIn35mmLength), 1);
    CLOGV("focalLengthIn35mmLength is (%d)",
        m_sensorStaticInfo->focalLengthIn35mmLength);
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorScalerControlData(__unused CameraMetadata *settings,
                                                                 __unused struct camera2_shot_ext *src_ext)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t prev_entry;
    __unused bool isMetaExist = false;

#ifdef SAMSUNG_TN_FEATURE
    /* SAMSUNG_ANDROID_SCALER_FLIP_MODE */
    entry = settings->find(SAMSUNG_ANDROID_SCALER_FLIP_MODE);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
        case SAMSUNG_ANDROID_SCALER_FLIP_MODE_HFLIP:
            m_configurations->setModeValue(CONFIGURATION_FLIP_HORIZONTAL, 1);
            break;
        case SAMSUNG_ANDROID_SCALER_FLIP_MODE_NONE:
        default:
            m_configurations->setModeValue(CONFIGURATION_FLIP_HORIZONTAL, 0);
            break;
        }

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_SCALER_FLIP_MODE);
        if (prev_entry.count > 0) {
            isMetaExist = true;
        }

        if (!isMetaExist || prev_entry.data.u8[0] != entry.data.u8[0]) {
            CLOGD("SAMSUNG_ANDROID_SCALER_FLIP_MODE(%d)", entry.data.u8[0]);
        }
        isMetaExist = false;
    } else {
        m_configurations->setModeValue(CONFIGURATION_FLIP_HORIZONTAL, 0);
    }
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorSensorMetaData(CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

    if (m_configurations->getSamsungCamera() == true) {
        int64_t exposureTime = 0L;
        int32_t sensitivity = 0;

        if (m_isManualAeControl == false) {
            exposureTime = (int64_t)(1000000000.0/src->udm.ae.vendorSpecific[64]);
            sensitivity = (int32_t)src->udm.internal.vendorSpecific[1];
        } else {
            exposureTime = (int64_t)src->dm.sensor.exposureTime;
            sensitivity = (int32_t) src->dm.sensor.sensitivity;
        }

        settings->update(ANDROID_SENSOR_EXPOSURE_TIME, &exposureTime, 1);

        if (sensitivity < m_sensorStaticInfo->sensitivityRange[MIN]) {
            sensitivity = m_sensorStaticInfo->sensitivityRange[MIN];
        } else if (sensitivity > m_sensorStaticInfo->sensitivityRange[MAX]) {
            sensitivity = m_sensorStaticInfo->sensitivityRange[MAX];
        }
        src->dm.sensor.sensitivity = sensitivity; //  for  EXIF Data
        settings->update(ANDROID_SENSOR_SENSITIVITY, &sensitivity, 1);

        CLOGV("[frameCount is %d] m_isManualAeControl(%d) exposureTime(%ju) sensitivity(%d)",
            src->dm.request.frameCount, m_isManualAeControl, exposureTime, sensitivity);
    }

#ifdef SAMSUNG_TN_FEATURE
    int32_t multi_luminances[9] = {0, };
    for (int i = 0; i < 9; i++) {
        multi_luminances[i] = (int32_t) src->udm.sensor.multiLuminances[i];
        CLOGV("multi_luminances[%d](%d)", i, multi_luminances[i]);
    }
    settings->update(SAMSUNG_ANDROID_SENSOR_MULTI_LUMINANCES, multi_luminances, 9);
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorPartialLensMetaData(__unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    /* SAMSUNG_ANDROID_LENS_INFO_CURRENTINFO */
    lens_current_info_t lensInfo;
    lensInfo.pan_pos = (int32_t)src->udm.af.lensPositionInfinity;
    lensInfo.macro_pos = (int32_t)src->udm.af.lensPositionMacro;
    lensInfo.driver_resolution = (int32_t)src->udm.pdaf.lensPosResolution;
    lensInfo.current_pos = (int32_t)src->udm.af.lensPositionCurrent;

    settings->update(SAMSUNG_ANDROID_LENS_INFO_CURRENTINFO,
        (int32_t *)&lensInfo, sizeof(lensInfo) / sizeof(int32_t));
    CLOGV("lensPositionInfinity(%d), lensPositionMacro(%d), lensPosResolution(%d), lensPositionCurrent(%d)",
        src->udm.af.lensPositionInfinity, src->udm.af.lensPositionMacro,
        src->udm.pdaf.lensPosResolution, src->udm.af.lensPositionCurrent);
#endif
}

void ExynosCameraMetadataConverter::translateVendorPartialControlMetaData(__unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    /* SAMSUNG_ANDROID_CONTROL_TOUCH_AE_STATE */
    /*
        0: touch ae searching or idle
           dm.aa.vendor_touchAeDone(0)  dm.aa.vendor_touchBvChange(0)
        1: touch ae done
           dm.aa.vendor_touchAeDone(1)  dm.aa.vendor_touchBvChange(0)
           dm.aa.vendor_touchAeDone(1)  dm.aa.vendor_touchBvChange(1)
        2: bv changed
           dm.aa.vendor_touchAeDone(0)  dm.aa.vendor_touchBvChange(1)
    */
    int32_t touchAeState = (int32_t) src->dm.aa.vendor_touchAeDone;
    if (touchAeState != SAMSUNG_ANDROID_CONTROL_TOUCH_AE_DONE
        && src->dm.aa.vendor_touchBvChange == 1) {
        touchAeState = SAMSUNG_ANDROID_CONTROL_BV_CHANGED;
    }
    settings->update(SAMSUNG_ANDROID_CONTROL_TOUCH_AE_STATE, &touchAeState, 1);
    CLOGV("SAMSUNG_ANDROID_CONTROL_TOUCH_AE_STATE (%d)", touchAeState);

    /* SAMSUNG_ANDROID_CONTROL_DOF_SINGLE_DATA */
    paf_result_t singleData;
    memset(&singleData, 0x0,  sizeof(singleData));
    singleData.mode = src->udm.pdaf.singleResult.mode;
    singleData.goal_pos = src->udm.pdaf.singleResult.goalPos;
    singleData.reliability = src->udm.pdaf.singleResult.reliability;
    singleData.val = src->udm.pdaf.singleResult.currentPos;
    settings->update(SAMSUNG_ANDROID_CONTROL_DOF_SINGLE_DATA,
        (int32_t *)&singleData, sizeof(singleData) / sizeof(int32_t));
    CLOGV("pdaf single result(%d, %d, %d, %d)",
        singleData.mode, singleData.goal_pos, singleData.reliability, singleData.val);

#if defined(SAMSUNG_DOF) || defined(SUPPORT_MULTI_AF)
    /* SAMSUNG_ANDROID_CONTROL_DOF_MULTI_INFO */
    int shotMode = m_configurations->getModeValue(CONFIGURATION_SHOT_MODE);
    if (shotMode == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELECTIVE_FOCUS ||
        shotMode == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_PRO) {
        paf_multi_info_t multiInfo;
        memset(&multiInfo, 0x0,  sizeof(multiInfo));
        multiInfo.column = src->udm.pdaf.numCol;
        multiInfo.row = src->udm.pdaf.numRow;
        if (shotMode == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELECTIVE_FOCUS) {
            multiInfo.usage = PAF_DATA_FOR_OUTFOCUS;
        } else {
            multiInfo.usage = PAF_DATA_FOR_MULTI_AF;
        }
        settings->update(SAMSUNG_ANDROID_CONTROL_DOF_MULTI_INFO,
            (int32_t *)&multiInfo, sizeof(multiInfo) / sizeof(int32_t));
        CLOGV("pdaf multi info(%d, %d, %d)",
            multiInfo.column, multiInfo.row, multiInfo.usage);

        if ((multiInfo.column > 0 && multiInfo.column <= CAMERA2_MAX_PDAF_MULTIROI_COLUMN)
            && (multiInfo.row > 0 && multiInfo.row <= CAMERA2_MAX_PDAF_MULTIROI_ROW)) {
            /* SAMSUNG_ANDROID_CONTROL_DOF_MULTI_DATA */
            paf_result_t multiData[multiInfo.row][multiInfo.column];
            memset(multiData, 0x0, sizeof(multiData));
            for (int i = 0; i < multiInfo.row; i++) {
                for (int j = 0; j < multiInfo.column; j++) {
                    multiData[i][j].mode = src->udm.pdaf.multiResult[i][j].mode;
                    multiData[i][j].goal_pos = src->udm.pdaf.multiResult[i][j].goalPos;
                    multiData[i][j].reliability = src->udm.pdaf.multiResult[i][j].reliability;
                    multiData[i][j].val = src->udm.pdaf.multiResult[i][j].focused;
                    CLOGV(" multiData[%d][%d] mode = %d", i, j, multiData[i][j].mode);
                    CLOGV(" multiData[%d][%d] goalPos = %d", i, j, multiData[i][j].goal_pos);
                    CLOGV(" multiData[%d][%d] reliability = %d", i, j, multiData[i][j].reliability);
                    CLOGV(" multiData[%d][%d] focused = %d", i, j, multiData[i][j].val);
                }
            }
            settings->update(SAMSUNG_ANDROID_CONTROL_DOF_MULTI_DATA,
                (int32_t *)multiData, sizeof(multiData) / sizeof(int32_t));
        } else {
            CLOGE("pdaf multi info size error!! (%d, %d, %d)",
                multiInfo.column, multiInfo.row, multiInfo.usage);
        }
    }
#endif
#ifdef SAMSUNG_OT
    if (src->ctl.aa.vendor_afmode_option & SET_BIT(AA_AFMODE_OPTION_BIT_OBJECT_TRACKING)) {
        UniPluginFocusData_t OTfocusData;

        m_configurations->getObjectTrackingFocusData(&OTfocusData);

        int32_t focusState = OTfocusData.focusState;
#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
        if (m_configurations->getModeValue(CONFIGURATION_DUAL_2X_BUTTON) == true) {
            focusState = SAMSUNG_ANDROID_CONTROL_OBJECT_TRACKING_STATE_TEMPLOST;
        } else
#endif
        {
            if (m_rectUiSkipCount > 0)
                focusState = SAMSUNG_ANDROID_CONTROL_OBJECT_TRACKING_STATE_TEMPLOST;
        }

        settings->update(SAMSUNG_ANDROID_CONTROL_OBJECT_TRACKING_STATE, &focusState, 1);
    }
#endif
#endif /* SAMSUNG_TN_FEATURE */
}

void ExynosCameraMetadataConverter::translateVendorPartialMetaData(__unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext, __unused enum metadata_type metaType)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

#ifdef SAMSUNG_TN_FEATURE
    switch (metaType) {
    case PARTIAL_JPEG:
    {
        ExynosCameraParameters *parameters = NULL;
        debug_attribute_t *debugInfo = NULL;

#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
        if (m_subPrameters != NULL
            && (m_configurations->getModeValue(CONFIGURATION_DUAL_DISP_CAM_TYPE)
                == UNI_PLUGIN_CAMERA_TYPE_TELE)) {
            parameters = m_subPrameters;
        } else
#endif
        {
            parameters = m_parameters;
        }

        debugInfo = parameters->getDebugAttribute();
        settings->update(SAMSUNG_ANDROID_JPEG_IMAGE_DEBUGINFO_APP4,
            (uint8_t *)debugInfo->debugData[APP_MARKER_4], debugInfo->debugSize[APP_MARKER_4]);
        settings->update(SAMSUNG_ANDROID_JPEG_IMAGE_DEBUGINFO_APP5,
            (uint8_t *)debugInfo->debugData[APP_MARKER_5], debugInfo->debugSize[APP_MARKER_5]);
        break;
    }
    case PARTIAL_3AA:
    {
        translateVendorPartialControlMetaData(settings, src_ext);
        translateVendorPartialLensMetaData(settings, src_ext);
        break;
    }
    default:
        break;
    }
#endif /* SAMSUNG_TN_FEATURE */

    return;
}

status_t ExynosCameraMetadataConverter::checkRangeOfValid(__unused int32_t tag, __unused int32_t value)
{
    status_t ret = NO_ERROR;
    __unused camera_metadata_entry_t entry;

    __unused const int32_t *i32Range = NULL;

#ifdef SAMSUNG_TN_FEATURE
    entry = m_staticInfo.find(tag);
    if (entry.count > 0) {
        /* TODO: handle all tags
         *       need type check
         */
        switch (tag) {
        case SAMSUNG_ANDROID_CONTROL_COLOR_TEMPERATURE_RANGE:
        case SAMSUNG_ANDROID_CONTROL_WBLEVEL_RANGE:
             i32Range = entry.data.i32;

            if (value < i32Range[0] || value > i32Range[1]) {
                CLOGE("Invalid Tag ID(%d) : value(%d), range(%d, %d)",
                    tag, value, i32Range[0], i32Range[1]);
                ret = BAD_VALUE;
            }
            break;
        default:
            CLOGE("Tag (%d) is not implemented", tag);
            break;
        }
    } else {
        CLOGE("Cannot find entry, tag(%d)", tag);
        ret = BAD_VALUE;
    }
#endif

    return ret;
}

void ExynosCameraMetadataConverter::setShootingMode(__unused int shotMode, struct camera2_shot_ext *dst_ext)
{
    __unused enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
    __unused bool changeSceneMode = true;
    enum aa_mode mode = dst_ext->shot.ctl.aa.mode;

    if (mode == AA_CONTROL_USE_SCENE_MODE) {
        return;
    }

#ifdef SAMSUNG_TN_FEATURE
    switch (shotMode) {
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_PANORAMA:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_WIDE_SELFIE:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_INTERACTIVE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PANORAMA;
        break;
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_NIGHT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_LLS;
        break;
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SPORTS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SPORTS;
        break;
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SINGLE:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_AUTO:
#ifdef SAMSUNG_FACTORY_LN_TEST
        if (m_configurations->getModeValue(CONFIGURATION_OPERATION_MODE) == OPERATION_MODE_LN2_TEST) {
            mode = AA_CONTROL_USE_SCENE_MODE;
            sceneMode = AA_SCENE_MODE_FACTORY_LN2;
        } else if (m_configurations->getModeValue(CONFIGURATION_OPERATION_MODE) == OPERATION_MODE_LN4_TEST) {
            mode = AA_CONTROL_USE_SCENE_MODE;
            sceneMode = AA_SCENE_MODE_FACTORY_LN4;
        } else
#endif
        {
            sceneMode = AA_SCENE_MODE_DISABLED;
        }
        break;
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_BEAUTY:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELFIE_FOCUS:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_HDR:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_STICKER:
#ifdef SAMSUNG_DOF
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELECTIVE_FOCUS:
#endif
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_ANTIFOG:
        sceneMode = AA_SCENE_MODE_DISABLED;
        break;
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_DUAL:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_DUAL;
        break;
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_PRO:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PRO_MODE;
        break;
#ifdef SAMSUNG_FOOD_MODE
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_FOOD:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_FOOD;
        break;
#endif
#ifdef SAMSUNG_COLOR_IRIS
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_COLOR_IRIS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_COLOR_IRIS;
        break;
#endif
#ifdef USES_DUAL_REAR_PORTRAIT
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_LIVE_FOCUS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_LIVE_OUTFOCUS;
        break;
#endif
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_FACE_LOCK:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_INTELLIGENT_SCAN:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_FACE_LOCK;
        break;
#ifdef SAMSUNG_SSM
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SUPER_SLOW_MOTION:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SUPER_SLOWMOTION;
        break;
#endif
#ifdef SAMSUNG_HYPERLAPSE
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_HYPER_MOTION:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_HYPERLAPSE;
        break;
#endif
    default:
        changeSceneMode = false;
        break;
    }

    if (changeSceneMode == true)
        setMetaCtlSceneMode(dst_ext, sceneMode, mode);
#endif /* SAMSUNG_TN_FEATURE */
}

void ExynosCameraMetadataConverter::setSceneMode(int value, struct camera2_shot_ext *dst_ext)
{
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;

    if (dst_ext->shot.ctl.aa.mode != AA_CONTROL_USE_SCENE_MODE) {
        return;
    }

    switch (value) {
    case ANDROID_CONTROL_SCENE_MODE_PORTRAIT:
        sceneMode = AA_SCENE_MODE_PORTRAIT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_LANDSCAPE:
        sceneMode = AA_SCENE_MODE_LANDSCAPE;
        break;
    case ANDROID_CONTROL_SCENE_MODE_NIGHT:
        sceneMode = AA_SCENE_MODE_NIGHT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_BEACH:
        sceneMode = AA_SCENE_MODE_BEACH;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SNOW:
        sceneMode = AA_SCENE_MODE_SNOW;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SUNSET:
        sceneMode = AA_SCENE_MODE_SUNSET;
        break;
    case ANDROID_CONTROL_SCENE_MODE_FIREWORKS:
        sceneMode = AA_SCENE_MODE_FIREWORKS;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SPORTS:
        sceneMode = AA_SCENE_MODE_SPORTS;
        break;
    case ANDROID_CONTROL_SCENE_MODE_PARTY:
        sceneMode = AA_SCENE_MODE_PARTY;
        break;
    case ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT:
        sceneMode = AA_SCENE_MODE_CANDLELIGHT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO:
        sceneMode = AA_SCENE_MODE_STEADYPHOTO;
        break;
    case ANDROID_CONTROL_SCENE_MODE_ACTION:
        sceneMode = AA_SCENE_MODE_ACTION;
        break;
    case ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT:
        sceneMode = AA_SCENE_MODE_NIGHT_PORTRAIT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_THEATRE:
        sceneMode = AA_SCENE_MODE_THEATRE;
        break;
    case ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY:
        sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        break;
    case ANDROID_CONTROL_SCENE_MODE_HDR:
        sceneMode = AA_SCENE_MODE_HDR;
        break;
    case ANDROID_CONTROL_SCENE_MODE_DISABLED:
    default:
        sceneMode = AA_SCENE_MODE_DISABLED;
        break;
    }

    setMetaCtlSceneMode(dst_ext, sceneMode);
}

enum aa_afstate ExynosCameraMetadataConverter::translateVendorAfStateMetaData(enum aa_afstate mainAfState)
{
    enum aa_afstate resultAfState = mainAfState;

#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
    ExynosCameraActivityControl *activityControl = NULL;
    ExynosCameraActivityAutofocus *autoFocusMgr = NULL;
    enum aa_afstate subAfState = AA_AFSTATE_INACTIVE;

    if (m_subPrameters != NULL
        && m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM
        && m_configurations->getDualOperationModeReprocessing() == DUAL_OPERATION_MODE_SYNC) {
        activityControl = m_subPrameters->getActivityControl();
        autoFocusMgr = activityControl->getAutoFocusMgr();
        subAfState = autoFocusMgr->getAfState();

        CLOGV("mainAfState(%d), subAfState(%d)", mainAfState, subAfState);
        if (mainAfState == AA_AFSTATE_PASSIVE_SCAN || subAfState == AA_AFSTATE_PASSIVE_SCAN) {
            resultAfState = AA_AFSTATE_PASSIVE_SCAN;
        } else if (mainAfState == AA_AFSTATE_ACTIVE_SCAN || subAfState == AA_AFSTATE_ACTIVE_SCAN) {
            resultAfState = AA_AFSTATE_ACTIVE_SCAN;
        }
    }
#endif

    return resultAfState;
}

void ExynosCameraMetadataConverter::translateVendorScalerMetaData(struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;
    bool samsungCamera = m_configurations->getSamsungCamera();
    float appliedZoomRatio = 1.0f;
    float userZoomRatio = 1.0f;
    ExynosRect zoomRect = {0, };
    int sensorMaxW = 0, sensorMaxH = 0;

    src = &(src_ext->shot);

    appliedZoomRatio = src->udm.zoomRatio;
    userZoomRatio = src->uctl.zoomRatio;

    if (samsungCamera && (userZoomRatio != appliedZoomRatio)) {
        m_parameters->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&sensorMaxW, (uint32_t *)&sensorMaxH);

        zoomRect.w = ceil((float)sensorMaxW / appliedZoomRatio);
        zoomRect.h = ceil((float)sensorMaxH / appliedZoomRatio);
        zoomRect.x = (sensorMaxW - zoomRect.w)/2;
        zoomRect.y = (sensorMaxH - zoomRect.h)/2;

        /* x - y */
        if (zoomRect.x < 0) {
            zoomRect.x = 0;
        }

        if (zoomRect.y < 0) {
            zoomRect.y = 0;
        }

        /* w - h */
        if (zoomRect.w > sensorMaxW) {
            zoomRect.x = 0;
            zoomRect.w = sensorMaxW;
        } else if (zoomRect.x + zoomRect.w > sensorMaxW) {
            zoomRect.x = ALIGN_DOWN((sensorMaxW - zoomRect.w)/2, 2);
        }

        if (zoomRect.h > sensorMaxH) {
            zoomRect.y = 0;
            zoomRect.h = sensorMaxH;
        } else if (zoomRect.y + zoomRect.h > sensorMaxH) {
            zoomRect.y = ALIGN_DOWN((sensorMaxH - zoomRect.h)/2, 2);
        }

        src->dm.scaler.cropRegion[0] = ALIGN_DOWN(zoomRect.x, 2);
        src->dm.scaler.cropRegion[1] = ALIGN_DOWN(zoomRect.y, 2);
        src->dm.scaler.cropRegion[2] = ALIGN_UP(zoomRect.w, 2);
        src->dm.scaler.cropRegion[3] = ALIGN_UP(zoomRect.h, 2);
    } else {
        src->dm.scaler.cropRegion[0] = src->ctl.scaler.cropRegion[0];
        src->dm.scaler.cropRegion[1] = src->ctl.scaler.cropRegion[1];
        src->dm.scaler.cropRegion[2] = src->ctl.scaler.cropRegion[2];
        src->dm.scaler.cropRegion[3] = src->ctl.scaler.cropRegion[3];
    }

    CLOGV("CropRegion(%f)(%d,%d,%d,%d)->(%f/%f)(%d,%d,%d,%d)",
            userZoomRatio,
            src->ctl.scaler.cropRegion[0],
            src->ctl.scaler.cropRegion[1],
            src->ctl.scaler.cropRegion[2],
            src->ctl.scaler.cropRegion[3],
            appliedZoomRatio,
            (float) sensorMaxW / (float)src->dm.scaler.cropRegion[2],
            src->dm.scaler.cropRegion[0],
            src->dm.scaler.cropRegion[1],
            src->dm.scaler.cropRegion[2],
            src->dm.scaler.cropRegion[3]
    );
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailablePreviewConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int (*yuvSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int yuvSizeListLength = 0;
    int (*hiddenPreviewSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int hiddenPreviewSizeListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->yuvList == NULL) {
        CLOGI2("VendorYuvList is NULL");
        return BAD_VALUE;
    }

    yuvSizeList = sensorStaticInfo->yuvList;
    yuvSizeListLength = sensorStaticInfo->yuvListMax;

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream supported size list */
    for (int i = 0; i < yuvSizeListLength; i++) {
        streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs->add(yuvSizeList[i][0]);
        streamConfigs->add(yuvSizeList[i][1]);
        streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
    }

    if (sensorStaticInfo->hiddenPreviewList != NULL) {
        hiddenPreviewSizeList = sensorStaticInfo->hiddenPreviewList;
        hiddenPreviewSizeListLength = sensorStaticInfo->hiddenPreviewListMax;

        /* add hidden size list */
        for (int i = 0; i < hiddenPreviewSizeListLength; i++) {
            streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
            streamConfigs->add(hiddenPreviewSizeList[i][0]);
            streamConfigs->add(hiddenPreviewSizeList[i][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailablePictureConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int (*jpegSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int jpegSizeListLength = 0;
    int (*hiddenPictureSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int hiddenPictureSizeListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->jpegList == NULL) {
        CLOGI2("VendorJpegList is NULL");
        return BAD_VALUE;
    }

    jpegSizeList = sensorStaticInfo->jpegList;
    jpegSizeListLength = sensorStaticInfo->jpegListMax;

    /* Stall output stream supported size list */
    for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
        for (int j = 0; j < jpegSizeListLength; j++) {
            streamConfigs->add(STALL_FORMATS[i]);
            streamConfigs->add(jpegSizeList[j][0]);
            streamConfigs->add(jpegSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    if (sensorStaticInfo->hiddenPictureList != NULL) {
        hiddenPictureSizeList = sensorStaticInfo->hiddenPictureList;
        hiddenPictureSizeListLength = sensorStaticInfo->hiddenPictureListMax;

        /* add hidden size list */
        for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
            for (int j = 0; j < hiddenPictureSizeListLength; j++) {
                streamConfigs->add(STALL_FORMATS[i]);
                streamConfigs->add(hiddenPictureSizeList[j][0]);
                streamConfigs->add(hiddenPictureSizeList[j][1]);
                streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
            }
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableVideoConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int (*availableVideoSizeList)[7] = NULL;
    int availableVideoSizeListLength = 0;
    int cropRatio = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->availableVideoList == NULL) {
        CLOGI2("VendorVideoList is NULL");
        return BAD_VALUE;
    }

    availableVideoSizeList = sensorStaticInfo->availableVideoList;
    availableVideoSizeListLength = sensorStaticInfo->availableVideoListMax;

    for (int i = 0; i < availableVideoSizeListLength; i++) {
        streamConfigs->add(availableVideoSizeList[i][0]);
        streamConfigs->add(availableVideoSizeList[i][1]);
        streamConfigs->add(availableVideoSizeList[i][2]/1000);
        streamConfigs->add(availableVideoSizeList[i][3]/1000);
        cropRatio = 0;
        /* cropRatio = vdisW * 1000 / nVideoW;
         * cropRatio = ((cropRatio - 1000) + 5) / 10 */
        if (availableVideoSizeList[i][4] != 0) {
            cropRatio = (int)(availableVideoSizeList[i][4] * 1000) / (int)availableVideoSizeList[i][0];
            cropRatio = ((cropRatio - 1000) + 5) / 10;
        }
        streamConfigs->add(cropRatio);
        streamConfigs->add(availableVideoSizeList[i][6]);
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableHighSpeedVideoConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int (*availableHighSpeedVideoList)[5] = NULL;
    int availableHighSpeedVideoListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->availableHighSpeedVideoList == NULL) {
        CLOGI2("availableHighSpeedVideoList is NULL");
        return BAD_VALUE;
    }

    availableHighSpeedVideoList = sensorStaticInfo->availableHighSpeedVideoList;
    availableHighSpeedVideoListLength = sensorStaticInfo->availableHighSpeedVideoListMax;

    for (int i = 0; i < availableHighSpeedVideoListLength; i++) {
        streamConfigs->add(availableHighSpeedVideoList[i][0]);
        streamConfigs->add(availableHighSpeedVideoList[i][1]);
        streamConfigs->add(availableHighSpeedVideoList[i][2]/1000);
        streamConfigs->add(availableHighSpeedVideoList[i][3]/1000);
        streamConfigs->add(availableHighSpeedVideoList[i][4]);
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableAeModeConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *vendorAeModes)
{
    status_t ret = NO_ERROR;
    __unused uint8_t (*baseAeModes) = NULL;
    __unused int baseAeModesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorAeModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->aeModes == NULL) {
        CLOGE2("aeModes is NULL");
        return BAD_VALUE;
    }

#ifdef SAMSUNG_TN_FEATURE
    baseAeModes = sensorStaticInfo->aeModes;
    baseAeModesLength = sensorStaticInfo->aeModesLength;

    /* default ae modes */
    for (int i = 0; i < baseAeModesLength; i++) {
        vendorAeModes->add(baseAeModes[i]);
    }

    /* vendor ae modes */
    if (sensorStaticInfo->screenFlashAvailable == true) {
        int vendorAeModesLength = ARRAY_LENGTH(AVAILABLE_VENDOR_AE_MODES_FLASH_MODES);
        for (int i = 0; i < vendorAeModesLength; i++) {
            vendorAeModes->add(AVAILABLE_VENDOR_AE_MODES_FLASH_MODES[i]);
        }
    }
#endif
    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableAfModeConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *vendorAfModes)
{
    status_t ret = NO_ERROR;
    uint8_t (*baseAfModes) = NULL;
    int baseAfModesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorAfModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->afModes == NULL) {
        CLOGE2("availableafModes is NULL");
        return BAD_VALUE;
    }

    baseAfModes = sensorStaticInfo->afModes;
    baseAfModesLength = sensorStaticInfo->afModesLength;

    /* default af modes */
    for (int i = 0; i < baseAfModesLength; i++) {
        vendorAfModes->add(baseAfModes[i]);
    }

#ifdef SAMSUNG_OT
    /* vendor af modes */
    if (sensorStaticInfo->objectTrackingAvailable == true) {
        int vendorAfModesLength = ARRAY_LENGTH(AVAILABLE_VENDOR_AF_MODES_OBJECT_TRACKING);
        for (int i = 0; i < vendorAfModesLength; i++) {
            vendorAfModes->add(AVAILABLE_VENDOR_AF_MODES_OBJECT_TRACKING[i]);
        }
    }
#endif

#ifdef SAMSUNG_FIXED_FACE_FOCUS
    /* vendor af modes */
    if (sensorStaticInfo->fixedFaceFocusAvailable == true) {
        int vendorAfModesLength = ARRAY_LENGTH(AVAILABLE_VENDOR_AF_MODES_FIXED_FACE_FOCUS);
        for (int i = 0; i < vendorAfModesLength; i++) {
            vendorAfModes->add(AVAILABLE_VENDOR_AF_MODES_FIXED_FACE_FOCUS[i]);
        }
    }
#endif

    return ret;
}

#ifdef SUPPORT_DEPTH_MAP
status_t ExynosCameraMetadataConverter::m_createVendorDepthAvailableDepthConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int availableDepthSizeListMax = 0;
    int (*availableDepthSizeList)[2] = NULL;
    int availableDepthFormatListMax = 0;
    int *availableDepthFormatList = NULL;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    availableDepthSizeList = sensorStaticInfo->availableDepthSizeList;
    availableDepthSizeListMax = sensorStaticInfo->availableDepthSizeListMax;
    availableDepthFormatList = sensorStaticInfo->availableDepthFormatList;
    availableDepthFormatListMax = sensorStaticInfo->availableDepthFormatListMax;

    for (int i = 0; i < availableDepthFormatListMax; i++) {
        for (int j = 0; j < availableDepthSizeListMax; j++) {
            streamConfigs->add(availableDepthFormatList[i]);
            streamConfigs->add(availableDepthSizeList[j][0]);
            streamConfigs->add(availableDepthSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

#ifdef DEBUG_STREAM_CONFIGURATIONS
    const int32_t* streamConfigArray = NULL;
    streamConfigArray = streamConfigs->array();
    for (int i = 0; i < streamConfigs->size(); i = i + 4) {
        CLOGD2("Size %4dx%4d Format %2x %6s",
                streamConfigArray[i+1], streamConfigArray[i+2],
                streamConfigArray[i],
                (streamConfigArray[i+3] == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT)?
                "OUTPUT" : "INPUT");
    }
#endif

    return ret;
}
#endif

status_t ExynosCameraMetadataConverter::m_createVendorScalerAvailableThumbnailConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int availableThumbnailCallbackSizeListMax = 0;
    int (*availableThumbnailCallbackSizeList)[2] = NULL;
    int availableThumbnailCallbackFormatListMax = 0;
    int *availableThumbnailCallbackFormatList = NULL;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    availableThumbnailCallbackSizeList = sensorStaticInfo->availableThumbnailCallbackSizeList;
    availableThumbnailCallbackSizeListMax = sensorStaticInfo->availableThumbnailCallbackSizeListMax;
    availableThumbnailCallbackFormatList = sensorStaticInfo->availableThumbnailCallbackFormatList;
    availableThumbnailCallbackFormatListMax = sensorStaticInfo->availableThumbnailCallbackFormatListMax;

    for (int i = 0; i < availableThumbnailCallbackFormatListMax; i++) {
        for (int j = 0; j < availableThumbnailCallbackSizeListMax; j++) {
            streamConfigs->add(availableThumbnailCallbackFormatList[i]);
            streamConfigs->add(availableThumbnailCallbackSizeList[j][0]);
            streamConfigs->add(availableThumbnailCallbackSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorScalerAvailableIrisConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int availableIrisSizeListMax = 0;
    int (*availableIrisSizeList)[2] = NULL;
    int availableIrisFormatListMax = 0;
    int *availableIrisFormatList = NULL;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    availableIrisSizeList = sensorStaticInfo->availableIrisSizeList;
    availableIrisSizeListMax = sensorStaticInfo->availableIrisSizeListMax;
    availableIrisFormatList = sensorStaticInfo->availableIrisFormatList;
    availableIrisFormatListMax = sensorStaticInfo->availableIrisFormatListMax;

    for (int i = 0; i < availableIrisFormatListMax; i++) {
        for (int j = 0; j < availableIrisSizeListMax; j++) {
            streamConfigs->add(availableIrisFormatList[i]);
            streamConfigs->add(availableIrisSizeList[j][0]);
            streamConfigs->add(availableIrisSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableEffectModesConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *vendorEffectModes)
{
    status_t ret = NO_ERROR;
    __unused uint8_t (*baseEffectModes) = NULL;
    __unused int baseEffectModesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorEffectModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->effectModes == NULL) {
        CLOGE2("effectModes is NULL");
        return BAD_VALUE;
    }

#ifdef SAMSUNG_TN_FEATURE
    baseEffectModes = sensorStaticInfo->effectModes;
    baseEffectModesLength = sensorStaticInfo->effectModesLength;

    /* default effect modes */
    for (int i = 0; i < baseEffectModesLength; i++) {
        vendorEffectModes->add(baseEffectModes[i]);
    }

    /* vendor effect modes */
    int vendorEffectModesLength = ARRAY_LENGTH(AVAILABLE_VENDOR_EFFECTS);
    for (int i = 0; i < vendorEffectModesLength; i++) {
        vendorEffectModes->add(AVAILABLE_VENDOR_EFFECTS[i]);
    }
#endif
    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorEffectAeAvailableFpsRanges(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *fpsRanges)
{
    int ret = OK;
    int (*fpsRangesList)[2] = NULL;
    size_t fpsRangesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (fpsRanges == NULL) {
        CLOGE2("FPS ranges is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->effectFpsRangesList == NULL) {
        CLOGI2("effectFpsRangesList is NULL");
        return BAD_VALUE;
    }

    fpsRangesList = sensorStaticInfo->effectFpsRangesList;
    fpsRangesLength = sensorStaticInfo->effectFpsRangesListMax;

    for (size_t i = 0; i < fpsRangesLength; i++) {
        fpsRanges->add(fpsRangesList[i][0]/1000);
        fpsRanges->add(fpsRangesList[i][1]/1000);
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorAvailableThumbnailSizes(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    int ret = OK;
    int (*hiddenThumbnailList)[2] = NULL;
    int hiddenThumbnailListMax = 0;
    int (*thumbnailList)[SIZE_OF_RESOLUTION] = NULL;
    int thumbnailListMax = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->thumbnailList == NULL) {
        CLOGI2("thumbnailList is NULL");
        return BAD_VALUE;
    }

    thumbnailList = sensorStaticInfo->thumbnailList;
    thumbnailListMax = sensorStaticInfo->thumbnailListMax;

    for (int i = 0; i < thumbnailListMax; i++) {
        streamConfigs->add(thumbnailList[i][0]);
        streamConfigs->add(thumbnailList[i][1]);
    }

    if (sensorStaticInfo->hiddenThumbnailList != NULL) {
        hiddenThumbnailList = sensorStaticInfo->hiddenThumbnailList;
        hiddenThumbnailListMax = sensorStaticInfo->hiddenThumbnailListMax;

        for (int i = 0; i < hiddenThumbnailListMax; i++) {
            streamConfigs->add(hiddenThumbnailList[i][0]);
            streamConfigs->add(hiddenThumbnailList[i][1]);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableFeatures(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *availableFeatures)
{
    status_t ret = NO_ERROR;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (availableFeatures == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->availableBasicFeaturesList == NULL) {
        CLOGE2("availableBasicFeaturesList is NULL");
        return BAD_VALUE;
    }

    int *basicFeatures = sensorStaticInfo->availableBasicFeaturesList;
    int basicFeaturesLength = sensorStaticInfo->availableBasicFeaturesListLength;
    int *sensorFeatures = sensorStaticInfo->availableOptionalFeaturesList;
    int sensorFeaturesLength = sensorStaticInfo->availableOptionalFeaturesListLength;

    /* basic features */
    for (int i = 0; i < basicFeaturesLength; i++) {
        availableFeatures->add(basicFeatures[i]);
    }

    /* sensor feature */
    if (sensorFeatures) {
        for (int i = 0; i < sensorFeaturesLength; i++) {
            availableFeatures->add(sensorFeatures[i]);
        }
    }

    return ret;
}

#ifdef SAMSUNG_SSM
void ExynosCameraMetadataConverter::m_convertActiveArrayToSSMRegion(ExynosRect2 *region)
{
    status_t ret = NO_ERROR;
    ExynosRect activeArraySize;
    ExynosRect hwSensorSize;
    ExynosRect hwActiveArraySize;
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;

    m_parameters->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&activeArraySize.w, (uint32_t *)&activeArraySize.h);
    m_parameters->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorSize.w, (uint32_t *)&hwSensorSize.h);

    hwSensorSize.x = 0;
    hwSensorSize.y = 0;

    /* Calculate HW active array size for current sensor aspect ratio
       based on active array size
     */
    ret = getCropRectAlign(activeArraySize.w, activeArraySize.h,
                           hwSensorSize.w, hwSensorSize.h,
                           &hwActiveArraySize.x, &hwActiveArraySize.y,
                           &hwActiveArraySize.w, &hwActiveArraySize.h,
                           2, 2, 1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                activeArraySize.w, activeArraySize.h,
                hwSensorSize.w, hwSensorSize.h);
        return;
    }

    CLOGV("ActiveArraySize %dx%d(%d) hwActiveArraySize %dx%d(%d) Region %d,%d %d,%d %dx%d(%d) hwSensorSize %dx%d(%d)",
            activeArraySize.w, activeArraySize.h, SIZE_RATIO(activeArraySize.w, activeArraySize.h),
            hwActiveArraySize.w, hwActiveArraySize.h, SIZE_RATIO(hwActiveArraySize.w, hwActiveArraySize.h),
            region->x1, region->y1, region->x2, region->y2, region->x2 - region->x1, region->y2 - region->y1,
            SIZE_RATIO(region->x2 - region->x1, region->y2 - region->y1),
            hwSensorSize.w, hwSensorSize.h, SIZE_RATIO(hwSensorSize.w, hwSensorSize.h));

    /* Scale down the 3AA region & HW active array size
       to adjust them to the 3AA input size without sensor margin
     */
    scaleRatioW = (float) hwSensorSize.w / (float) hwActiveArraySize.w;
    scaleRatioH = (float) hwSensorSize.h / (float) hwActiveArraySize.h;

    region->x1 = (int) (((float) region->x1) * scaleRatioW);
    region->y1 = (int) (((float) region->y1) * scaleRatioH);
    region->x2 = (int) (((float) region->x2) * scaleRatioW);
    region->y2 = (int) (((float) region->y2) * scaleRatioH);

    hwActiveArraySize.x = (int) (((float) hwActiveArraySize.x) * scaleRatioW);
    hwActiveArraySize.y = (int) (((float) hwActiveArraySize.y) * scaleRatioH);
    hwActiveArraySize.w = (int) (((float) hwActiveArraySize.w) * scaleRatioW);
    hwActiveArraySize.h = (int) (((float) hwActiveArraySize.h) * scaleRatioH);

    /* Remove HW active array size offset */
    /* Top-left */
    if (region->x1 < hwActiveArraySize.x) region->x1 = 0;
    else region->x1 -= hwActiveArraySize.x;
    if (region->y1 < hwActiveArraySize.y) region->y1 = 0;
    else region->y1 -= hwActiveArraySize.y;
    if (region->x2 < hwActiveArraySize.x) region->x2 = 0;
    else region->x2 -= hwActiveArraySize.x;
    if (region->y2 < hwActiveArraySize.y) region->y2 = 0;
    else region->y2 -= hwActiveArraySize.y;

    /* Bottom-right */
    if (region->x1 > hwActiveArraySize.w) region->x1 = hwActiveArraySize.w;
    if (region->y1 > hwActiveArraySize.h) region->y1 = hwActiveArraySize.h;
    if (region->x2 > hwActiveArraySize.w) region->x2 = hwActiveArraySize.w;
    if (region->y2 > hwActiveArraySize.h) region->y2 = hwActiveArraySize.h;

    if (region->x1 < 0 || region->x2 > hwSensorSize.w || region->y1 < 0 || region->y2 > hwSensorSize.h) {
        CLOGD("Region error! %d,%d %d,%d %dx%d(%d)",
                region->x1, region->y1, region->x2, region->y2, region->x2 - region->x1,
                region->y2 - region->y1, SIZE_RATIO(region->x2 - region->x1, region->y2 - region->y1));
    }

    CLOGV("Region %d,%d %d,%d %dx%d(%d)",
            region->x1, region->y1, region->x2, region->y2, region->x2 - region->x1,
            region->y2 - region->y1, SIZE_RATIO(region->x2 - region->x1, region->y2 - region->y1));
}
#endif

#ifdef SAMSUNG_DUAL_ZOOM_PREVIEW
void ExynosCameraMetadataConverter::m_convertOTRectToActiveArrayRegion(CameraMetadata *settings,
                                                                        struct camera2_shot_ext *shot_ext,
                                                                        ExynosRect2 *region)
{
    status_t ret = NO_ERROR;
    ExynosRect activeArraySize;
    ExynosRect hwSensorSize;
    ExynosRect hwActiveArraySize;
    ExynosRect hwBcropSize;
    ExynosRect hwBdsSize;
    ExynosRect mcscInputSize;
    ExynosRect hwYuvSize;
    ExynosRect baseSize;
    ExynosRect fusionSrcRect;
    ExynosRect fusionDstRect;
    int previewPortId = m_parameters->getPreviewPortId();
    int offsetX = 0, offsetY = 0;
    int margin = 0;

    if (settings == NULL) {
        CLOGE("CameraMetadata is NULL");
        return;
    }

    if (shot_ext == NULL) {
        CLOGE("camera2_shot_ext is NULL");
        return;
    }

    /* Original OT region : based on OT input size (e.g. preview size)
     * Camera Metadata OT region : based on sensor active array size (e.g. max sensor size)
     * The OT region from firmware must be scaled into the size based on sensor active array size
     */
    m_parameters->getSize(HW_INFO_MAX_SENSOR_SIZE, (uint32_t *)&activeArraySize.w, (uint32_t *)&activeArraySize.h);
    m_parameters->getPreviewBayerCropSize(&hwSensorSize, &hwBcropSize);
    m_parameters->getPreviewBdsSize(&hwBdsSize);
    m_parameters->getYuvVendorSize(&hwYuvSize.w, &hwYuvSize.h, previewPortId, hwBdsSize);
    margin = m_parameters->getActiveZoomMargin();

    /* only use wide margin & hwYuvSize*/
    if (margin == DUAL_SOLUTION_MARGIN_VALUE_30 ||
        margin == DUAL_SOLUTION_MARGIN_VALUE_20) {
        m_parameters->getFusionSize(hwYuvSize.w, hwYuvSize.h,
                &fusionSrcRect, &fusionDstRect,
                margin);
        hwYuvSize.w = fusionSrcRect.w;
        hwYuvSize.h = fusionSrcRect.h;
    }

    CLOGV("[F(%d)]ActiveArraySize %dx%d(%d) HWSensorSize %dx%d(%d) HWBcropSize %d,%d %dx%d(%d)",
            shot_ext->shot.dm.request.frameCount,
            activeArraySize.w, activeArraySize.h, SIZE_RATIO(activeArraySize.w, activeArraySize.h),
            hwSensorSize.w, hwSensorSize.h, SIZE_RATIO(hwSensorSize.w, hwSensorSize.h),
            hwBcropSize.x, hwBcropSize.y, hwBcropSize.w, hwBcropSize.h, SIZE_RATIO(hwBcropSize.w, hwBcropSize.h));
    CLOGV("[F(%d)]HWBdsSize %dx%d(%d) HWYUVSize %dx%d(%d)",
            shot_ext->shot.dm.request.frameCount,
            hwBdsSize.w, hwBdsSize.h, SIZE_RATIO(hwBdsSize.w, hwBdsSize.h),
            hwYuvSize.w, hwYuvSize.h, SIZE_RATIO(hwYuvSize.w, hwYuvSize.h));

    /* Calculate HW active array size for current sensor aspect ratio
     * based on active array size
     */
    ret = getCropRectAlign(activeArraySize.w, activeArraySize.h,
            hwSensorSize.w, hwSensorSize.h,
            &hwActiveArraySize.x, &hwActiveArraySize.y,
            &hwActiveArraySize.w, &hwActiveArraySize.h,
            2, 2, 1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                activeArraySize.w, activeArraySize.h,
                hwSensorSize.w, hwSensorSize.h);
        return;
    }

    /* Calculate HW YUV crop region */
    ret = getCropRectAlign(hwBdsSize.w, hwBdsSize.h,
                           hwYuvSize.w, hwYuvSize.h,
                           &mcscInputSize.x, &mcscInputSize.y,
                           &mcscInputSize.w, &mcscInputSize.h,
                           2, 2, 1.0f);
    if (ret != NO_ERROR) {
        CLOGE("Failed to getCropRectAlign. Src %dx%d, Dst %dx%d",
                hwBdsSize.w, hwBdsSize.h,
                hwYuvSize.w, hwYuvSize.h);
        return;
    }

    if (m_subPrameters != NULL
        && (m_configurations->getModeValue(CONFIGURATION_DUAL_DISP_CAM_TYPE) == UNI_PLUGIN_CAMERA_TYPE_TELE)) {
        m_subPrameters->getFusionSize(hwYuvSize.w, hwYuvSize.h,
                &fusionSrcRect, &fusionDstRect, 0);
        baseSize.w = fusionDstRect.w;
        baseSize.h = fusionDstRect.h;
    } else {
        m_parameters->getFusionSize(hwYuvSize.w, hwYuvSize.h,
                &fusionSrcRect, &fusionDstRect, 0);
        baseSize.w = fusionDstRect.w;
        baseSize.h = fusionDstRect.h;
    }

    /* considering preview margin */
    if (hwYuvSize.w < baseSize.w) {
        offsetX = 0;
    } else {
        offsetX = (hwYuvSize.w - baseSize.w)/2;
        baseSize.x = offsetX;
    }

    if (hwYuvSize.h < baseSize.h) {
        offsetY = 0;
    } else {
        offsetY = (hwYuvSize.h - baseSize.h)/2;
        baseSize.y = offsetY;
    }

    CLOGV("[F(%d)]HwActiveArraySize %d,%d %dx%d(%d) MCSCInputSize %d,%d %dx%d(%d) baseSize %d,%d %dx%d(%d)",
            shot_ext->shot.dm.request.frameCount,
            hwActiveArraySize.x, hwActiveArraySize.y, hwActiveArraySize.w, hwActiveArraySize.h,
            SIZE_RATIO(hwActiveArraySize.w, hwActiveArraySize.h),
            mcscInputSize.x, mcscInputSize.y, mcscInputSize.w, mcscInputSize.h,
            SIZE_RATIO(mcscInputSize.w, mcscInputSize.h),
            baseSize.x, baseSize.y, baseSize.w, baseSize.h,
            SIZE_RATIO(baseSize.w, baseSize.h));

    /* Calculate YUV scaling ratio */
    float yuvScaleRatioW = 0.0f, yuvScaleRatioH = 0.0f;
    yuvScaleRatioW = (float) mcscInputSize.w / (float) hwYuvSize.w;
    yuvScaleRatioH = (float) mcscInputSize.h / (float) hwYuvSize.h;

    /* Scale offset with YUV scaling ratio */
    offsetX *= yuvScaleRatioW;
    offsetY *= yuvScaleRatioH;

    /* Add MCSC input crop offset */
    offsetX += mcscInputSize.x;
    offsetY += mcscInputSize.y;

    /* Calculate BDS scaling ratio */
    float bdsScaleRatioW = 0.0f, bdsScaleRatioH = 0.0f;
    bdsScaleRatioW = (float) hwBcropSize.w / (float) hwBdsSize.w;
    bdsScaleRatioH = (float) hwBcropSize.h / (float) hwBdsSize.h;

    /* Scale offset with BDS scaling ratio */
    offsetX *= bdsScaleRatioW;
    offsetY *= bdsScaleRatioH;

    /* Add HW bcrop offset */
    offsetX += hwBcropSize.x;
    offsetY += hwBcropSize.y;

    /* Calculate Sensor scaling ratio */
    float sensorScaleRatioW = 0.0f, sensorScaleRatioH = 0.0f;
    sensorScaleRatioW = (float) hwActiveArraySize.w / (float) hwSensorSize.w;
    sensorScaleRatioH = (float) hwActiveArraySize.h / (float) hwSensorSize.h;

    /* Scale offset with Sensor scaling ratio */
    offsetX *= sensorScaleRatioW;
    offsetY *= sensorScaleRatioH;

    /* Add HW active array offset */
    offsetX += hwActiveArraySize.x;
    offsetY += hwActiveArraySize.y;

    /* Calculate final scale ratio */
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;
    scaleRatioW = yuvScaleRatioW * bdsScaleRatioW * sensorScaleRatioW;
    scaleRatioH = yuvScaleRatioH * bdsScaleRatioH * sensorScaleRatioH;

    region->x1 = region->x1 * scaleRatioW + offsetX;
    region->x2 = region->x2 * scaleRatioW + offsetX;
    region->y1 = region->y1 * scaleRatioH + offsetY;
    region->y2 = region->y2 * scaleRatioH + offsetY;

    return;
}
#endif

}; /* namespace android */
