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

#ifndef __SECRIL_CLIENT_H__
#define __SECRIL_CLIENT_H__

typedef enum _SoundType {
    SOUND_TYPE_NONE,
    SOUND_TYPE_VOICE,
    SOUND_TYPE_SPEAKER,
    SOUND_TYPE_HEADSET,
    SOUND_TYPE_BTVOICE
} SoundType;

typedef enum _AudioPath {
    SOUND_AUDIO_PATH_NONE = 0,
    SOUND_AUDIO_PATH_HANDSET = 1,
    SOUND_AUDIO_PATH_HEADSET = 2,
    SOUND_AUDIO_PATH_HANDSFREE = 3,
    SOUND_AUDIO_PATH_BLUETOOTH = 4,
    SOUND_AUDIO_PATH_STEREO_BLUETOOTH = 5,
    SOUND_AUDIO_PATH_SPEAKRERPHONE = 6,
    SOUND_AUDIO_PATH_35PI_HEADSET = 7,
    SOUND_AUDIO_PATH_BT_NS_EC_OFF = 8,
    SOUND_AUDIO_PATH_WB_BLUETOOTH = 9,
    SOUND_AUDIO_PATH_WB_BT_NS_EC_OFF = 10,
    SOUND_AUDIO_PATH_HANDSET_HAC = 11,
    SOUND_AUDIO_PATH_LINEOUT = 12,
    SOUND_AUDIO_PATH_VOLTE_HANDSET = 65,
    SOUND_AUDIO_PATH_VOLTE_HEADSET = 66,
    SOUND_AUDIO_PATH_VOLTE_HFK = 67,
    SOUND_AUDIO_PATH_VOLTE_BLUETOOTH = 68,
    SOUND_AUDIO_PATH_VOLTE_STEREO_BLUETOOTH = 69,
    SOUND_AUDIO_PATH_VOLTE_SPEAKRERPHONE = 70,
    SOUND_AUDIO_PATH_VOLTE_35PI_HEADSET = 71,
    SOUND_AUDIO_PATH_VOLTE_BT_NS_EC_OFF = 72,
    SOUND_AUDIO_PATH_VOLTE_WB_BLUETOOTH = 73,
    SOUND_AUDIO_PATH_VOLTE_WB_BT_NS_EC_OFF = 74,
    SOUND_AUDIO_PATH_VOLTE_HANDSET_HAC = 75,
    SOUND_AUDIO_PATH_VOLTE_LINEOUT = 76,
    SOUND_AUDIO_PATH_MAX
} AudioPath;

/* Voice Audio Multi-MIC */
enum ril_audio_multimic {
    VOICE_MULTI_MIC_OFF,
    VOICE_MULTI_MIC_ON,
};

typedef enum _ExtraVolume {
    ORIGINAL_PATH,
    EXTRA_VOLUME_PATH,
    EMERGENCY_PATH
} ExtraVolume;

typedef enum _SoundClockCondition {
    SOUND_CLOCK_STOP,
    SOUND_CLOCK_START
} SoundClockCondition;

typedef enum _MuteCondition {
      TX_UNMUTE, /* 0x00: TX UnMute */
      TX_MUTE,   /* 0x01: TX Mute */
      RX_UNMUTE, /* 0x02: RX UnMute */
      RX_MUTE,   /* 0x03: RX Mute */
      RXTX_UNMUTE, /* 0x04: RXTX UnMute */
      RXTX_MUTE,   /* 0x05: RXTX Mute */
} MuteCondition;

enum ril_audio_clockmode {
    VOICE_AUDIO_TURN_OFF_I2S,
    VOICE_AUDIO_TURN_ON_I2S
};

/* Voice Call Mode */
enum voice_call_mode {
    VOICE_CALL_NONE = 0,
    VOICE_CALL_CS,              // CS(Circit Switched) Call
    VOICE_CALL_PS,              // PS(Packet Switched) Call
    VOICE_CALL_MAX,
};

/* Event from RIL Audio Client */
#define VOICE_AUDIO_EVENT_BASE                     10000
#define VOICE_AUDIO_EVENT_RINGBACK_STATE_CHANGED   (VOICE_AUDIO_EVENT_BASE + 1)
#define VOICE_AUDIO_EVENT_IMS_SRVCC_HANDOVER       (VOICE_AUDIO_EVENT_BASE + 2)

#endif // __SECRIL_CLIENT_H__
