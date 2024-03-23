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

void ExynosCameraMetadataConverter::m_constructVendorDefaultRequestSettings( int type, CameraMetadata *settings)
{
    /** android.stats */
    uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;

    if (m_parameters->getSamsungCamera() == true) {
        switch (type) {
        case CAMERA3_TEMPLATE_PREVIEW:
        case CAMERA3_TEMPLATE_STILL_CAPTURE:
        case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
            faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE;
            break;
        case CAMERA3_TEMPLATE_VIDEO_RECORD:
        case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        default:
            faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
            break;
        }
    }
    settings->update(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);

#ifdef SAMSUNG_TN_FEATURE
#ifdef SAMSUNG_PAF
    /* PAF */
    int32_t pafmode = SAMSUNG_ANDROID_CONTROL_PAF_MODE_ON;
    settings->update(SAMSUNG_ANDROID_CONTROL_PAF_MODE, &pafmode, 1);
#endif

#ifdef SAMSUNG_PAF
    /* RT - HDR */
    int32_t hdrmode = SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_OFF;
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
    }

#ifdef SAMSUNG_HYPER_MOTION
    /* recording motion speed mode*/
    int32_t motionSpeedMode = SAMSUNG_ANDROID_CONTROL_RECORDING_MOTION_SPEED_MODE_AUTO;
    settings->update(SAMSUNG_ANDROID_CONTROL_RECORDING_MOTION_SPEED_MODE, &motionSpeedMode, 1);
#endif
#endif /* SAMSUNG_TN_FEATURE*/

    return;
}

