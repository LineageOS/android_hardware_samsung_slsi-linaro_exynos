/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "audio_hw_primary"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <system/thread_defs.h>

#include <log/log.h>
#include <cutils/str_parms.h>
#include <cutils/sched_policy.h>

#include <audio_utils/channels.h>
#include <audio_utils/primitives.h>

#include "audio_hw.h"
#include "audio_tables.h"
#include "audio_mixers.h"
#include "audio_proxy_interface.h"
#include "audio_odm_interface.h"



/******************************************************************************/
/** Note: the following macro is used for extremely verbose logging message. **/
/** In order to run with ALOG_ASSERT turned on, we need to have LOG_NDEBUG   **/
/** set to 0; but one side effect of this is to turn all LOGV's as well. Some**/
/** messages are so verbose that we want to suppress them even when we have  **/
/** ALOG_ASSERT turned on.  Do not uncomment the #def below unless you       **/
/** really know what you are doing and want to see all of the extremely      **/
/** verbose messages.                                                        **/
/******************************************************************************/
//#define VERY_VERY_VERBOSE_LOGGING
#ifdef VERY_VERY_VERBOSE_LOGGING
#define ALOGVV ALOGD
#else
#define ALOGVV(a...) do { } while(0)
#endif

//#define ROUTING_VERBOSE_LOGGING
#ifdef ROUTING_VERBOSE_LOGGING
#define ALOGRV ALOGD
#else
#define ALOGRV(a...) do { } while(0)
#endif


/******************************************************************************/
/**                                                                          **/
/** Audio Device is Singleton                                                **/
/**                                                                          **/
/******************************************************************************/

/*
 * There are some race condition in HW Binder to open/close HAL.
 * In order to avoid abnormal audio device open and close, adev_init_lock will
 * protect opening before close.
 */
static pthread_mutex_t adev_init_lock = PTHREAD_MUTEX_INITIALIZER;
static struct audio_device *instance = NULL;
static unsigned int adev_ref_count = 0;

static struct audio_device* getAudioDeviceInstance(void)
{
    if (instance == NULL) {
        instance = calloc(1, sizeof(struct audio_device));
        ALOGI("proxy-%s: created Audio HW Device Instance!", __func__);
    }
    return instance;
}

static void destroyAudioDeviceInstance(void)
{
    if (instance) {
        free(instance);
        instance = NULL;
        ALOGI("proxy-%s: destroyed Audio HW Device Instance!", __func__);
    }
    return;
}

/******************************************************************************/
/**                                                                          **/
/** The Local Functions for Mode Selection                                   **/
/**                                                                          **/
/******************************************************************************/
/* CP Centric Call Mode or not */
bool isCPCallMode(struct audio_device *adev)
{
    audio_mode_t mode = adev->amode;

    if (mode == AUDIO_MODE_IN_CALL)
        return true;
    else
        return false;
}

/* AP Centric Call Mode or not */
bool isAPCallMode(struct audio_device *adev)
{
    audio_mode_t mode = adev->amode;

    if (mode == AUDIO_MODE_IN_COMMUNICATION)
        return true;
    else
        return false;
}

static bool isCallMode(struct audio_device *adev)
{
    if (isCPCallMode(adev) || isAPCallMode(adev))
        return true;
    else
        return false;
}

/* Factory Mode or not */
static bool isFactoryMode(struct audio_device *adev)
{
    if (adev->factory && adev->factory->mode != FACTORY_MODE_NONE)
        return true;
    else
        return false;
}

static bool isCallRecording(audio_source_t source)
{
    if (source == AUDIO_SOURCE_VOICE_UPLINK ||
        source == AUDIO_SOURCE_VOICE_DOWNLINK ||
        source == AUDIO_SOURCE_VOICE_CALL)
        return true;
    else
        return false;
}

#if AUDIO_PLATFORM_ABOX_V1
/* VoIP SE (Speech Enhancement) Triggering or not */
static bool need_voipse_on(struct audio_device *adev)
{
    if (isAPCallMode(adev) && adev->voipse_on == false)
        return true;
    else
        return false;
}
#endif

static bool need_to_reconfig_capture(struct audio_device *adev, struct stream_in *in)
{
    if (!isCPCallMode(adev) && (in->common.stream_type == ASTREAM_CAPTURE_CALL))
        return true;
    else if (isCPCallMode(adev) && isCallRecording(in->requested_source) &&
             in->common.stream_type == ASTREAM_CAPTURE_PRIMARY)
        return true;
    else if (isCPCallMode(adev) && in->common.stream_type == ASTREAM_CAPTURE_PRIMARY)
        return true;  // Normal rec during CP Call
    else
        return false;
}

/******************************************************************************/
/**                                                                          **/
/** The Local Functions for Customer Features                                **/
/**                                                                          **/
/******************************************************************************/
static char *bool_to_str(int value)
{
    return value?"True":"False";
}

static void adjust_out_volume(struct audio_stream_out *stream, const void* buffer, size_t bytes)
{
    struct stream_out *out = (struct stream_out *)stream;
    int num_req_channels = audio_channel_count_from_in_mask(out->common.requested_channel_mask);
    int frames = bytes / (num_req_channels *
        audio_bytes_per_sample(out->common.requested_format));
    int i = 0;
    int leftvolume = 0;
    int16_t *pcm16_buf = NULL;
    int32_t *pcm32_buf = NULL;

    if (out->common.requested_format == AUDIO_FORMAT_PCM_16_BIT) {
        pcm16_buf = (int16_t *)buffer;
    } else if (out->common.requested_format == AUDIO_FORMAT_PCM_8_24_BIT ||
        out->common.requested_format == AUDIO_FORMAT_PCM_32_BIT) {
        pcm32_buf = (int32_t *)buffer;
    } else {
        ALOGE("Unknown format %u\n", out->common.requested_format);
        return;
    }

    leftvolume = (int)(out->vol_left * DIRECT_PLAYBACK_VOLUME_MAX);
    while(frames--) {
        for (i = 0; i < num_req_channels; i++) {
            if (out->common.requested_format == AUDIO_FORMAT_PCM_16_BIT) {
                *pcm16_buf = clamp16((int32_t)(*pcm16_buf * leftvolume) >> 12);
                pcm16_buf++;
            } else if (out->common.requested_format == AUDIO_FORMAT_PCM_32_BIT){
                *pcm32_buf = clamp32(((int64_t)*pcm32_buf * leftvolume) >> 12);
                pcm32_buf++;
            } else {
                *pcm32_buf = clamp24_from_q8_23((int32_t)(*pcm32_buf * leftvolume) >> 12);
                pcm32_buf++;
            }
        }
    }

    return;
}

/******************************************************************************/
/**                                                                          **/
/** The Local Functions for checking type of device                          **/
/**                                                                          **/
/******************************************************************************/
/*
 * Function will check whether requested device is USB offloadable output device or not, returns
 * true : Connected USB Output device is USB_DEVICE or USB_HEADSET
 * false : Connected output device is USB_ACCESSORY or other device
 *
 */
static bool is_usb_out_device(audio_devices_t device)
{
    return ((popcount(device) == 1) &&
            (device & (AUDIO_DEVICE_OUT_USB_DEVICE | AUDIO_DEVICE_OUT_USB_HEADSET)));
}

/*
 * Function will check whether requested device is USB offloadable input device or not, returns
 * true : Connected USB Input device is IN_USB_DEVICE or IN_USB_HEADSET
 * false : Connected Input device is IN_USB_ACCESSORY or other device
 *
 */
static bool is_usb_in_device(audio_devices_t device)
{
    if ((device & AUDIO_DEVICE_BIT_IN) != 0) {
        device &= ~AUDIO_DEVICE_BIT_IN;
        if (popcount(device) == 1 && (device &
            (AUDIO_DEVICE_IN_USB_DEVICE | AUDIO_DEVICE_IN_USB_HEADSET)) != 0)
            return true;
    }
    return false;
}

/******************************************************************************/
/**                                                                          **/
/** The Local Functions for mixer control number                             **/
/**                                                                          **/
/******************************************************************************/
int adev_mixer_ctl_get_number(struct audio_device *adev, char *name)
{
    int retVal = -1;

    if (name == NULL) {
        ALOGE("Failed to get alsaCommands\n");
        return retVal;
    } else if (adev->proxy == NULL) {
        ALOGE("%s: Failed to get proxy\n", name);
        return retVal;
    }
    return proxy_get_mixer_value_int(adev->proxy, name);
}

/******************************************************************************/
/**                                                                          **/
/** The Local Functions for Audio Path Routing                               **/
/**                                                                          **/
/******************************************************************************/
// Factory Mode Usage
static audio_usage adev_get_ausage_for_factory(struct audio_device *adev)
{
    audio_usage ausage = AUSAGE_NONE;

    if (adev->factory && adev->factory->mode == FACTORY_MODE_LOOPBACK) {
        ALOGI("device-%s: AUSAGE_MODE_LOOPBACK", __func__);
        if(adev->factory->loopback_mode == FACTORY_LOOPBACK_PACKET ) {
            ausage = AUSAGE_LOOPBACK;
        } else if(adev->factory->loopback_mode == FACTORY_LOOPBACK_CODEC) {
            ausage = AUSAGE_LOOPBACK_CODEC;
        } else if(adev->factory->loopback_mode == FACTORY_LOOPBACK_REALTIME) {
            ausage = AUSAGE_LOOPBACK_REALTIME;
        } else if(adev->factory->loopback_mode == FACTORY_LOOPBACK_PACKET_NODELAY) {
            ausage = AUSAGE_LOOPBACK_NODELAY;
        } else {
            ausage = AUSAGE_LOOPBACK; // loopback_packet
        }
    } else if (adev->factory && adev->factory->mode == FACTORY_MODE_RMS) {
        ALOGI("device-%s: AUSAGE_MODE_RMS", __func__);
        ausage = AUSAGE_RMS;
    }

    return ausage;
}

// VoLTE Call Usage
static audio_usage adev_get_ausage_for_volte(struct audio_device *adev)
{
    volte_status_t volte_status = VOLTE_OFF;
    int samplingrate = VOICE_SR_NB;
    audio_usage ausage = AUSAGE_NONE;

    if (adev->voice) {
        volte_status = voice_get_volte_status(adev->voice);
        if (volte_status == VOLTE_VOICE) {
            /* VoLTE Voice Call Usage */
            samplingrate = voice_get_samplingrate(adev->voice);
            switch (samplingrate) {
                case VOICE_SR_SWB:
                    ausage = AUSAGE_VOLTE_CALL_SWB;
                    break;

                case VOICE_SR_WB:
                    ausage = AUSAGE_VOLTE_CALL_WB;
                    break;

                case VOICE_SR_NB:
                default:
                    ausage = AUSAGE_VOLTE_CALL_NB;
                    break;
            }
        } else if (volte_status == VOLTE_VIDEO) {
            /* VoLTE Video Call Usage */
            samplingrate = voice_get_samplingrate(adev->voice);
            switch (samplingrate) {
                case VOICE_SR_SWB:
                    ausage = AUSAGE_VOLTE_VT_CALL_SWB;
                    break;

                case VOICE_SR_WB:
                    ausage = AUSAGE_VOLTE_VT_CALL_WB;
                    break;

                case VOICE_SR_NB:
                default:
                    ausage = AUSAGE_VOLTE_VT_CALL_NB;
                    break;
            }
        }
    }

    return ausage;
}

// Voice Call or VoLTE Call Usage
static audio_usage adev_get_ausage_for_call(struct audio_device *adev)
{
    audio_usage ausage = AUSAGE_NONE;

    if (adev->voice) {
        /* Check If Special Call Usage or Normal Call Usage */
        if (adev->voice->tty_mode != TTY_MODE_OFF) {
            ausage = AUSAGE_TTY;
        } else if (adev->incallmusic_on) {
            ausage = AUSAGE_INCALL_MUSIC;
            ALOGI("%s: incallmusic_on (%d)", __func__, adev->incallmusic_on);
        }

        /* Check Normal Call Usage(Voice Call or VoLTE Call) */
        if (voice_get_volte_status(adev->voice) != VOLTE_OFF)
            ausage = adev_get_ausage_for_volte(adev);
        else {
            int samplingrate = voice_get_samplingrate(adev->voice);
            switch (samplingrate) {
                case VOICE_SR_WB:
                    if (adev->voice->hac_mode == HAC_MODE_ON)
                        ausage = AUSAGE_VOICE_CALL_WB_HAC;
                    else
                        ausage = AUSAGE_VOICE_CALL_WB;
                    break;

                case VOICE_SR_NB:
                default:
                    if (adev->voice->hac_mode == HAC_MODE_ON)
                        ausage = AUSAGE_VOICE_CALL_NB_HAC;
                    else
                        ausage = AUSAGE_VOICE_CALL_NB;
                    break;
            }
        }
    }

    ALOGI("%s: Call usage is %s", __func__, usage_table[ausage]);

    return ausage;
}

// VoIP Call Usage such as VoWiFi Call
static audio_usage adev_get_ausage_for_voip(struct audio_device *adev)
{
    audio_usage ausage = AUSAGE_COMMUNICATION;

    if (adev->voice && adev->voice->csvtcall) {
        ausage = AUSAGE_VIDEO_CALL;
    } else if (adev->voice && adev->voice->voip_wificalling) {
        if (adev->voice->tty_mode != TTY_MODE_OFF) {
            ausage = AUSAGE_AP_TTY;
        } else {
            if (adev->primary_output->common.requested_devices & AUDIO_DEVICE_OUT_ALL_SCO) {
                if (adev->voice->bluetooth_samplerate == WB_SAMPLING_RATE)
                    ausage = AUSAGE_WIFI_CALL_WB;
                else
                    ausage = AUSAGE_WIFI_CALL_NB;
            } else {
                int vowifi_band = voice_get_vowifi_band(adev->voice);
                switch (vowifi_band) {
                    case VOICE_SR_SWB:
                        ausage = AUSAGE_WIFI_CALL_SWB;
                        break;

                    case VOICE_SR_WB:
                        ausage = AUSAGE_WIFI_CALL_WB;
                        break;

                    case VOICE_SR_NB:
                    default:
                        ausage = AUSAGE_WIFI_CALL_NB;
                        break;
                }
            }
        }
    } else {
        if (adev->active_input) {
            if (adev->active_input->requested_source == AUDIO_SOURCE_MIC)
                ausage = AUSAGE_VOIP_CALL;
        }
    }

    return ausage;
}

static audio_usage adev_get_playback_ausage(struct audio_device *adev)
{
    audio_mode_t plat_amode = adev->amode;
    audio_usage ausage = AUSAGE_NONE;

    if (isFactoryMode(adev)) {
        ausage = adev_get_ausage_for_factory(adev);
    } else if (isCPCallMode(adev)) {
        ausage = adev_get_ausage_for_call(adev);
    } else if (isAPCallMode(adev)) {
        ausage = adev_get_ausage_for_voip(adev);
    } else if (isFMRadioOn(adev) && (popcount(adev->primary_output->common.requested_devices) == 1)) {
        ausage = AUSAGE_FM_RADIO_TUNER;
    } else {
        ausage = AUSAGE_NONE;
    }

    return ausage;
}

static audio_usage adev_get_capture_ausage(struct audio_device *adev, struct stream_in *in)
{
    audio_usage ausage = AUSAGE_RECORDING;

    if (isFactoryMode(adev)) {
        ausage = adev_get_ausage_for_factory(adev);
    } else if (isCPCallMode(adev)) {
        if (in->requested_source == AUDIO_SOURCE_VOICE_UPLINK)
            ausage = AUSAGE_INCALL_UPLINK;
        else if (in->requested_source == AUDIO_SOURCE_VOICE_DOWNLINK)
            ausage = AUSAGE_INCALL_DOWNLINK;
        else if (in->requested_source == AUDIO_SOURCE_VOICE_CALL)
            ausage = AUSAGE_INCALL_UPLINK_DOWNLINK;
        else
            ausage = AUSAGE_INCALL_UPLINK;
    } else if (isAPCallMode(adev)) {
        ausage = adev_get_ausage_for_voip(adev);
    } else {
        if (in->requested_source == AUDIO_SOURCE_CAMCORDER) {
            ausage = AUSAGE_CAMCORDER;
        } else if (in->requested_source == AUDIO_SOURCE_VOICE_RECOGNITION) {
            ausage = AUSAGE_RECOGNITION;
        }
    }

    return ausage;
}

static audio_usage adev_get_ausage_from_stream(void *stream, audio_usage_type usage_type)
{
    struct audio_device *adev = NULL;
    audio_usage selected_usage = AUSAGE_NONE;

    if (usage_type == AUSAGE_PLAYBACK) {
        struct stream_out *stream_out = (struct stream_out *)stream;
        adev = stream_out->adev;

        selected_usage = adev_get_playback_ausage(adev);
        if (selected_usage == AUSAGE_NONE)
            selected_usage = stream_out->common.stream_usage;
    } else {
        struct stream_in *stream_in = (struct stream_in *)stream;
        adev = stream_in->adev;

        selected_usage = adev_get_capture_ausage(adev, stream);
        if (selected_usage == AUSAGE_NONE)
            selected_usage = stream_in->common.stream_usage;
    }

    return selected_usage;
}

device_type get_indevice_id_from_outdevice(struct audio_device *adev, device_type devices)
{
    if (isCallMode(adev)) {
        device_type in = DEVICE_NONE;

        switch (devices) {
            case DEVICE_HANDSET:
                in = DEVICE_HANDSET_MIC;
                break;
            case DEVICE_SPEAKER:
                in = DEVICE_SPEAKER_MIC;
                break;
            case DEVICE_HEADSET:
                in = DEVICE_HEADSET_MIC;
                break;
            case DEVICE_HEADPHONE:
                in = DEVICE_HEADPHONE_MIC;
                break;
            case DEVICE_BT_SCO_HEADSET:
                if (isAPCallMode(adev)) {
                    if (adev->voice->bluetooth_nrec) {
                        in = DEVICE_BT_SCO_HEADSET_MIC; // nrec is supported by BT side
                    } else {
                        in = DEVICE_BT_NREC_HEADSET_MIC;
                    }
                } else {
                    in = DEVICE_BT_SCO_HEADSET_MIC;
                }
                break;
            case DEVICE_USB_HEADSET:
                if (is_usb_in_device(adev->actual_capture_device))
                    in = DEVICE_USB_HEADSET_MIC;
                else
                    in = DEVICE_HEADPHONE_MIC;
                break;
            case DEVICE_NONE:
                in = DEVICE_NONE;
                break;
            default:
                if(adev->support_reciever) {
                    in = DEVICE_HANDSET_MIC;
                } else {
                    in = DEVICE_SPEAKER_MIC;
                }
                break;
        }

        if ((adev->voice && (adev->voice->tty_mode != TTY_MODE_OFF))
            && (devices & (DEVICE_HEADPHONE|DEVICE_HEADSET|DEVICE_SPEAKER))) {
            switch (adev->voice->tty_mode) {
                case TTY_MODE_FULL:
                    in = DEVICE_FULL_MIC;
                    break;
                case TTY_MODE_HCO:
                    if (devices & (DEVICE_HEADPHONE|DEVICE_HEADSET)) {
                        in = DEVICE_HCO_MIC;
                    }
                    break;
                case TTY_MODE_VCO:
                    in = DEVICE_VCO_MIC;
                    break;
                default:
                    ALOGE("%s: Invalid TTY mode (%#x)", __func__, adev->voice->tty_mode);
            }
        }

        return in;
    }

    switch (devices) {
        case DEVICE_SPEAKER:
        case DEVICE_HANDSET:
        case DEVICE_HEADPHONE:
        case DEVICE_SPEAKER_AND_HEADPHONE:
        case DEVICE_AUX_DIGITAL:
            return DEVICE_MAIN_MIC;

        case DEVICE_HEADSET:
        case DEVICE_SPEAKER_AND_HEADSET:
            return DEVICE_HEADSET_MIC;

        case DEVICE_BT_SCO_HEADSET:
        case DEVICE_SPEAKER_AND_BT_SCO_HEADSET:
            return DEVICE_BT_SCO_HEADSET_MIC;

        case DEVICE_USB_HEADSET:
        case DEVICE_SPEAKER_AND_USB_HEADSET:
            if (is_usb_in_device(adev->actual_capture_device))
                return DEVICE_USB_HEADSET_MIC;
            else
                return DEVICE_HEADPHONE_MIC;

        default:
            return DEVICE_NONE;
    }

    return DEVICE_NONE;
}

