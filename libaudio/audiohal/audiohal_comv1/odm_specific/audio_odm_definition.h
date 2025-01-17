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

#ifndef __ODM_AUDIOHAL_DEFINITION_H__
#define __ODM_AUDIOHAL_DEFINITION_H__

/*
 * This header file has ODM specific definitions and parameters for AudioHAL and AudioProxy
 */

/* Definition and Parameter for FM Radio */
typedef enum {
    FM_OFF   = 0,
    FM_ON,
    FM_RECORDING,
} fm_state;

/* Definition and Parameter for Call Path Index */
typedef enum {
    VoIP_APP = 0,
    CALL_BAND,
    CALL_ROT,
    RX_DEVICE,
    EXTRA_VOL,
    BT_NREC,
    PARAM_NONE,
} param_item;

typedef enum {
    CALL_PATH_NONE = 0,
    CALL_PATH_SET,
} call_path_state;

// VoIP App Info, WeChat: 0, QQ: 1, Skype: 2, Line: 3, Google Duo: 4
#define AUDIO_PARAMETER_VOIP_APP_INFO "voip_app_state"
#define VOIP_GAME_MODE_INFO           5

/* Below parameter name should be defined by Customer */
#define AUDIO_PARAMETER_KEY_BIG_VOLUME_MODE "big volume"
#define AUDIO_PARAMETER_GAME_VOIP_MODE      "game mode"

#endif  // __ODM_AUDIOHAL_DEFINITION_H__
