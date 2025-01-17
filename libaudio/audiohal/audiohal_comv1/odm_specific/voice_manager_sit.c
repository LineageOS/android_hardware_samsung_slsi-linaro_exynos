/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "voice_manager"
#define LOG_NDEBUG 0

#include <stdlib.h>
#include <errno.h>

#include <log/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>
#include <pthread.h>

#include "audio_hw.h"
#include "voice_manager.h"
#include "sitril_interface.h"

#include "audio_proxy_interface.h"

#define VOLUME_STEPS_DEFAULT  "5"
#define VOLUME_STEPS_PROPERTY "ro.vendor.config.vc_call_vol_steps"

#define DEVICE_INVALID           -1

/*
 * Local Functions
 */
static int voice_set_sco_solution(struct voice_manager *voice, bool echo_cancel, int sample_rate)
{
    int ret = 0;
    if (voice) {
        SitRilSetScoSolution(echo_cancel, sample_rate);
    }
    return ret;
}

static int voice_set_volte_status(struct voice_manager *voice, int status)
{
    int ret = 0;
    if (voice) {
        SitRilSetVoLTEState(status);
    }
    return ret;
}

static int voice_set_band_rot_route(struct audio_device *adev, const void *data)
{
    int ret = 0;
    struct voice_manager *voice = adev->voice;
    uint32_t band = ((const int *)data)[0];  // Narrow Band 0 / Wide Band 1 / Super WB 3 / Full Band 4
    uint32_t rot = ((const int *)data)[1];  // GSM voice call 1 / CDMA Voice call 2 / IMS call case 4 / Etc 8

    ALOGI("vm-%s: BAND: %d, ROT: %d", __func__, band, rot);

    proxy_set_call_path_param(CALL_PATH_NONE, CALL_BAND, band);
    proxy_set_call_path_param(CALL_PATH_SET, CALL_ROT, rot);

    voice->voice_samplingrate = band;

    return ret;
}

static int onCallbackFromRILAudio(void *handle, int event, const void *data, unsigned int datalen)
{
    struct audio_device *adev = (struct audio_device *)handle;

    switch (event) {
        case VOICE_CALL_AUDIO_EVENT_RINGBACK_STATE_CHANGED:
            ALOGI("vm-%s: REMOTE_RINGBACK_STATE_CHANGED event!", __func__);
            if (datalen > 0)
                ALOGI("vm-%s: remote ringback tone staus: %d", __func__, *(const int *)data);
            break;

        case VOICE_CALL_AUDIO_EVENT_IMS_SRVCC_HANDOVER:
            ALOGI("vm-%s: On IMS_SRVCC_HANDOVER event!", __func__);
            break;

        case VOICE_CALL_AUDIO_EVENT_WB_AMR_REPORT:  // report Band and Rot info
            ALOGI("vm-%s: VOICE_AUDIO_EVENT_WB_AMR_REPORT event!", __func__);
            voice_set_band_rot_route(adev, data);
            break;

        default:
            ALOGI("vm-%s: On Unsupported event (%d)!", __func__, event);
            break;
    }

    return 0;
}

/*
 * Status Check Functions
 */
bool voice_is_call_mode(struct voice_manager *voice)
{
    // True means Android Audio Mode is IN_CALL Mode
    return (voice->call_status >= CALL_STATUS_INCALLMODE);
}

bool voice_is_call_active(struct voice_manager *voice)
{
    // True means Voice Call is working (Audio PCM for CP Call is Opened)
    return (voice->call_status == CALL_STATUS_ACTIVE);
}


/*
 * Set Functions
 */
int voice_set_call_mode(struct voice_manager *voice, bool on)
{
    int ret = 0;

    if (voice->call_status == CALL_STATUS_INVALID && on) {
        // RIL Audio Client is not connected yet, Re-Try!!!
        ALOGD("vm-%s: RilClient is not opened yet! Retry!", __func__);
        ret = SitRilOpen();
        if (ret == 0)
            voice->call_status = CALL_STATUS_CONNECTED;
        else
            voice->call_status = CALL_STATUS_INVALID;
    }

    if (voice->call_status == CALL_STATUS_CONNECTED && on)
        voice->call_status = CALL_STATUS_INCALLMODE;
    else if (voice->call_status == CALL_STATUS_INCALLMODE && !on)
        voice->call_status = CALL_STATUS_CONNECTED;
    else
        ALOGE("vm-%s: Invalid Voice Call Status(%d) with %d", __func__, voice->call_status, on);

    return ret;
}