device_type get_device_id(struct audio_device *adev, audio_devices_t devices)
{
    device_type ret = DEVICE_NONE;

    if (isCallMode(adev)) {
        if (devices > AUDIO_DEVICE_BIT_IN) {
            /* Input Devices */
            return get_indevice_id_from_outdevice(adev,
                       get_device_id(adev, adev->primary_output->common.requested_devices));
        } else {
            switch (devices) {
                case AUDIO_DEVICE_OUT_EARPIECE:
                    ret = DEVICE_HANDSET;
                    break;
                case AUDIO_DEVICE_OUT_SPEAKER:
                    ret = DEVICE_SPEAKER;
                    break;
                case AUDIO_DEVICE_OUT_WIRED_HEADSET:
                    ret = DEVICE_HEADSET;
                    break;
                case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
                    ret = DEVICE_HEADPHONE;
                    break;
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch"
#endif  // __clang__
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET|AUDIO_DEVICE_OUT_SPEAKER:
                    ret = DEVICE_BT_SCO_HEADSET;
                    break;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
                case AUDIO_DEVICE_OUT_USB_HEADSET:
                case AUDIO_DEVICE_OUT_USB_DEVICE:
                    ret = DEVICE_USB_HEADSET;
                    break;
                case AUDIO_DEVICE_OUT_TELEPHONY_TX:
                    /* Assuming primary-out routing might have already requested, as call started */
                    ret = get_device_id(adev, adev->primary_output->common.requested_devices);
                    break;
                case AUDIO_DEVICE_NONE:
                    ret = DEVICE_NONE;
                    break;
                default:
                    if(adev->support_reciever) {
                        ret = DEVICE_HANDSET;
                    } else {
                        ret = DEVICE_SPEAKER;
                    }
                    break;
            }

            if ((adev->voice->tty_mode != TTY_MODE_OFF)
                    && (devices & (AUDIO_DEVICE_OUT_WIRED_HEADPHONE|
                               AUDIO_DEVICE_OUT_WIRED_HEADSET|AUDIO_DEVICE_OUT_SPEAKER))) {
                switch (adev->voice->tty_mode) {
                    case TTY_MODE_FULL:
                        ret = DEVICE_HEADSET;
                        break;
                    case TTY_MODE_HCO:
                        if (devices & (AUDIO_DEVICE_OUT_WIRED_HEADPHONE|AUDIO_DEVICE_OUT_WIRED_HEADSET)) {
                            ret = DEVICE_HANDSET;
                        }
                        break;
                    case TTY_MODE_VCO:
                        ret = DEVICE_HEADSET;
                        break;
                    default:
                        ALOGE("%s: Invalid TTY mode (%#x)", __func__, adev->voice->tty_mode);
                }
            }

            if (isCPCallMode(adev)) {
                if (adev->voice->call_forwarding)
                    ret = DEVICE_CALL_FWD;
            }
        }

        if (ret != DEVICE_NONE)
            return ret;
    }

    if (devices > AUDIO_DEVICE_BIT_IN) {
        /* Input Devices */
        if (popcount(devices) >= 2) {
            switch (devices) {
                case AUDIO_DEVICE_IN_BUILTIN_MIC:           return DEVICE_MAIN_MIC;
                case AUDIO_DEVICE_IN_WIRED_HEADSET:         return DEVICE_HEADSET_MIC;
                case AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET: return DEVICE_BT_SCO_HEADSET_MIC;
                case AUDIO_DEVICE_IN_BACK_MIC:              return DEVICE_SUB_MIC;
                case AUDIO_DEVICE_IN_USB_DEVICE:
                case AUDIO_DEVICE_IN_USB_HEADSET:           return DEVICE_USB_HEADSET_MIC;
                case AUDIO_DEVICE_IN_FM_TUNER:              return DEVICE_FM_TUNER;
                default:                                    return DEVICE_MAIN_MIC;
            }
        }
    } else {
        /* Output Devices */
        if (adev->fm_via_a2dp && isFMRadioOn(adev)) {
            return DEVICE_FM_EXTERNAL;
        }

        if (popcount(devices) == 1) {
            /* Single Device */
            switch (devices) {
                case AUDIO_DEVICE_OUT_EARPIECE:                 return DEVICE_HANDSET;
                case AUDIO_DEVICE_OUT_SPEAKER:                  return DEVICE_SPEAKER;
                case AUDIO_DEVICE_OUT_WIRED_HEADSET:            return DEVICE_HEADSET;
                case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:          return DEVICE_HEADPHONE;
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:     return DEVICE_BT_SCO_HEADSET;
#ifdef SUPPORT_BTA2DP_OFFLOAD
                case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP:
                case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_HEADPHONES:
                case AUDIO_DEVICE_OUT_BLUETOOTH_A2DP_SPEAKER:   return DEVICE_BT_A2DP_HEADPHONE;
#endif
                case AUDIO_DEVICE_OUT_AUX_DIGITAL:              return DEVICE_AUX_DIGITAL;
                case AUDIO_DEVICE_OUT_USB_DEVICE:
                case AUDIO_DEVICE_OUT_USB_HEADSET:              return DEVICE_USB_HEADSET;

                case AUDIO_DEVICE_NONE:
                default:                                        return DEVICE_NONE;
            }
        } else if (popcount(devices) == 2) {
            /* Dual Device */
            if (devices == (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_WIRED_HEADSET))
                return DEVICE_SPEAKER_AND_HEADSET;
            if (devices == (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_WIRED_HEADPHONE))
                return DEVICE_SPEAKER_AND_HEADPHONE;

            if ((devices & AUDIO_DEVICE_OUT_ALL_SCO) && (devices & AUDIO_DEVICE_OUT_SPEAKER))
                return DEVICE_SPEAKER_AND_BT_SCO_HEADSET;
#ifdef SUPPORT_BTA2DP_OFFLOAD
            if ((devices & AUDIO_DEVICE_OUT_ALL_A2DP) && (devices & AUDIO_DEVICE_OUT_SPEAKER))
                return DEVICE_SPEAKER_AND_BT_A2DP_HEADPHONE;
#endif
            if ((devices & AUDIO_DEVICE_OUT_ALL_USB) && (devices & AUDIO_DEVICE_OUT_SPEAKER))
                return DEVICE_SPEAKER_AND_USB_HEADSET;
        }
    }

    return ret;
}

static device_type adev_get_device(void *stream, audio_usage_type usage_type)
{
    struct audio_device *adev = NULL;
    device_type selected_device = DEVICE_NONE;

    if (usage_type == AUSAGE_PLAYBACK) {
        struct stream_out *out = (struct stream_out *)stream;
        adev = out->adev;

        if (adev->factory && adev->factory->mode != FACTORY_MODE_NONE)
            selected_device = get_device_id(adev, adev->factory->out_device);
        else
            selected_device = get_device_id(adev, out->common.requested_devices);
    } else {
        struct stream_in *in = (struct stream_in *)stream;
        adev = in->adev;

        if (adev->factory && adev->factory->mode != FACTORY_MODE_NONE)
            selected_device = get_device_id(adev, adev->factory->in_device);
        else
            selected_device = get_device_id(adev, in->common.requested_devices);
    }

    return selected_device;
}

static device_type adev_get_capture_device(void *stream, audio_usage_type usage_type)
{
    struct audio_device *adev = NULL;
    device_type selected_device = DEVICE_NONE;

    if (usage_type == AUSAGE_PLAYBACK) {
        struct stream_out *out = (struct stream_out *)stream;
        adev = out->adev;

        if(adev->factory && adev->factory->mode != FACTORY_MODE_NONE)
            selected_device = get_device_id(adev, adev->factory->in_device);
        else
            selected_device = get_indevice_id_from_outdevice(adev, get_device_id(adev, out->common.requested_devices));
    } else {
        struct stream_in *in = (struct stream_in *)stream;
        adev = in->adev;

        if(adev->factory && adev->factory->mode != FACTORY_MODE_NONE)
            selected_device = get_device_id(adev, adev->factory->in_device);
        else
            selected_device = get_device_id(adev, in->common.requested_devices);
    }

    return selected_device;
}

// Modifier Selection
static modifier_type adev_get_modifier(struct audio_device *adev, device_type device)
{
    modifier_type type = MODIFIER_NONE;

    switch (device) {
        case DEVICE_BT_SCO_HEADSET:
        case DEVICE_SPEAKER_AND_BT_SCO_HEADSET:
            if (adev->voice && adev->voice->bluetooth_samplerate == WB_SAMPLING_RATE)
                type = MODIFIER_BT_SCO_RX_WB;
            else
                type = MODIFIER_BT_SCO_RX_NB;
            break;
#ifdef SUPPORT_BTA2DP_OFFLOAD
        case DEVICE_BT_A2DP_HEADPHONE:
        case DEVICE_SPEAKER_AND_BT_A2DP_HEADPHONE:
            type = MODIFIER_BT_A2DP_PLAYBACK;
            break;
#endif
        case DEVICE_BT_SCO_HEADSET_MIC:
        case DEVICE_BT_NREC_HEADSET_MIC:
            if (adev->voice && adev->voice->bluetooth_samplerate == WB_SAMPLING_RATE)
                type = MODIFIER_BT_SCO_TX_WB;
            else
                type = MODIFIER_BT_SCO_TX_NB;
            break;

        default:
            ALOGVV("device-%s: (%d) device, skip to set mixer", __func__, device);
            break;
    }

    ALOGVV("device-%s return type:%d", __func__, type);
    return type;
}

// Active Stream Count
static int get_active_playback_count(struct audio_device *adev, struct stream_out *out)
{
    int active_count = 0;
    struct listnode *node;
    struct playback_stream *out_node;
    struct stream_out *temp_out;

    list_for_each(node, &adev->playback_list)
    {
        out_node = node_to_item(node, struct playback_stream, list_node);
        if (out_node) {
            temp_out = out_node->out;
            if ((temp_out != out) &&
                (temp_out->common.stream_type != ASTREAM_PLAYBACK_AUX_DIGITAL) &&
                (temp_out->common.stream_status > STATUS_STANDBY))
                active_count++;
        }
    }

    return active_count;
}

static int get_active_capture_count(struct audio_device *adev)
{
    int active_count = 0;
    struct listnode *node;
    struct capture_stream *in_node;
    struct stream_in *in;

    list_for_each(node, &adev->capture_list)
    {
        in_node = node_to_item(node, struct capture_stream, node);
        if (in_node) {
            in = in_node->in;

            if ((in->common.stream_type != ASTREAM_CAPTURE_CALL) &&
                (in->common.stream_status > STATUS_STANDBY))
                active_count++;
        }
    }

    return active_count;
}

void update_call_stream(struct stream_out *out, audio_devices_t current_devices, audio_devices_t new_devices)
{
    struct audio_device *adev = out->adev;
    struct listnode *node, *auxi;
    struct playback_stream *out_node;
    struct capture_stream  *in_node;
    bool call_state_changed = false;
    bool device_changed = false;
    device_type cur_device = get_device_id(adev, current_devices);
    device_type new_device = get_device_id(adev, new_devices);
    device_type cur_in_device = DEVICE_NONE;
    device_type new_in_device = DEVICE_NONE;

    ALOGD("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_t *lock[ASTREAM_MAX] = {NULL, };

    if (isCallMode(adev)) {
        cur_in_device = adev->active_capture_device;
        new_in_device = get_indevice_id_from_outdevice(adev, new_device);
    }

    // call fwd <-> other path
    // by call_fwd on, get_device_id will return DEVICE_CALL_FWD cur/new both.
    // need to re-mapping cur device to set path_changed true.
    if (adev->active_playback_device == DEVICE_CALL_FWD) {
        cur_device = DEVICE_CALL_FWD;
    } else if (is_active_usage_CPCall(adev->proxy) && (new_device == DEVICE_CALL_FWD)) {
        cur_device = adev->active_playback_device;
    }

    if ((cur_device != new_device) ||
       ((cur_in_device != new_in_device) &&
        (cur_in_device == DEVICE_USB_HEADSET_MIC || new_in_device == DEVICE_USB_HEADSET_MIC))) {
        device_changed = true;
    }

    if ((isAPCallMode(adev) && !is_usage_APCall(adev->active_capture_ausage)) ||
        (isCPCallMode(adev) && is_usage_APCall(adev->active_capture_ausage)) ||
        (!isCPCallMode(adev) && is_usage_CPCall(adev->active_capture_ausage))) {
        call_state_changed = true;
    }

    list_for_each_safe(node, auxi, &adev->playback_list) {
        out_node = node_to_item(node, struct playback_stream, list_node);
        if (out_node && out_node->out &&
            (out_node->out->common.stream_type == ASTREAM_PLAYBACK_PRIMARY) &&
            out_node->out->common.stream_status != STATUS_STANDBY) {
            lock[out_node->out->common.stream_type] = &out_node->out->common.lock;
            if (lock[out_node->out->common.stream_type]) {
                pthread_mutex_lock(lock[out_node->out->common.stream_type]);
                 /* RDMA PCM re-open for rcv <-> other path during call or when ending call*/
                if (out_node->out->common.proxy_stream
                    && (device_changed || call_state_changed)) {
                    proxy_stop_playback_stream((void *)(out_node->out->common.proxy_stream));
                    proxy_close_playback_stream((void *)(out_node->out->common.proxy_stream));
                    out_node->out->common.stream_status = STATUS_STANDBY;
                }
            }
        }
    }

    list_for_each_safe(node, auxi, &adev->capture_list) {
        in_node = node_to_item(node, struct capture_stream, node);
        if (in_node && in_node->in && in_node->in->common.stream_status > STATUS_STANDBY) {
            lock[in_node->in->common.stream_type] = &in_node->in->common.lock;
            if (lock[in_node->in->common.stream_type] &&
                ((in_node->in->common.stream_type == ASTREAM_CAPTURE_PRIMARY))) {
                pthread_mutex_lock(lock[in_node->in->common.stream_type]);
                /* WDMA PCM re-open for path change during call or when entering or ending call*/
                if (in_node->in->common.proxy_stream && in_node->in->common.stream_status == STATUS_PLAYING &&
                    ((isCallMode(adev) && device_changed) || call_state_changed ||
                    need_to_reconfig_capture(adev, in_node->in))) {
                    proxy_stop_capture_stream((void *)(in_node->in->common.proxy_stream));
                    proxy_close_capture_stream((void *)(in_node->in->common.proxy_stream));
                    in_node->in->pcm_reconfig = true;
                }
            }
        }
    }

    pthread_mutex_lock(&adev->lock);
    if ((device_changed && adev->voice && voice_is_call_active(adev->voice)) ||
         adev->active_playback_ausage == AUSAGE_INCALL_MUSIC) {
        if (!adev->voice->mute_voice) {
            voice_set_rx_mute(adev->voice, true);
            proxy_set_volume(adev->proxy, VOLUME_TYPE_CALL_RX_MUTE, true, true);
        }

        voice_set_call_active(adev->voice, false);
        proxy_stop_voice_call(adev->proxy);
        proxy_set_volume(adev->proxy, VOLUME_TYPE_CALL_RX_MUTE, false, false);
    }

    /* Do actual routing */
    if (isCallMode(adev)) {
        adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, CALL_DRIVE);
        proxy_set_call_path_param(CALL_PATH_SET, PARAM_NONE, 0);
    } else if (out->common.stream_status == STATUS_STANDBY) {
        adev_set_route((void *)out, AUSAGE_PLAYBACK, UNROUTE, CALL_DRIVE); // to disable call rx/tx path
    } else {
        ALOGI("%s-%s: skip route", stream_table[out->common.stream_type], __func__);
    }

    pthread_mutex_unlock(&adev->lock);

    for (int idx = 0; idx < ASTREAM_MAX; idx++) {
        if (lock[idx] != NULL)
            pthread_mutex_unlock(lock[idx]);
    }

    ALOGD("%s-%s: exit", stream_table[out->common.stream_type], __func__);
}

void update_capture_stream(struct stream_in *in, audio_devices_t current_devices, audio_devices_t new_devices)
{
    struct audio_device *adev = in->adev;
    device_type cur_in_device = get_device_id(adev, current_devices);
    device_type new_in_device = get_device_id(adev, new_devices);

    if(!isAPCallMode(adev) && !isCPCallMode(adev)) {
        if((cur_in_device == DEVICE_BT_SCO_HEADSET_MIC && new_in_device != DEVICE_BT_SCO_HEADSET_MIC)
            || (cur_in_device != DEVICE_BT_SCO_HEADSET_MIC && new_in_device == DEVICE_BT_SCO_HEADSET_MIC)
            || (cur_in_device == DEVICE_USB_HEADSET_MIC && new_in_device != DEVICE_USB_HEADSET_MIC)
            || (cur_in_device != DEVICE_USB_HEADSET_MIC && new_in_device == DEVICE_USB_HEADSET_MIC)
            || (cur_in_device == DEVICE_HEADSET_MIC && new_in_device != DEVICE_HEADSET_MIC)
            || (cur_in_device != DEVICE_HEADSET_MIC && new_in_device == DEVICE_HEADSET_MIC)
            || (cur_in_device != DEVICE_MAIN_MIC && new_in_device == DEVICE_MAIN_MIC)
            || (cur_in_device == DEVICE_MAIN_MIC && new_in_device != DEVICE_MAIN_MIC)) {
            if (in->common.stream_status > STATUS_STANDBY) {
                in->pcm_reconfig = true;
                ALOGI("%s pcm_reconfig=true", __func__);
            }
        }
    }
}

/* This function checks that output stream is Primary Output Stream or not */
static bool is_primary_output(struct audio_device *adev, struct stream_out *out)
{
    return out == adev->primary_output;
}

static bool is_fast_output(struct audio_device *adev, struct stream_out *out)
{
    return out == adev->fast_output;
}

/* This function checks that output stream can control Voice Call */
static bool output_drives_call(struct audio_device *adev, struct stream_out *out)
{
    /* Only Primary Output Stream can control Voice Call */
    return (out == adev->primary_output ||
            out->common.stream_usage == AUSAGE_INCALL_MUSIC);
}

static bool adev_init_route(struct audio_device *adev)
{
    FILE *fp = NULL;
    char mixer_info[MAX_MIXER_LENGTH] = MIXER_PATH_INFO;
    char mixer_file[MAX_MIXER_LENGTH];
    char mixer_path[MAX_MIXER_LENGTH];
    bool use_default = true;
    bool ret;

    fp = fopen(mixer_info, "r");
    if (fp) {
        int readBytes = fread(mixer_file, sizeof(char), sizeof(mixer_file), fp);
        if (readBytes > 0) {
            mixer_file[readBytes] = 0;  // add termination character

            memset(mixer_path, 0, MAX_MIXER_LENGTH);
            strcpy(mixer_path, DEFAULT_MIXER_PATH);
            strcat(mixer_path, mixer_file);

            ALOGI("proxy-%s: there is mixer_info, will use specific mixer file(%s)",
                             __func__, mixer_path);
            ret = proxy_init_route(adev->proxy, mixer_path);
            if (ret)
                use_default = false;
        }
        fclose(fp);
    }

    if (use_default) {
        memset(mixer_path, 0, MAX_MIXER_LENGTH);
        strcpy(mixer_path, DEFAULT_MIXER_PATH);
        strcat(mixer_path, DEFAULT_MIXER_FILE);

        ALOGI("proxy-%s: no mixer_info or there is error, will use default mixer file(%s)",
                         __func__, mixer_path);
        ret = proxy_init_route(adev->proxy, mixer_path);
    }

    return ret;
}

static void adev_deinit_route(struct audio_device *adev)
{
    proxy_deinit_route(adev->proxy);
    return ;
}

