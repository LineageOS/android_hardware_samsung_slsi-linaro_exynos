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

#define LOG_TAG "audio_hw_primary_factory"
//#define LOG_NDEBUG 0

#include <stdlib.h>
#include <errno.h>

#include <log/log.h>
#include <cutils/str_parms.h>

#include "factory_manager.h"
#include "audio_definition.h"
#include "audio_proxy_interface.h"

/******************************************************************************/
/**                                                                          **/
/** The Factory Functions                                                    **/
/**                                                                          **/
/******************************************************************************/
bool is_factory_mode(struct factory_manager *factory)
{
    if (factory && factory->mode != FACTORY_MODE_NONE)
        return true;
    else
        return false;
}

bool is_factory_loopback_mode(struct factory_manager *factory)
{
    if (factory && factory->mode == FACTORY_MODE_LOOPBACK)
        return true;
    else
        return false;
}

bool is_factory_bt_realtime_loopback_mode(struct factory_manager *factory)
{
    if (factory && (factory->loopback_mode == FACTORY_LOOPBACK_REALTIME)
        && (factory->out_device == AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET)
        && (factory->in_device == AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET)) {
        return true;
    }
    return false;
}

bool is_factory_rms_mode(struct factory_manager *factory)
{
    if (factory && factory->mode == FACTORY_MODE_RMS)
        return true;
    else
        return false;
}


static void factory_set_path_for_loopback(struct audio_device *adev, struct factory_manager *factory, const char *value)
{
    ALOGI("%s: enter: loopback = %s ", __func__, value);
    if (strncmp(value, "ear_ear", 7) == 0) {
        if (audio_is_usb_in_device(adev->actual_capture_device)) {
            factory->out_device = (AUDIO_DEVICE_OUT_USB_HEADSET);
            factory->in_device = (AUDIO_DEVICE_IN_USB_HEADSET);
        } else {
            factory->out_device = (AUDIO_DEVICE_OUT_WIRED_HEADSET);
            factory->in_device = (AUDIO_DEVICE_IN_WIRED_HEADSET);
        }
    } else if (strncmp(value, "mic1_spk", 8) == 0) {
        factory->out_device = (AUDIO_DEVICE_OUT_SPEAKER);
        factory->in_device = (AUDIO_DEVICE_IN_BUILTIN_MIC);
    } else if (strncmp(value, "mic2_spk", 8) == 0) {
        factory->out_device = (AUDIO_DEVICE_OUT_SPEAKER);
        factory->in_device = (AUDIO_DEVICE_IN_BACK_MIC);
    } else if (strncmp(value, "mic_rcv", 7) == 0) {
        factory->out_device = (AUDIO_DEVICE_OUT_EARPIECE);
        factory->in_device = (AUDIO_DEVICE_IN_BUILTIN_MIC);
    } else if (strncmp(value, "mic_ear", 7) == 0) {
        if (audio_is_usb_out_device(adev->actual_playback_device)) {
            factory->out_device = (AUDIO_DEVICE_OUT_USB_HEADSET);
            factory->in_device = (AUDIO_DEVICE_IN_BUILTIN_MIC);
        } else {
            factory->out_device = (AUDIO_DEVICE_OUT_WIRED_HEADSET);
            factory->in_device = (AUDIO_DEVICE_IN_BUILTIN_MIC);
        }
    } else if (strncmp(value, "mic2_ear", 8) == 0) {
        if (audio_is_usb_out_device(adev->actual_playback_device)) {
            factory->out_device = (AUDIO_DEVICE_OUT_USB_HEADSET);
            factory->in_device = (AUDIO_DEVICE_IN_BACK_MIC);
        } else {
            factory->out_device = (AUDIO_DEVICE_OUT_WIRED_HEADSET);
            factory->in_device = (AUDIO_DEVICE_IN_BACK_MIC);
        }
    } else if (strncmp(value, "bt_bt", 5) == 0) {
        factory->out_device = (AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET);
        factory->in_device = (AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET);
    } else {
        ALOGW("factory test path doesn't exist. default set earpiece.");
        factory->out_device = (AUDIO_DEVICE_OUT_EARPIECE);
        factory->in_device = (AUDIO_DEVICE_IN_BUILTIN_MIC);
    }

    return;
}

