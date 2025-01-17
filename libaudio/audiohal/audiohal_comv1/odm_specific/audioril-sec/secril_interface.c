/*
 * Copyright (C) 2017 The Android Open Source Project
 * Copyright (C) 2023 The LineageOS Project
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

#define LOG_TAG "secril_interface"
#define LOG_NDEBUG 0

#include <dlfcn.h>
#include <stdlib.h>
#include <log/log.h>
#include <unistd.h>

#include "AudioRil.h"
#include "secril_interface.h"
#include "voice_definition.h"

/* The path of RIL Audio Client Library */
#define RIL_CLIENT_LIBPATH "libsecril-client.so"

/* Retry for RILClient Open */
#define MAX_RETRY   10      // Try 10
#define SLEEP_RETRY 10000   // 10ms sleep

#define CP1 0 // DualCP CP1, DualMode Sim1
#define CP2 1 // DualCP CP2, DualMode Sim2


/******************************************************************************/
/**                                                                          **/
/** RILClient_Interface is Singleton                                         **/
/**                                                                          **/
/******************************************************************************/
static struct rilclient_intf *instance = NULL;

static struct rilclient_intf* getInstance(void)
{
    if (instance == NULL) {
        instance = calloc(1, sizeof(struct rilclient_intf));
        ALOGI("proxy-%s: created RILClient Interface Instance!", __func__);
    }
    return instance;
}

static void destroyInstance(void)
{
    if (instance) {
        free(instance);
        instance = NULL;
        ALOGI("proxy-%s: destroyed RILClient Interface Instance!", __func__);
    }
    return;
}


/******************************************************************************/
/**                                                                          **/
/** Static functions for RILClient_Interface                                 **/
/**                                                                          **/
/******************************************************************************/
static int map_incall_device(struct rilclient_intf *voice, audio_devices_t devices)
{
    int device_type = SOUND_AUDIO_PATH_NONE;

    switch(devices) {
        case AUDIO_DEVICE_OUT_EARPIECE:
            if (voice->volte_status == VOLTE_OFF) {
                if (voice->hac_mode)
                    device_type = SOUND_AUDIO_PATH_HANDSET_HAC;
                else
                    device_type = SOUND_AUDIO_PATH_HANDSET;
            } else {
                if (voice->hac_mode)
                    device_type = SOUND_AUDIO_PATH_VOLTE_HANDSET_HAC;
                else
                    device_type = SOUND_AUDIO_PATH_VOLTE_HANDSET;
            }
            break;

        case AUDIO_DEVICE_OUT_SPEAKER:
            if (voice->volte_status == VOLTE_OFF)
                device_type = SOUND_AUDIO_PATH_SPEAKRERPHONE;
            else
                device_type = SOUND_AUDIO_PATH_VOLTE_SPEAKRERPHONE;
            break;

        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
            if (voice->volte_status == VOLTE_OFF)
                device_type = SOUND_AUDIO_PATH_HEADSET;
            else
                device_type = SOUND_AUDIO_PATH_VOLTE_HEADSET;
            break;

        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            if (voice->volte_status == VOLTE_OFF)
                device_type = SOUND_AUDIO_PATH_35PI_HEADSET;
            else
                device_type = SOUND_AUDIO_PATH_VOLTE_35PI_HEADSET;
            break;

        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
            if (voice->volte_status == VOLTE_OFF)
                if (voice->btsco_ec == BT_NREC_OFF)
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = SOUND_AUDIO_PATH_BT_NS_EC_OFF;
                    else
                        device_type = SOUND_AUDIO_PATH_WB_BT_NS_EC_OFF;
                else
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = SOUND_AUDIO_PATH_BLUETOOTH;
                    else
                        device_type = SOUND_AUDIO_PATH_WB_BLUETOOTH;
            else
                if (voice->btsco_ec == BT_NREC_OFF)
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = SOUND_AUDIO_PATH_VOLTE_BT_NS_EC_OFF;
                    else
                        device_type = SOUND_AUDIO_PATH_VOLTE_WB_BT_NS_EC_OFF;
                else
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = SOUND_AUDIO_PATH_VOLTE_BLUETOOTH;
                    else
                        device_type = SOUND_AUDIO_PATH_VOLTE_WB_BLUETOOTH;
            break;

        case AUDIO_DEVICE_OUT_LINE:
            if (voice->volte_status == VOLTE_OFF)
                device_type = SOUND_AUDIO_PATH_LINEOUT;
            else
                device_type = SOUND_AUDIO_PATH_VOLTE_LINEOUT;
            break;

        default:
            if (voice->volte_status == VOLTE_OFF)
                device_type = SOUND_AUDIO_PATH_HANDSET;
            else
                device_type = SOUND_AUDIO_PATH_VOLTE_HANDSET;
            break;
    }

    return device_type;
}