bool adev_set_route(void *stream, audio_usage_type usage_type, bool set, force_route force)
{
    struct audio_device *adev = NULL;
    struct stream_out *out = NULL;
    struct stream_in *in = NULL;

    bool drive_capture_route = false;
    bool capture_set = set;

    bool ret = true;

    // Check Audio Path Routing Skip
    {
        if (usage_type == AUSAGE_PLAYBACK) {
            out = (struct stream_out *)stream;
            adev = out->adev;
        }
        else if (usage_type == AUSAGE_CAPTURE) {
            in = (struct stream_in *)stream;
            adev = in->adev;
        }

        //
        if ((usage_type == AUSAGE_CAPTURE) && isAPCallMode(adev)) {
            if (set) {
                if (in->common.stream_status > STATUS_STANDBY) {
                    ALOGI("%s: need to routing tx/rx both for ap call case", __func__);
                    adev_set_route((void *)adev->primary_output, AUSAGE_PLAYBACK, ROUTE, CALL_DRIVE);
                    return ret;
                }
            } else {
                ALOGI("%s: disable only tx for ap call case", __func__);
            }
        }

        // In case of FM Recording, it could need to skip audio path routing
        if (fmradio_need_skip_routing(adev, in, usage_type))
            return ret;
    }

    // Playback(Output) Path Control
    if (usage_type == AUSAGE_PLAYBACK) {
        audio_usage   new_playback_ausage   = AUSAGE_NONE;
        device_type   new_playback_device   = DEVICE_NONE;
        modifier_type new_playback_modifier = MODIFIER_NONE;

        audio_usage   old_playback_ausage   = AUSAGE_NONE;
        device_type   old_playback_device   = DEVICE_NONE;
        modifier_type old_playback_modifier = MODIFIER_NONE;

        out = (struct stream_out *)stream;
        adev = out->adev;

        if (output_drives_call(adev, out) && force == CALL_DRIVE) {
            drive_capture_route = true;

            if ((isAPCallMode(adev) || (!isCPCallMode(adev) && is_active_usage_CPCall(adev->proxy)))
                && !(adev->active_input && (adev->active_input->common.stream_status > STATUS_STANDBY))) {
                    capture_set = UNROUTE; // need to enable rx and disable rx/tx both
            } else if (adev->active_input && !isCallMode(adev) &&
                       adev->active_input->common.stream_type == ASTREAM_CAPTURE_PRIMARY) {
                /* after CP Call stopped, normal rec is in progress
                   don't need capture path routing, in this routing req */
                capture_set = UNROUTE;
            }
            ALOGI("%s: %s rx and %s tx both for CALL_DRIVE state",
                    __func__, set ? "enable" : "disable", capture_set ? "enable" : "disable");
        }

        if (set) {
            // Selection Audio Usage & Audio Device & Modifier
            new_playback_ausage   = adev_get_ausage_from_stream(stream, usage_type);
            new_playback_device   = adev_get_device(stream, usage_type);
            new_playback_modifier = adev_get_modifier(adev, new_playback_device);

            /*
            ** In cases of DP Audio Playback, special DMA is used
            ** without any relation to audio path routing. So, keep previous path.
            */
            if (new_playback_ausage == AUSAGE_MEDIA &&
               new_playback_device == DEVICE_AUX_DIGITAL) {
                ALOGI("%s-%s: keep current route for device(%s) and usage(%s)",
                              stream_table[out->common.stream_type], __func__,
                              device_table[adev->active_playback_device],
                              usage_table[adev->active_playback_ausage]);
                return ret;
            }

            if (adev->is_playback_path_routed) {
                /* Route Change Case */
                if ((adev->active_playback_ausage == new_playback_ausage) &&
                    (adev->active_playback_device == new_playback_device)) {
                    // Requested same usage and same device, skip!!!
                    ALOGI("%s-%s-1: skip re-route as same device(%s) and same usage(%s)",
                           stream_table[out->common.stream_type], __func__,
                           device_table[new_playback_device], usage_table[new_playback_ausage]);
                } else {
                    // Requested different usage or device, re-route!!!
                    proxy_set_route(adev->proxy, (int)new_playback_ausage,
                                   (int)new_playback_device, (int)new_playback_modifier, ROUTE);
                    ALOGI("%s-%s-1: re-routes to device(%s) for usage(%s)",
                           stream_table[out->common.stream_type], __func__,
                           device_table[new_playback_device], usage_table[new_playback_ausage]);

                    fmradio_set_mixer_for_route(adev, new_playback_ausage);
                }
            } else {
                /* New Route Case */
                proxy_set_route(adev->proxy, (int)new_playback_ausage,
                               (int)new_playback_device, (int)new_playback_modifier, ROUTE);
                ALOGI("%s-%s-1: routes to device(%s) for usage(%s)",
                      stream_table[out->common.stream_type], __func__,
                      device_table[new_playback_device], usage_table[new_playback_ausage]);

                adev->is_playback_path_routed = true;

                fmradio_set_mixer_for_route(adev, new_playback_ausage);
            }
        } else {
            // Get current active Audio Usage & Audio Device & Modifier
            old_playback_ausage   = adev->active_playback_ausage;
            old_playback_device   = adev->active_playback_device;
            old_playback_modifier = adev->active_playback_modifier;

            if (adev->is_playback_path_routed) {
                /* Route Reset Case */
                if ((force != FORCE_ROUTE) &&
                    ((get_active_playback_count(adev, out) > 0) || isCPCallMode(adev)
                    || (adev->factory && adev->factory->mode == FACTORY_MODE_LOOPBACK)
                    || (isFMRadioOn(adev)))) {
                    // There are some active playback stream or CP Call Mode or Loopback Mode or FM Radio is ON
                    ALOGI("%s-%s-1: current device(%s) is still in use by other streams or CP Call Mode or Loopback Mode or FM Radio is ON",
                          stream_table[out->common.stream_type], __func__,
                          device_table[old_playback_device]);

                    new_playback_ausage   = old_playback_ausage;
                    new_playback_device   = old_playback_device;
                    new_playback_modifier = old_playback_modifier;
                } else {
                    // There are no active playback stream
                    proxy_set_route(adev->proxy, (int)old_playback_ausage,
                                   (int)old_playback_device, (int)old_playback_modifier, UNROUTE);

                    ALOGI("%s-%s-1: unroutes to device(%s) for usage(%s)",
                          stream_table[out->common.stream_type], __func__,
                          device_table[old_playback_device], usage_table[old_playback_ausage]);

                    adev->is_playback_path_routed = false;
                }
            } else {
                /* Abnormal Case */
                ALOGE("%s-%s-1: already unrouted", stream_table[out->common.stream_type], __func__);
            }
        }

        adev->active_playback_ausage   = new_playback_ausage;
        adev->active_playback_device   = new_playback_device;
        adev->active_playback_modifier = new_playback_modifier;
    }

    // Capture(Input) Path Control
    if (usage_type == AUSAGE_CAPTURE ||
       (usage_type == AUSAGE_PLAYBACK && drive_capture_route)) {
        audio_usage   new_capture_ausage   = AUSAGE_NONE;
        device_type   new_capture_device   = DEVICE_NONE;
        modifier_type new_capture_modifier = MODIFIER_NONE;

        audio_usage   old_capture_ausage   = AUSAGE_NONE;
        device_type   old_capture_device   = DEVICE_NONE;
        modifier_type old_capture_modifier = MODIFIER_NONE;

        // In case of Call, Primary Playback Stream will drive Capture(Input) Path Control
        if (drive_capture_route) {
            if (capture_set) {
                // Selection Audio Usage & Audio Device & Modifier
                new_capture_ausage   = adev_get_ausage_from_stream(stream, usage_type);
                new_capture_device   = adev_get_capture_device(stream, usage_type);
                new_capture_modifier = adev_get_modifier(adev, new_capture_device);

                if (adev->is_capture_path_routed) {
                    /* Route Change Case */
                    if ((adev->active_capture_ausage == new_capture_ausage) &&
                        (adev->active_capture_device == new_capture_device)) {
                        // Requested same usage and same device, skip!!!
                        ALOGI("%s-%s-2: skip re-route as same device(%s) and same usage(%s)",
                              stream_table[out->common.stream_type], __func__,
                              device_table[new_capture_device], usage_table[new_capture_ausage]);
                    } else {
                        // Requested different usage or device, re-route!!!
                        proxy_set_route(adev->proxy, (int)new_capture_ausage,
                                       (int)new_capture_device, (int)new_capture_modifier, ROUTE);
                        ALOGI("%s-%s-2: re-routes to device(%s) for usage(%s)",
                              stream_table[out->common.stream_type], __func__,
                              device_table[new_capture_device], usage_table[new_capture_ausage]);
                    }
                } else {
                    /* New Route Case */
                    proxy_set_route(adev->proxy, (int)new_capture_ausage,
                                   (int)new_capture_device, (int)new_capture_modifier, ROUTE);
                    ALOGI("%s-%s-2: routes to device(%s) for usage(%s)",
                          stream_table[out->common.stream_type], __func__,
                          device_table[new_capture_device], usage_table[new_capture_ausage]);

                    adev->is_capture_path_routed = true;
                }
            } else {
                // Get current active Audio Usage & Audio Device & Modifier
                old_capture_ausage   = adev->active_capture_ausage;
                old_capture_device   = adev->active_capture_device;
                old_capture_modifier = adev->active_capture_modifier;

                if (adev->is_capture_path_routed) {
                    /* Route Reset Case */
                    if (get_active_capture_count(adev) > 0) {
                        // There are some active capture stream
                        ALOGI("%s-%s-2: current device(%s) is still in use by other streams",
                              stream_table[out->common.stream_type], __func__,
                              device_table[old_capture_device]);

                        new_capture_ausage   = old_capture_ausage;
                        new_capture_device   = old_capture_device;
                        new_capture_modifier = old_capture_modifier;
                    } else {
                        // There are no active capture stream
                        proxy_set_route(adev->proxy, (int)old_capture_ausage,
                                       (int)old_capture_device, (int)old_capture_modifier, UNROUTE);

                        ALOGI("%s-%s-2: unroutes to device(%s) for usage(%s)",
                              stream_table[out->common.stream_type], __func__,
                              device_table[old_capture_device], usage_table[old_capture_ausage]);

                        adev->is_capture_path_routed = false;
                    }
                } else {
                    /* Abnormal Case */
                    ALOGE("%s-%s-2: already unrouted", stream_table[out->common.stream_type], __func__);
                }
            }
        }
        // General Capture Stream
        else {
            in = (struct stream_in *)stream;
            adev = in->adev;

            if (set) {
                // Selection Audio Usage & Audio Device & Modifier
                new_capture_ausage   = adev_get_ausage_from_stream(stream, usage_type);
                new_capture_device   = adev_get_device(stream, usage_type);
                new_capture_modifier = adev_get_modifier(adev, new_capture_device);

                if (adev->is_capture_path_routed) {
                    /* Route Change Case */
                    if ((adev->active_capture_ausage == new_capture_ausage) &&
                        (adev->active_capture_device == new_capture_device)) {
                        // Requested same usage and same device, skip!!!
                        ALOGI("%s-%s-3: skip re-route as same device(%s) and same usage(%s)",
                              stream_table[in->common.stream_type], __func__,
                              device_table[new_capture_device], usage_table[new_capture_ausage]);
                    } else {
                        // Requested different usage or device, re-route!!!
                        proxy_set_route(adev->proxy, (int)new_capture_ausage,
                                       (int)new_capture_device, (int)new_capture_modifier, ROUTE);
                        ALOGI("%s-%s-3: re-routes to device(%s) for usage(%s)",
                              stream_table[in->common.stream_type], __func__,
                              device_table[new_capture_device], usage_table[new_capture_ausage]);
                    }
                } else {
                    /* New Route Case */
                    proxy_set_route(adev->proxy, (int)new_capture_ausage,
                                   (int)new_capture_device, (int)new_capture_modifier, ROUTE);
                    ALOGI("%s-%s-3: routes to device(%s) for usage(%s)",
                          stream_table[in->common.stream_type], __func__,
                          device_table[new_capture_device], usage_table[new_capture_ausage]);

                    adev->is_capture_path_routed = true;
                }
            } else {
                // Get current active Audio Usage & Audio Device & Modifier
                old_capture_ausage   = adev->active_capture_ausage;
                old_capture_device   = adev->active_capture_device;
                old_capture_modifier = adev->active_capture_modifier;

                if (adev->is_capture_path_routed) {
                    /* Route Reset Case */
                    if ((force != FORCE_ROUTE) &&
                        ((get_active_capture_count(adev) > 0) || isCPCallMode(adev))) {
                        // There are some active capture stream
                        ALOGI("%s-%s-3: current device(%s) is still in use by other streams or CallMode",
                              stream_table[in->common.stream_type], __func__,
                              device_table[old_capture_device]);

                        new_capture_ausage   = old_capture_ausage;
                        new_capture_device   = old_capture_device;
                        new_capture_modifier = old_capture_modifier;
                    } else {
                        // There are no active capture stream
                        proxy_set_route(adev->proxy, (int)old_capture_ausage,
                                       (int)old_capture_device, (int)old_capture_modifier, UNROUTE);

                        ALOGI("%s-%s-3: unroutes to device(%s) for usage(%s)",
                              stream_table[in->common.stream_type], __func__,
                              device_table[old_capture_device], usage_table[old_capture_ausage]);

                        adev->is_capture_path_routed = false;
                    }
                } else {
                    /* Abnormal Case */
                    ALOGE("%s-%s-3: already unrouted", stream_table[in->common.stream_type], __func__);
                }
            }
        }

        adev->active_capture_ausage   = new_capture_ausage;
        adev->active_capture_device   = new_capture_device;
        adev->active_capture_modifier = new_capture_modifier;
    }

    return ret;
}

// set call fwd path
void set_call_forwarding(struct audio_device *adev, bool mode)
{
    ALOGI("%s: enter : %s", __func__, mode ? "on" : "off");
    if (adev->voice && voice_is_call_mode(adev->voice)) {
        pthread_mutex_unlock(&adev->lock);
        if (adev->primary_output && adev->primary_output->common.requested_devices != 0)
            update_call_stream(adev->primary_output, adev->primary_output->common.requested_devices, adev->primary_output->common.requested_devices);
        pthread_mutex_lock(&adev->lock);

        /* Start Call */
        if (adev->proxy)
            proxy_start_voice_call(adev->proxy);
        voice_set_call_active(adev->voice, true);
    } else {
        if (adev->primary_output)
            adev_set_route((void *)adev->primary_output, AUSAGE_PLAYBACK, ROUTE, CALL_DRIVE);
    }
}


/****************************************************************************/
/**                                                                        **/
/** Compress Offload Specific Functions Implementation                     **/
/**                                                                        **/
/****************************************************************************/
static int send_offload_msg(struct stream_out *out, offload_msg_type msg)
{
    struct offload_msg *msg_node = NULL;
    int ret = 0;

    msg_node = (struct offload_msg *)calloc(1, sizeof(struct offload_msg));
    if (msg_node) {
        msg_node->msg = msg;

        list_add_tail(&out->offload.msg_list, &msg_node->node);
        pthread_cond_signal(&out->offload.msg_cond);

        ALOGVV("offload_out-%s: Sent Message = %s", __func__, offload_msg_table[msg]);
    } else {
        ALOGE("offload_out-%s: Failed to allocate memory for Offload MSG", __func__);
        ret = -ENOMEM;
    }

    return ret;
}

static offload_msg_type recv_offload_msg(struct stream_out *out)
{
    struct listnode *offload_msg_list = &(out->offload.msg_list);

    struct listnode *head = list_head(offload_msg_list);
    struct offload_msg *msg_node = node_to_item(head, struct offload_msg, node);
    offload_msg_type msg = msg_node->msg;

    list_remove(head);
    free(msg_node);

    ALOGVV("offload_out-%s: Received Message = %s", __func__, offload_msg_table[msg]);
    return msg;
}

static void *offload_cbthread_loop(void *context)
{
    struct stream_out *out = (struct stream_out *) context;
    bool get_exit = false;
    int ret = 0;

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    set_sched_policy(0, SP_FOREGROUND);
    prctl(PR_SET_NAME, (unsigned long)"Offload Callback", 0, 0, 0);

    ALOGI("%s-%s: Started running Offload Callback Thread", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    do {
        offload_msg_type msg = OFFLOAD_MSG_INVALID;
        stream_callback_event_t event;
        bool need_callback = true;

        if (list_empty(&out->offload.msg_list)) {
            ALOGVV("%s-%s: transit to sleep", stream_table[out->common.stream_type], __func__);
            pthread_cond_wait(&out->offload.msg_cond, &out->common.lock);
            ALOGVV("%s-%s: transit to wake-up", stream_table[out->common.stream_type], __func__);
        }

        if (!list_empty(&out->offload.msg_list))
            msg = recv_offload_msg(out);

        if (msg == OFFLOAD_MSG_EXIT) {
            get_exit = true;
            continue;
        }

        out->offload.callback_thread_blocked = true;
        pthread_mutex_unlock(&out->common.lock);

        switch (msg) {
            case OFFLOAD_MSG_WAIT_WRITE:
                // call compress_wait
                ret = proxy_offload_compress_func(out->common.proxy_stream, COMPRESS_TYPE_WAIT);
                // In case of Wait(Write Block), Error Callback is not needed.
                event = STREAM_CBK_EVENT_WRITE_READY;
                break;

            case OFFLOAD_MSG_WAIT_PARTIAL_DRAIN:
                // call compress_next_track
                ret = proxy_offload_compress_func(out->common.proxy_stream, COMPRESS_TYPE_NEXTTRACK);

                // call compress_partial_drain
                ret = proxy_offload_compress_func(out->common.proxy_stream, COMPRESS_TYPE_PARTIALDRAIN);
                if (ret) {
                    event = STREAM_CBK_EVENT_ERROR;
                    ALOGE("%s-%s: will Callback Error", stream_table[out->common.stream_type], __func__);
                } else
                    event = STREAM_CBK_EVENT_DRAIN_READY;

                /* gapless playback requires compress_start for kernel 4.4 while moving to Next track
                   therefore once partial drain is completed state changed to IDLE and when next
                   compress_write is called state is changed back to PLAYING */
                pthread_mutex_lock(&out->common.lock);
                out->common.stream_status = STATUS_IDLE;
                pthread_mutex_unlock(&out->common.lock);
                ALOGI("%s-%s: Transit to Idle Without compress-stop ", stream_table[out->common.stream_type], __func__);
                break;

            case OFFLOAD_MSG_WAIT_DRAIN:
                // call compress_drain
                ret = proxy_offload_compress_func(out->common.proxy_stream, COMPRESS_TYPE_DRAIN);
                if (ret) {
                    event = STREAM_CBK_EVENT_ERROR;
                    ALOGE("%s-%s: will Callback Error", stream_table[out->common.stream_type], __func__);
                } else
                    event = STREAM_CBK_EVENT_DRAIN_READY;
                break;

            default:
                ALOGE("Invalid message = %u", msg);
                need_callback = false;
                break;
        }

        pthread_mutex_lock(&out->common.lock);
        out->offload.callback_thread_blocked = false;
        pthread_cond_signal(&out->offload.sync_cond);

        if (need_callback) {
            out->offload.callback(event, NULL, out->offload.cookie);
            if (event == STREAM_CBK_EVENT_DRAIN_READY)
                ALOGD("%s-%s: Callback to Platform with %d", stream_table[out->common.stream_type],
                                                             __func__, event);
        }
    } while(!get_exit);

    /* Clean the message list */
    pthread_cond_signal(&out->offload.sync_cond);
    while(!list_empty(&out->offload.msg_list))
        recv_offload_msg(out);
    pthread_mutex_unlock(&out->common.lock);

    ALOGI("%s-%s: Stopped running Offload Callback Thread", stream_table[out->common.stream_type], __func__);
    return NULL;
}

static int create_offload_callback_thread(struct stream_out *out)
{
    pthread_cond_init(&out->offload.msg_cond, (const pthread_condattr_t *) NULL);
    pthread_cond_init(&out->offload.sync_cond, (const pthread_condattr_t *) NULL);

    pthread_create(&out->offload.callback_thread, (const pthread_attr_t *) NULL, offload_cbthread_loop, out);
    out->offload.callback_thread_blocked = false;

    return 0;
}

static int destroy_offload_callback_thread(struct stream_out *out)
{
    int ret = 0;

    pthread_mutex_lock(&out->common.lock);
    ret = send_offload_msg(out, OFFLOAD_MSG_EXIT);
    pthread_mutex_unlock(&out->common.lock);

    pthread_join(out->offload.callback_thread, (void **) NULL);
    ALOGI("%s-%s: Joined Offload Callback Thread!", stream_table[out->common.stream_type], __func__);

    pthread_cond_destroy(&out->offload.sync_cond);
    pthread_cond_destroy(&out->offload.msg_cond);

    return 0;
}

/****************************************************************************/
/**                                                                        **/
/** The Stream_Out Function Implementation                                 **/
/**                                                                        **/
/****************************************************************************/
static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGVV("%s-%s: exit with sample rate = %u", stream_table[out->common.stream_type], __func__,
                                                out->common.requested_sample_rate);
    return out->common.requested_sample_rate;
}

static int out_set_sample_rate(struct audio_stream *stream __unused, uint32_t rate __unused)
{
    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    size_t buffer_size = 0;

    ALOGVV("%s-%s: enter", stream_table[out->common.stream_type], __func__);

    // will return buffer size based on requested PCM configuration
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD)
        buffer_size = proxy_get_actual_period_size(out->common.proxy_stream);
    else
    buffer_size = proxy_get_actual_period_size(out->common.proxy_stream) *
                  (unsigned int)audio_stream_out_frame_size((const struct audio_stream_out *)stream);

    ALOGVV("%s-%s: exit with %d bytes", stream_table[out->common.stream_type], __func__, (int)buffer_size);
    return buffer_size;
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGVV("%s-%s: exit with channel mask = 0x%x", stream_table[out->common.stream_type], __func__,
                                                  out->common.requested_channel_mask);
    return out->common.requested_channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGVV("%s-%s: exit with audio format = 0x%x", stream_table[out->common.stream_type], __func__,
                                                   out->common.requested_format);
    return out->common.requested_format;
}

static int out_set_format(struct audio_stream *stream __unused, audio_format_t format __unused)
{
    return -ENOSYS;
}

static int out_standby(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;

    ALOGVV("%s-%s: enter", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_status > STATUS_STANDBY) {
        /* Stops stream & transit to Idle. */
        if (out->common.stream_status > STATUS_IDLE) {
            if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD &&
                out->offload.callback_thread_blocked)
                pthread_cond_wait(&out->offload.sync_cond, &out->common.lock);

            proxy_stop_playback_stream((void *)(out->common.proxy_stream));
            out->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[out->common.stream_type], __func__);
        }

        /* Closes device & transit to Standby. */
        proxy_close_playback_stream((void *)(out->common.proxy_stream));
        out->common.stream_status = STATUS_STANDBY;
        ALOGI("%s-%s: transited to Standby", stream_table[out->common.stream_type], __func__);

        // Have to unroute Audio Path after close PCM Device
        pthread_mutex_lock(&adev->lock);
        if (adev->is_playback_path_routed) {
            if (out->common.stream_type == ASTREAM_PLAYBACK_INCALL_MUSIC &&
                adev->incallmusic_on && isCPCallMode(adev)) {
                ALOGI("%s-%s: try to re-route to call path for standby", stream_table[out->common.stream_type], __func__);
                // Incall-music should be disabled before re-routing call path, to get proper call path
                adev->incallmusic_on = false;
                if (adev->primary_output) {
                    pthread_mutex_unlock(&adev->lock);
                    pthread_mutex_unlock(&out->common.lock);
                    update_call_stream(adev->primary_output, adev->primary_output->common.requested_devices, adev->primary_output->common.requested_devices);
                    proxy_start_voice_call(adev->proxy);
                    ALOGI("%s-%s: *** Started CP Voice Call ***",
                           stream_table[out->common.stream_type], __func__);
                    pthread_mutex_lock(&out->common.lock);
                    pthread_mutex_lock(&adev->lock);
                } else {
                    ALOGW("%s-%s: Call routing skipped", stream_table[out->common.stream_type], __func__);
                }
            } else {
                ALOGI("%s-%s: try to unroute for standby", stream_table[out->common.stream_type], __func__);
                if (out->common.stream_type == ASTREAM_PLAYBACK_INCALL_MUSIC)
                    adev->incallmusic_on = false;
                adev_set_route((void *)out, AUSAGE_PLAYBACK, UNROUTE, NON_FORCE_ROUTE);
            }
        }
        out->force = NON_FORCE_ROUTE;
        out->rollback_devices = AUDIO_DEVICE_NONE;

        pthread_mutex_unlock(&adev->lock);
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGVV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s-%s: enter with fd(%d)", stream_table[out->common.stream_type], __func__, fd);

    const size_t len = 256;
    char buffer[len];
    snprintf(buffer, len, "\nAudio Stream Out(%x)::dump\n", out->requested_flags);
    write(fd,buffer,strlen(buffer));
    bool justLocked = pthread_mutex_trylock(&out->common.lock) == 0;
    snprintf(buffer, len, "\tMutex: %s\n", justLocked ? "locked" : "unlocked");
    write(fd,buffer,strlen(buffer));
    if(justLocked)
        pthread_mutex_unlock(&out->common.lock);

    snprintf(buffer, len, "\toutput devices %d\n",out->common.requested_devices);
    write(fd,buffer,strlen(buffer));

    snprintf(buffer, len, "\toutput flgas: %x\n",out->requested_flags);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\toutput sample rate: %u\n",out->common.requested_sample_rate);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\toutput channel mask: %d\n",out->common.requested_channel_mask);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\toutput format: %d\n",out->common.requested_format);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\toutput audio usage: %d\n",out->common.stream_usage);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\toutput standby state: %d\n",out->common.stream_status);
    write(fd,buffer,strlen(buffer));

    proxy_dump_playback_stream(out->common.proxy_stream, fd);

    ALOGV("%s-%s: exit with fd(%d)", stream_table[out->common.stream_type], __func__, fd);

    return 0;
}