int voice_set_call_active(struct voice_manager *voice, bool on)
{
    int ret = 0;

    if (voice->call_status == CALL_STATUS_INCALLMODE && on) {
        voice->call_status = CALL_STATUS_ACTIVE;
        SitRilSetSoundClkMode(1);
    } else if (voice->call_status == CALL_STATUS_ACTIVE && !on) {
        voice->call_status = CALL_STATUS_INCALLMODE;
        SitRilSetSoundClkMode(0);
    } else
        ALOGE("vm-%s: Invalid Voice Call Status(%d) with %d", __func__, voice->call_status, on);

    return ret;
}

int voice_set_audio_mode(struct voice_manager *voice, int mode, bool status)
{
    int ret = 0;
    if (voice) {
        SitRilSetAudioMode(mode, status);
    }
    return ret;
}

int voice_set_volume(struct voice_manager *voice, float volume)
{
    int ret = 0;

    if (voice->call_status == CALL_STATUS_ACTIVE) {
        SitRilSetVoiceVolume(voice->out_device, (int)(volume * voice->volume_steps_max), volume);
        ALOGD("vm-%s: Volume = %d(%f)!", __func__, (int)(volume * voice->volume_steps_max), volume);
    } else {
        ALOGE("vm-%s: Voice is not Active", __func__);
        ret = -1;
    }

    return ret;
}

int voice_set_extra_volume(struct voice_manager *voice, bool on)
{
    int ret = 0;
    if (voice) {
        SitRilSetExtraVolume(on);
    }
    return ret;
}

int voice_set_path(struct voice_manager *voice, audio_devices_t devices)
{
    int ret = 0;
    int mode = AUDIO_MODE_IN_CALL;

    if (voice_is_call_mode(voice)) {
        voice->out_device = devices;
        SitRilSetVoicePath(mode, devices);
    } else {
        ALOGE("%s: Voice is not created", __func__);
        ret = -1;
    }

    return ret;
}

int voice_set_mic_mute(struct voice_manager *voice, bool status)
{
    int ret = 0;
    if (voice_is_call_active(voice) || (voice->loopback_mode != FACTORY_LOOPBACK_OFF)) {
        SitRilSetTxMute(status);
    }
    ALOGD("vm-%s: MIC Mute = %d!", __func__, status);
    return ret;
}

int voice_set_rx_mute(struct voice_manager *voice, bool status)
{
    int ret = 0;
    if (voice) {
        SitRilSetRxMute(status);
    }
    return ret;
}

int voice_set_usb_mic(struct voice_manager *voice, bool status)
{
    int ret = 0;
    if (voice) {
        SitRilSetUSBMicState(status);
    }
    return ret;
}

void voice_set_call_forwarding(struct voice_manager *voice, bool callfwd)
{
    if (voice) {
        SitRilSetCallFowardingMode(callfwd);
    }
    return;
}

void voice_set_cur_indevice_id(struct voice_manager *voice, int device)
{
    voice->in_device_id = device;
    return;
}