static void factory_set_path_for_force_route(struct factory_manager *factory, const char *value)
{

    if (!factory->rms_test_enable)
        factory->mode = FACTORY_MODE_FORCE_ROUTE;

    ALOGD("%s: enter: factory_test_route = %s", __func__, value);
    if (!strcmp(value, "spk")) {
        factory->out_device = (AUDIO_DEVICE_OUT_SPEAKER & AUDIO_DEVICE_OUT_ALL);
    } else if (!strcmp(value, "rcv")) {
        factory->out_device = (AUDIO_DEVICE_OUT_EARPIECE & AUDIO_DEVICE_OUT_ALL);
    } else if (!strcmp(value, "ear")) {
        factory->out_device = (AUDIO_DEVICE_OUT_WIRED_HEADSET & AUDIO_DEVICE_OUT_ALL);
    } else if (!strcmp(value, "off")) {
        factory->mode = FACTORY_MODE_NONE;
        factory->out_device = (AUDIO_DEVICE_NONE);
    } else {
        ALOGD("%s: invalid value (%s)", __func__, value);
    }

    return;
}

static void factory_set_path_for_rms(struct factory_manager *factory, const char *value)
{
    ALOGD("%s: enter: factory_test_mic_check = %s", __func__, value);
    if (strcmp(value, "on") == 0) {
        factory->mode = FACTORY_MODE_RMS;
        factory->rms_test_enable = true;
    } else if (strcmp(value, "main") == 0) {
        factory->in_device = (AUDIO_DEVICE_IN_BUILTIN_MIC);
    } else if (strcmp(value, "sub") == 0) {
        factory->in_device = (AUDIO_DEVICE_IN_BACK_MIC);
    } else if (strcmp(value, "spk_mic1") == 0) {    /*at + looptest=0,3,4 */
        factory->in_device = (AUDIO_DEVICE_IN_BUILTIN_MIC);
    } else if (strcmp(value, "off") == 0) {
        factory->mode = FACTORY_MODE_NONE;
        factory->in_device = (AUDIO_DEVICE_NONE);
        factory->rms_test_enable = false;
    } else {
        ALOGD("%s: invalid value (%s)", __func__, value);
    }

    return;
}