static audio_devices_t out_get_device(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGVV("%s-%s: exit with device = %u", stream_table[out->common.stream_type], __func__,
                                                   out->common.requested_devices);
    return out->common.requested_devices;
}

static int out_set_device(struct audio_stream *stream __unused, audio_devices_t device __unused)
{
    return -ENOSYS;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    audio_usage ausage;

    struct str_parms *parms;
    char value[32];
    int ret = 0;
    int val = 0;

    ALOGD("%s-%s: enter with param = %s", stream_table[out->common.stream_type], __func__, kvpairs);

    parms = str_parms_create_str(kvpairs);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        proxy_setparam_playback_stream(out->common.proxy_stream, (void *)parms);
    }

    odm_out_set_parameters(adev, out, parms);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        audio_devices_t requested_devices = atoi(value);
        audio_devices_t current_devices = out->common.requested_devices;
        bool need_routing = false;
#ifdef SUPPORT_BTA2DP_OFFLOAD
        bool bypass_a2dp = false;
#endif
        /*
         * AudioFlinger informs Audio path is this device.
         * AudioHAL has to decide to do actual routing or not.
         */
        if (requested_devices != AUDIO_DEVICE_NONE) {
            ALOGI("%s-%s: requested to change route from %s to %s",
                  stream_table[out->common.stream_type], __func__,
                  device_table[get_device_id(adev, current_devices)],
                  device_table[get_device_id(adev, requested_devices)]);

#ifdef SUPPORT_BTA2DP_OFFLOAD
            /* To avoid a2dp to sco overlapping / BT device improper state
             * check with BT lib about a2dp streaming support before routing
             */
            if ((requested_devices & AUDIO_DEVICE_OUT_ALL_A2DP) && !proxy_is_bt_a2dp_ready()) {
                if (requested_devices & AUDIO_DEVICE_OUT_SPEAKER) {
                    //combo usecase just by pass a2dp
                    ALOGW("%s-%s: A2DP profile is not ready, routing to speaker only",
                        stream_table[out->common.stream_type], __func__);
                    bypass_a2dp = true;
                }
            }
#endif

            /* Assign requested device to Output Stream */
            out->common.requested_devices = requested_devices;

            pthread_mutex_lock(&adev->lock);
            /* Check actual routing is needed or not */
            // Check Force Routing for Alarm sound/Shutter tone or Call Routing
            if (is_primary_output(adev, out) && (popcount(requested_devices) == 2)) {
                need_routing = true;
            }
            // Actual routing is needed at CP/AP Voice Call routing request or device change request
            if ((output_drives_call(adev, out) && (isCPCallMode(adev) || isAPCallMode(adev))) ||
                (adev->is_playback_path_routed || (out->common.stream_status > STATUS_STANDBY))) {
                need_routing = true;
            }
            pthread_mutex_unlock(&adev->lock);

            if (need_routing) {
                /* Check routing type */
                out->force = NON_FORCE_ROUTE;
                if ((is_primary_output(adev, out) || is_fast_output(adev,out))&& (popcount(requested_devices) == 2)) {
                    out->force = FORCE_ROUTE;
                    out->rollback_devices = adev->current_devices;
                    ALOGI("%s-%s: adev->rollback_devices = %d", stream_table[out->common.stream_type], __func__, adev->current_devices);

                } else if (output_drives_call(adev, out)) {
                    // CALL_DRIVE routing is for both playback and capture device change
                    // case 1. Device change during CP call mode
                    // case 2. To reset call rx/tx devices When disconnect CP call
                    // case 3. Device change when it has active input during AP call mode
                    if (isCPCallMode(adev)
                        || (!isCPCallMode(adev) && is_active_usage_CPCall(adev->proxy))
                        || (isAPCallMode(adev) && adev->active_input && (adev->active_input->common.stream_status > STATUS_STANDBY)))
                        out->force = CALL_DRIVE;

                    // update incall-music if routing is request for incall-music stream
                    if (out->common.stream_type == ASTREAM_PLAYBACK_INCALL_MUSIC && isCPCallMode(adev) &&
                        requested_devices != AUDIO_DEVICE_NONE) {
                        adev->incallmusic_on = true;
                        ALOGD("%s-%s: enable Incall Music = %d", stream_table[out->common.stream_type],
                                                                 __func__, adev->incallmusic_on);
                    }
                } else {
                    adev->current_devices = requested_devices;
                    ALOGI("%s-%s: adev->current_devices = %d", stream_table[out->common.stream_type], __func__, adev->current_devices);
                }

                pthread_mutex_lock(&adev->lock);
				// CHECK AGAIN
                float voice_vol = 0.0;
                ret = str_parms_get_float(parms, "voice_volume", &voice_vol);
                if (ret >= 0) {
                    adev->voice_volume = voice_vol;
                }

                if (output_drives_call(adev, out) && isCallMode(adev)) {
                    pthread_mutex_unlock(&adev->lock);
                    pthread_mutex_unlock(&out->common.lock);
                    update_call_stream(out, current_devices, requested_devices);
                    pthread_mutex_lock(&out->common.lock);
                    pthread_mutex_lock(&adev->lock);
                } else {
#ifdef SUPPORT_BTA2DP_OFFLOAD
                    if (bypass_a2dp) {
                        out->common.requested_devices = AUDIO_DEVICE_OUT_SPEAKER;
                        adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, out->force);
                        out->common.requested_devices = requested_devices;
                    } else
#endif
                    /* Do actual routing */
                    adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, out->force);
                }

                /* Primary output stream can be handled routing for CP Centric call */
                if (output_drives_call(adev, out) && isCPCallMode(adev)) {
                    if (adev->voice) {
                        /* Set Path to RIL-Client */
                        // In order to reduce set_path latency, call it before Voice PCM Create/Start
                        voice_set_path(adev->voice, out->common.requested_devices);
                        ALOGV("%s-%s: RIL Route Updated for %s",
                              stream_table[out->common.stream_type], __func__,
                              device_table[get_device_id(adev, out->common.requested_devices)]);

                        if (!voice_is_call_active(adev->voice)) {
                            /* Start Call */
                            proxy_start_voice_call(adev->proxy);
                            voice_set_call_active(adev->voice, true);
                            ALOGI("%s-%s: *** Started CP Voice Call ***",
                                  stream_table[out->common.stream_type], __func__);
                        }

                        /* Set Volume to RIL-Client */
                        proxy_set_volume(adev->proxy, VOLUME_TYPE_CALL,
                                         adev->voice_volume, adev->voice_volume);
                        ALOGV("%s-%s: CP Call Volume Updated with %f",
                               stream_table[out->common.stream_type],__func__, adev->voice_volume);

                    }
                } else {
				    // CHECK AGAIN
                    if (output_drives_call(adev, out) && voice_is_call_active(adev->voice)) {
                        ALOGD("%s-%s: requested_devices = %d", stream_table[out->common.stream_type],
                                                               __func__, requested_devices);
                    }
                }

                if (output_drives_call(adev, out) && isCallMode(adev)) {
                    proxy_set_volume(adev->proxy, VOLUME_TYPE_CALL_TX_MUTE, adev->mic_mute, adev->mic_mute);
                    ALOGI("%s-%s: Call Mute status updated with %d", stream_table[out->common.stream_type],
                                                                     __func__, adev->mic_mute);
                }

                pthread_mutex_unlock(&adev->lock);
            } else {
                if (!is_primary_output(adev, out) && !is_fast_output(adev,out)) {
                    adev->current_devices = requested_devices;
                    ALOGD("%s-%s: adev->current_devices = %d", stream_table[out->common.stream_type], __func__, adev->current_devices);
                }
            }
        } else {
            /* When audio device will be changed, AudioFlinger requests to route with AUDIO_DEVICE_NONE */
            ALOGV("%s-%s: requested to change route to AUDIO_DEVICE_NONE",
                  stream_table[out->common.stream_type], __func__);
#ifdef SUPPORT_USB_OFFLOAD
            if (out->common.stream_status > STATUS_STANDBY &&
               (current_devices & AUDIO_DEVICE_OUT_ALL_USB) && !is_usb_connected()) {
                pthread_mutex_lock(&adev->lock);
                out->common.requested_devices = AUDIO_DEVICE_OUT_SPEAKER;
                adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, NON_FORCE_ROUTE);
                pthread_mutex_unlock(&adev->lock);
            }
#endif
        }
    }

    /*
     * Android Audio System can change PCM Configurations(Format, Channel and Rate) for Audio Stream
     * when this Audio Stream is not working.
     */
    // Change Audio Format
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_FORMAT, value, sizeof(value));
    if (ret >= 0) {
        if (out->common.stream_status > STATUS_READY)
            goto not_acceptable;
        else {
            audio_format_t new_format = (audio_format_t)atoi(value);
            if ((new_format != out->common.requested_format) && (new_format != AUDIO_FORMAT_DEFAULT)) {
                struct audio_config new_config;

                out->common.requested_format = new_format;

                new_config.sample_rate = out->common.requested_sample_rate;
                new_config.channel_mask = out->common.requested_channel_mask;
                new_config.format = new_format;
                proxy_reconfig_playback_stream(out->common.proxy_stream, out->common.stream_type,
                                               &new_config);
                ALOGD("%s-%s: changed format to %#x from %#x",
                                      stream_table[out->common.stream_type], __func__,
                                      new_format, out->common.requested_format);
            } else
                ALOGD("%s-%s: requested to change same format to %#x",
                                      stream_table[out->common.stream_type], __func__, new_format);
        }
    }

    // Change Audio Channel Mask
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_CHANNELS, value, sizeof(value));
    if (ret >= 0) {
        if (out->common.stream_status > STATUS_READY)
            goto not_acceptable;
        else {
            audio_channel_mask_t new_chmask = (audio_channel_mask_t)atoi(value);
            if ((new_chmask != out->common.requested_channel_mask) && (new_chmask != AUDIO_CHANNEL_NONE)) {
                struct audio_config new_config;

                out->common.requested_channel_mask = new_chmask;

                new_config.sample_rate = out->common.requested_sample_rate;
                new_config.channel_mask = new_chmask;
                new_config.format = out->common.requested_format;
                proxy_reconfig_playback_stream(out->common.proxy_stream, out->common.stream_type,
                                               &new_config);
                ALOGD("%s-%s: changed channel mask to %#x from %#x",
                                      stream_table[out->common.stream_type], __func__,
                                      new_chmask, out->common.requested_channel_mask);
            } else
                ALOGD("%s-%s: requested to change same channel mask to %#x",
                                      stream_table[out->common.stream_type], __func__, new_chmask);
        }
    }

    // Change Audio Sampling Rate
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_SAMPLING_RATE, value, sizeof(value));
    if (ret >= 0) {
        if (out->common.stream_status > STATUS_READY)
            goto not_acceptable;
        else {
            uint32_t new_rate = (uint32_t)atoi(value);
            if ((new_rate != out->common.requested_sample_rate) && (new_rate != 0)) {
                struct audio_config new_config;

                out->common.requested_sample_rate = new_rate;

                new_config.sample_rate = new_rate;
                new_config.channel_mask = out->common.requested_channel_mask;
                new_config.format = out->common.requested_format;
                proxy_reconfig_playback_stream(out->common.proxy_stream, out->common.stream_type,
                                               &new_config);
                ALOGD("%s-%s: changed sampling rate to %dHz from %dHz",
                                      stream_table[out->common.stream_type], __func__,
                                      new_rate, out->common.requested_sample_rate);
            } else
                ALOGD("%s-%s: requested to change same sampling rate to %dHz",
                                      stream_table[out->common.stream_type], __func__, new_rate);
        }
    }
    pthread_mutex_unlock(&out->common.lock);

    str_parms_destroy(parms);
    ALOGVV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return 0;

not_acceptable:
    pthread_mutex_unlock(&out->common.lock);
    str_parms_destroy(parms);
    ALOGE("%s-%s: This parameter cannot accept as Stream is working",
                                                  stream_table[out->common.stream_type], __func__);
    return -ENOSYS;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct str_parms *query = str_parms_create_str(keys);
    struct str_parms *reply = str_parms_create();
    char * result_str;

    ALOGD("%s-%s: enter with param = %s", stream_table[out->common.stream_type], __func__, keys);

    pthread_mutex_lock(&out->common.lock);

    // Get Current Devices
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_ROUTING)) {
        str_parms_add_int(reply, AUDIO_PARAMETER_STREAM_ROUTING, (int)out->common.requested_devices);
    }

    // Get Current Audio Format
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_FORMAT)) {
        str_parms_add_int(reply, AUDIO_PARAMETER_STREAM_FORMAT, (int)out->common.requested_format);
    }

    // Get Current Audio Frame Count
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_FRAME_COUNT)) {
        str_parms_add_int(reply, AUDIO_PARAMETER_STREAM_FRAME_COUNT,
                                      (int)proxy_get_actual_period_size(out->common.proxy_stream));
    }

    // Some parameters can be gotten from Audio Stream Proxy
    proxy_getparam_playback_stream(out->common.proxy_stream, query, reply);

    result_str = str_parms_to_str(reply);
    str_parms_destroy(query);
    str_parms_destroy(reply);
    pthread_mutex_unlock(&out->common.lock);

    ALOGD("%s-%s: exit with %s", stream_table[out->common.stream_type], __func__, result_str);
    return result_str;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect __unused)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGVV("%s-%s: exit with effect(%p)", stream_table[out->common.stream_type], __func__, effect);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect __unused)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGVV("%s-%s: exit with effect(%p)", stream_table[out->common.stream_type], __func__, effect);
    return 0;
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    return proxy_get_playback_latency(out->common.proxy_stream);
}

static int out_set_volume(struct audio_stream_out *stream, float left, float right)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    int ret = 0;

    ALOGVV("%s-%s: enter", stream_table[out->common.stream_type], __func__);

    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        if (out->vol_left != left || out->vol_right != right) {
            out->vol_left = left;
            out->vol_right = right;

            uint32_t compare_volume = (uint32_t)(out->vol_left * DIRECT_PLAYBACK_VOLUME_MAX);
            if (compare_volume == 0) {
                // source metadata will check whether mute is set or not
                adev->update_offload_volume = false;
                return ret;
            }

            proxy_set_volume(adev->proxy, VOLUME_TYPE_OFFLOAD, left, right);
        }
    } else if (out->common.stream_type == ASTREAM_PLAYBACK_DIRECT) {
        if (out->vol_left != left || out->vol_right != right) {
            out->vol_left = left;
            out->vol_right = right;
            out->direct_volume_enabled = true;
        }
#ifdef SUPPORT_MMAP_HW_VOLUME_CONTROL
    /*
     * Hardware volume control can be supported only when RDMA is used for MMAP,
     * ABOX_V3 onwards MMAP uses Virtual-DAI output node therefore volume control
     * cannot be supported
     */
    } else if (out->common.stream_type == ASTREAM_PLAYBACK_MMAP) {
        if (out->vol_left != left || out->vol_right != right || adev->update_mmap_volume) {
            out->vol_left = left;
            out->vol_right = right;
            proxy_set_volume(adev->proxy, VOLUME_TYPE_MMAP, left, right);
            if (adev->update_mmap_volume)
                adev->update_mmap_volume = false;
        }
#endif
    } else{
        ALOGE("%s-%s: Don't support volume control for this stream",
              stream_table[out->common.stream_type], __func__);
        ret = -ENOSYS;
    }

    ALOGVV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer, size_t bytes)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    int ret = 0, wrote = 0;

    //ALOGVV("%s-%s: enter", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_status == STATUS_STANDBY) {
        out->common.stream_status = STATUS_READY;
        ALOGI("%s-%s: transited to Ready", stream_table[out->common.stream_type], __func__);

        // Have to route Audio Path before open PCM Device
        pthread_mutex_lock(&adev->lock);
        if (out->common.stream_type == ASTREAM_PLAYBACK_INCALL_MUSIC && isCPCallMode(adev)) {
            ALOGI("%s-%s: try to route incall-music call path", stream_table[out->common.stream_type], __func__);
            /* Incall-music should be enabled before routing call path, to get proper call path,
                 incase if standby is called and re-started */
            adev->incallmusic_on = true;
            voice_set_call_active(adev->voice, false);
            proxy_stop_voice_call(adev->proxy);
            adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, CALL_DRIVE);
            proxy_start_voice_call(adev->proxy);
            voice_set_call_active(adev->voice, true);
            ALOGI("%s-%s: *** Started CP Voice Call in Incall Music ***",
                   stream_table[out->common.stream_type], __func__);
        } else if (!adev->is_playback_path_routed) {
            ALOGI("%s-%s: try to route for playback", stream_table[out->common.stream_type], __func__);
#ifdef SUPPORT_BTA2DP_OFFLOAD
            if ((out->common.requested_devices & AUDIO_DEVICE_OUT_ALL_A2DP) &&
                (out->common.requested_devices & AUDIO_DEVICE_OUT_SPEAKER)) {
                audio_devices_t dev = out->common.requested_devices;
                out->common.requested_devices = AUDIO_DEVICE_OUT_SPEAKER;
                adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, NON_FORCE_ROUTE);
                out->common.requested_devices = dev;
            } else
#endif
            adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, NON_FORCE_ROUTE);
        }
        pthread_mutex_unlock(&adev->lock);

#if AUDIO_PLATFORM_ABOX_V1
        // Check VoIP SE Trigger
        if (need_voipse_on(adev) && out->common.stream_type == ASTREAM_PLAYBACK_PRIMARY &&
            is_active_usage_APCall(adev->proxy)) {
            proxy_set_mixer_value_int(adev->proxy, ABOX_APCALLBUFFTYPE_CONTROL_NAME, MIXER_VALUE_ON);
            pthread_mutex_lock(&adev->lock);
            adev->voipse_on = true;
            pthread_mutex_unlock(&adev->lock);
            ALOGI("%s-%s: VoIP SE Triggered-1!", stream_table[out->common.stream_type], __func__);
        }
#endif

        ret = proxy_open_playback_stream((void *)(out->common.proxy_stream), 0, NULL);
        if (ret != 0) {
            ALOGE("%s-%s: failed to open Proxy Playback Stream!",
                  stream_table[out->common.stream_type], __func__);
            pthread_mutex_unlock(&out->common.lock);
            return ret;
        } else {
            out->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[out->common.stream_type], __func__);
        }
    }

    if (out->common.stream_status > STATUS_READY) {
        if (buffer && bytes > 0) {
            /* Pre-Processing */
#if AUDIO_PLATFORM_ABOX_V1
            // Check VoIP SE Trigger
            if (need_voipse_on(adev) && out->common.stream_type == ASTREAM_PLAYBACK_PRIMARY &&
                is_active_usage_APCall(adev->proxy)) {
                // In case of check VoIP after PCM Open, this PCM device needs to re-open
                proxy_close_playback_stream((void *)(out->common.proxy_stream));

                proxy_set_mixer_value_int(adev->proxy, ABOX_APCALLBUFFTYPE_CONTROL_NAME, MIXER_VALUE_ON);
                pthread_mutex_lock(&adev->lock);
                adev->voipse_on = true;
                pthread_mutex_unlock(&adev->lock);
                ALOGI("%s-%s: VoIP SE Triggered-2!", stream_table[out->common.stream_type], __func__);

                proxy_open_playback_stream((void *)(out->common.proxy_stream), 0, NULL);
            } else if (out->common.stream_type == ASTREAM_PLAYBACK_PRIMARY && adev->voipse_on && !isAPCallMode(adev)) {
                // In case of check VoIP after PCM Open, this PCM device needs to re-open
                proxy_close_playback_stream((void *)(out->common.proxy_stream));

                proxy_set_mixer_value_int(adev->proxy, ABOX_APCALLBUFFTYPE_CONTROL_NAME, MIXER_VALUE_OFF);
                pthread_mutex_lock(&adev->lock);
                adev->voipse_on = false;
                pthread_mutex_unlock(&adev->lock);
                ALOGI("%s-%s: VoIP SE Un-Triggered!", stream_table[out->common.stream_type], __func__);

                proxy_open_playback_stream((void *)(out->common.proxy_stream), 0, NULL);
            }
#endif

            /* Volume control for Direct stream pcm data */
            if (out->common.stream_type == ASTREAM_PLAYBACK_DIRECT && out->direct_volume_enabled) {
                adjust_out_volume(stream, buffer, bytes);
                ALOGVV("%s-%s: Direct stream volume control left %f right %f",
                    stream_table[out->common.stream_type], __func__,
                    out->vol_left, out->vol_right);
            }

            wrote = proxy_write_playback_buffer((void *)(out->common.proxy_stream), (void *)buffer, (int)bytes);
            if (wrote >= 0) {
                if (out->common.stream_status == STATUS_IDLE) {
                    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD)
                        update_offload_effect(adev, 0);

                    ret = proxy_start_playback_stream((void *)(out->common.proxy_stream));
                    if (ret != 0) {
                        ALOGE("%s-%s: failed to start Proxy Playback Stream!",
                              stream_table[out->common.stream_type], __func__);
                        pthread_mutex_unlock(&out->common.lock);
                        return ret;
                    } else {
                        out->common.stream_status = STATUS_PLAYING;
                        ALOGI("%s-%s: transited to Playing",
                              stream_table[out->common.stream_type], __func__);
                    }
                }

                if ((out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) && (wrote < (ssize_t)bytes)) {
                    /* Compress Device has no available buffer, we have to wait */
                    ALOGVV("%s-%s: There are no available buffer in Compress Device, Need to wait",
                                  stream_table[out->common.stream_type], __func__);
                    ret = send_offload_msg(out, OFFLOAD_MSG_WAIT_WRITE);
                }
            }
        }
    }
    pthread_mutex_unlock(&out->common.lock);

    //ALOGVV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return wrote;
}

