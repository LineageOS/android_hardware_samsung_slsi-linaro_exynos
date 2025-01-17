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

#ifndef __AUDIO_RIL_H__
#define __AUDIO_RIL_H__

#include "./include/secril-client.h"

// VoLTE Status : same as volte_status_t in VoiceManager
enum {
    VOLTE_OFF = 0,
    VOLTE_VOICE,
    VOLTE_VIDEO
};

struct rilclient_intf {
    /* The pointer of interface library for RIL Client*/
    void *handle;

    /* The SIPC RIL Client Handle */
    /* This will be used as parameter of RIL CLient Functions */
    void *client;

    SoundType sound_type;

    bool extraVolume;
    bool wbAmr;
    bool emergencyMode;
    bool btsco_ec;
    int  btsco_sr;
    int  audio_mode;
    bool audio_state;
    bool tx_mute;
    bool rx_mute;
    bool dualmic;
    int  current_modem;

    bool realcall;
    int  volte_status;
    bool call_forward;

    bool usbmic_state;
    bool hac_mode;

    /* Function pointers */
    void *(*ril_open_client)(void);
    int (*ril_close_client)(void *);
    int (*ril_connect)(void *);
    int (*ril_is_connected)(void *);
    int (*ril_disconnect)(void *);
    int (*ril_set_audio_volume)(void *, SoundType, AudioPath);
    int (*ril_set_audio_path)(void *, AudioPath, ExtraVolume);
    int (*ril_set_multi_mic)(int);
    int (*ril_set_mute)(void *, int);
    int (*ril_set_audio_clock)(void *, int);
    int (*ril_set_tty_mode)(int);
};

#endif /* __AUDIO_RIL_H__ */
