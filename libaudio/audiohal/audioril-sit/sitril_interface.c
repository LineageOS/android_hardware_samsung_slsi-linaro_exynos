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

#define LOG_TAG "sitril_interface"
#define LOG_NDEBUG 0

#include <dlfcn.h>
#include <stdlib.h>
#include <log/log.h>
#include <unistd.h>

#include "AudioRil.h"
#include "sitril_interface.h"
#include "voice_definition.h"

/* The path of RIL Audio Client Library */
#define RIL_CLIENT_LIBPATH "libsitril-audio.so"

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
    int device_type = RILAUDIO_PATH_NONE;

    switch(devices) {
        case AUDIO_DEVICE_OUT_EARPIECE:
            if (voice->volte_status == VOLTE_OFF) {
                if (voice->hac_mode)
                    device_type = RILAUDIO_PATH_HANDSET_HAC;
                else
                    device_type = RILAUDIO_PATH_HANDSET;
            } else {
                if (voice->hac_mode)
                    device_type = RILAUDIO_PATH_VOLTE_HANDSET_HAC;
                else
                    device_type = RILAUDIO_PATH_VOLTE_HANDSET;
            }
            break;

        case AUDIO_DEVICE_OUT_SPEAKER:
            if (voice->volte_status == VOLTE_OFF)
                device_type = RILAUDIO_PATH_SPEAKRERPHONE;
            else
                device_type = RILAUDIO_PATH_VOLTE_SPEAKRERPHONE;
            break;

        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
            if (voice->volte_status == VOLTE_OFF)
                device_type = RILAUDIO_PATH_HEADSET;
            else
                device_type = RILAUDIO_PATH_VOLTE_HEADSET;
            break;

        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            if (voice->volte_status == VOLTE_OFF)
                device_type = RILAUDIO_PATH_35PI_HEADSET;
            else
                device_type = RILAUDIO_PATH_VOLTE_35PI_HEADSET;
            break;

        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
            if (voice->volte_status == VOLTE_OFF)
                if (voice->btsco_ec == BT_NREC_OFF)
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = RILAUDIO_PATH_BT_NS_EC_OFF;
                    else
                        device_type = RILAUDIO_PATH_WB_BT_NS_EC_OFF;
                else
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = RILAUDIO_PATH_BLUETOOTH;
                    else
                        device_type = RILAUDIO_PATH_WB_BLUETOOTH;
            else
                if (voice->btsco_ec == BT_NREC_OFF)
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = RILAUDIO_PATH_VOLTE_BT_NS_EC_OFF;
                    else
                        device_type = RILAUDIO_PATH_VOLTE_WB_BT_NS_EC_OFF;
                else
                    if (voice->btsco_sr == NB_SAMPLING_RATE)
                        device_type = RILAUDIO_PATH_VOLTE_BLUETOOTH;
                    else
                        device_type = RILAUDIO_PATH_VOLTE_WB_BLUETOOTH;
            break;

        case AUDIO_DEVICE_OUT_LINE:
            if (voice->volte_status == VOLTE_OFF)
                device_type = RILAUDIO_PATH_LINEOUT;
            else
                device_type = RILAUDIO_PATH_VOLTE_LINEOUT;
            break;

        default:
            if (voice->volte_status == VOLTE_OFF)
                device_type = RILAUDIO_PATH_HANDSET;
            else
                device_type = RILAUDIO_PATH_VOLTE_HANDSET;
            break;
    }

    return device_type;
}


/******************************************************************************/
/**                                                                          **/
/** RILClient_Interface Functions                                            **/
/**                                                                          **/
/******************************************************************************/
int SitRilOpen()
{
    struct rilclient_intf *rilc = NULL;
    int ret = 0;

    /* Create SITRIL Audio Client Interface Structure */
    rilc = getInstance();
    if (!rilc) {
        ALOGE("%s: failed to create for RILClient Interface", __func__);
        goto create_err;
    }

    /* Initialize SITRIL Audio Client Interface Structure */
    rilc->handle = NULL;
    rilc->connection = false;
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
     * Open & Connect SITRIL Audio Client Library
     * This library will do real connection with SITRIL
     */
    rilc->handle = dlopen(RIL_CLIENT_LIBPATH, RTLD_NOW);
    if (rilc->handle) {
        rilc->ril_open_client        = (int (*)(void))dlsym(rilc->handle, "RilAudioOpen");
        rilc->ril_close_client       = (int (*)(void))dlsym(rilc->handle, "RilAudioClose");
        rilc->ril_register_callback  = (int (*)(void *, int *))dlsym(rilc->handle, "RegisterEventCallback");
        rilc->ril_set_audio_volume   = (int (*)(int))dlsym(rilc->handle, "SetAudioVolume");
        rilc->ril_set_audio_path     = (int (*)(int))dlsym(rilc->handle, "SetAudioPath");
        rilc->ril_set_multi_mic      = (int (*)(int))dlsym(rilc->handle, "SetMultiMic");
        rilc->ril_set_mute           = (int (*)(int))dlsym(rilc->handle, "SetMute");
        rilc->ril_set_audio_clock    = (int (*)(int))dlsym(rilc->handle, "SetAudioClock");
        rilc->ril_set_audio_loopback = (int (*)(int, int))dlsym(rilc->handle, "SetAudioLoopback");
        rilc->ril_set_tty_mode       = (int (*)(int))dlsym(rilc->handle, "SetTtyMode");

        ALOGD("%s: Succeeded in getting RIL AudioClient Interface!", __func__);
    } else {
        ALOGE("%s: Failed to open RIL AudioClient Interface(%s)!", __func__, RIL_CLIENT_LIBPATH);
        goto libopen_err;
    }

    /* Early open RIL Client */
    if (rilc->ril_open_client) {
        for (int retry = 0; retry < MAX_RETRY; retry++) {
            ret = rilc->ril_open_client();
            if (ret != 0) {
                ALOGE("%s: Failed to open RIL AudioClient! (Try %d)", __func__, retry+1);
                usleep(SLEEP_RETRY);  // 10ms
            } else {
                ALOGD("%s: Succeeded in opening RIL AudioClient!", __func__);
                rilc->connection = true;
                break;
            }
        }

        if (!rilc->connection)
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

int SitRilClose()
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if (rilc) {
        if (rilc->connection && rilc->ril_close_client) {
            ret = rilc->ril_close_client();
            if (ret == 0)
                ALOGD("%s: Closed RIL AudioClient", __func__);
            else
                ALOGE("%s: Failed to close RIL AudioClient!", __func__);

            rilc->connection = false;
        }

        if (rilc->handle) {
            dlclose(rilc->handle);
            rilc->handle = NULL;
        }

        destroyInstance();
    }

    return 0;
}

int SitRilDuosInit()
{
    int ret = 0;

    // Not Supported Dual RIL

    return ret;
}

int SitRilCheckConnection()
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->connection)
            ret = 1;
    }

    return ret;
}