static SoundType map_sound_from_device(AudioPath path)
{
    SoundType sound_type = SOUND_TYPE_VOICE;

    switch(path) {
    case SOUND_AUDIO_PATH_SPEAKRERPHONE:
        sound_type = SOUND_TYPE_SPEAKER;
        break;
    case SOUND_AUDIO_PATH_HEADSET:
        sound_type = SOUND_TYPE_HEADSET;
        break;
    case SOUND_AUDIO_PATH_HANDSET:
    default:
        sound_type = SOUND_TYPE_VOICE;
        break;
    }

    return sound_type;
}

/******************************************************************************/
/**                                                                          **/
/** RILClient_Interface Functions                                            **/
/**                                                                          **/
/******************************************************************************/
int SecRilOpen()
{
    struct rilclient_intf *rilc = NULL;
    int ret = 0;

    /* Create SecRil Audio Client Interface Structure */
    rilc = getInstance();
    if (!rilc) {
        ALOGE("%s: failed to create for RILClient Interface", __func__);
        goto create_err;
    }

    /* Initialize SecRil Audio Client Interface Structure */
    rilc->handle = NULL;
    rilc->extraVolume = false;
    rilc->wbAmr = false;
    rilc->emergencyMode = false;
    rilc->btsco_ec = false;
    rilc->btsco_sr = 8000;
    rilc->audio_mode = 0;
    rilc->audio_state = false;
    rilc->tx_mute = false;
    rilc->rx_mute = false;
    rilc->dualmic = false;
    rilc->current_modem = CP1;

    rilc->realcall = false;
    rilc->volte_status = VOLTE_OFF;
    rilc->call_forward = false;

    rilc->usbmic_state = false;
    rilc->hac_mode = false;

    /*
     * Open & Connect SecRil Audio Client Library
     * This library will do real connection with SecRil
     */
    rilc->handle = dlopen(RIL_CLIENT_LIBPATH, RTLD_NOW);
    if (rilc->handle) {
        rilc->ril_open_client        = (void *(*)(void))dlsym(rilc->handle, "OpenClient_RILD");
        rilc->ril_close_client       = (int (*)(void *))dlsym(rilc->handle, "CloseClient_RILD");
        rilc->ril_connect            = (int (*)(void *))dlsym(rilc->handle, "Connect_RILD");
        rilc->ril_is_connected       = (int (*)(void *))dlsym(rilc->handle, "isConnected_RILD");
        rilc->ril_disconnect         = (int (*)(void *))dlsym(rilc->handle, "Disconnect_RILD");
        rilc->ril_set_audio_volume   = (int (*)(void *, SoundType, AudioPath))dlsym(rilc->handle, "SetCallVolume");
        rilc->ril_set_audio_path     = (int (*)(void *, AudioPath, ExtraVolume))dlsym(rilc->handle, "SetCallAudioPath");
        rilc->ril_set_multi_mic      = (int (*)(int))dlsym(rilc->handle, "SetTwoMicControl");
        rilc->ril_set_mute           = (int (*)(void *, int))dlsym(rilc->handle, "SetMute");
        rilc->ril_set_audio_clock    = (int (*)(void *, int))dlsym(rilc->handle, "SetCallClockSync");

        ALOGD("%s: Succeeded in getting RIL AudioClient Interface!", __func__);
    } else {
        ALOGE("%s: Failed to open RIL AudioClient Interface(%s)!", __func__, RIL_CLIENT_LIBPATH);
        goto libopen_err;
    }

    /* Early open RIL Client */
    if (rilc->ril_open_client) {
        for (int retry = 0; retry < MAX_RETRY; retry++) {
            rilc->client = rilc->ril_open_client();
            if (!rilc->client) {
                ALOGE("%s: Failed to open RIL AudioClient! (Try %d)", __func__, retry+1);
                usleep(SLEEP_RETRY);  // 10ms
            } else {
                ALOGD("%s: Succeeded in opening RIL AudioClient!", __func__);

                if (rilc->ril_is_connected(rilc->client))
                    ALOGD("%s: RIL Client is already connected with RIL", __func__);
                else if (rilc->ril_connect(rilc->client)) {
                    ALOGE("%s: RIL Client cannot connect with RIL", __func__);
                    rilc->ril_close_client(rilc->client);
                    rilc->client = NULL;
                    continue;
                }

                ALOGD("%s: RIL Client is connected with RIL", __func__);
                break;
            }
        }

        if (!rilc->client || !rilc->ril_is_connected(rilc->client))
            goto open_err;
    } else {
        ALOGE("%s: ril_open_client is not available.", __func__);
        goto open_err;
    }

    return ret;

open_err:
    if (rilc->handle) {
        dlclose(rilc->handle);
        rilc->handle = NULL;
    }

libopen_err:
    if (rilc) {
        destroyInstance();
        rilc = NULL;
    }

create_err:
    return -1;
}