void voice_set_parameters(struct audio_device *adev, struct str_parms *parms)
{
    struct voice_manager *voice = adev->voice;
    char value[40];
    int ret = 0;

    char *kv_pairs = str_parms_to_str(parms);

    //ALOGV_IF(kv_pairs != NULL, "%s: enter: %s", __func__, kv_pairs);

    // VoLTE Status Configuration
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_VOLTE_STATUS, value, sizeof(value));
    if (ret >= 0) {
        if (!strcmp(value, "voice")) {
            ALOGI("vm-%s: VoLTE Voice Call is On", __func__);
        } else if (!strcmp(value, "end")) {
            ALOGI("vm-%s: VoLTE Voice Call is Off", __func__);
        } else {
            ALOGE("vm-%s: Unknown VoLTE parameters = %s!!", __func__, value);
        }

        voice_set_volte_status(voice, voice->volte_status);
    }

    // BT SCO NREC Configuration
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_NREC, value, sizeof(value));
    if (ret >= 0) {
        if (!strcmp(value, AUDIO_PARAMETER_VALUE_ON)) {
            voice->bluetooth_nrec = BT_NREC_ON;
            ALOGI("vm-%s: BT NREC is On", __func__);
        } else if (!strcmp(value, AUDIO_PARAMETER_VALUE_OFF)) {
            voice->bluetooth_nrec = BT_NREC_OFF;
            ALOGI("vm-%s: BT NREC is Off", __func__);
        }
        proxy_set_call_path_param(CALL_PATH_SET, BT_NREC, voice->bluetooth_nrec);

        str_parms_del(parms, AUDIO_PARAMETER_KEY_BT_NREC);
    }

    // BT SCO WideBand Configuration
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BT_SCO_WB, value, sizeof(value));
    if (ret >= 0) {
        if (!strcmp(value, AUDIO_PARAMETER_VALUE_ON)) {
            ALOGI("vm-%s: set to WB for BT-SCO", __func__);
            voice->bluetooth_samplerate = WB_SAMPLING_RATE;
        } else if (!strcmp(value, AUDIO_PARAMETER_VALUE_OFF)) {
            ALOGI("vm-%s: set to NB for BT-SCO", __func__);
            voice->bluetooth_samplerate = NB_SAMPLING_RATE;
        }

        uint32_t device = 0;
        if (adev->primary_output)
            device = adev->primary_output->common.requested_devices & AUDIO_DEVICE_OUT_ALL_SCO;

        voice_set_sco_solution(voice, voice->bluetooth_nrec, voice->bluetooth_samplerate);

        if(voice_is_call_active(voice) && (device != 0)) {
            voice_set_path(voice, adev->primary_output->common.requested_devices);
        }

        str_parms_del(parms, AUDIO_PARAMETER_KEY_BT_SCO_WB);
    }

    // TTY Status Configuration
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_TTY_MODE, value, sizeof(value));
    if (ret >= 0) {
        if (!strcmp(value, AUDIO_PARAMETER_VALUE_TTY_OFF)) {
            voice->tty_mode = TTY_MODE_OFF;
            ALOGD("vm-%s: TTY_MODE_OFF", __func__);
        } else if (!strcmp(value, AUDIO_PARAMETER_VALUE_TTY_VCO)) {
            voice->tty_mode = TTY_MODE_VCO;
            ALOGD("vm-%s: TTY_MODE_VCO", __func__);
        } if (!strcmp(value, AUDIO_PARAMETER_VALUE_TTY_HCO)) {
            voice->tty_mode = TTY_MODE_HCO;
            ALOGD("vm-%s: TTY_MODE_HCO", __func__);
        } if (!strcmp(value, AUDIO_PARAMETER_VALUE_TTY_FULL)) {
            voice->tty_mode = TTY_MODE_FULL;
            ALOGD("vm-%s: TTY_MODE_FULL", __func__);
        } else
            ALOGD("vm-%s: Unknown TTY_MODE parameters = %s!!", __func__, value);

        str_parms_del(parms, AUDIO_PARAMETER_KEY_TTY_MODE);
    }

    // HAC(Hearing Aid Compatibility) Status Configuration
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_HAC, value, sizeof(value));
    if (ret >= 0) {
        if (!strcmp(value, AUDIO_PARAMETER_VALUE_HAC_ON)) {
            voice->hac_mode = HAC_MODE_ON;
            ALOGD("vm-%s: HAC_MODE_ON", __func__);
        } else if (!strcmp(value, AUDIO_PARAMETER_VALUE_HAC_OFF)) {
            voice->hac_mode = HAC_MODE_OFF;
            ALOGD("vm-%s: HAC_MODE_OFF", __func__);
        } else
            ALOGD("vm-%s: Unknown HAC_MODE parameters = %s!!", __func__, value);

        str_parms_del(parms, AUDIO_PARAMETER_KEY_HAC);
    }

    //ALOGV("%s: exit with code(%d)", __func__, ret);

    free(kv_pairs);
    return ;
}

int voice_set_callback(struct voice_manager * voice __unused, void * callback_func )
{
    int ret = 0;

    SitRilRegisterCallback(0, callback_func);
    return ret;
}

int voice_set_callback_with_handle(struct audio_device *adev, void * callback_func)
{
    int ret = 0;

    if(adev) {
        SitRilRegisterCallbackWithHandle((void*)adev, callback_func);
    }

    return ret;
}

/*
 * Get Functions
 */
volte_status_t voice_get_volte_status(struct voice_manager *voice)
{
    return voice->volte_status;
}