void factory_set_parameters(struct audio_device *adev, struct str_parms *parms)
{
    struct factory_manager *factory = adev->factory;

    if (!factory) {
        ALOGW("%s: adev->factory is null", __func__);
        return ;
    }

    char *kv_pairs = str_parms_to_str(parms);
    //ALOGV_IF(kv_pairs != NULL, "%s: enter: %s", __func__, kv_pairs);

    char value[32];
    int err = 0;

    err = str_parms_get_str(parms, AUDIO_PARAMETER_FACTORY_TEST_TYPE, value, sizeof(value));
    if (err >= 0) {
        if (!strcmp(value, "codec")) {
            factory->loopback_mode= FACTORY_LOOPBACK_CODEC;
        } else if (!strcmp(value, "realtime")) {
            factory->loopback_mode = FACTORY_LOOPBACK_REALTIME;
        } else if (!strcmp(value, "pcm")) {
            factory->loopback_mode = FACTORY_LOOPBACK_PCM;
        } else if (!strcmp(value, "packet")) {
            factory->loopback_mode = FACTORY_LOOPBACK_PACKET;
        } else if (!strcmp(value, "packet_nodelay")) {
            factory->loopback_mode = FACTORY_LOOPBACK_PACKET_NODELAY;
        }
        ALOGD("%s: FACTORY_TEST_TYPE=%d", __func__, factory->loopback_mode);
        str_parms_del(parms, AUDIO_PARAMETER_FACTORY_TEST_TYPE);
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_FACTORY_TEST_LOOPBACK, value, sizeof(value));
    if (err >= 0) {
        if (!strcmp(value, AUDIO_PARAMETER_VALUE_ON)) {
            ALOGD("%s: FACTORY_TEST_LOOPBACK=on", __func__);
            factory->mode = FACTORY_MODE_LOOPBACK;
#ifdef SUPPORT_STHAL_INTERFACE
            proxy_call_status(adev->proxy, true);
#endif
        } else if (!strcmp(value, AUDIO_PARAMETER_VALUE_OFF)) {
            ALOGD("%s: FACTORY_TEST_LOOPBACK=off", __func__);
            factory->mode = FACTORY_MODE_NONE;
            factory->out_device = (AUDIO_DEVICE_NONE);
            factory->in_device = (AUDIO_DEVICE_NONE);

            /* 1. pcm device close */
            if (factory->loopback_mode != FACTORY_LOOPBACK_REALTIME) {
                voice_set_loopback_device(adev->voice, FACTORY_LOOPBACK_OFF,
                                          AUDIO_DEVICE_OUT_EARPIECE, AUDIO_DEVICE_IN_BUILTIN_MIC);
                proxy_stop_voice_call(adev->proxy);
            }
            factory->loopback_mode = FACTORY_LOOPBACK_OFF;

            /* 2. reset device */
            if (adev->primary_output != NULL) {
                if (adev->primary_output->common.stream_status != STATUS_STANDBY)
                    adev_set_route((void *)adev->primary_output, AUSAGE_PLAYBACK, ROUTE, CALL_DRIVE);
                else
                    adev_set_route((void *)adev->primary_output, AUSAGE_PLAYBACK, UNROUTE, CALL_DRIVE);
            }
            voice_set_call_mode(adev->voice, false);
#ifdef SUPPORT_STHAL_INTERFACE
            proxy_call_status(adev->proxy, false);
#endif
        }
        str_parms_del(parms, AUDIO_PARAMETER_FACTORY_TEST_LOOPBACK);
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_FACTORY_TEST_PATH, value, sizeof(value));
    if (err > 0) {
        factory_set_path_for_loopback(adev, factory, value);

        /* 1. set ril device */
        if (factory->loopback_mode != FACTORY_LOOPBACK_REALTIME) {
            voice_set_loopback_device(adev->voice, adev->factory->loopback_mode,
                                      adev->factory->out_device, adev->factory->in_device);
        }

        /* 2. pcm device close for reset */
        proxy_stop_voice_call(adev->proxy);

        /* 3. set route */
        if (adev->primary_output != NULL)
            adev_set_route((void *)adev->primary_output, AUSAGE_PLAYBACK, ROUTE, CALL_DRIVE);

        if (factory->loopback_mode != FACTORY_LOOPBACK_REALTIME) {
            /* 4. pcm device open */
            proxy_start_voice_call(adev->proxy);

            /* 5. set volume */
            voice_set_volume(adev->voice, adev->voice_volume);

             /* 6. mic unmute */
            voice_set_mic_mute(adev->voice, false);
        }
        str_parms_del(parms, AUDIO_PARAMETER_FACTORY_TEST_PATH);
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_FACTORY_TEST_ROUTE, value, sizeof(value));
    if (err > 0) {
        factory_set_path_for_force_route(factory, value);
        if (adev->primary_output != NULL) {
            if (adev->primary_output->common.stream_status != STATUS_STANDBY)
                adev_set_route((void *)adev->primary_output, AUSAGE_PLAYBACK, ROUTE, FORCE_ROUTE);
            else
                adev_set_route((void *)adev->primary_output, AUSAGE_PLAYBACK, UNROUTE, FORCE_ROUTE);
        }

        str_parms_del(parms, AUDIO_PARAMETER_FACTORY_TEST_ROUTE);
    }

    err = str_parms_get_str(parms, AUDIO_PARAMETER_KEY_FACTORY_RMS_TEST, value, sizeof(value));
    if (err >= 0) {
        factory_set_path_for_rms(factory, value);
        if (adev->active_input != NULL
            && !(adev->active_input->requested_flags & AUDIO_INPUT_FLAG_HW_HOTWORD)
            && strcmp(value, "on") != 0)
            adev_set_route((void *)adev->active_input, AUSAGE_CAPTURE, ROUTE, NON_FORCE_ROUTE);
        str_parms_del(parms, AUDIO_PARAMETER_KEY_FACTORY_RMS_TEST);
    }

    //ALOGD("%s: exit", __func__);

    free(kv_pairs);
    return ;
}


void factory_deinit(struct factory_manager *factory)
{
    if (factory)
        free(factory);

    return ;
}

struct factory_manager* factory_init()
{
    struct factory_manager *factory = NULL;

    factory = calloc(1, sizeof(struct factory_manager));
    if (factory) {
        factory->mode = FACTORY_MODE_NONE;
        factory->loopback_mode= FACTORY_LOOPBACK_OFF;
        factory->out_device = AUDIO_DEVICE_NONE;
        factory->in_device = AUDIO_DEVICE_NONE;
        factory->rms_test_enable = false;
        factory->is_dspk_changed = false;

        // false : SPK2 (rcv side spk) test, using mixer string "speaker2"
        // true : SPK CAL test, using mixer string "dual-speaker"
        factory->is_calibration_test = false;
    }

    return factory;
}