#ifdef SAMSUNG_TN_FEATURE
void ExynosCameraMetadataConverter::m_constructVendorStaticInfo(struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                CameraMetadata *info, int cameraId)
{
    status_t ret = NO_ERROR;
    Vector<int32_t> i32Vector;
    Vector<uint8_t> ui8Vector;

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
    int32_t burstShotFpsRange[RANGE_TYPE_MAX] = {BURSTSHOT_MIN_FPS, BURSTSHOT_MAX_FPS};
    ret = info->update(SAMSUNG_ANDROID_CONTROL_BURST_SHOT_FPS_RANGE,
            burstShotFpsRange,
            ARRAY_LENGTH(burstShotFpsRange));
    if (ret < 0) {
        CLOGD2("SAMSUNG_ANDROID_CONTROL_BURST_SHOT_FPS_RANGE update failed(%d)", ret);
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

    /* samsung.android.control.availableFeaturesv */
    if (sensorStaticInfo->availableFeaturesListLength > 0) {
        ret = info->update(SAMSUNG_ANDROID_CONTROL_AVAILABLE_FEATURES,
                sensorStaticInfo->availableFeaturesList,
                sensorStaticInfo->availableFeaturesListLength);
        if (ret < 0)
            CLOGD2("SAMSUNG_ANDROID_CONTROL_AVAILABLE_FEATURES update failed(%d)", ret);
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
    return;
}
#endif /* SAMSUNG_TN_FEATURE */

status_t ExynosCameraMetadataConverter::initShotVendorData(struct camera2_shot *shot)
{
    /* utrl */
    shot->uctl.companionUd.caf_mode = COMPANION_CAF_ON;
    shot->uctl.companionUd.drc_mode = COMPANION_DRC_ON;
#ifdef SAMSUNG_PAF
    shot->uctl.companionUd.paf_mode = COMPANION_PAF_ON;
#else
    shot->uctl.companionUd.paf_mode = COMPANION_PAF_OFF;
#endif
    shot->uctl.companionUd.wdr_mode = COMPANION_WDR_OFF;
#ifdef SUPPORT_DEPTH_MAP
    if (m_parameters->getUseDepthMap()) {
        shot->uctl.companionUd.disparity_mode = COMPANION_DISPARITY_CENSUS_CENTER;
    } else
#endif
    {
        shot->uctl.companionUd.disparity_mode = COMPANION_DISPARITY_OFF;
    }

    shot->uctl.companionUd.lsc_mode = COMPANION_LSC_ON;
    shot->uctl.companionUd.bpc_mode = COMPANION_BPC_ON;
    shot->uctl.companionUd.bypass_mode= COMPANION_FULL_BYPASS_OFF;

    if (m_parameters->getFactorytest() == true)
        shot->uctl.opMode = CAMERA_OP_MODE_HAL3_FAC;
    else if (m_parameters->getSamsungCamera() == true)
        shot->uctl.opMode = CAMERA_OP_MODE_HAL3_TW;
    else
    shot->uctl.opMode = CAMERA_OP_MODE_HAL3_GED;

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

    if (m_flashMgr == NULL) {
        CLOGE("FlashMgr is NULL!!");
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

#ifdef SAMSUNG_TN_FEATURE
    translatePreVendorControlControlData(settings, dst_ext);
#endif

    /* ANDROID_CONTROL_AE_ANTIBANDING_MODE */
    entry = settings->find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        if (dst->ctl.aa.aeAntibandingMode != AA_AE_ANTIBANDING_OFF) {
            dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode) m_defaultAntibanding;
        }
        CLOGV("ANDROID_COLOR_AE_ANTIBANDING_MODE(%d) m_defaultAntibanding(%d)",
            entry.data.u8[0], m_defaultAntibanding);
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
        m_flashMgr->setFlashExposure(aeMode);
        m_flashMgr->setFlashReq(flashReq, m_overrideFlashControl);

        /* ANDROID_CONTROL_AE_MODE */
        entry = m_prevMeta->find(ANDROID_CONTROL_AE_MODE);
        if (entry.count > 0) {
            prev_value = entry.data.u8[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != (int32_t)vendorAeMode) {
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
            prev_value = (enum aa_ae_lock) FIMC_IS_METADATA(prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.aeLock) {
            CLOGD("ANDROID_CONTROL_AE_LOCK(%d)", dst->ctl.aa.aeLock);
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

        m_parameters->checkPreviewFpsRange(entry.data.i32[0], entry.data.i32[1]);
        m_parameters->getPreviewFpsRange(&min_fps, &max_fps);
        dst->ctl.aa.aeTargetFpsRange[0] = min_fps;
        dst->ctl.aa.aeTargetFpsRange[1] = max_fps;
        m_maxFps = dst->ctl.aa.aeTargetFpsRange[1];

        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
        if (prev_entry.count > 0) {
            prev_fps[0] = prev_entry.data.i32[0];
            prev_fps[1] = prev_entry.data.i32[1];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_fps[0] != (int32_t)dst->ctl.aa.aeTargetFpsRange[0] ||
            prev_fps[1] != (int32_t)dst->ctl.aa.aeTargetFpsRange[1]) {
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
        if (m_parameters->getShotMode() == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELECTIVE_FOCUS) {
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
            prev_value = (enum aa_af_trigger) (prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.afTrigger) {
            CLOGD("ANDROID_CONTROL_AF_TRIGGER(%d)", dst->ctl.aa.afTrigger);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AWB_LOCK */
    entry = settings->find(ANDROID_CONTROL_AWB_LOCK);
    if (entry.count > 0) {
        dst->ctl.aa.awbLock = (enum aa_awb_lock) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AWB_LOCK);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_awb_lock) FIMC_IS_METADATA(prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.awbLock) {
            CLOGD("ANDROID_CONTROL_AWB_LOCK(%d)", dst->ctl.aa.awbLock);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_AWB_MODE */
    entry = settings->find(ANDROID_CONTROL_AWB_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.awbMode = (enum aa_awbmode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_AWB_MODE);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_awbmode) FIMC_IS_METADATA(prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.awbMode) {
            CLOGD("ANDROID_CONTROL_AWB_MODE(%d)", dst->ctl.aa.awbMode);
        }
        isMetaExist = false;
    }

    if (m_parameters->getRecordingHint()) {
        dst->ctl.aa.vendor_videoMode = AA_VIDEOMODE_ON;
    }

    /* ANDROID_CONTROL_EFFECT_MODE */
    entry = settings->find(ANDROID_CONTROL_EFFECT_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.effectMode = (enum aa_effect_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_EFFECT_MODE);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_effect_mode) FIMC_IS_METADATA(prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.effectMode) {
            CLOGD("ANDROID_CONTROL_EFFECT_MODE(%d)", dst->ctl.aa.effectMode);
        }
        isMetaExist = false;
    }

    /* ANDROID_CONTROL_MODE */
    entry = settings->find(ANDROID_CONTROL_MODE);
    if (entry.count > 0) {
        dst->ctl.aa.mode = (enum aa_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        prev_entry = m_prevMeta->find(ANDROID_CONTROL_MODE);
        if (prev_entry.count > 0) {
            prev_value = (enum aa_mode) FIMC_IS_METADATA(prev_entry.data.u8[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->ctl.aa.mode) {
            CLOGD("ANDROID_CONTROL_MODE(%d)", dst->ctl.aa.mode);
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

        if (m_parameters->getRestartStream() == false) {
            if (isMetaExist && prev_value != scene_mode) {
                if (prev_value == ANDROID_CONTROL_SCENE_MODE_HDR
                    || scene_mode == ANDROID_CONTROL_SCENE_MODE_HDR) {
                    m_parameters->setRestartStream(true);
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
    bool isFlashStepChanged = false;

    /* Check Precapture Trigger to turn on the pre-flash */
    switch (dst->ctl.aa.aePrecaptureTrigger) {
    case AA_AE_PRECAPTURE_TRIGGER_START:
        if (m_flashMgr->getNeedCaptureFlash() == true
            && m_flashMgr->getFlashStatus() == AA_FLASHMODE_OFF) {
#ifdef SAMSUNG_TN_FEATURE
            if (vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_AUTO_SCREEN_FLASH ||
                vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_ALWAYS_SCREEN_FLASH) {
                flashStep = ExynosCameraActivityFlash::FLASH_STEP_PRE_LCD_ON;
            } else
#endif
            {
                flashStep = ExynosCameraActivityFlash::FLASH_STEP_PRE_START;
            }
            m_flashMgr->setCaptureStatus(true);
            isFlashStepChanged = true;
        }
        break;
    case AA_AE_PRECAPTURE_TRIGGER_CANCEL:
        if (m_flashMgr->getNeedCaptureFlash() == true
            && m_flashMgr->getFlashStatus() != AA_FLASHMODE_OFF
            && m_flashMgr->getFlashStatus() != AA_FLASHMODE_CANCEL) {
            flashStep = ExynosCameraActivityFlash::FLASH_STEP_CANCEL;
            m_flashMgr->setCaptureStatus(false);
            isFlashStepChanged = true;
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
        if (m_flashMgr->getNeedCaptureFlash() == true) {
#ifdef SAMSUNG_TN_FEATURE
            if (vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_AUTO_SCREEN_FLASH ||
                vendorAeMode == SAMSUNG_ANDROID_CONTROL_AE_MODE_ON_ALWAYS_SCREEN_FLASH) {
                flashStep = ExynosCameraActivityFlash::FLASH_STEP_LCD_ON;
            } else
#endif
            {
                flashStep = ExynosCameraActivityFlash::FLASH_STEP_MAIN_START;
            }

            isFlashStepChanged = true;
            m_parameters->setMarkingOfExifFlash(1);
        } else {
            m_parameters->setMarkingOfExifFlash(0);
        }
        break;
    case AA_CAPTURE_INTENT_VIDEO_RECORD:
        m_parameters->setRecordingHint(true);
        break;
    case AA_CAPTURE_INTENT_CUSTOM:
    case AA_CAPTURE_INTENT_PREVIEW:
    case AA_CAPTURE_INTENT_VIDEO_SNAPSHOT:
    case AA_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
    case AA_CAPTURE_INTENT_MANUAL:
    default:
        break;
    }

    if (isFlashStepChanged == true && m_flashMgr != NULL)
        m_flashMgr->setFlashStep(flashStep);

    /* If aeMode or Mode is NOT Off, Manual AE control can NOT be operated */
    if (dst->ctl.aa.aeMode == AA_AEMODE_OFF
        || dst->ctl.aa.mode == AA_CONTROL_OFF) {
        m_isManualAeControl = true;
        CLOGV("Operate Manual AE Control, aeMode(%d), Mode(%d)",
                dst->ctl.aa.aeMode, dst->ctl.aa.mode);
    } else {
        m_isManualAeControl = false;
    }
    m_parameters->setManualAeControl(m_isManualAeControl);

#ifdef SAMSUNG_TN_FEATURE
    translateVendorControlControlData(settings, dst_ext);
#endif

    return OK;
}

#ifdef SAMSUNG_TN_FEATURE
void ExynosCameraMetadataConverter::translatePreVendorControlControlData(CameraMetadata *settings,
                                                                         struct camera2_shot_ext *dst_ext)
{
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    struct camera2_shot *dst = NULL;
    int32_t prev_value;
    bool isMetaExist = false;

    dst = &dst_ext->shot;

    /* SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE);
    if (entry.count > 0) {
        int32_t shooting_mode = (int32_t)entry.data.i32[0];

        m_parameters->setShotMode(shooting_mode);

        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE);
        if (prev_entry.count > 0) {
            prev_value = (int32_t)prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != shooting_mode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE(%d)", shooting_mode);
        }
        isMetaExist = false;
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

    return;
}

void ExynosCameraMetadataConverter::translateVendorControlControlData(CameraMetadata *settings,
							                                         struct camera2_shot_ext *dst_ext)
{
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    struct camera2_shot *dst = NULL;
    int32_t prev_value;
    bool isMetaExist = false;

    dst = &dst_ext->shot;

#ifdef SAMSUNG_CONTROL_METERING
    if (dst->ctl.aa.aeMode != AA_AEMODE_OFF) {
        /* SAMSUNG_ANDROID_CONTROL_METERING_MODE */
        entry = settings->find(SAMSUNG_ANDROID_CONTROL_METERING_MODE);
        if (entry.count > 0) {
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
            CLOGV("SAMSUNG_ANDROID_CONTROL_METERING_MODE(%d,aeMode = %d)",
                entry.data.i32[0], dst->ctl.aa.aeMode);
        }
    }
#endif

#ifdef SAMSUNG_PAF
    /* SAMSUNG_ANDROID_CONTROL_PAF_MODE */
    entry = settings->find(SAMSUNG_ANDROID_CONTROL_PAF_MODE);
    if (entry.count > 0) {
        dst->uctl.companionUd.paf_mode = (enum companion_paf_mode) FIMC_IS_METADATA(entry.data.i32[0]);
        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_PAF_MODE);
        if (prev_entry.count > 0) {
            prev_value = (enum companion_paf_mode) FIMC_IS_METADATA(prev_entry.data.i32[0]);
            isMetaExist = true;
        }

        if (!isMetaExist || prev_value != dst->uctl.companionUd.paf_mode) {
            CLOGD("SAMSUNG_ANDROID_CONTROL_PAF_MODE(%d)", dst->uctl.companionUd.paf_mode);
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

        dst->uctl.companionUd.wdr_mode = (enum companion_wdr_mode) FIMC_IS_METADATA(entry.data.i32[0]);
        dst->uctl.companionUd.drc_mode = COMPANION_DRC_ON;
        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_LEVEL);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (m_parameters->getRestartStream() == false) {
            if (isMetaExist && prev_value != hdr_level) {
                m_parameters->setRestartStream(true);
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
            dst->uctl.companionUd.wdr_mode = (enum companion_wdr_mode) FIMC_IS_METADATA(entry.data.i32[0]);
        }
        dst->uctl.companionUd.drc_mode = COMPANION_DRC_ON;
        prev_entry = m_prevMeta->find(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE);
        if (prev_entry.count > 0) {
            prev_value = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (m_parameters->getSamsungCamera() == false
            && m_parameters->getRestartStream() == false) {
            if (isMetaExist && prev_value != hdr_mode) {
                m_parameters->setRestartStream(true);
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
        dst->uctl.companionUd.wdr_mode = COMPANION_WDR_ON;
        dst->uctl.companionUd.drc_mode = COMPANION_DRC_ON;
    }

    m_parameters->setRTHdr(dst->uctl.companionUd.wdr_mode);
#endif

#ifdef SAMSUNG_CONTROL_METERING
    /* Forcely set metering mode to matrix in HDR ON as requested by IQ team */
    if (dst->uctl.companionUd.wdr_mode == COMPANION_WDR_ON
        && dst->ctl.aa.aeMode != (enum aa_aemode)AA_AEMODE_MATRIX_TOUCH) {
        dst->ctl.aa.aeMode = (enum aa_aemode)AA_AEMODE_MATRIX;
    }
#endif

#ifdef SAMSUNG_FOCUS_PEAKING
    if (m_parameters->getShotMode() == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_PRO) {
        dst->uctl.companionUd.disparity_mode = COMPANION_DISPARITY_CENSUS_CENTER;
    }
#endif

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
            m_parameters->setLLSOn(true);
        } else {
            m_parameters->setLLSOn(false);
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
    int32_t shooting_mode = m_parameters->getShotMode();
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

        if (m_parameters->getSamsungCamera() == true) {
            if (dst->ctl.aa.aeMode == AA_AEMODE_SPOT) {
                int hwSensorW,hwSensorH;
                m_parameters->getHwSensorSize(&hwSensorW, &hwSensorH);

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
            m_parameters->setTransientActionMode(transientAction);
        }
        isMetaExist = false;
    }

    return;
}

void ExynosCameraMetadataConverter::translateVendorAeControlControlData(struct camera2_shot_ext *dst_ext,
                                                                    uint32_t vendorAeMode,
                                                                    aa_aemode *aeMode,
                                                                    ExynosCameraActivityFlash::FLASH_REQ *flashReq,
                                                                    struct CameraMetaParameters *metaParameters)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

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

        m_parameters->setObjectTrackingAreas(0, newRect, weight);
    }
#endif
}

void ExynosCameraMetadataConverter::translateVendorLensControlData(CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext,
                                                                    struct CameraMetaParameters *metaParameters)
{
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    struct camera2_shot *dst = NULL;
    int32_t prev_value;
    bool isMetaExist = false;

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
#ifdef SAMSUNG_VDIS_ON_OIS
                    dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_VDIS;
#else
                    dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
#endif
                } else {
                    if (zoomRatio >= 4000) {
                        dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_STILL_ZOOM;
                    } else {
#ifdef SAMSUNG_SW_VDIS
                        if (m_parameters->getSWVdisMode() == true) {
                            //TODO: change the OPTICAL_STABILIZATION_MODE_VDIS and adapt OIS_VDIS
                            dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
                        } else
#endif
                        {
                        dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_VIDEO;
                        }
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

#ifdef SAMSUNG_DOF
    entry = settings->find(SAMSUNG_ANDROID_LENS_FOCUS_LENS_POS_STALL);
    if (entry.count > 0) {
        int32_t focus_pos = 0;

        focus_pos = (int32_t)entry.data.i32[0];
        if (focus_pos > 0) {
            m_parameters->setFocusLensPos(focus_pos);
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

    return;
}

void ExynosCameraMetadataConverter::translateVendorSensorControlData(CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext)
{
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    struct camera2_shot *dst = NULL;
    int32_t prev_value;
    bool isMetaExist = false;

    dst = &dst_ext->shot;

    if (m_parameters->getSamsungCamera() == true) {
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

    if ((m_parameters->getSamsungCamera() == true)
        && settings->exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
        /* Capture exposure time (us) */
        dst->ctl.aa.vendor_captureExposureTime = (uint32_t)(dst->ctl.sensor.exposureTime / 1000);

        if (dst->ctl.sensor.exposureTime > (uint64_t)(CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT)*1000) {
            dst->ctl.sensor.exposureTime = (uint64_t)(CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT)*1000;
            dst->ctl.sensor.frameDuration = dst->ctl.sensor.exposureTime;
        }

        m_parameters->setCaptureExposureTime(dst->ctl.aa.vendor_captureExposureTime);

        CLOGV("CaptureExposureTime(%d) PreviewExposureTime(%lld)",
            dst->ctl.aa.vendor_captureExposureTime,
            dst->ctl.sensor.exposureTime);
    }

    if (m_cameraId == CAMERA_ID_SECURE) {
        if (m_isManualAeControl == true) {
            m_parameters->setExposureTime((int64_t)dst->ctl.sensor.exposureTime);
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
                m_parameters->setGain(gain);
            }
            isMetaExist = false;
        }
    }

    return;
}

void ExynosCameraMetadataConverter::translateVendorLedControlData(CameraMetadata *settings,
                                                                  struct camera2_shot_ext *dst_ext)
{
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    struct camera2_shot *dst = NULL;
    int32_t prev_value;
    bool isMetaExist = false;

    dst = &dst_ext->shot;

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
                m_parameters->setLedCurrent(current);
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
                m_parameters->setLedPulseDelay(pulseDelay);
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
                m_parameters->setLedPulseWidth(pulseWidth);
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
                m_parameters->setLedMaxTime(maxTime);
            }
            isMetaExist = false;
        }
    }

    return;
}

void ExynosCameraMetadataConverter::translateVendorControlMetaData(CameraMetadata *settings,
                                                                   struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;
    camera_metadata_entry_t entry;

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
        int32_t vendorHdrMode = entry.data.i32[0];

        if (vendorHdrMode == SAMSUNG_ANDROID_CONTROL_LIVE_HDR_MODE_AUTO) {
            int32_t vendorHdrState = (int32_t) CAMERA_METADATA(src->dm.stats.vendor_wdrAutoState);
            settings->update(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_STATE, &vendorHdrState, 1);
            CLOGV(" dm.stats.vendor_wdrAutoState(%d)", src->dm.stats.vendor_wdrAutoState);
        } else {
            int32_t vendorHdrState = SAMSUNG_ANDROID_CONTROL_LIVE_HDR_AUTO_OFF;
            settings->update(SAMSUNG_ANDROID_CONTROL_LIVE_HDR_STATE, &vendorHdrState, 1);
        }
    }
#endif

    int32_t light_condition = m_parameters->getLightCondition();
    settings->update(SAMSUNG_ANDROID_CONTROL_LIGHT_CONDITION, &light_condition, 1);
    CLOGV(" light_condition(%d)", light_condition);

    int32_t burstShotFps = m_parameters->getBurstShotFps();
    settings->update(SAMSUNG_ANDROID_CONTROL_BURST_SHOT_FPS, &burstShotFps, 1);
    CLOGV(" burstShotFps(%d)", burstShotFps);

    int32_t exposure_value = (int32_t) src->dm.aa.vendor_exposureValue;
    settings->update(SAMSUNG_ANDROID_CONTROL_EV_COMPENSATION_VALUE, &exposure_value, 1);
    CLOGV(" exposure_value(%d)", exposure_value);

    int32_t beauty_scene_index = (int32_t) src->udm.ae.vendorSpecific[395];
    settings->update(SAMSUNG_ANDROID_CONTROL_BEAUTY_SCENE_INDEX, &beauty_scene_index, 1);
    CLOGV(" beauty_scene_index(%d)", beauty_scene_index);

#ifdef SAMSUNG_IDDQD
    uint8_t lens_dirty_detect = (uint8_t) m_parameters->getIDDQDresult();
    settings->update(SAMSUNG_ANDROID_CONTROL_LENS_DIRTY_DETECT, &lens_dirty_detect, 1);
    CLOGV(" lens_dirty_detect(%d)", lens_dirty_detect);
#endif

    int32_t brightness_value = (int32_t) src->udm.internal.vendorSpecific2[102];
    settings->update(SAMSUNG_ANDROID_CONTROL_BRIGHTNESS_VALUE, &brightness_value, 1);
    CLOGV(" brightness_value(%d)", brightness_value);

    return;
}

void ExynosCameraMetadataConverter::translateVendorJpegMetaData(CameraMetadata *settings)
{
    String8 uniqueIdStr;
    uniqueIdStr = String8::format("%s", m_parameters->getImageUniqueId());

    settings->update(SAMSUNG_ANDROID_JPEG_IMAGE_UNIQUE_ID, uniqueIdStr);
    CLOGV("imageUniqueId is %s", uniqueIdStr.c_str());

    return;
}

void ExynosCameraMetadataConverter::translateVendorLensMetaData(CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

    settings->update(SAMSUNG_ANDROID_LENS_INFO_FOCALLENGTH_IN_35MM_FILM,
        &(m_sensorStaticInfo->focalLengthIn35mmLength), 1);
    CLOGV("focalLengthIn35mmLength is (%d)",
        m_sensorStaticInfo->focalLengthIn35mmLength);

    return;
}

void ExynosCameraMetadataConverter::translateVendorScalerControlData(CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;
    bool isMetaExist = false;

    /* SAMSUNG_ANDROID_SCALER_FLIP_MODE */
    entry = settings->find(SAMSUNG_ANDROID_SCALER_FLIP_MODE);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
        case SAMSUNG_ANDROID_SCALER_FLIP_MODE_HFLIP:
            m_parameters->setFlipHorizontal(1);
            break;
        case SAMSUNG_ANDROID_SCALER_FLIP_MODE_NONE:
        default:
            m_parameters->setFlipHorizontal(0);
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
        m_parameters->setFlipHorizontal(0);
    }

    return;
}

void ExynosCameraMetadataConverter::translateVendorSensorMetaData(CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

    if (m_parameters->getSamsungCamera() == true) {
        int64_t exposureTime = 0L;
        int32_t sensitivity = 0;

        if (m_isManualAeControl == false) {
            exposureTime = (int64_t)(1000000000.0/src->udm.ae.vendorSpecific[64]);
            sensitivity = (int32_t)src->udm.internal.vendorSpecific2[101];
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

        CLOGV("[frameCount is %d] m_isManualAeControl(%d) exposureTime(%lld) sensitivity(%d)",
            src->dm.request.frameCount, m_isManualAeControl, exposureTime, sensitivity);
    }

    return;
}

void ExynosCameraMetadataConverter::translateVendorPartialLensMetaData(CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

    /* SAMSUNG_ANDROID_LENS_INFO_CURRENTINFO */
    lens_current_info_t lensInfo;
    lensInfo.pan_pos = (int32_t)src->udm.af.lensPositionInfinity;
    lensInfo.macro_pos = (int32_t)src->udm.af.lensPositionMacro;
    lensInfo.driver_resolution = (int32_t)src->udm.companion.pdaf.lensPosResolution;
    lensInfo.current_pos = (int32_t)src->udm.af.lensPositionCurrent;

    settings->update(SAMSUNG_ANDROID_LENS_INFO_CURRENTINFO,
        (int32_t *)&lensInfo, sizeof(lensInfo) / sizeof(int32_t));
    CLOGV("lensPositionInfinity(%d), lensPositionMacro(%d), lensPosResolution(%d), lensPositionCurrent(%d)",
        src->udm.af.lensPositionInfinity, src->udm.af.lensPositionMacro,
        src->udm.companion.pdaf.lensPosResolution, src->udm.af.lensPositionCurrent);
}

void ExynosCameraMetadataConverter::translateVendorPartialControlMetaData(CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

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
    singleData.mode = src->udm.companion.pdaf.singleResult.mode;
    singleData.goal_pos = src->udm.companion.pdaf.singleResult.goalPos;
    singleData.reliability = src->udm.companion.pdaf.singleResult.reliability;
    singleData.val = src->udm.companion.pdaf.singleResult.currentPos;
    settings->update(SAMSUNG_ANDROID_CONTROL_DOF_SINGLE_DATA,
        (int32_t *)&singleData, sizeof(singleData) / sizeof(int32_t));
    CLOGV("pdaf single result(%d, %d, %d, %d)",
        singleData.mode, singleData.goal_pos, singleData.reliability, singleData.val);

#if defined(SAMSUNG_DOF) || defined(SUPPORT_MULTI_AF)
    /* SAMSUNG_ANDROID_CONTROL_DOF_MULTI_INFO */
    int shotMode = m_parameters->getShotMode();
    if (shotMode == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELECTIVE_FOCUS ||
        shotMode == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_PRO) {
        paf_multi_info_t multiInfo;
        memset(&multiInfo, 0x0,  sizeof(multiInfo));
        multiInfo.column = src->udm.companion.pdaf.numCol;
        multiInfo.row = src->udm.companion.pdaf.numRow;
        if (shotMode == SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_SELECTIVE_FOCUS) {
            multiInfo.usage = PAF_DATA_FOR_OUTFOCUS;
        } else {
            multiInfo.usage = PAF_DATA_FOR_MULTI_AF;
        }
        settings->update(SAMSUNG_ANDROID_CONTROL_DOF_MULTI_INFO,
            (int32_t *)&multiInfo, sizeof(multiInfo) / sizeof(int32_t));
        CLOGV("pdaf multi info(%d, %d, %d)",
            multiInfo.column, multiInfo.row, multiInfo.usage);

        /* SAMSUNG_ANDROID_CONTROL_DOF_MULTI_DATA */
        paf_result_t multiData[multiInfo.row][multiInfo.column];
        memset(multiData, 0x0, sizeof(multiData));
        for (int i = 0; i < multiInfo.row; i++) {
            for (int j = 0; j < multiInfo.column; j++) {
                multiData[i][j].mode = src->udm.companion.pdaf.multiResult[i][j].mode;
                multiData[i][j].goal_pos = src->udm.companion.pdaf.multiResult[i][j].goalPos;
                multiData[i][j].reliability = src->udm.companion.pdaf.multiResult[i][j].reliability;
                multiData[i][j].val = src->udm.companion.pdaf.multiResult[i][j].focused;
                CLOGV(" multiData[%d][%d] mode = %d", i, j, multiData[i][j].mode);
                CLOGV(" multiData[%d][%d] goalPos = %d", i, j, multiData[i][j].goal_pos);
                CLOGV(" multiData[%d][%d] reliability = %d", i, j, multiData[i][j].reliability);
                CLOGV(" multiData[%d][%d] focused = %d", i, j, multiData[i][j].val);
            }
        }
        settings->update(SAMSUNG_ANDROID_CONTROL_DOF_MULTI_DATA,
            (int32_t *)multiData, sizeof(multiData) / sizeof(int32_t));
    }
#endif
#ifdef SAMSUNG_OT
    if (src->ctl.aa.vendor_afmode_option & SET_BIT(AA_AFMODE_OPTION_BIT_OBJECT_TRACKING)) {
        ExynosCameraActivityControl *activityControl = NULL;
        ExynosCameraActivityAutofocus *autoFocusMgr = NULL;
        UniPluginFocusData_t OTpredictedData;

        activityControl = m_parameters->getActivityControl();
        autoFocusMgr = activityControl->getAutoFocusMgr();
        autoFocusMgr->getObjectTrackingAreas(&OTpredictedData);

        int32_t focusState = OTpredictedData.FocusState;
        settings->update(SAMSUNG_ANDROID_CONTROL_OBJECT_TRACKING_STATE, &focusState, 1);
    }
#endif
}

void ExynosCameraMetadataConverter::translateVendorPartialMetaData(CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext, enum metadata_type metaType)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

    switch (metaType) {
    case PARTIAL_JPEG:
    {
        debug_attribute_t *debugInfo = m_parameters->getDebugAttribute();

#if 0 /* For bring-up */
        settings->update(SAMSUNG_ANDROID_JPEG_IMAGE_DEBUGINFO_APP4,
            (uint8_t *)debugInfo->debugData[APP_MARKER_4], debugInfo->debugSize[APP_MARKER_4]);
        settings->update(SAMSUNG_ANDROID_JPEG_IMAGE_DEBUGINFO_APP5,
            (uint8_t *)debugInfo->debugData[APP_MARKER_5], debugInfo->debugSize[APP_MARKER_5]);
#endif
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

    return;
}

status_t ExynosCameraMetadataConverter::checkRangeOfValid(int32_t tag, int32_t value)
{
    status_t ret = NO_ERROR;
    camera_metadata_entry_t entry;

    const int32_t *i32Range = NULL;

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

    return ret;
}

void ExynosCameraMetadataConverter::setShootingMode(int shotMode, struct camera2_shot_ext *dst_ext)
{
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
    bool changeSceneMode = true;
    enum aa_mode mode = dst_ext->shot.ctl.aa.mode;

    if (mode == AA_CONTROL_USE_SCENE_MODE) {
        return;
    }

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
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_BEAUTY:
    case SAMSUNG_ANDROID_CONTROL_SHOOTING_MODE_HDR:
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
    default:
        changeSceneMode = false;
        break;
    }

    if (changeSceneMode == true)
        setMetaCtlSceneMode(dst_ext, sceneMode, mode);
}
#endif /* SAMSUNG_TN_FEATURE */

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
        CLOGE2("availableVideoList is NULL");
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
        CLOGE2("availableHighSpeedVideoList is NULL");
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

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorAeModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->aeModes == NULL) {
        CLOGE2("availableVideoList is NULL");
        return BAD_VALUE;
    }

#ifdef SAMSUNG_TN_FEATURE
    uint8_t *baseAeModes = sensorStaticInfo->aeModes;
    int baseAeModesLength = sensorStaticInfo->aeModesLength;

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

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableEffectModesConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *vendorEffectModes)
{
    status_t ret = NO_ERROR;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorEffectModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->effectModes == NULL) {
        CLOGE2("availableVideoList is NULL");
        return BAD_VALUE;
    }

#ifdef SAMSUNG_TN_FEATURE
    uint8_t *baseEffectModes = sensorStaticInfo->effectModes;
    int baseEffectModesLength = sensorStaticInfo->effectModesLength;

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

}; /* namespace android */
