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

#ifndef __AUDIOHAL_VOICE_DEFINITION_H__
#define __AUDIOHAL_VOICE_DEFINITION_H__

/**
 ** Customization
 ** If these macros are defined in other header file, please disable them here
 **/

#define CP1 0 // DualCP CP1, DualMode Sim1
#define CP2 1 // DualCP CP2, DualMode Sim2

enum bt_nrec_status {
    BT_NREC_INITIALIZED = -1,
    BT_NREC_OFF = 0,
    BT_NREC_ON = 1,
};

#define TTY_MODE_OFF             0x00000010
#define TTY_MODE_FULL            0x00000020
#define TTY_MODE_VCO             0x00000040
#define TTY_MODE_HCO             0x00000080
#define TTY_MODE_CLEAR           0xFFFFFF0F

#define HAC_MODE_OFF             0x00000100
#define HAC_MODE_ON              0x00000200
#define HAC_MODE_CLEAR           0xFFFFF0FF

#define AUDIO_PARAMETER_KEY_EXTRA_VOLUME                "extraVolume"
#define AUDIO_PARAMETER_KEY_CALL_FORWARDING             "call_forwarding"

#define AUDIO_PARAMETER_VOLTE_STATUS   "VoLTEstate"


#define SWB      2
#define WB       1
#define NB       0
#define SWB_SAMPLING_RATE 32000
#define WB_SAMPLING_RATE  16000
#define NB_SAMPLING_RATE  8000

/* Event from RIL Audio Client */
#define VOICE_CALL_AUDIO_EVENT_BASE                     10000
#define VOICE_CALL_AUDIO_EVENT_RINGBACK_STATE_CHANGED   (VOICE_CALL_AUDIO_EVENT_BASE + 1)
#define VOICE_CALL_AUDIO_EVENT_IMS_SRVCC_HANDOVER       (VOICE_CALL_AUDIO_EVENT_BASE + 2)
#define VOICE_CALL_AUDIO_EVENT_WB_AMR_REPORT            (VOICE_CALL_AUDIO_EVENT_BASE + 3)

#endif  // __AUDIOHAL_VOICE_DEFINITION_H__