int voice_get_samplingrate(struct voice_manager *voice)
{
    return voice->voice_samplingrate;
}

int voice_get_vowifi_band(struct voice_manager *voice)
{
    return voice->vowifi_band;
}

int voice_get_cur_indevice_id(struct voice_manager *voice)
{
    return voice->in_device_id;
}


/*
 * Other Functions
 */
int voice_set_loopback_device(struct voice_manager *voice, int mode, int rx_dev, int tx_dev)
{
    int ret = 0;
    if (voice) {
        voice->loopback_mode = mode;
        if (rx_dev == (AUDIO_DEVICE_OUT_EARPIECE | AUDIO_DEVICE_OUT_SPEAKER)) {
            ALOGI("%s: Set loopback rx path as RCV (Speaker2)", __func__);
            rx_dev = AUDIO_DEVICE_OUT_EARPIECE; // set rcv as speaker2
        }
        SitRilSetLoopback(mode, rx_dev, tx_dev);
    }
    return ret;
}

void voice_ril_dump(int fd __unused)
{
    SitRilDump(fd);
}

int voice_get_volume_index(struct voice_manager *voice, float volume)
{
    return (int)(volume * voice->volume_steps_max);
}

int voice_callback(void * handle, int event, const void *data, unsigned int datalen)
{
    struct voice_manager *voice = (struct voice_manager *)handle;
    int (*funcp)(int, const void *, unsigned int) = NULL;

    ALOGD("vm-%s: Called Callback Function from RIL Audio Client!", __func__);
    if (voice) {
#if 0
        switch (event) {
            case VOICE_AUDIO_EVENT_RINGBACK_STATE_CHANGED:
                ALOGD("vm-%s: Received RINGBACK_STATE_CHANGED event!", __func__);
                break;

            case VOICE_AUDIO_EVENT_IMS_SRVCC_HANDOVER:
                ALOGD("vm-%s: Received IMS_SRVCC_HANDOVER event!", __func__);
                break;

            default:
                ALOGD("vm-%s: Received Unsupported event (%d)!", __func__, event);
                return 0;
        }
#endif
        funcp = voice->callback;
        funcp(event, data, datalen);
    }

    return 0;
}

void voice_deinit(struct voice_manager *voice)
{
    if (voice) {
        /* RIL */
        SitRilClose();
        free(voice);
    }

    return ;
}

struct voice_manager* voice_init(struct audio_device *adev)
{
    struct voice_manager *voice = NULL;
    char property[PROPERTY_VALUE_MAX];
    int ret = 0;

    voice = calloc(1, sizeof(struct voice_manager));
    if (voice) {

        // At initial time, try to connect to AudioRIL
        ret = SitRilOpen();
        if (ret == 0)
            voice->call_status = CALL_STATUS_CONNECTED;
        else
            voice->call_status = CALL_STATUS_INVALID;

        // Variables
        voice->realcall = false;
        voice->csvtcall = false;
        voice->keep_call_mode = false;

        voice->out_device = AUDIO_DEVICE_NONE;
        voice->in_device_id = DEVICE_INVALID;
        voice->bluetooth_nrec = BT_NREC_INITIALIZED;
        voice->bluetooth_samplerate = NB_SAMPLING_RATE;
        voice->tty_mode = TTY_MODE_OFF;
        voice->call_forwarding = false;
        voice->mute_voice = false;
        voice->cur_modem = CP1;
        voice->extra_volume = false;

        voice->voice_samplingrate = VOICE_SR_NB;
        voice->loopback_mode = FACTORY_LOOPBACK_OFF;

        // VoIP
        voice->voip_wificalling = false;
        voice->voip_rx_active = false;
        voice->vowifi_band = WB;

        // VoLTE
        voice->volte_status = VOLTE_OFF;
        voice->previous_volte_status = VOLTE_OFF;

        property_get(VOLUME_STEPS_PROPERTY, property, VOLUME_STEPS_DEFAULT);
        voice->volume_steps_max = atoi(property);
        /* this catches the case where VOLUME_STEPS_PROPERTY does not contain an integer */
        if (voice->volume_steps_max == 0)
            voice->volume_steps_max = atoi(VOLUME_STEPS_DEFAULT);

        voice->callback = NULL;

        voice_set_callback_with_handle(adev, onCallbackFromRILAudio);
    }

    return voice;
}