static int out_get_render_position(const struct audio_stream_out *stream, uint32_t *dsp_frames)
{
    struct stream_out *out = (struct stream_out *)stream;

    pthread_mutex_lock(&out->common.lock);
    int ret = proxy_get_render_position(out->common.proxy_stream, dsp_frames);
    pthread_mutex_unlock(&out->common.lock);

    return ret;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream __unused,
                                        int64_t *timestamp __unused)
{
    return -ENOSYS;
}

static int out_get_presentation_position(const struct audio_stream_out *stream,
                                         uint64_t *frames, struct timespec *timestamp)
{
    struct stream_out *out = (struct stream_out *)stream;

    pthread_mutex_lock(&out->common.lock);
    int ret = proxy_get_presen_position(out->common.proxy_stream, frames, timestamp);
    pthread_mutex_unlock(&out->common.lock);

    return ret;
}

static int out_set_callback(struct audio_stream_out *stream, stream_callback_t callback, void *cookie)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -EINVAL;

    ALOGV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        if (callback && cookie) {
            out->offload.callback = callback;
            out->offload.cookie = cookie;
            ALOGD("%s-%s: set callback function & cookie", stream_table[out->common.stream_type], __func__);

            ret = 0;
        }
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

static int out_pause(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        if (out->common.stream_status == STATUS_PLAYING) {
            // Stop Visualizer

            ret = proxy_offload_pause(out->common.proxy_stream);
            if (ret == 0) {
                out->common.stream_status = STATUS_PAUSED;
                ALOGI("%s-%s: transit to Paused", stream_table[out->common.stream_type], __func__);
            } else {
                ALOGE("%s-%s: failed to pause", stream_table[out->common.stream_type], __func__);
            }
        } else {
            ALOGV("%s-%s: abnormal status(%u) for pausing", stream_table[out->common.stream_type],
                  __func__, out->common.stream_status);
        }
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

static int out_resume(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        if (out->common.stream_status== STATUS_PAUSED) {
            ret = proxy_offload_resume(out->common.proxy_stream);
            if (ret == 0) {
                out->common.stream_status = STATUS_PLAYING;
                ALOGI("%s-%s: transit to Playing", stream_table[out->common.stream_type], __func__);

                // Start Visualizer
            } else {
                ALOGE("%s-%s: failed to resume", stream_table[out->common.stream_type], __func__);
            }
        } else {
            ALOGV("%s-%s: abnormal State(%u) for resuming", stream_table[out->common.stream_type],
                  __func__, out->common.stream_status);
        }
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

static int out_drain(struct audio_stream_out* stream, audio_drain_type_t type )
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered with type = %d", stream_table[out->common.stream_type], __func__, type);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        if (out->common.stream_status > STATUS_IDLE) {
            if (type == AUDIO_DRAIN_EARLY_NOTIFY)
                ret = send_offload_msg(out, OFFLOAD_MSG_WAIT_PARTIAL_DRAIN);
            else
                ret = send_offload_msg(out, OFFLOAD_MSG_WAIT_DRAIN);
        } else {
            out->offload.callback(STREAM_CBK_EVENT_DRAIN_READY, NULL, out->offload.cookie);
            ALOGD("%s-%s: State is IDLE. Return callback with drain_ready",
                  stream_table[out->common.stream_type], __func__);
        }
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

static int out_flush(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        if (out->common.stream_status > STATUS_IDLE) {
            ret = proxy_stop_playback_stream((void *)(out->common.proxy_stream));
            out->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transit to Idle due to flush", stream_table[out->common.stream_type], __func__);
        } else {
            ret = 0;
            ALOGV("%s-%s: this stream is already stopped", stream_table[out->common.stream_type], __func__);
        }
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

// For MMAP NOIRQ Stream
static int out_stop(const struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_MMAP) {
        if (out->common.stream_status == STATUS_PLAYING) {
            /* Stops stream & transit to Idle. */
            proxy_stop_playback_stream((void *)(out->common.proxy_stream));
            out->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[out->common.stream_type], __func__);

        } else
            ALOGV("%s-%s: invalid operation - stream status (%d)",
                  stream_table[out->common.stream_type], __func__, out->common.stream_status);
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

static int out_start(const struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_MMAP) {
        if (out->common.stream_status == STATUS_IDLE) {
            /* Starts stream & transit to Playing. */
            ret = proxy_start_playback_stream((void *)(out->common.proxy_stream));
            if (ret != 0) {
                ALOGE("%s-%s: failed to start Proxy Playback Stream!",
                      stream_table[out->common.stream_type], __func__);
            } else {
                out->common.stream_status = STATUS_PLAYING;
                ALOGI("%s-%s: transited to Playing",
                      stream_table[out->common.stream_type], __func__);
            }
        } else
            ALOGV("%s-%s: invalid operation - stream status (%d)",
                  stream_table[out->common.stream_type], __func__, out->common.stream_status);
    }
    pthread_mutex_unlock(&out->common.lock);

    ALOGV("%s-%s: exit", stream_table[out->common.stream_type], __func__);
    return ret;
}

static int out_create_mmap_buffer(const struct audio_stream_out *stream,
                                  int32_t min_size_frames,
                                  struct audio_mmap_buffer_info *info)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    int ret = 0;

    ALOGD("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    pthread_mutex_lock(&out->common.lock);

    if (info == NULL || min_size_frames == 0) {
        ALOGE("%s-%s: info = %p, min_size_frames = %d", stream_table[out->common.stream_type],
                                                        __func__, info, min_size_frames);
        ret = -EINVAL;
        goto exit;
    }

    if (out->common.stream_type != ASTREAM_PLAYBACK_MMAP || out->common.stream_status != STATUS_STANDBY) {
        ALOGE("%s-%s: invalid operation - stream status (%d)", stream_table[out->common.stream_type],
                                                               __func__, out->common.stream_status);
        ret = -ENOSYS;
        goto exit;
    } else {
        out->common.stream_status = STATUS_READY;
        ALOGI("%s-%s: transited to Ready", stream_table[out->common.stream_type], __func__);

        // Have to route Audio Path before open PCM Device
        pthread_mutex_lock(&adev->lock);
        if (!adev->is_playback_path_routed) {
            ALOGI("%s-%s: try to route for playback", stream_table[out->common.stream_type], __func__);
            adev_set_route((void *)out, AUSAGE_PLAYBACK, ROUTE, NON_FORCE_ROUTE);
        }
        pthread_mutex_unlock(&adev->lock);

        /* Opens stream & transit to Idle. */
        ret = proxy_open_playback_stream((void *)(out->common.proxy_stream), min_size_frames, (void *)info);
        if (ret != 0) {
            ALOGE("%s-%s: failed to open Proxy Playback Stream!",
                  stream_table[out->common.stream_type], __func__);

            out->common.stream_status = STATUS_STANDBY;
            ALOGI("%s-%s: transited to StandBy", stream_table[out->common.stream_type], __func__);
        } else {
            out->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[out->common.stream_type], __func__);
        }
    }

exit:
    pthread_mutex_unlock(&out->common.lock);
    ALOGD("%s-%s: exited", stream_table[out->common.stream_type], __func__);
    return ret;
}

static int out_get_mmap_position(const struct audio_stream_out *stream,
                                 struct audio_mmap_position *position)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = 0;

    ALOGV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    if (position == NULL) return -EINVAL;
    if (out->common.stream_type != ASTREAM_PLAYBACK_MMAP) return -ENOSYS;

    ret = proxy_get_mmap_position((void *)(out->common.proxy_stream), (void *)position);

    ALOGV("%s-%s: exited", stream_table[out->common.stream_type], __func__);
    return ret;
}

static void out_update_source_metadata(struct audio_stream_out *stream,
                                       const struct source_metadata* source_metadata)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;

    ALOGVV("%s-%s: entered", stream_table[out->common.stream_type], __func__);

    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {

        if (source_metadata->track_count > 0) {
            ALOGD("%s-%s: This stream has %zu tracks", stream_table[out->common.stream_type], __func__,
                                                       source_metadata->track_count);

            for (int i = 0; i < (int)source_metadata->track_count; i++) {
                ALOGD("%d Track has Usage(%d), Content Type(%d), Gain(%f)", i+1,
                      (int)source_metadata->tracks[i].usage, (int)source_metadata->tracks[i].content_type,
                       source_metadata->tracks[i].gain);

                if (adev->update_offload_volume == false && source_metadata->tracks[i].gain == out->vol_left) {
                    proxy_set_volume(adev->proxy, VOLUME_TYPE_OFFLOAD, out->vol_left, out->vol_right);
                    adev->update_offload_volume = true;
                }
            }
        }
    }

    return ;
}


/****************************************************************************/
/**                                                                        **/
/** The Stream_In Function Implementation                                 **/
/**                                                                        **/
/****************************************************************************/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: exit with sample rate = %u", stream_table[in->common.stream_type], __func__,
                                                in->common.requested_sample_rate);
    return in->common.requested_sample_rate;
}

static int in_set_sample_rate(struct audio_stream *stream __unused, uint32_t rate __unused)
{
    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    size_t buffer_size = 0;

    ALOGVV("%s-%s: enter", stream_table[in->common.stream_type], __func__);

    // will return buffer size based on requested PCM configuration
    if (in->common.requested_sample_rate != proxy_get_actual_sampling_rate(in->common.proxy_stream)) {
        if (in->common.stream_type == ASTREAM_CAPTURE_PRIMARY)
            buffer_size = (in->common.requested_sample_rate * PREDEFINED_MEDIA_CAPTURE_DURATION) / 1000;
        else if (in->common.stream_type == ASTREAM_CAPTURE_LOW_LATENCY)
            buffer_size = (in->common.requested_sample_rate * PREDEFINED_LOW_CAPTURE_DURATION) / 1000;
        else
            buffer_size = proxy_get_actual_period_size(in->common.proxy_stream);
    } else
        buffer_size = proxy_get_actual_period_size(in->common.proxy_stream);

    buffer_size *= (unsigned int)audio_stream_in_frame_size((const struct audio_stream_in *)stream);
    ALOGVV("%s-%s: exit with %d bytes", stream_table[in->common.stream_type], __func__, (int)buffer_size);
    return buffer_size;
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: exit with channel mask = 0x%x", stream_table[in->common.stream_type], __func__,
                                                  in->common.requested_channel_mask);
    return in->common.requested_channel_mask;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: exit with audio format = 0x%x", stream_table[in->common.stream_type], __func__,
                                                   in->common.requested_format);
    return in->common.requested_format;
}

static int in_set_format(struct audio_stream *stream __unused, audio_format_t format __unused)
{
    return -ENOSYS;
}

static int in_standby(struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->adev;

    ALOGVV("%s-%s: enter", stream_table[in->common.stream_type], __func__);

    pthread_mutex_lock(&in->common.lock);
    if (in->common.stream_status > STATUS_STANDBY) {
        /* Stops stream & transit to Idle. */
        if (in->common.stream_status > STATUS_IDLE) {
            proxy_stop_capture_stream((void *)(in->common.proxy_stream));
            in->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[in->common.stream_type], __func__);
        }

        /* Closes device & transit to Standby. */
        proxy_close_capture_stream((void *)(in->common.proxy_stream));
        in->common.stream_status = STATUS_STANDBY;
        ALOGI("%s-%s: transited to Standby", stream_table[in->common.stream_type], __func__);

        // Have to unroute Audio Path after close PCM Device
        pthread_mutex_lock(&adev->lock);
#ifdef SUPPORT_STHAL_INTERFACE
        if (in->common.stream_type != ASTREAM_CAPTURE_HOTWORD &&
            adev->is_capture_path_routed)
#else
        if (adev->is_capture_path_routed)
#endif
        {
            ALOGI("%s-%s: try to unroute for standby", stream_table[in->common.stream_type], __func__);
            adev_set_route((void *)in, AUSAGE_CAPTURE, UNROUTE, NON_FORCE_ROUTE);
        }
        if (adev->active_input)
            adev->active_input = NULL;

        pthread_mutex_unlock(&adev->lock);
    }
    pthread_mutex_unlock(&in->common.lock);

    ALOGVV("%s-%s: exit", stream_table[in->common.stream_type], __func__);
    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: enter with fd(%d)", stream_table[in->common.stream_type], __func__, fd);

    const size_t len = 256;
    char buffer[len];
    bool justLocked = false;

    snprintf(buffer, len, "Audio Stream Input::dump\n");
    write(fd,buffer,strlen(buffer));
    justLocked = pthread_mutex_trylock(&in->common.lock) == 0;
    if(justLocked)
        pthread_mutex_unlock(&in->common.lock);
    snprintf(buffer, len, "\tinput Mutex: %s\n", justLocked ? "locked" : "unlocked");
    write(fd,buffer,strlen(buffer));

    snprintf(buffer, len, "\tinput devices: %d\n", in->common.requested_devices);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tinput source: %d\n", in->requested_source);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tinput flags: %x\n",in->requested_flags);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tinput sample rate: %u\n",in->common.requested_sample_rate);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tinput channel mask: %u\n",in->common.requested_channel_mask);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tinput format: %d\n",in->common.requested_format);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tinput audio usage: %d\n",in->common.stream_usage);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tinut standby state: %d\n",in->common.stream_status);
    write(fd,buffer,strlen(buffer));
    //snprintf(buffer, len, "\tinput mixer_path_setup: %s\n",bool_to_str(in->mixer_path_setup));
    //write(fd,buffer,strlen(buffer));

    proxy_dump_capture_stream(in->common.proxy_stream, fd);

    ALOGVV("%s-%s: exit with fd(%d)", stream_table[in->common.stream_type], __func__, fd);
    return 0;
}

static audio_devices_t in_get_device(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: exit with device = %u", stream_table[in->common.stream_type], __func__,
                                                   in->common.requested_devices);
    return in->common.requested_devices;
}

static int in_set_device(struct audio_stream *stream __unused, audio_devices_t device __unused)
{
    return -ENOSYS;
}

void stop_active_input(struct stream_in *in)
{
    struct audio_device *adev = in->adev;

    if (in->common.stream_status > STATUS_STANDBY) {
        in->common.stream_status = STATUS_STANDBY;
        pthread_mutex_lock(&adev->lock);
        proxy_stop_capture_stream((void *)(in->common.proxy_stream));
        proxy_close_capture_stream((void *)(in->common.proxy_stream));
        pthread_mutex_unlock(&adev->lock);
    }
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->adev;

    struct str_parms *parms;
    char value[32];
    int ret = 0;

    ALOGD("%s-%s: enter with param = %s", stream_table[in->common.stream_type], __func__, kvpairs);

#ifdef SUPPORT_STHAL_INTERFACE
        if (in->common.stream_type == ASTREAM_CAPTURE_HOTWORD) {
            ALOGD("%s-%s: exit", stream_table[in->common.stream_type], __func__);
            return 0;
        }
#endif

    if (is_usb_in_device(in->common.requested_devices)) {
        pthread_mutex_lock(&in->common.lock);
        proxy_setparam_capture_stream(in->common.proxy_stream, (void *)kvpairs);
        pthread_mutex_unlock(&in->common.lock);
    }

    parms = str_parms_create_str(kvpairs);

    pthread_mutex_lock(&in->common.lock);
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        audio_devices_t requested_devices = atoi(value);
        audio_devices_t current_devices = in->common.requested_devices;
        bool need_routing = false;

        /*
         * AudioFlinger informs Audio path is this device.
         * AudioHAL has to decide to do actual routing or not.
         */
        if (requested_devices != AUDIO_DEVICE_NONE) {
            ALOGI("%s-%s: requested to change route from %s to %s",
                  stream_table[in->common.stream_type], __func__,
                  device_table[get_device_id(adev, current_devices)],
                  device_table[get_device_id(adev, requested_devices)]);

            /* Assign requested device to Input Stream */
            in->common.requested_devices = requested_devices;

            update_capture_stream(in, current_devices, requested_devices);

            pthread_mutex_lock(&adev->lock);
            /* Check actual routing is needed or not */
            // Actual routing is needed at device change request
            if (adev->is_capture_path_routed || (in->common.stream_status > STATUS_STANDBY)) {
                if (!isCPCallMode(adev))
                    need_routing = true;
            }
            pthread_mutex_unlock(&adev->lock);

            if (need_routing) {
          /* Force close active input pcm node before routing to new input device,
                 * to avoid auto triggering of loopback pcm nodes while routing to new path */
                if (in->pcm_reconfig) {
                    ALOGD(" %s: pcm reconfig", __func__);
                    stop_active_input(in);
                }

                pthread_mutex_lock(&adev->lock);
                /* Do actual routing */
                adev_set_route((void *)in, AUSAGE_CAPTURE, ROUTE, NON_FORCE_ROUTE);
                pthread_mutex_unlock(&adev->lock);
            }
        } else {
            /* When audio device will be changed, AudioFlinger requests to route with AUDIO_DEVICE_NONE */
            ALOGV("%s-%s: requested to change route to AUDIO_DEVICE_NONE",
                  stream_table[in->common.stream_type], __func__);
            //pthread_mutex_lock(&adev->lock);
            //adev_set_route((void *)in, AUSAGE_CAPTURE, UNROUTE, NON_FORCE_ROUTE);
            //pthread_mutex_unlock(&adev->lock);
        }
    }

    /*
     * Android Audio System can change PCM Configurations(Format, Channel and Rate) for Audio Stream
     * when this Audio Stream is not working.
     */
    // Change Audio Format
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_FORMAT, value, sizeof(value));
    if (ret >= 0) {
        if (in->common.stream_status > STATUS_READY)
            goto not_acceptable;
        else {
            audio_format_t new_format = (audio_format_t)atoi(value);
            if ((new_format != in->common.requested_format) && (new_format != AUDIO_FORMAT_DEFAULT)) {
                struct audio_config new_config;

                in->common.requested_format = new_format;

                new_config.sample_rate = in->common.requested_sample_rate;
                new_config.channel_mask = in->common.requested_channel_mask;
                new_config.format = new_format;
                proxy_reconfig_capture_stream(in->common.proxy_stream, in->common.stream_type,
                                              &new_config);
                ALOGD("%s-%s: changed format to %#x from %#x",
                                      stream_table[in->common.stream_type], __func__,
                                      new_format, in->common.requested_format);
            } else
                ALOGD("%s-%s: requested to change same format to %#x",
                                      stream_table[in->common.stream_type], __func__, new_format);
        }
    }

    // Change Audio Channel Mask
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_CHANNELS, value, sizeof(value));
    if (ret >= 0) {
        if (in->common.stream_status > STATUS_READY)
            goto not_acceptable;
        else {
            audio_channel_mask_t new_chmask = (audio_channel_mask_t)atoi(value);
            if ((new_chmask != in->common.requested_channel_mask) && (new_chmask != AUDIO_CHANNEL_NONE)) {
                struct audio_config new_config;

                in->common.requested_channel_mask = new_chmask;

                new_config.sample_rate = in->common.requested_sample_rate;
                new_config.channel_mask = new_chmask;
                new_config.format = in->common.requested_format;
                proxy_reconfig_capture_stream(in->common.proxy_stream, in->common.stream_type,
                                              &new_config);
                ALOGD("%s-%s: changed channel mask to %#x from %#x",
                                      stream_table[in->common.stream_type], __func__,
                                      new_chmask, in->common.requested_channel_mask);
            } else
                ALOGD("%s-%s: requested to change same channel mask to %#x",
                                      stream_table[in->common.stream_type], __func__, new_chmask);
        }
    }

    // Change Audio Sampling Rate
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_SAMPLING_RATE, value, sizeof(value));
    if (ret >= 0) {
        if (in->common.stream_status > STATUS_READY)
            goto not_acceptable;
        else {
            uint32_t new_rate = (uint32_t)atoi(value);
            if ((new_rate != in->common.requested_sample_rate) && (new_rate != 0)) {
                struct audio_config new_config;

                in->common.requested_sample_rate = new_rate;

                new_config.sample_rate = new_rate;
                new_config.channel_mask = in->common.requested_channel_mask;
                new_config.format = in->common.requested_format;
                proxy_reconfig_capture_stream(in->common.proxy_stream, in->common.stream_type,
                                              &new_config);
                ALOGD("%s-%s: changed sampling rate to %dHz from %dHz",
                                      stream_table[in->common.stream_type], __func__,
                                      new_rate, in->common.requested_sample_rate);
            } else
                ALOGD("%s-%s: requested to change same sampling rate to %dHz",
                                      stream_table[in->common.stream_type], __func__, new_rate);
        }
    }

    // Change Audio Input Source
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value, sizeof(value));
    if (ret >= 0) {
        if (in->common.stream_status > STATUS_READY)
            goto not_acceptable;
        else {
            unsigned int new_source = (unsigned int)atoi(value);
            if ((new_source != in->requested_source) && (new_source != AUDIO_SOURCE_DEFAULT)) {
                in->requested_source = new_source;

                if (in->requested_source == AUDIO_SOURCE_VOICE_CALL) {
                    in->common.stream_usage = adev_get_capture_ausage(adev, in);
                    proxy_update_capture_usage((void *)(in->common.proxy_stream),(int)in->common.stream_usage);
                }
                ALOGD("%s-%s: changed source to %d from %d ", stream_table[in->common.stream_type],
                                                       __func__, new_source, in->requested_source);
            } else
                ALOGD("%s-%s: requested to change same source to %d",
                                      stream_table[in->common.stream_type], __func__, new_source);
        }
    }
    pthread_mutex_unlock(&in->common.lock);

    str_parms_destroy(parms);
    ALOGVV("%s-%s: exit", stream_table[in->common.stream_type], __func__);
    return 0;

not_acceptable:
    pthread_mutex_unlock(&in->common.lock);
    str_parms_destroy(parms);
    ALOGE("%s-%s: This parameter cannot accept as Stream is working",
                                                  stream_table[in->common.stream_type], __func__);
    return -ENOSYS;
}

