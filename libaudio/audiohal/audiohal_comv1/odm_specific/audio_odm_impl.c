/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "audio_hw_primary_odm"
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
#include "audio_mixers.h"
#include "audio_usages.h"
#include "audio_proxy_interface.h"
#include "audio_odm_interface.h"
#include "audio_common_interface.h"
#include "audio_odm_tables.h"

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
/** The ODM Functions for Offload                                            **/
/**                                                                          **/
/******************************************************************************/
void update_offload_effect(struct audio_device *adev, int type)
{
    // no effect from AudioHAL
}


/******************************************************************************/
/**                                                                          **/
/** The ODM Functions for Customer Feature                                   **/
/**                                                                          **/
/******************************************************************************/


/******************************************************************************/
/**                                                                          **/
/** The ODM Functions for Customer's specific parameters                     **/
/**                                                                          **/
/******************************************************************************/
void odm_out_set_parameters(struct audio_device *adev, struct stream_out *out, struct str_parms *parms)
{
    int val = 0;
    int ret = 0;
    char value[256];

    ret = str_parms_get_int(parms, AUDIO_PARAMETER_VOIP_APP_INFO, &val);
    if (ret >= 0) {
        if (val >= 0) {
            proxy_set_call_path_param(CALL_PATH_NONE, VoIP_APP, val);
            ALOGI("device-%s: VoIP App info: %d", __func__, val);
        }
        str_parms_del(parms, AUDIO_PARAMETER_VOIP_APP_INFO);
    }
}

void odm_adev_set_parameters(struct audio_device *adev, struct str_parms *parms)
{
    struct stream_out *primary_out = adev->primary_output;
    int val = 0;
    int ret = 0;
    char value[256];

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_FMRADIO_MODE, value, sizeof(value));
    if (ret >= 0) {
        ALOGV("device-%s: FM_Mode = %s", __func__, value);
        if(strncmp(value, "on", 2) == 0) {
            ALOGI("device-%s: FM Radio Start", __func__);
        } else {
            if (adev->fm_state == FM_ON || adev->fm_state == FM_RECORDING) adev->fm_state = FM_OFF;
        }
        str_parms_del(parms, AUDIO_PARAMETER_KEY_FMRADIO_MODE);
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_FMRADIO_VOLUME, value, sizeof(value));
    if (ret >= 0) {
        ALOGV("device-%s: FM_Radio_Volume = %s", __func__, value);
        if(strncmp(value, "on", 2) == 0) {
            adev_set_route((void *)primary_out, AUSAGE_PLAYBACK, ROUTE, NON_FORCE_ROUTE);
        } else {
            if (is_active_usage_APCall(adev->proxy) || (adev->voice && voice_is_call_active(adev->voice)))
                ALOGV("device-%s: FM_Radio_Volume = %s, skip unroute path during call", __func__, value);
            else {
                if(adev->primary_output->common.stream_status == STATUS_STANDBY)
                    adev_set_route((void *)primary_out, AUSAGE_PLAYBACK, UNROUTE, NON_FORCE_ROUTE);
            }
        }
        str_parms_del(parms, AUDIO_PARAMETER_KEY_FMRADIO_VOLUME);
    }

    ret = str_parms_get_int(parms, AUDIO_PARAMETER_VOIP_APP_INFO, &val);
    if (ret >= 0) {
        if (val >= 0) {
            proxy_set_call_path_param(CALL_PATH_NONE, VoIP_APP, val);
            ALOGI("device-%s: VoIP App info: %d", __func__, val);
        }
        str_parms_del(parms, AUDIO_PARAMETER_VOIP_APP_INFO);
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_BIG_VOLUME_MODE, value, sizeof(value));
    if (ret >= 0) {
        if (strncmp(value, "on", 2) == 0) {
            ALOGI("device-%s: BIG_Volume_Mode is On", __func__);
            proxy_set_call_path_param(CALL_PATH_SET, EXTRA_VOL, MIXER_VALUE_ON);
        } else {
            ALOGI("device-%s: BIG_Volume_Mode is Off", __func__);
            proxy_set_call_path_param(CALL_PATH_SET, EXTRA_VOL, MIXER_VALUE_OFF);
        }
        str_parms_del(parms, AUDIO_PARAMETER_KEY_BIG_VOLUME_MODE);
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_GAME_VOIP_MODE, value, sizeof(value));
    if (ret >= 0) {
        if (strncmp(value, "on", 2) == 0) {
            ALOGI("device-%s: Game VoIP mode is On", __func__);
            proxy_set_call_path_param(CALL_PATH_SET, VoIP_APP, VOIP_GAME_MODE_INFO);
        } else {
            ALOGI("device-%s: Game VoIP mode is Off", __func__);
            proxy_set_call_path_param(CALL_PATH_SET, VoIP_APP, MIXER_VALUE_OFF);
        }
        str_parms_del(parms, AUDIO_PARAMETER_KEY_BIG_VOLUME_MODE);
    }
}

void odm_adev_get_parameters(struct audio_device *adev, struct str_parms *query)
{
    int ret = 0;
    int beam_value = 0;
    char value[32];

    // Do Nothing get odm adev parameter for ww project, it will add, if it is necessary.
}


/******************************************************************************/
/**                                                                          **/
/** The ODM Functions for FM Radio                                           **/
/**                                                                          **/
/******************************************************************************/
bool isFMRadioOn(struct audio_device *adev)
{
    if (adev->fm_state == FM_ON || adev->fm_state == FM_RECORDING)
	    return true;
    else
	    return false;
}

bool fmradio_need_skip_routing(struct audio_device *adev, struct stream_in *in, audio_usage_type usage_type)
{
    bool ret = false;

    // Do Nothing, in case of LSI FM Radio

    return ret;
}

void fmradio_set_mixer_for_route(struct audio_device *adev, audio_usage new_playback_ausage)
{
    // Do Nothing, in case of LSI FM Radio
}

bool fmradio_check_input_stream(struct audio_device *adev, audio_source_t source, audio_devices_t devices)
{
    if ((source == AUDIO_SOURCE_FM_TUNER) && (devices == AUDIO_DEVICE_IN_FM_TUNER)) {
        ALOGI("device-%s: requested to open FM Radio Recording stream", __func__);
        return true;
    } else {
        return false;
    }
}

int fmradio_open_input_stream(struct audio_device *adev, struct stream_in *in)
{
    int ret = 0;

    ALOGI("device-%s: requested to open FM Radio Tuner stream", __func__);
    adev->fm_state = FM_ON;
    in->common.stream_type  = ASTREAM_CAPTURE_FM_TUNER;
    in->common.stream_usage = AUSAGE_FM_RADIO_TUNER;
    // FM Tuner routing
    adev_set_route((void *)in, AUSAGE_CAPTURE, ROUTE, NON_FORCE_ROUTE);

    return ret;
}