int SecRilClose()
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if (rilc) {
        if (rilc->client && rilc->ril_close_client) {
            ret = rilc->ril_close_client(rilc->client);
            if (ret == 0)
                ALOGD("%s: Closed RIL AudioClient", __func__);
            else
                ALOGE("%s: Failed to close RIL AudioClient!", __func__);
        }

        if (rilc->handle) {
            dlclose(rilc->handle);
            rilc->handle = NULL;
        }

        destroyInstance();
    }

    return 0;
}

int SecRilDuosInit()
{
    int ret = 0;

    // Not Supported Dual RIL

    return ret;
}

int SecRilCheckConnection()
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->client && rilc->ril_is_connected(rilc->client))
            ret = 1;
    }

    return ret;
}

int SecRilSetVoiceSolution()
{
    int ret = 0;

    // Not Supported Voice Solution by CP

    return ret;
}

int SecRilSetExtraVolume(bool on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        // Not Supported Extra Volume
        rilc->extraVolume = on;
    }

    return ret;
}

int SecRilSetWbamr(int on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        // Not Supported WideBand
        rilc->wbAmr = (bool)on;
    }

    return ret;
}

int SecRilsetEmergencyMode(bool on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        // Not Supported Emergency Mode
        rilc->emergencyMode = on;
    }

    return ret;
}

int SecRilSetScoSolution(bool echoCancle, int sampleRate)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        // Not Supported BT SCO Solution by CP
        rilc->btsco_ec = echoCancle;
        rilc->btsco_sr = sampleRate;
    }

    return ret;
}

int SecRilSetVoiceVolume(int device __unused, int volume, float fvolume)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_audio_volume && rilc->client)
            rilc->ril_set_audio_volume(rilc->client, rilc->sound_type, volume);

        ALOGD("%s: Volume = %d(%f)!", __func__, volume, fvolume);
    }

    return ret;
}

int SecRilSetVoicePath(int mode __unused, int device)
{
    struct rilclient_intf *rilc = getInstance();
    int path;
    int ret = 0;

    if(rilc) {
        path = map_incall_device(rilc, device);
        rilc->sound_type = map_sound_from_device(path);
        if (rilc->ril_set_audio_path && rilc->client) {
            ret = rilc->ril_set_audio_path(rilc->client, path, ORIGINAL_PATH);
            ALOGD("%s: set Voice Path to %d!", __func__, path);
        }
    }

    return ret;
}

int SecRilSetAudioMode(int mode, bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->audio_mode = mode;
        rilc->audio_state = state;
    }

    return ret;
}

int SecRilSetTxMute(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_mute && rilc->client) {
            rilc->ril_set_mute(rilc->client, (int)state);
        }
        rilc->tx_mute = state;
    }

    return ret;
}

int SecRilSetRxMute(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->rx_mute = state;
    }

    return ret;
}

int SecRilSetDualMic(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->dualmic = state;
    }

    return ret;
}

int SecRilSetLoopback(int loopbackMode __unused, int rx_device __unused, int tx_device __unused)
{
    return 0;
}

int SecRilSetCurrentModem(int curModem)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->current_modem = curModem;
    }

    return ret;
}

int SecRilRegisterCallback(int rilState __unused, int * __unused callback)
{
    return 0;
}

int SecRilSetRealCallStatus(bool on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->realcall = on;
    }

    return ret;
}

int SecRilSetVoLTEState(int state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->volte_status = state;
    }

    return ret;
}
int SecRilSetHACMode(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->hac_mode = state;
    }

    return ret;
}

void SecRilSetCallFowardingMode(bool callFwd)
{
    struct rilclient_intf *rilc = getInstance();

    if(rilc) {
        rilc->call_forward = callFwd;
    }

    return ;
}

void SecRilDump(int fd __unused)
{
    return ;
}

void SecRilCheckMultiSim()
{
    return ;
}

int SecRilSetSoundClkMode(int mode)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_audio_clock) {
            if (mode)
                rilc->ril_set_audio_clock(rilc->client, VOICE_AUDIO_TURN_ON_I2S);
            else
                rilc->ril_set_audio_clock(rilc->client, VOICE_AUDIO_TURN_OFF_I2S);
        }
    }

    return ret;
}

void SecRilSetUSBMicState(bool state)
{
    struct rilclient_intf *rilc = getInstance();

    if(rilc) {
        rilc->usbmic_state = state;
    }

    return ;
}

int SecRilSetTTYmode(int __unused ttymode)
{
    return 0;
}