static char * in_get_parameters(const struct audio_stream *stream, const char *keys)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct str_parms *query = str_parms_create_str(keys);
    struct str_parms *reply = str_parms_create();
    char * result_str;

    ALOGD("%s-%s: enter with param = %s", stream_table[in->common.stream_type], __func__, keys);

    pthread_mutex_lock(&in->common.lock);

    // Get Current Devices
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_ROUTING)) {
        str_parms_add_int(reply, AUDIO_PARAMETER_STREAM_ROUTING, (int)in->common.requested_devices);
    }

    // Get Current Audio Format
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_FORMAT)) {
        str_parms_add_int(reply, AUDIO_PARAMETER_STREAM_FORMAT, (int)in->common.requested_format);
    }

    // Get Current Audio Frame Count
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_FRAME_COUNT)) {
        str_parms_add_int(reply, AUDIO_PARAMETER_STREAM_FRAME_COUNT,
                                      (int)proxy_get_actual_period_size(in->common.proxy_stream));
    }

    // Get Current Audio Input Source
    if (str_parms_has_key(query, AUDIO_PARAMETER_STREAM_INPUT_SOURCE)) {
        str_parms_add_int(reply, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, (int)in->requested_source);
    }

    // Some parameters can be gotten from Audio Stream Proxy
    proxy_getparam_capture_stream(in->common.proxy_stream, query, reply);

    result_str = str_parms_to_str(reply);
    str_parms_destroy(query);
    str_parms_destroy(reply);
    pthread_mutex_unlock(&in->common.lock);

    ALOGD("%s-%s: exit with %s", stream_table[in->common.stream_type], __func__, result_str);
    return result_str;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect __unused)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGI("%s-%s: exit with effect(%p)", stream_table[in->common.stream_type], __func__, effect);

    if (in->adev->amode == AUDIO_MODE_IN_COMMUNICATION)
        proxy_set_apcall_txse();

    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect __unused)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGI("%s-%s: exit with effect(%p)", stream_table[in->common.stream_type], __func__, effect);

    proxy_clear_apcall_txse();

    return 0;
}

static int in_set_gain(struct audio_stream_in *stream, float gain __unused)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: enter", stream_table[in->common.stream_type], __func__);

    ALOGVV("%s-%s: exit", stream_table[in->common.stream_type], __func__);
    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer, size_t bytes)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->adev;
    int ret = 0;

    //ALOGVV("%s-%s: enter", stream_table[in->common.stream_type], __func__);

    pthread_mutex_lock(&in->common.lock);
    if (in->pcm_reconfig) {
        ALOGD(" %s: pcm reconfig", __func__);
        stop_active_input(in);
        in->pcm_reconfig = false;
    }

    if (in->common.stream_status == STATUS_STANDBY) {
        in->common.stream_status = STATUS_READY;
        ALOGI("%s-%s: transited to Ready", stream_table[in->common.stream_type], __func__);

        // Have to route Audio Path before open PCM Device
        pthread_mutex_lock(&adev->lock);
        if ((is_usage_CPCall(adev->active_capture_ausage) && (in->common.stream_type == ASTREAM_CAPTURE_PRIMARY)) ||
           (is_usage_APCall(adev->active_capture_ausage) && (in->common.stream_type == ASTREAM_CAPTURE_PRIMARY)) ||
           (!isCPCallMode(adev) && (in->common.stream_type == ASTREAM_CAPTURE_CALL))) {
            audio_stream_type new_stream_type = in->common.stream_type;
            if (isCPCallMode(adev) && isCallRecording(in->requested_source)) {
                new_stream_type = ASTREAM_CAPTURE_CALL;
                ALOGI(" %s: pcm reconfig as ASTREAM_CAPTURE_CALL", __func__);
            } else if (isCPCallMode(adev)) {
                new_stream_type = ASTREAM_CAPTURE_PRIMARY;
                ALOGI(" %s: pcm reconfig as ASTREAM_CAPTURE_PRIMARY on CP Call mode", __func__);
            } else {
                new_stream_type = ASTREAM_CAPTURE_PRIMARY;
                ALOGI(" %s: pcm reconfig as ASTREAM_CAPTURE_PRIMARY on Normal mode", __func__);
                // CP Call stopped, normal rec is in progress. un-route incall tx path.
                adev_set_route((void *)in, AUSAGE_CAPTURE, UNROUTE, FORCE_ROUTE);
            }

#if AUDIO_PLATFORM_ABOX_V2
            if (new_stream_type != in->common.stream_type)
#endif
            {
                in->common.stream_type = new_stream_type;
                in->common.stream_usage = adev_get_capture_ausage(adev, in);
                ALOGI("%s-%s: updated capture usage(%s)", stream_table[in->common.stream_type], __func__, usage_table[in->common.stream_usage]);

                proxy_reconfig_capture_usage((void *)(in->common.proxy_stream),
                                              (int)in->common.stream_type,
                                              (int)in->common.stream_usage);
            }
        }

        adev->active_input = in;

#ifdef SUPPORT_STHAL_INTERFACE
        if (in->common.stream_type != ASTREAM_CAPTURE_HOTWORD &&
            !adev->is_capture_path_routed)
#else
        if (!adev->is_capture_path_routed)
#endif
        {
            if (is_factory_bt_realtime_loopback_mode(adev->factory)) {
                ALOGI("%s skip routing for BT realtime loopback", __func__);
            } else if (in->common.stream_type == ASTREAM_CAPTURE_CALL) {
                ALOGI("%s skip routing for call recording", __func__);
            } else {
                ALOGI("%s-%s: try to route for capture", stream_table[in->common.stream_type], __func__);
                adev_set_route((void *)in, AUSAGE_CAPTURE, ROUTE, NON_FORCE_ROUTE);
            }
        }
        pthread_mutex_unlock(&adev->lock);

        ret = proxy_open_capture_stream((void *)(in->common.proxy_stream), 0, NULL);
        if (ret != 0) {
            ALOGE("%s-%s: failed to open Proxy Capture Stream!",
                  stream_table[in->common.stream_type], __func__);
            pthread_mutex_unlock(&in->common.lock);
            return (ssize_t)ret;
        } else {
            in->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[in->common.stream_type], __func__);
        }
    }

    if (in->common.stream_status == STATUS_IDLE) {
        ret = proxy_start_capture_stream((void *)(in->common.proxy_stream));
        if (ret != 0) {
            ALOGE("%s-%s: failed to start Proxy Capture Stream!",
                  stream_table[in->common.stream_type], __func__);
            pthread_mutex_unlock(&in->common.lock);
            return (ssize_t)ret;
        } else {
            in->common.stream_status = STATUS_PLAYING;
            ALOGI("%s-%s: transited to Capturing",
                  stream_table[in->common.stream_type], __func__);
        }
    }

    if ((in->common.stream_status == STATUS_PLAYING) && (buffer && bytes > 0)) {
        if (in->common.stream_type == ASTREAM_CAPTURE_FM_TUNER && !isFMRadioOn(adev)) {
            ret = bytes;
            memset(buffer, 0, bytes);
            ALOGI("%s: skip pcm_read while stoping fm recording, ret = %d", __func__, ret);
            usleep(bytes * 1000000 / audio_stream_in_frame_size(stream) / in_get_sample_rate(&in->stream.common));
        } else
            ret = proxy_read_capture_buffer((void *)(in->common.proxy_stream),
                                            (void *)buffer, (int)bytes);

        /* Post-Processing */
        // Instead of writing zeroes here, we could trust the hardware to always provide zeroes when muted.
        if (adev->mic_mute
            && !isCallRecording(in->requested_source)) {
            if (ret >= 0)
                memset(buffer, 0, bytes);
        }
    }
    pthread_mutex_unlock(&in->common.lock);

    //ALOGVV("%s-%s: exit", stream_table[in->common.stream_type], __func__);
    return (ssize_t)ret;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    //ALOGVV("%s-%s: enter", stream_table[in->common.stream_type], __func__);

    //ALOGVV("%s-%s: exit", stream_table[in->common.stream_type], __func__);
    return 0;
}

static int in_get_capture_position(const struct audio_stream_in *stream,
                                     int64_t *frames, int64_t *time)
{
    struct stream_in *in = (struct stream_in *)stream;

    pthread_mutex_lock(&in->common.lock);
    int ret = proxy_get_capture_pos(in->common.proxy_stream, frames, time);
    pthread_mutex_unlock(&in->common.lock);

    return ret;
}

// For MMAP NOIRQ Stream
static int in_stop(const struct audio_stream_in* stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered", stream_table[in->common.stream_type], __func__);

    pthread_mutex_lock(&in->common.lock);
    if (in->common.stream_type == ASTREAM_CAPTURE_MMAP) {
        if (in->common.stream_status == STATUS_PLAYING) {
            /* Stops stream & transit to Idle. */
            proxy_stop_capture_stream((void *)(in->common.proxy_stream));
            in->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[in->common.stream_type], __func__);

        } else
            ALOGV("%s-%s: invalid operation - stream status (%d)",
                  stream_table[in->common.stream_type], __func__, in->common.stream_status);
    }
    pthread_mutex_unlock(&in->common.lock);

    ALOGV("%s-%s: exit", stream_table[in->common.stream_type], __func__);
    return ret;
}

static int in_start(const struct audio_stream_in* stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: entered", stream_table[in->common.stream_type], __func__);

    pthread_mutex_lock(&in->common.lock);
    if (in->common.stream_type == ASTREAM_CAPTURE_MMAP) {
        if (in->common.stream_status == STATUS_IDLE) {
            /* Starts stream & transit to Playing. */
            ret = proxy_start_capture_stream((void *)(in->common.proxy_stream));
            if (ret != 0) {
                ALOGE("%s-%s: failed to start Proxy Capture Stream!",
                      stream_table[in->common.stream_type], __func__);
            } else {
                in->common.stream_status = STATUS_PLAYING;
                ALOGI("%s-%s: transited to Playing",
                      stream_table[in->common.stream_type], __func__);
            }
        } else
            ALOGV("%s-%s: invalid operation - stream status (%d)",
                  stream_table[in->common.stream_type], __func__, in->common.stream_status);
    }
    pthread_mutex_unlock(&in->common.lock);

    ALOGV("%s-%s: exit", stream_table[in->common.stream_type], __func__);
    return ret;
}

static int in_create_mmap_buffer(const struct audio_stream_in *stream,
                                  int32_t min_size_frames,
                                  struct audio_mmap_buffer_info *info)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->adev;
    int ret = 0;

    ALOGD("%s-%s: entered", stream_table[in->common.stream_type], __func__);

    pthread_mutex_lock(&in->common.lock);

    if (info == NULL || min_size_frames == 0) {
        ALOGE("%s-%s: info = %p, min_size_frames = %d", stream_table[in->common.stream_type],
                                                        __func__, info, min_size_frames);
        ret = -EINVAL;
        goto exit;
    }

    if (in->common.stream_type != ASTREAM_CAPTURE_MMAP || in->common.stream_status != STATUS_STANDBY) {
        ALOGE("%s-%s: invalid operation - stream status (%d)", stream_table[in->common.stream_type],
                                                               __func__, in->common.stream_status);
        ret = -ENOSYS;
        goto exit;
    } else {
        in->common.stream_status = STATUS_READY;
        ALOGI("%s-%s: transited to Ready", stream_table[in->common.stream_type], __func__);

        // Have to route Audio Path before open PCM Device
        pthread_mutex_lock(&adev->lock);
        if (!adev->is_capture_path_routed) {
            ALOGI("%s-%s: try to route for capture", stream_table[in->common.stream_type], __func__);
            adev_set_route((void *)in, AUSAGE_CAPTURE, ROUTE, NON_FORCE_ROUTE);
        }
        pthread_mutex_unlock(&adev->lock);

        /* Opens stream & transit to Idle. */
        ret = proxy_open_capture_stream((void *)(in->common.proxy_stream), min_size_frames, (void *)info);
        if (ret != 0) {
            ALOGE("%s-%s: failed to open Proxy Capture Stream!",
                  stream_table[in->common.stream_type], __func__);

            in->common.stream_status = STATUS_STANDBY;
            ALOGI("%s-%s: transited to StandBy", stream_table[in->common.stream_type], __func__);
        } else {
            in->common.stream_status = STATUS_IDLE;
            ALOGI("%s-%s: transited to Idle", stream_table[in->common.stream_type], __func__);
        }
    }

exit:
    pthread_mutex_unlock(&in->common.lock);
    ALOGD("%s-%s: exited", stream_table[in->common.stream_type], __func__);
    return ret;
}

static int in_get_mmap_position(const struct audio_stream_in *stream,
                                  struct audio_mmap_position *position)
{
    struct stream_in *in = (struct stream_in *)stream;
    int ret = 0;

    ALOGV("%s-%s: entered", stream_table[in->common.stream_type], __func__);

    if (position == NULL) return -EINVAL;
    if (in->common.stream_type != ASTREAM_CAPTURE_MMAP) return -ENOSYS;

    ret = proxy_get_mmap_position((void *)(in->common.proxy_stream), (void *)position);

    ALOGV("%s-%s: exited", stream_table[in->common.stream_type], __func__);

    return 0;
}

static int in_get_active_microphones(const struct audio_stream_in *stream,
                                     struct audio_microphone_characteristic_t *mic_array,
                                     size_t *mic_count)
{
    struct stream_in *in = (struct stream_in *)stream;
    int ret = 0;

    ALOGVV("%s-%s: entered", stream_table[in->common.stream_type], __func__);

    if (mic_array == NULL || mic_count == NULL) return -EINVAL;

    ret = proxy_get_active_microphones(in->common.proxy_stream, (void *)mic_array, (int *)mic_count);

    ALOGVV("%s-%s: exited", stream_table[in->common.stream_type], __func__);
    return ret;
}

static void in_update_sink_metadata(struct audio_stream_in *stream,
                                    const struct sink_metadata* sink_metadata)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("%s-%s: called, but not implemented yet", stream_table[in->common.stream_type], __func__);
    if (sink_metadata->track_count > 0) {
        ALOGD("%s-%s: This stream has %zu tracks", stream_table[in->common.stream_type], __func__,
                                                   sink_metadata->track_count);

        for (int i = 0; i < (int)sink_metadata->track_count; i++) {
            ALOGD("%d Track has Source(%d), Gain(%f)", i+1, (int)sink_metadata->tracks[i].source,
                                                                 sink_metadata->tracks[i].gain);
        }
    }

    return ;
}

static int in_set_microphone_direction(
    const struct audio_stream_in *stream,
    audio_microphone_direction_t dir)
{
    (void)stream;
    (void)dir;
    ALOGV("%s", __func__);
    return -ENOSYS;
}

static int in_set_microphone_field_dimension(
    const struct audio_stream_in *stream,
    float zoom)
{
    (void)stream;
    (void)zoom;
    ALOGV("%s", __func__);
    return -ENOSYS;
}

/*****************************************************************************/
/**                                                                       **/
/** The Audio Device Function Implementation                              **/
/**                                                                       **/
/*****************************************************************************/
static uint32_t adev_get_supported_devices(const struct audio_hw_device *dev __unused)
{
    ALOGVV("device-%s: enter", __func__);

    ALOGVV("device-%s: exit", __func__);
    return 0;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = -ENODEV;

    ALOGVV("device-%s: enter", __func__);

    if (adev) {
        if (adev->proxy) {
            if (adev->is_route_created)
                ret = 0;
            else
                ALOGE("device-%s: Audio Route is not created yet", __func__);
        } else
            ALOGE("device-%s: Audio Proxy is not created yet", __func__);
    } else
        ALOGE("device-%s: Primary Audio HW Device is not created yet", __func__);

    ALOGVV("device-%s: exit", __func__);
    return ret;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *primary_output = adev->primary_output;
    struct stream_out *fast_output = adev->fast_output;

    if(volume < 0.0f || volume > 1.0f) {
        ALOGD("device-%s: invalid volume (%f)", __func__, volume);
        return -EINVAL;
    }

    pthread_mutex_lock(&adev->lock);
    if (adev->voice && voice_is_call_mode(adev->voice))
        voice_set_volume(adev->voice, volume);
    else if (adev->voice_volume == 0.0 &&
        (get_active_playback_count(adev, primary_output) > 0) &&
        (primary_output->force == FORCE_ROUTE &&
        ((primary_output->rollback_devices == AUDIO_DEVICE_OUT_WIRED_HEADSET) ||
        (primary_output->rollback_devices == AUDIO_DEVICE_OUT_USB_HEADSET)))) {
        /*
         * When Force-Route stream such as Alarm sound/Shutter tone starts, voice_volume sets 0.
         * And when this stream stopped, voice_volume is also rolled back.
         * This is right route rollback time without abnormal routing during 3 seconds standby.
         */
        ALOGI("device-%s: Primary Stream try to roll back : %d", __func__,primary_output->rollback_devices);
        primary_output->common.requested_devices = primary_output->rollback_devices;
        adev_set_route((void *)primary_output, AUSAGE_PLAYBACK, ROUTE, NON_FORCE_ROUTE);
    } else if (fast_output && adev->voice_volume == 0.0 &&
        (get_active_playback_count(adev, fast_output) > 0) &&
        (fast_output->force == FORCE_ROUTE &&
        ((fast_output->rollback_devices == AUDIO_DEVICE_OUT_WIRED_HEADSET) ||
        (fast_output->rollback_devices == AUDIO_DEVICE_OUT_USB_HEADSET)))) {
        /*
         * When Force-Route stream such asShutter tone starts with fast stream, voice_volume sets 0.
         * And when this stream stopped, voice_volume is also rolled back.
         * This is right route rollback time without abnormal routing during 3 seconds standby.
         */
        ALOGI("device-%s: Fast Stream try to roll back : %d", __func__,fast_output->rollback_devices);
        fast_output->common.requested_devices = fast_output->rollback_devices;
        adev_set_route((void *)fast_output, AUSAGE_PLAYBACK, ROUTE, NON_FORCE_ROUTE);
    }

    adev->voice_volume = volume;

    if (isCallMode(adev) == true) {
        proxy_set_volume(adev->proxy, VOLUME_TYPE_CALL, volume, volume);
    }

    ALOGD("device-%s: set volume to (%f)", __func__, volume);
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct audio_device *adev = (struct audio_device *)dev;

    ALOGD("device-%s: enter", __func__);

    pthread_mutex_lock(&adev->lock);

    /* Add code to keep mode 2 during realcall state, prevent call mute issue */
    if (adev->voice) {
        if ((mode == AUDIO_MODE_IN_COMMUNICATION) && isCPCallMode(adev) && adev->voice->realcall) {
            mode = adev->amode; // AUDIO_MODE_IN_CALL
            adev->voice->keep_call_mode = true;
            ALOGI("%s: Keep the mode %d while CP Voice call is active", __func__, mode);
        } else if ((mode == AUDIO_MODE_IN_CALL) && adev->voice->keep_call_mode) {
            /* Reset pre mode, don't need to backup voip mode after call end (2->3->2)*/
            adev->voice->keep_call_mode = false;
        }
    }

    if (adev->amode != mode) {
        /* Changing Android Audio Mode */
        adev->previous_amode = adev->amode;;
        adev->amode = mode;
        ALOGD("device-%s: changed audio mode from (%s) to (%s)", __func__,
              audiomode_table[(int)adev->previous_amode], audiomode_table[(int)adev->amode]);

        proxy_set_audiomode(adev->proxy, (int)mode);

        if (adev->voice) {
            if ((mode == AUDIO_MODE_NORMAL || mode == AUDIO_MODE_IN_COMMUNICATION) &&
                 voice_is_call_active(adev->voice)) {
                /* Change from Voice Call Mode to Normal Mode */
                /* Reset keep mode state, don't need to backup voip mode after call end */
                adev->voice->keep_call_mode = false;

                /* Stop Voice Call */
                voice_set_call_active(adev->voice, false);
                proxy_stop_voice_call(adev->proxy);
                ALOGD("device-%s: *** Stopped CP Voice Call ***", __func__);

                pthread_mutex_unlock(&adev->lock);
                audio_devices_t call_device = adev->primary_output->common.requested_devices;
                update_call_stream(adev->primary_output, call_device, call_device);
                pthread_mutex_lock(&adev->lock);

                /* Changing Voice Call Status */
                voice_set_call_mode(adev->voice, false);
            } else if (mode == AUDIO_MODE_IN_CALL) {
                /* Change from Normal/Ringtone Mode to Voice Call Mode */
                /* We cannot start Voice Call right now because we don't know which device will be used.
                   So, we need to delay Voice Call start when get the routing information for Voice Call */

                /* Changing Voice Call Status */
                voice_set_call_mode(adev->voice, true);
            }

            /* Updating Call Status to proxy */
            if (mode == AUDIO_MODE_IN_CALL || mode == AUDIO_MODE_IN_COMMUNICATION)
                proxy_call_status(adev->proxy, true);
            else
                proxy_call_status(adev->proxy, false);

            if(adev->previous_amode == AUDIO_MODE_IN_COMMUNICATION && mode == AUDIO_MODE_NORMAL){
                if(adev->active_input && (adev->active_input->common.stream_status > STATUS_STANDBY) &&
                (adev->active_input->common.stream_type == ASTREAM_CAPTURE_PRIMARY)){
                    ALOGI("device-%s: AP Call -> Normal Input PCM reconfig", __func__);
                    adev->active_input->pcm_reconfig = true;
                }
            }
        } else
            ALOGE("device-%s: There is no Voice manager", __func__);
    }
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit", __func__);
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct audio_device *adev = (struct audio_device *)dev;

    pthread_mutex_lock(&adev->lock);
    if (adev->voice && (adev->mic_mute != state))
        voice_set_mic_mute(adev->voice, state);

    adev->mic_mute = state;

    if (isCallMode(adev) == true)
        proxy_set_volume(adev->proxy, VOLUME_TYPE_CALL_TX_MUTE, state, state);

    ALOGD("device-%s: set MIC Mute to (%d)", __func__, (int)state);
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct audio_device *adev = (struct audio_device *)dev;

    pthread_mutex_lock(&adev->lock);
    *state = adev->mic_mute;
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *primary_out = adev->primary_output;

    struct str_parms *parms;
    char value[256];
    int val;
    int ret = 0;     // for parameter handling
    int status = 0;  // for return value

    ALOGD("device-%s: enter with key(%s)", __func__, kvpairs);

    pthread_mutex_lock(&adev->lock);

    parms = str_parms_create_str(kvpairs);

    status = proxy_set_parameters(adev->proxy, parms);
    if (status != 0) goto done;

    ret = str_parms_get_int(parms, AUDIO_PARAMETER_DEVICE_CONNECT, &val);
    if (ret >= 0) {
        audio_devices_t device = (audio_devices_t)val;

        if (device > AUDIO_DEVICE_BIT_IN) {
            adev->actual_capture_device = device;

            if (is_usb_in_device(device)) {
                voice_set_usb_mic(adev->voice, true);
            }
        } else {
            adev->previous_playback_device = adev->actual_playback_device;
            adev->actual_playback_device = device;
        }
        ALOGI("device-%s: connected device(%s)", __func__, device_table[get_device_id(adev, device)]);

        str_parms_del(parms, AUDIO_PARAMETER_DEVICE_CONNECT);
    }

    ret = str_parms_get_int(parms, AUDIO_PARAMETER_DEVICE_DISCONNECT, &val);
    if (ret >= 0) {
        audio_devices_t device = (audio_devices_t)val;
        ALOGI("device-%s: disconnected device(%s)", __func__, device_table[get_device_id(adev, device)]);

        if (device > AUDIO_DEVICE_BIT_IN) {
            adev->actual_capture_device = AUDIO_DEVICE_NONE;

            if (is_usb_in_device(device)) {
                voice_set_usb_mic(adev->voice, false);
            }
        } else {
            adev->actual_playback_device = adev->previous_playback_device;
            adev->previous_playback_device = device;
        }
        str_parms_del(parms, AUDIO_PARAMETER_DEVICE_DISCONNECT);
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_SCREEN_STATE, value, sizeof(value));
    if (ret >= 0) {
        ALOGI("device-%s: Screen State = %s)", __func__, value);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->screen_on = true;
        else
            adev->screen_on = false;

        str_parms_del(parms, AUDIO_PARAMETER_KEY_SCREEN_STATE);
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_SEAMLESS_VOICE, value, sizeof(value));
    if (ret >= 0) {
        bool seamless_mode = (strcmp(value, "on")) ? false : true;

        if(seamless_mode && !adev->seamless_enabled) {
            adev->seamless_enabled = true;
        } else if(!seamless_mode && adev->seamless_enabled) {
            adev->seamless_enabled = false;
        }
        ALOGV("seamless_enabled = %d", adev->seamless_enabled);
        str_parms_del(parms, AUDIO_PARAMETER_SEAMLESS_VOICE);
    }

    // For Voice Manager
    if (adev->voice)
        voice_set_parameters(adev, parms);

    // For Factory Manager
    if (adev->factory && adev->voice)
        factory_set_parameters(adev, parms);

    odm_adev_set_parameters(adev, parms);

done:
    str_parms_destroy(parms);
    pthread_mutex_unlock(&adev->lock);

    ALOGVV("device-%s: exit", __func__);
    return status;
}

