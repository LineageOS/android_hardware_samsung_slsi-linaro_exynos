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

#ifndef __EXYNOS_AUDIOHAL_DEFINITION_H__
#define __EXYNOS_AUDIOHAL_DEFINITION_H__

#include "audio_odm_definition.h"

/*
 * This header file has common definition and parameter for AudioHAL and AudioProxy
 */

/* mixer_paths.xml */
#define MAX_MIXER_LENGTH    256
#define DEFAULT_MIXER_PATH  "/vendor/etc/"
#define DEFAULT_MIXER_FILE  "mixer_paths.xml"
#define MIXER_PATH_INFO     "/proc/device-tree/sound/mixer-paths"

/* Stream Status */
typedef enum {
    STATUS_STANDBY   = 0,   // Stream is opened, but Device(PCM or Compress) is not opened yet.
    STATUS_READY,           // Stream is opened, but Device(PCM or Compress) is not opened yet. But, started something
    STATUS_IDLE,            // Stream is opened & Device(PCM or Compress) is opened.
    STATUS_PLAYING,         // Stream is opened & Device(PCM or Compress) is opened & Device is working.
    STATUS_PAUSED,          // Stream is opened & Device(Compress) is opened & Device is pausing.(only available for Compress Offload Stream)
} stream_status;

/* Call Mode */
typedef enum {
    CALL_OFF   = 0,         // No Call Mode
    VOICE_CALL,             // CP Centric Voice Call Mode
    VOLTE_CALL,             // CP Centric VOLTE Call Mode
    VOWIFI_CALL,            // AP Centric VoWiFi Call Mode

    CALL_MODE_CNT,
    CALL_MODE_MAX   = CALL_MODE_CNT - 1,
} call_mode;

/* Routing Definition */
typedef enum {
    NON_FORCE_ROUTE     = 0,
    FORCE_ROUTE,
    CALL_DRIVE
} force_route;

#define ROUTE               true
#define UNROUTE             false

/* Duration and Rate for Playback and Capture*/
#define PREDEFINED_CAPTURE_DURATION         20  // 20ms
#define PREDEFINED_MEDIA_CAPTURE_DURATION   20  // 20ms
#ifndef PREDEFINED_LOW_CAPTURE_DURATION
#define PREDEFINED_LOW_CAPTURE_DURATION     4   // 4ms
#endif

// USB Playback and Capture
#ifndef PREDEFINED_USB_PLAYBACK_DURATION
#define PREDEFINED_USB_PLAYBACK_DURATION    10  // 10ms
#endif

#ifndef PREDEFINED_USB_CAPTURE_DURATION
#define PREDEFINED_USB_CAPTURE_DURATION     10  // 10ms
#endif

#ifndef PREDEFINED_USB_ASYNC_PLAYBACK_DURATION
#define PREDEFINED_USB_ASYNC_PLAYBACK_DURATION    10  // 10ms
#endif

#ifndef PREDEFINED_USB_ASYNC_CAPTURE_DURATION
#define PREDEFINED_USB_ASYNC_CAPTURE_DURATION     10  // 10ms
#endif

#define LOW_LATENCY_CAPTURE_SAMPLE_RATE     48000

/* FM Radio common definition and parameter */
#define AUDIO_PARAMETER_KEY_FMRADIO_MODE                "fm_mode"
#define AUDIO_PARAMETER_KEY_FMRADIO_VOLUME              "fm_radio_volume"

/* Factory Manager common parameter */
#define AUDIO_PARAMETER_KEY_FACTORY_RMS_TEST            "factory_test_mic_check"

#define AUDIO_PARAMETER_FACTORY_TEST_LOOPBACK           "factory_test_loopback"
#define AUDIO_PARAMETER_FACTORY_TEST_TYPE               "factory_test_type"
#define AUDIO_PARAMETER_FACTORY_TEST_PATH               "factory_test_path"
#define AUDIO_PARAMETER_FACTORY_TEST_ROUTE              "factory_test_route"

/* VTS parameter */
#define AUDIO_PARAMETER_SEAMLESS_VOICE                  "seamless_voice"

/* Direct Stream playback Volume Unit */
#define DIRECT_PLAYBACK_VOLUME_MAX   8192

#endif  // __EXYNOS_AUDIOHAL_DEFINITION_H__
