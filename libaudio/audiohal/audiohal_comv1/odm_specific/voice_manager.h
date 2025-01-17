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

#ifndef __EXYNOS_VOICE_SERVICE_H__
#define __EXYNOS_VOICE_SERVICE_H__

#include <system/audio.h>
#include <hardware/audio.h>

#include "voice_definition.h"

enum {
    VOICE_SR_NB     = 0,            // Narrow Band
    VOICE_SR_WB,                    // Wide Band
    VOICE_SR_SWB                    // Super Wide Band
};

typedef enum {
    VOLTE_OFF = 0,
    VOLTE_VOICE,
    VOLTE_VIDEO
} volte_status_t;

typedef enum {
    CALL_STATUS_INVALID    = 0,     // RIL Audio Client is not connected yet
    CALL_STATUS_CONNECTED,          // RIL Audio Client is connected, but it is not IN_CALL Mode
    CALL_STATUS_INCALLMODE,         // RIL Audio Client is connected and it is IN_CALL Mode, but Voice is not working
    CALL_STATUS_ACTIVE              // RIL Audio Client is connected, it is IN_CALL Mode and Voice is working
} call_status_t;


struct voice_manager {
    // Call Status
    call_status_t call_status;           // Current Call Status
    bool realcall;
    bool csvtcall;
    bool keep_call_mode;

    audio_devices_t out_device;
    int  in_device_id; /* for disable cur path */
    int  bluetooth_nrec;
    int  bluetooth_samplerate;
    int  tty_mode;
    int  hac_mode;
    bool call_forwarding;
    bool mute_voice;
    int  cur_modem;
    bool extra_volume;

    int voice_samplingrate;
    int loopback_mode;

    // VoIP
    int  vowifi_band;
    bool voip_rx_active;
    bool voip_wificalling;

    // VoLTE
    volte_status_t volte_status;
    volte_status_t previous_volte_status;

    int volume_steps_max;       // Voice Volume maximum steps

    int (*callback)(int, const void *, unsigned int); // Callback Function Pointer
};

struct audio_device;

/* Local Functions */
//int  voice_check_ril_connection(struct voice_manager *voice);
//void voice_check_multisim(struct voice_manager *voice);
//int  voice_set_current_modem(struct voice_manager *voice, int cur_modem);
//int  voice_set_wb_amr(struct voice_manager *voice, bool on);
//int  voice_set_sco_solution(struct voice_manager *voice, bool echo_cancel, int sample_rate);
//int  voice_set_dha_solution(struct voice_manager *voice, const char *data);
//int  voice_set_cover_status(struct voice_manager *voice, bool status);
//int  voice_set_volte_status(struct voice_manager *voice, int status);
//int  voice_set_hac_mode(struct voice_manager *voice, bool hac_flag);

/* Status Check Functions */
bool voice_is_call_mode (struct voice_manager *voice);
bool voice_is_call_active (struct voice_manager *voice);
//bool voice_is_in_voip(struct audio_device *adev); // Deprecated

/* Set Functions */
int  voice_set_call_mode(struct voice_manager *voice, bool on);
int  voice_set_call_active (struct voice_manager *voice, bool on);
int  voice_set_audio_mode(struct voice_manager *voice, int mode, bool status);
int  voice_set_volume(struct voice_manager *voice, float volume);
int  voice_set_extra_volume(struct voice_manager *voice, bool on);
int  voice_set_path(struct voice_manager * voice, audio_devices_t devices);
int  voice_set_mic_mute(struct voice_manager *voice, bool status);
int  voice_set_rx_mute(struct voice_manager *voice, bool status);
int  voice_set_usb_mic(struct voice_manager *voice, bool status);
void voice_set_call_forwarding(struct voice_manager *voice, bool callfwd);
void voice_set_cur_indevice_id(struct voice_manager *voice, int device);
void voice_set_parameters(struct audio_device *adev, struct str_parms *parms);

/* Get Functions */
volte_status_t voice_get_volte_status(struct voice_manager *voice);
int  voice_get_samplingrate(struct voice_manager *voice);
int  voice_get_vowifi_band(struct voice_manager *voice);
int  voice_get_cur_indevice_id(struct voice_manager *voice);
bool voice_get_mic_mute(struct voice_manager *voice);
int  voice_get_volume_index(struct voice_manager *voice, float volume);

/* Other Functions */
int voice_set_loopback_device(struct voice_manager *voice, int mode, int rx_dev, int tx_dev);
void voice_ril_dump(int fd __unused);
int  voice_set_callback(struct voice_manager * voice, void * callback_func);
int  voice_set_callback_with_handle(struct audio_device *adev, void * callback_func);

/* Voice Manager related Functiuons */
void voice_deinit(struct voice_manager *voice);
struct voice_manager * voice_init(struct audio_device *adev);

#endif  // __EXYNOS_VOICE_SERVICE_H__