static char * adev_get_parameters(const struct audio_hw_device *dev, const char *keys)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct str_parms *reply = str_parms_create();
    struct str_parms *query = str_parms_create_str(keys);
    char *str;

    ALOGVV("device-%s: enter with key(%s)", __func__, keys);

    pthread_mutex_lock(&adev->lock);

    int ret = 0;
    char value[32];
    int beam_value;

    odm_adev_get_parameters(adev, query);

    //requested by phone app to check call_forwarding status.
    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_CALL_FORWARDING, value, sizeof(value));
    if (ret >= 0) {
        strlcpy(value, ((adev->voice && adev->voice->call_forwarding) ? "true" : "false"), sizeof(value));
        ALOGI("device-%s: call_forwarding is %s", __func__, value);
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_CALL_FORWARDING, value);
    }

    ret = str_parms_get_str(query, AUDIO_PARAMETER_KEY_EXTRA_VOLUME, value, sizeof(value));
    if (ret >= 0) {
        strlcpy(value, ((adev->voice && adev->voice->extra_volume) ? "true" : "false"), sizeof(value));
        ALOGI("device-%s: extra_volume is %s", __func__, value);
        str_parms_add_str(reply, AUDIO_PARAMETER_KEY_EXTRA_VOLUME, value);
    }

    str = str_parms_to_str(reply);
    str_parms_destroy(query);
    str_parms_destroy(reply);

    pthread_mutex_unlock(&adev->lock);

    ALOGVV("device-%s: exit with %s", __func__, str);
    return str;
}

static size_t adev_get_input_buffer_size(
            const struct audio_hw_device *dev __unused,
            const struct audio_config *config)
{
    size_t size = 0;
    unsigned int period_size = 0;

    ALOGVV("device-%s: enter with SR(%d Hz), Channel(%d)", __func__,
           config->sample_rate, audio_channel_count_from_in_mask(config->channel_mask));

    /*
     * To calcurate actual buffer size, it needs period size.
     * However, it cannot fix period size at this time.
     * So, pre-defined period size will be using.
     */

    // return buffer size based on requested PCM configuration
    period_size = (config->sample_rate * PREDEFINED_CAPTURE_DURATION) / 1000;
    size = period_size * audio_bytes_per_sample(config->format) *
           audio_channel_count_from_in_mask(config->channel_mask);
    ALOGVV("device-%s: exit with %d bytes for %d ms", __func__, (int)size, PREDEFINED_CAPTURE_DURATION);
    return size;
}

static int adev_open_output_stream(
        struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        audio_output_flags_t flags,
        struct audio_config *config,
        struct audio_stream_out **stream_out,
        const char *address)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out = NULL;
    int ret;

    ALOGD("device-%s: enter: io_handle (%d), sample_rate(%u) channel_mask(%#x) format(%#x) framecount(%u) devices(%#x) flags(%#x)",
          __func__, handle, config->sample_rate, config->channel_mask, config->format, config->frame_count, devices, flags);

    *stream_out = NULL;

    /* Allocates the memory for structure audio_stream_out. */
    out = (struct stream_out *)calloc(1, sizeof(struct stream_out));
    if (!out) {
        ALOGE("device-%s: failed to allocate memory for stream_out", __func__);
        return -ENOMEM;
    }
    out->adev = adev;

    /* Saves the requested parameters from Android Platform. */
    out->common.handle = handle;
    out->common.requested_devices = devices;
    out->common.requested_sample_rate = config->sample_rate;
    out->common.requested_channel_mask = config->channel_mask;
    out->common.requested_format = config->format;
    out->common.requested_frame_count = config->frame_count;
    out->requested_flags = flags;

    /*
     * Sets Stream Type & Audio Usage Type from Audio Flags and Devices.
     * These information can be used to decide Mixer Path.
     */
    out->common.stream_type = ASTREAM_NONE;
    out->common.stream_usage = AUSAGE_NONE;
    out->direct_volume_enabled = false;

    if (flags == AUDIO_OUTPUT_FLAG_NONE) {
        /* Case: No Attributes Playback Stream */
        if (devices == AUDIO_DEVICE_OUT_AUX_DIGITAL) {
            /* Sub-Case: Playback with Aux Digital */
            ALOGI("device-%s: requested to open AUX-DIGITAL playback stream", __func__);
            out->common.stream_type = ASTREAM_PLAYBACK_AUX_DIGITAL;
            out->common.stream_usage = AUSAGE_MEDIA;
        } else {
            ALOGI("device-%s: requested to open No Attributes playback stream", __func__);
            out->common.stream_type = ASTREAM_PLAYBACK_NO_ATTRIBUTE;
            out->common.stream_usage = AUSAGE_MEDIA;
        }
    } else if ((flags & AUDIO_OUTPUT_FLAG_DIRECT) != 0) {
        /* Case: Direct Playback Stream */
        if (((flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0) &&
            ((flags & AUDIO_OUTPUT_FLAG_NON_BLOCKING) != 0)) {
            /* Sub-Case: Direct Playback Stream with Non-Blocking Compress Offload */
            ALOGI("device-%s: requested to open Compress Offload playback stream", __func__);
            out->common.stream_type = ASTREAM_PLAYBACK_COMPR_OFFLOAD;
            out->common.stream_usage = AUSAGE_MEDIA;

            /* Maps the function pointers in structure audio_stream_out as actual function. */
            out->stream.set_callback = out_set_callback;
            out->stream.pause = out_pause;
            out->stream.resume = out_resume;
            out->stream.drain = out_drain;
            out->stream.flush = out_flush;

            pthread_mutex_lock(&adev->lock);
            if (adev->compress_output == NULL)
                adev->compress_output = out;
            pthread_mutex_unlock(&adev->lock);
         } else if ((flags & AUDIO_OUTPUT_FLAG_MMAP_NOIRQ) != 0) {
            /* Case: MMAP No IRQ Playback Stream */
            ALOGI("device-%s: requested to open MMAP No IRQ playback stream", __func__);

            out->common.stream_type = ASTREAM_PLAYBACK_MMAP;
            out->common.stream_usage = AUSAGE_MEDIA;

            /* Maps the function pointers in structure audio_stream_out as actual function. */
            out->stream.start = out_start;
            out->stream.stop = out_stop;
            out->stream.create_mmap_buffer = out_create_mmap_buffer;
            out->stream.get_mmap_position = out_get_mmap_position;
         }
#ifdef SUPPORT_DIRECT_MULTI_CHANNEL_STREAM
         else {
            /* Case: Direct output for multi-channel Playback Stream */
            ALOGI("device-%s: requested to Direct Multi-channel playback stream", __func__);

            out->common.stream_type = ASTREAM_PLAYBACK_DIRECT;
            out->common.stream_usage = AUSAGE_MEDIA;
         }
#endif
    } else if ((flags & AUDIO_OUTPUT_FLAG_PRIMARY) != 0) {
        /*
        * Initializes Voice Manager.
        * Voice Manager is handling Voice to support Call scenarios.
        */
        if (!adev->voice) {
            adev->voice = voice_init(adev);
            if(!adev->voice)
                ALOGE("device-%s: failed to init Voice Manager!", __func__);
            else
                ALOGD("device-%s: initialized Voice Manager!", __func__);
        }

        /* Case: Primary Playback Stream */
        ALOGI("device-%s: requested to open Primary playback stream", __func__);

        pthread_mutex_lock(&adev->lock);
        if (adev->primary_output == NULL)
            adev->primary_output = out;
        else {
            ALOGE("device-%s: Primary playback stream is already opened", __func__);
            ret = -EEXIST;
            pthread_mutex_unlock(&adev->lock);
            goto err_open;
        }
        pthread_mutex_unlock(&adev->lock);

        out->common.stream_type = ASTREAM_PLAYBACK_PRIMARY;
        out->common.stream_usage = AUSAGE_MEDIA;
    } else if ((flags & AUDIO_OUTPUT_FLAG_FAST) != 0) {
        /* Case: Fast Playback Stream */
        if ((flags & AUDIO_OUTPUT_FLAG_RAW) != 0) {
            /* Sub-Case: Low Latency Playback Stream for ProAudio */
            ALOGI("device-%s: requested to open Low Latency playback stream", __func__);

            out->common.stream_type = ASTREAM_PLAYBACK_LOW_LATENCY;
        } else {
            /* Sub-Case: Normal Fast Playback Stream */
            ALOGI("device-%s: requested to open Fast playback stream", __func__);
            pthread_mutex_lock(&adev->lock);
            if (adev->fast_output == NULL)
                adev->fast_output = out;
            pthread_mutex_unlock(&adev->lock);

            out->common.stream_type = ASTREAM_PLAYBACK_FAST;
        }
        out->common.stream_usage = AUSAGE_MEDIA;
    } else if ((flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) != 0) {
        /* Case: Deep Buffer Playback Stream */
        ALOGI("device-%s: requested to open Deep Buffer playback stream", __func__);

        out->common.stream_type = ASTREAM_PLAYBACK_DEEP_BUFFER;
        out->common.stream_usage = AUSAGE_MEDIA;
    } else if ((flags & AUDIO_OUTPUT_FLAG_INCALL_MUSIC) != 0) {
        /* Case: Incall Music Playback Stream */
        ALOGI("device-%s: requested to open Incall Music playback stream", __func__);

        out->common.stream_type = ASTREAM_PLAYBACK_INCALL_MUSIC;
        out->common.stream_usage = AUSAGE_INCALL_MUSIC;
    } else {
        /* Error Case: Not Supported usage */
        ALOGI("device-%s: requested to open un-supported output", __func__);

        ret = -EINVAL;
        goto err_open;
    }

    /* Maps the function pointers in structure audio_stream_out as actual function. */
    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.get_device = out_get_device;
    out->stream.common.set_device = out_set_device;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;

    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    out->stream.get_presentation_position = out_get_presentation_position;

    // For AudioHAL V4
    out->stream.update_source_metadata = out_update_source_metadata;

    /* Sets platform-specific information. */
    pthread_mutex_init(&out->common.lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_lock(&out->common.lock);

    // Creates Proxy Stream
    out->common.proxy_stream = proxy_create_playback_stream(adev->proxy,
                                                            (int)out->common.stream_type,
                                                            (void *)config, (char *)address);
    if (!out->common.proxy_stream) {
        ALOGE("%s-%s: failed to create Audio Proxy Stream", stream_table[out->common.stream_type], __func__);

        ret = -EINVAL;
        pthread_mutex_unlock(&out->common.lock);
        goto err_open;
    }

    // Special Process for Compress Offload
    if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
        /* Creates Callback Thread for supporting Non-Blocking Mode */
        if (flags & AUDIO_OUTPUT_FLAG_NON_BLOCKING) {
            ALOGV("%s-%s: Need to work as Nonblock Mode!", stream_table[out->common.stream_type], __func__);
            proxy_offload_set_nonblock(out->common.proxy_stream);
            out->offload.nonblock_flag = 1;

            create_offload_callback_thread(out);
            list_init(&out->offload.msg_list);
        }

        /* Connects Offload Effect Libraries */

        out->common.offload_audio_format = AUDIO_FORMAT_PCM_16_BIT;
    }

    // Special Process for DP Audio
    // In general, this stream will be opened at Null Configuration.
    // So it needs to update configuration as actual value
    if (out->common.stream_type == ASTREAM_PLAYBACK_AUX_DIGITAL ||
        (out->common.stream_type == ASTREAM_PLAYBACK_NO_ATTRIBUTE &&
        is_usb_out_device(devices))) {
        if (config->sample_rate == 0) {
            config->sample_rate = proxy_get_actual_sampling_rate(out->common.proxy_stream);
            out->common.requested_sample_rate = config->sample_rate;
        }

        if (config->channel_mask == AUDIO_CHANNEL_NONE) {
            config->channel_mask = audio_channel_out_mask_from_count(
                                         proxy_get_actual_channel_count(out->common.proxy_stream));
            out->common.requested_channel_mask = config->channel_mask;
        }

        if (config->format == AUDIO_FORMAT_DEFAULT) {
            config->format = (audio_format_t)proxy_get_actual_format(out->common.proxy_stream);
            out->common.requested_format = config->format;
        }
    }

    out->common.stream_status = STATUS_STANDBY;   // Not open PCM Device, yet
    ALOGI("%s-%s: transited to Standby", stream_table[out->common.stream_type], __func__);

    // Adds this stream into playback list
    pthread_mutex_lock(&adev->lock);
    struct playback_stream *out_node = (struct playback_stream *)calloc(1, sizeof(struct playback_stream));
    out_node->out = out;
    list_add_tail(&adev->playback_list, &out_node->list_node);
    pthread_mutex_unlock(&adev->lock);

    out->force = NON_FORCE_ROUTE;
    out->rollback_devices = AUDIO_DEVICE_NONE;

    pthread_mutex_unlock(&out->common.lock);

    /* Sets the structure audio_stream_out for return. */
    *stream_out = &out->stream;

    ALOGI("device-%s: opened %s stream", __func__, stream_table[out->common.stream_type]);
    return 0;

err_open:
    if (out->common.proxy_stream) {
        proxy_destroy_playback_stream(out->common.proxy_stream);
        out->common.proxy_stream = NULL;
    }

    pthread_mutex_lock(&adev->lock);
    if (out->common.stream_type == ASTREAM_PLAYBACK_PRIMARY)
        adev->primary_output = NULL;
    else if (out->common.stream_type == ASTREAM_PLAYBACK_FAST)
        adev->fast_output = NULL;
    pthread_mutex_unlock(&adev->lock);

    free(out);
    *stream_out = NULL;

    ALOGI("device-%s: failed to open this stream as error(%d)", __func__, ret);
    return ret;
}

static void adev_close_output_stream(
            struct audio_hw_device *dev,
            struct audio_stream_out *stream)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out = (struct stream_out *)stream;

    struct listnode *node, *auxi;
    struct playback_stream *out_node;

    ALOGVV("device-%s: enter", __func__);

    if (out) {
        ALOGI("%s-%s: try to close plyback stream", stream_table[out->common.stream_type], __func__);
        out_standby(&stream->common);

        pthread_mutex_lock(&out->common.lock);
        pthread_mutex_lock(&adev->lock);

        // Removes this stream from playback list
        list_for_each_safe(node, auxi, &adev->playback_list)
        {
            out_node = node_to_item(node, struct playback_stream, list_node);
            if (out_node->out == out) {
                list_remove(node);
                free(out_node);
            }
        }

        if (adev->primary_output == out) {
            ALOGI("%s-%s: requested to close Primary playback stream",
                                                   stream_table[out->common.stream_type], __func__);
            adev->primary_output = NULL;
        }

        if (adev->fast_output == out) {
            ALOGI("%s-%s: requested to close Fast playback stream",
                          stream_table[out->common.stream_type], __func__);
            adev->fast_output = NULL;
        }

        if (adev->compress_output == out) {
            ALOGI("%s-%s: requested to close Compress Offload playback stream",
                                                   stream_table[out->common.stream_type], __func__);
            adev->compress_output = NULL;
        }

        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&out->common.lock);

        if (out->common.stream_type == ASTREAM_PLAYBACK_COMPR_OFFLOAD) {
            if (out->offload.nonblock_flag)
                destroy_offload_callback_thread(out);
        }

        if (out->common.stream_type == ASTREAM_PLAYBACK_INCALL_MUSIC) {
            adev->incallmusic_on = false;
        }

        pthread_mutex_lock(&out->common.lock);
        proxy_destroy_playback_stream(out->common.proxy_stream);
        out->common.proxy_stream = NULL;
        pthread_mutex_unlock(&out->common.lock);

        pthread_mutex_destroy(&out->common.lock);

        ALOGI("%s-%s: closed playback stream", stream_table[out->common.stream_type], __func__);
        free(out);
    }

    ALOGVV("device-%s: exit", __func__);
    return;
}

static int adev_open_input_stream(
        struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        struct audio_config *config,
        struct audio_stream_in **stream_in,
        audio_input_flags_t flags,
        const char *address,
        audio_source_t source)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in = NULL;
    int ret;

    ALOGD("device-%s: enter: io_handle (%d), sample_rate(%d) channel_mask(%#x) format(%#x) framecount(%u) devices(%#x) flags(%#x) sources(%d)",
          __func__, handle, config->sample_rate, config->channel_mask, config->format, config->frame_count, devices, flags, source);

    *stream_in = NULL;

    /* Allocates the memory for structure audio_stream_in. */
    in = (struct stream_in *)calloc(1, sizeof(struct stream_in));
    if (!in) {
        ALOGE("device-%s: failed to allocate memory for stream_in", __func__);
        return -ENOMEM;
    }
    in->adev = adev;

    /* Saves the requested parameters from Android Platform. */
    in->common.handle = handle;
    in->common.requested_devices = devices;
    in->common.requested_sample_rate = config->sample_rate;
    in->common.requested_channel_mask = config->channel_mask;
    in->common.requested_format = config->format;
    in->common.requested_frame_count = config->frame_count;
    in->requested_flags = flags;
    in->requested_source = source;

    /*
     * Sets Stream Type & Audio Usage Type from Audio Flags, Sources and Devices.
     * These information can be used to decide Mixer Path.
     */
    in->common.stream_type = ASTREAM_NONE;
    in->common.stream_usage = AUSAGE_NONE;

    if ((flags & AUDIO_INPUT_FLAG_FAST) != 0) {
        if (isCallMode(adev) && config->sample_rate != LOW_LATENCY_CAPTURE_SAMPLE_RATE) {
            flags &= ~AUDIO_INPUT_FLAG_FAST;
            flags |= AUDIO_INPUT_FLAG_VOIP_TX;
            ALOGD("device-%s: Denied to open Low Latency input. flags changed(%#x)", __func__, flags);
        }
    }

#ifdef SUPPORT_STHAL_INTERFACE
    if (source == AUDIO_SOURCE_HOTWORD ||
        (adev->seamless_enabled == true && source == AUDIO_SOURCE_VOICE_RECOGNITION) ||
        (((flags & AUDIO_INPUT_FLAG_HW_HOTWORD) != 0) && source == AUDIO_SOURCE_VOICE_RECOGNITION)) {
        /* Case: Use ST HAL interface for Capture */
        in->common.stream_type = ASTREAM_CAPTURE_HOTWORD;
        if (source == AUDIO_SOURCE_HOTWORD ||
        (adev->seamless_enabled == true && source == AUDIO_SOURCE_VOICE_RECOGNITION)) {
            in->common.stream_usage = AUSAGE_HOTWORD_SEAMLESS;
            ALOGD("device-%s: Requested to open VTS Seamless input", __func__);
        } else {
            in->common.stream_usage = AUSAGE_HOTWORD_RECORD;
            ALOGD("device-%s: Requested to open VTS Record input", __func__);
        }
    } else if (flags == AUDIO_INPUT_FLAG_NONE || (flags & AUDIO_INPUT_FLAG_VOIP_TX) != 0)