int SitRilSetVoiceSolution()
{
    int ret = 0;

    // Not Supported Voice Solution by CP

    return ret;
}

int SitRilSetExtraVolume(bool on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        // Not Supported Extra Volume
        rilc->extraVolume = on;
    }

    return ret;
}

int SitRilSetWbamr(int on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        // Not Supported WideBand
        rilc->wbAmr = (bool)on;
    }

    return ret;
}

int SitRilsetEmergencyMode(bool on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        // Not Supported Emergency Mode
        rilc->emergencyMode = on;
    }

    return ret;
}

int SitRilSetScoSolution(bool echoCancle, int sampleRate)
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

int SitRilSetVoiceVolume(int device __unused, int volume, float fvolume)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_audio_volume)
            rilc->ril_set_audio_volume(volume);

        ALOGD("%s: Volume = %d(%f)!", __func__, volume, fvolume);
    }

    return ret;
}

int SitRilSetVoicePath(int mode __unused, int device)
{
    struct rilclient_intf *rilc = getInstance();
    int path = RILAUDIO_PATH_NONE;
    int ret = 0;

    if(rilc) {
        path = map_incall_device(rilc, device);
        if (rilc->ril_set_audio_path) {
            ret = rilc->ril_set_audio_path(path);
            ALOGD("%s: set Voice Path to %d!", __func__, path);
        }
    }

    return ret;
}

int SitRilSetAudioMode(int mode, bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->audio_mode = mode;
        rilc->audio_state = state;
    }

    return ret;
}

int SitRilSetTxMute(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_mute) {
            if (state)
                rilc->ril_set_mute(RILAUDIO_MUTE_ENABLED);
            else
                rilc->ril_set_mute(RILAUDIO_MUTE_DISABLED);
        }
        rilc->tx_mute = state;
    }

    return ret;
}

int SitRilSetRxMute(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->rx_mute = state;
    }

    return ret;
}

int SitRilSetDualMic(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->dualmic = state;
    }

    return ret;
}

int SitRilSetLoopback(int loopbackMode __unused, int rx_device __unused, int tx_device __unused)
{
    struct rilclient_intf *rilc = getInstance();
    int onoff = 0;
    int path = RILAUDIO_LOOPBACK_PATH_NA;
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_audio_loopback)
            rilc->ril_set_audio_loopback(onoff, path);
    }

    return ret;
}

int SitRilSetCurrentModem(int curModem)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->current_modem = curModem;
    }

    return ret;
}

int SitRilRegisterCallback(int rilState __unused, int * callback)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_register_callback)
            ret = rilc->ril_register_callback((void *)rilc, callback);
    }

    return ret;
}

int SitRilSetRealCallStatus(bool on)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->realcall = on;
    }

    return ret;
}

int SitRilSetVoLTEState(int state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->volte_status = state;
    }

    return ret;
}
int SitRilSetHacModeState(bool state)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        rilc->hac_mode = state;
    }

    return ret;
}

void SitRilSetCallFowardingMode(bool callFwd)
{
    struct rilclient_intf *rilc = getInstance();

    if(rilc) {
        rilc->call_forward = callFwd;
    }

    return ;
}

void SitRilDump(int fd __unused)
{
    return ;
}

void SitRilCheckMultiSim()
{
    return ;
}

int SitRilSetSoundClkMode(int mode)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_audio_clock) {
            if (mode)
                rilc->ril_set_audio_clock(RILAUDIO_TURN_ON_I2S);
            else
                rilc->ril_set_audio_clock(RILAUDIO_TURN_OFF_I2S);
        }
    }

    return ret;
}

void SitRilSetUSBMicState(bool state)
{
    struct rilclient_intf *rilc = getInstance();

    if(rilc) {
        rilc->usbmic_state = state;
    }

    return ;
}

int SitRilSetTTYMode(int ttymode)
{
    struct rilclient_intf *rilc = getInstance();
    int ret = 0;

    if(rilc) {
        if (rilc->ril_set_tty_mode) {
            rilc->ril_set_tty_mode(ttymode);
        }
    }

    return ret;
}