#else
    if (flags == AUDIO_INPUT_FLAG_NONE || (flags & AUDIO_INPUT_FLAG_VOIP_TX) != 0)
#endif
    {
        pthread_mutex_lock(&adev->lock);

        if (isCPCallMode(adev) &&
            is_usage_CPCall(adev->active_capture_ausage) &&
            isCallRecording(in->requested_source)) {
            /* Case: CP Voice Call Recording Stream */
            ALOGI("device-%s: requested to open CP Voice Call Recording stream", __func__);
            in->common.stream_type = ASTREAM_CAPTURE_CALL;

            switch (source) {
                case AUDIO_SOURCE_VOICE_UPLINK:
                    ALOGI("device-%s: needs only uplink voice", __func__);
                    in->common.stream_usage = AUSAGE_INCALL_UPLINK;
                    break;

                case AUDIO_SOURCE_VOICE_DOWNLINK:
                    ALOGI("device-%s: needs only downlink voice", __func__);
                    in->common.stream_usage = AUSAGE_INCALL_DOWNLINK;
                    break;

                case AUDIO_SOURCE_VOICE_CALL:
                default:
                    ALOGI("device-%s: needs uplink/downlink voice both", __func__);
                    in->common.stream_usage = AUSAGE_INCALL_UPLINK_DOWNLINK;
                    break;
            }
        } else if (fmradio_check_input_stream(adev, source, devices) == true) {
            if (fmradio_open_input_stream(adev, in) == -1) {
                ret = -EINVAL;
                pthread_mutex_unlock(&adev->lock);
                goto err_open;
            }
        } else {
            /* Case: Normal Recording Stream */
            if ((devices == AUDIO_DEVICE_IN_DEFAULT) && (source == AUDIO_SOURCE_DEFAULT)) {
                /* Case: No Attributes Capture Stream */
                ALOGI("device-%s: requested to open No Attribute capture stream", __func__);
                in->common.stream_type = ASTREAM_CAPTURE_NO_ATTRIBUTE;
                in->common.stream_usage = AUSAGE_RECORDING;
            } else {
                /* Case: Primary Capture Stream */
                ALOGI("device-%s: requested to open Primay capture stream", __func__);
                in->common.stream_type = ASTREAM_CAPTURE_PRIMARY;

                switch (source) {
                    case AUDIO_SOURCE_MIC:
                        in->common.stream_usage = AUSAGE_RECORDING;
                        break;

                    case AUDIO_SOURCE_CAMCORDER:
                        in->common.stream_usage = AUSAGE_CAMCORDER;
                        adev_set_route((void *)in, AUSAGE_CAPTURE, ROUTE, NON_FORCE_ROUTE);
                        break;

                    case AUDIO_SOURCE_VOICE_RECOGNITION:
                        in->common.stream_usage = AUSAGE_RECOGNITION;
                        break;

                    case AUDIO_SOURCE_DEFAULT:
                    default:
                        in->common.stream_usage = AUSAGE_RECORDING;
                        break;
                }
            }
        }
        pthread_mutex_unlock(&adev->lock);
    } else if ((flags & AUDIO_INPUT_FLAG_FAST) != 0) {
        /* Case: Low Latency Capture Stream */
        ALOGI("device-%s: requested to open Low Latency capture stream", __func__);
        in->common.stream_type = ASTREAM_CAPTURE_LOW_LATENCY;
        in->common.stream_usage = AUSAGE_MEDIA;
    } else if ((flags & AUDIO_INPUT_FLAG_MMAP_NOIRQ) != 0) {
        if (config->sample_rate == LOW_LATENCY_CAPTURE_SAMPLE_RATE) {
            /* Case: MMAP No IRQ Capture Stream */
            ALOGI("device-%s: requested to open MMAP No IRQ capture stream", __func__);

            in->common.stream_type = ASTREAM_CAPTURE_MMAP;
            in->common.stream_usage = AUSAGE_MEDIA;

            /* Maps the function pointers in structure audio_stream_in as actual function. */
            in->stream.start = in_start;
            in->stream.stop = in_stop;
            in->stream.create_mmap_buffer = in_create_mmap_buffer;
            in->stream.get_mmap_position = in_get_mmap_position;
        } else {
            /* Error Case: Not Supported samspling rate */
            ALOGI("device-%s: requested to open MMAP input with abnormal sampling rate(%d)",
                  __func__, config->sample_rate);

            ret = -EINVAL;
            goto err_open;
        }
    } else {
        /* Error Case: Not Supported usage */
        ALOGI("device-%s: requested to open un-supported output", __func__);

        ret = -EINVAL;
        goto err_open;
    }


    /* Maps the function pointers in structure audio_stream_in as actual function. */
    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.get_device = in_get_device;
    in->stream.common.set_device = in_set_device;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;

    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;
    in->stream.get_capture_position = in_get_capture_position;

    // For AudioHAL V4
    in->stream.get_active_microphones = in_get_active_microphones;
    in->stream.update_sink_metadata = in_update_sink_metadata;

    // for AudioHAL V5
    in->stream.set_microphone_direction = in_set_microphone_direction;
    in->stream.set_microphone_field_dimension = in_set_microphone_field_dimension;

    /* Sets Platform-specific information. */
    pthread_mutex_init(&in->common.lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_lock(&in->common.lock);

    // Creates Proxy Stream
    in->common.proxy_stream = proxy_create_capture_stream(adev->proxy,
                                                          (int)in->common.stream_type,
                                                          (int)in->common.stream_usage,
                                                          (void *)config, (char *)address);
    if (!in->common.proxy_stream) {
        ALOGE("%s-%s: failed to create Audio Proxy Stream", stream_table[in->common.stream_type], __func__);

        ret = -EINVAL;
        pthread_mutex_unlock(&in->common.lock);
        goto err_open;
    }

    // Process for USB Device
    if (is_usb_in_device(devices)) {
        if (config->sample_rate == 0) {
            config->sample_rate = proxy_get_actual_sampling_rate(in->common.proxy_stream);
            in->common.requested_sample_rate = config->sample_rate;
        }

        if (config->channel_mask == AUDIO_CHANNEL_NONE) {
            config->channel_mask = audio_channel_in_mask_from_count(
                                         proxy_get_actual_channel_count(in->common.proxy_stream));
            in->common.requested_channel_mask = config->channel_mask;
        }

        if (config->format == AUDIO_FORMAT_DEFAULT) {
            config->format = (audio_format_t)proxy_get_actual_format(in->common.proxy_stream);
            in->common.requested_format = config->format;
        }
    }

    in->common.stream_status = STATUS_STANDBY;   // Not open PCM Device, yet
    ALOGI("%s-%s: transited to Standby", stream_table[in->common.stream_type], __func__);

    // Adds this stream into capture list
    pthread_mutex_lock(&adev->lock);
    struct capture_stream *in_node = (struct capture_stream *)calloc(1, sizeof(struct capture_stream));
    in_node->in = in;
    list_add_tail(&adev->capture_list, &in_node->node);

    // Customer Specific
    adev->pcmread_latency = proxy_get_actual_period_size(in->common.proxy_stream) * 1000 /
                            proxy_get_actual_sampling_rate(in->common.proxy_stream);
    in->pcm_reconfig = false;

    pthread_mutex_unlock(&adev->lock);

    pthread_mutex_unlock(&in->common.lock);

    /* Sets the structure audio_stream_in for return. */
    *stream_in = &in->stream;

    ALOGI("device-%s: opened %s stream", __func__, stream_table[in->common.stream_type]);
    return 0;

err_open:
    if (in)
        free(in);
    *stream_in = NULL;

    ALOGI("device-%s: failed to open this stream as error(%d)", __func__, ret);
    return ret;
}

static void adev_close_input_stream(
        struct audio_hw_device *dev,
        struct audio_stream_in *stream)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in = (struct stream_in *)stream;

    struct listnode *node, *auxi;
    struct capture_stream *in_node;

    ALOGVV("device-%s: enter", __func__);

    if (in) {
        ALOGI("%s-%s: try to close capture stream", stream_table[in->common.stream_type], __func__);
        in_standby(&stream->common);

        pthread_mutex_lock(&in->common.lock);
        pthread_mutex_lock(&adev->lock);
        // Removes this stream from capture list
        list_for_each_safe(node, auxi, &adev->capture_list)
        {
            in_node = node_to_item(node, struct capture_stream, node);
            if (in_node->in == in) {
                list_remove(node);
                free(in_node);
            }
        }
        pthread_mutex_unlock(&adev->lock);

        proxy_destroy_capture_stream(in->common.proxy_stream);
        in->common.proxy_stream = NULL;

        pthread_mutex_unlock(&in->common.lock);
        pthread_mutex_destroy(&in->common.lock);

        ALOGI("%s-%s: closed capture stream", stream_table[in->common.stream_type], __func__);
        free(in);
    }

    ALOGVV("device-%s: exit", __func__);
    return;
}

static int adev_get_microphones(const audio_hw_device_t *device,
                                struct audio_microphone_characteristic_t *mic_array,
                                size_t *mic_count)
{
    struct audio_device *adev = (struct audio_device *)device;
    int ret = 0;

    ALOGVV("device-%s: entered", __func__);

    if (mic_array == NULL || mic_count == NULL) return -EINVAL;

    ret = proxy_get_microphones(adev->proxy, (void *)mic_array, (int *)mic_count);

    ALOGVV("device-%s: exited", __func__);
    return ret;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    struct audio_device *adev = (struct audio_device *)device;

    ALOGV("device-%s: enter with file descriptor(%d)", __func__, fd);

    const size_t len = 256;
    char buffer[len];

    snprintf(buffer, len, "\nAudioDevice HAL::dump\n");
    write(fd,buffer,strlen(buffer));

    bool justLocked = pthread_mutex_trylock(&adev->lock) == 0;
    snprintf(buffer, len, "\tMutex: %s\n", justLocked ? "locked" : "unlocked");
    write(fd,buffer,strlen(buffer));
    if(justLocked)
        pthread_mutex_unlock(&adev->lock);

    snprintf(buffer, len, "1. Common part\n");
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tAudio Mode: %d\n",adev->amode);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tAudio Previous Mode: %d\n",adev->previous_amode);
    write(fd,buffer,strlen(buffer));

    snprintf(buffer, len, "\n2. About Call part\n");
    write(fd,buffer,strlen(buffer));
    if(adev->voice) {
        snprintf(buffer, len, "\tCall Active: %d\n", adev->voice->call_status);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tRealCall: %s\n",bool_to_str(adev->voice->realcall));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tVoLTE state: %s\n",bool_to_str(adev->voice->volte_status));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tPrevious VoLTE state: %s\n",bool_to_str(adev->voice->previous_volte_status));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tcurrent modem: %d\n",adev->voice->cur_modem);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tVoice Volume: %f\n",adev->voice_volume);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tVoice Volume Max Index: %d\n",adev->voice->volume_steps_max);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tMute Voice: %s\n",bool_to_str(adev->voice->mute_voice));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\twb_amr: %d\n",adev->voice->voice_samplingrate);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\textra volume: %s\n",bool_to_str(adev->voice->extra_volume));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tcall forwarding: %s\n",bool_to_str(adev->voice->call_forwarding)) ;
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tTTY mode: %d\n",adev->voice->tty_mode) ;
        write(fd,buffer,strlen(buffer));
    }
    snprintf(buffer, len, "\tMic Mute: %s\n",bool_to_str(adev->mic_mute));
    write(fd,buffer,strlen(buffer));
    //snprintf(buffer, len, "\tspectro: %s\n",bool_to_str(adev->spectro)) ;
    //write(fd,buffer,strlen(buffer));

    snprintf(buffer, len, "\n3. About Communications part\n");
    write(fd,buffer,strlen(buffer));
    if(adev->voice) {
        snprintf(buffer, len, "\tVoip wificalling: %s\n",bool_to_str(adev->voice->voip_wificalling));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tCSVT Call: %s\n",bool_to_str(adev->voice->csvtcall));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tVoIP RX active: %s\n",bool_to_str(adev->voice->voip_rx_active)) ;
        write(fd,buffer,strlen(buffer));
    }
//    snprintf(buffer, len, "\tRingtone active: %s\n",bool_to_str(adev->ringtone_active));
//    write(fd,buffer,strlen(buffer));

    snprintf(buffer, len, "\n4. About Connectivity part\n");
    write(fd,buffer,strlen(buffer));
    if(adev->voice) {
        snprintf(buffer, len, "\tBluetooth_nrec: %d\n",adev->voice->bluetooth_nrec);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tBluetooth samplerate: %u\n",adev->voice->bluetooth_samplerate);
        write(fd,buffer,strlen(buffer));
    }

    snprintf(buffer, len, "\n5. About Device Routing part\n");
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tAudio Playback Usage Mode: %d\n",adev->active_playback_ausage);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tAudio Capture Usage Mode: %d\n",adev->active_capture_ausage);
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tSupport rev: %s\n",bool_to_str(adev->support_reciever));
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tSupport backmic: %s\n",bool_to_str(adev->support_backmic));
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tSupport thirdmic: %s\n",bool_to_str(adev->support_thirdmic));
    write(fd,buffer,strlen(buffer));

    snprintf(buffer, len, "\n6. About Offload Playback part\n");
    write(fd,buffer,strlen(buffer));

    if(adev->factory) {
        snprintf(buffer, len, "\n7. About Factory Test part\n");
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tLoopback Out Device: %x\n",adev->factory->out_device);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tLoopback In Device: %x\n",adev->factory->in_device);
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tFactory mode: %s\n",bool_to_str(adev->factory->mode));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tis RMS Enable: %s\n",bool_to_str(adev->factory->rms_test_enable));
        write(fd,buffer,strlen(buffer));
        snprintf(buffer, len, "\tfactory loopback mode: %d\n\n",adev->factory->loopback_mode);
        write(fd,buffer,strlen(buffer));
    }

    snprintf(buffer, len, "\n9. FM Radio part\n");
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tFM Radio State: %s\n",adev->fm_state == FM_ON ? "FM_ON" : "FM_OFF");
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tFM Radio via BT: %s\n",bool_to_str(adev->fm_via_a2dp));
    write(fd,buffer,strlen(buffer));
    snprintf(buffer, len, "\tFM Radio Mute: %s\n",bool_to_str(adev->fm_radio_mute));
    write(fd,buffer,strlen(buffer));

    voice_ril_dump(fd);

    if(adev->primary_output)
        out_dump((struct audio_stream *)adev->primary_output,fd);
    if(adev->active_input)
        in_dump((struct audio_stream *)adev->active_input,fd);

    proxy_fw_dump(fd);

    ALOGV("device-%s: exit with file descriptor(%d)", __func__, fd);
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct audio_device *adev = (struct audio_device *)device;

    ALOGI("device-%s: enter", __func__);

    if (adev) {
        pthread_mutex_lock(&adev_init_lock);

        if ((--adev_ref_count) == 0) {
            /* Clean up Platform-specific information. */
            pthread_mutex_lock(&adev->lock);

            if (adev->voice) {
                voice_deinit(adev->voice);
                adev->voice = NULL;
            }

            adev_deinit_route(adev);

            /* Clear external structures. */
            if (adev->proxy) {
                proxy_deinit(adev->proxy);
                adev->proxy = NULL;
            }

            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_destroy(&adev->lock);

            destroyAudioDeviceInstance();
            ALOGI("device-%s: closed Primary Audio HW Device(ref = %d)", __func__, adev_ref_count);
        } else
            ALOGI("device-%s: closed existing Primary Audio HW Device(ref = %d)", __func__, adev_ref_count);

        pthread_mutex_unlock(&adev_init_lock);
    }

    return 0;
}

static int adev_open(
        const hw_module_t* module,
        const char* name,
        hw_device_t** device)
{
    struct audio_device *adev = NULL;

    ALOGI("device-%s: enter", __func__);

    /* Check Interface Name. It must be AUDIO_HARDWARE_INTERFACE. */
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) {
        ALOGE("device-%s: invalid request: Interface Name = %s", __func__, name);
        return -EINVAL;
    }

    pthread_mutex_lock(&adev_init_lock);
    /* Create globally unique instance for Structure audio_device. */
    adev = getAudioDeviceInstance();
    if (!adev) {
        ALOGE("device-%s: failed to allocate memory for audio_device", __func__);
        pthread_mutex_unlock(&adev_init_lock);
        return -ENOMEM;
    }

    if (adev_ref_count != 0) {
        *device = &adev->hw_device.common;
        adev_ref_count++;
        ALOGI("device-%s: opened existing Primary Audio HW Device(ref = %d)", __func__, adev_ref_count);
        pthread_mutex_unlock(&adev_init_lock);
        return 0;
    }

    /* Initialize Audio Device Mutex Lock. */
    pthread_mutex_init(&adev->lock, (const pthread_mutexattr_t *) NULL);

    /*
     * Map function pointers in Structure audio_hw_device as real function.
     */
    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    /* Enumerates what devices are supported by audio_hw_device implementation. */
    adev->hw_device.get_supported_devices = adev_get_supported_devices;
    /* Checks to see if the audio hardware interface has been initialized. */
    adev->hw_device.init_check = adev_init_check;
    /* Sets the audio volume of a voice call. */
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    /* Sets/Gets the master volume value for the HAL. */
    // Not Supported
    adev->hw_device.set_master_volume = NULL;
    adev->hw_device.get_master_volume = NULL;
    /* Called when the audio mode changes. */
    adev->hw_device.set_mode = adev_set_mode;
    /* Treats mic mute. */
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    /* Sets/Gets global audio parameters. */
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    /* Returns audio input buffer size according to parameters passed. */
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    /* Creates ,opens, closes and destroys the audio hardware input/output stream. */
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    /* Read available microphones characteristics for AudioHAL V4. */
    adev->hw_device.get_microphones = adev_get_microphones;
    /* Dumps the status of the audio hardware. */
    adev->hw_device.dump = adev_dump;
    /* Set/Get the master mute status for the HAL. */
    // Not Supported
    adev->hw_device.set_master_mute = NULL;
    adev->hw_device.get_master_mute = NULL;

    /* Creates and releases an audio patch between several source and sink ports. */
    // Not Supported
    adev->hw_device.create_audio_patch = NULL;
    adev->hw_device.release_audio_patch = NULL;
    /* Sets/Gets audio port configuration. */
    // Not Supported
    adev->hw_device.get_audio_port = NULL;
    adev->hw_device.set_audio_port_config = NULL;


    pthread_mutex_lock(&adev->lock);

    /*
     * Initializes Audio Proxy.
     * Audio Proxy is handling ALSA & Audio Routes to support SoC dependency.
     */
    adev->proxy = proxy_init();
    if (!adev->proxy) {
        ALOGE("device-%s: failed to init Audio Proxy", __func__);
        goto err_open;
    }

    /* Initializes Audio Route. */
    if (adev_init_route(adev) == false) {
        ALOGE("device-%s: failed to init Audio Route", __func__);
        goto err_open;
    }
    adev->is_route_created = true;

    adev->actual_playback_device = AUDIO_DEVICE_OUT_SPEAKER;
    adev->actual_capture_device  = AUDIO_DEVICE_IN_BUILTIN_MIC;
    adev->previous_playback_device = AUDIO_DEVICE_NONE;

    adev->is_playback_path_routed  = false;
    adev->active_playback_ausage   = AUSAGE_NONE;
    adev->active_playback_device   = DEVICE_NONE;
    adev->active_playback_modifier = MODIFIER_NONE;

    adev->is_capture_path_routed  = false;
    adev->active_capture_ausage   = AUSAGE_NONE;
    adev->active_capture_device   = DEVICE_NONE;
    adev->active_capture_modifier = MODIFIER_NONE;

    /* Initialize Audio Stream Lists */
    list_init(&adev->playback_list);
    list_init(&adev->capture_list);

    /* Sets initial values. */
    adev->primary_output = NULL;
    adev->active_input = NULL;
    adev->fast_output = NULL;

    adev->amode = AUDIO_MODE_NORMAL;
    adev->previous_amode = AUDIO_MODE_NORMAL;
    adev->call_mode = CALL_OFF;
    adev->incallmusic_on = false;

    adev->voipse_on = false;

    adev->voice_volume = 0;
    adev->mic_mute = false;

    adev->fm_state = FM_OFF;

   /*
     * Initializes Factory Manager.
     * Factory Manager is handling factory mode test scenario.
     */
    adev->factory = factory_init();
    if (!adev->factory)
        ALOGE("device-%s: failed to init Factory Manager!", __func__);
    else
        ALOGD("device-%s: initialized Factory Manager!", __func__);

    /* Customer Specific initial values */
    adev->support_reciever = false;
    adev->support_backmic = false;
    adev->support_thirdmic = false;
    adev->fm_via_a2dp= false;
    adev->fm_radio_mute = false;

    adev->pcmread_latency = 0;
    adev->update_offload_volume = false;
    adev->update_mmap_volume = false;
    adev->current_devices = AUDIO_DEVICE_NONE;

    pthread_mutex_unlock(&adev->lock);

    /* Sets Structure audio_hw_device for return. */
    *device = &adev->hw_device.common;
    adev_ref_count++;
    ALOGI("device-%s: opened Primary Audio HW Device(ref = %d)", __func__, adev_ref_count);
    pthread_mutex_unlock(&adev_init_lock);
    return 0;

err_open:
    if (adev->proxy)
        proxy_deinit(adev->proxy);

    pthread_mutex_unlock(&adev->lock);
    pthread_mutex_destroy(&adev->lock);

    free(adev);
    *device = NULL;

    ALOGI("device-%s: failed to open Primary Audio HW Device", __func__);
    pthread_mutex_unlock(&adev_init_lock);
    return -ENOSYS;
}

/* Entry Point for AudioHAL (Primary Audio HW Module for Android) */
static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_CURRENT,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Exynos Primary AudioHAL",
        .author = "Samsung",
        .methods = &hal_module_methods,
    },
};
