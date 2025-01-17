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

#ifndef __EXYNOS_AUDIOHAL_FACTORY_H__
#define __EXYNOS_AUDIOHAL_FACTORY_H__

#include <system/audio.h>
#include <hardware/audio.h>

#include "audio_hw.h"

typedef enum {
    FACTORY_MODE_NONE,
    FACTORY_MODE_FORCE_ROUTE,
    FACTORY_MODE_LOOPBACK,
    FACTORY_MODE_RMS,
} factory_mode_status;

typedef enum factory_loopback_types {
    FACTORY_LOOPBACK_OFF = 0,
    FACTORY_LOOPBACK_PCM = 1,
    FACTORY_LOOPBACK_PACKET = 2,
    FACTORY_LOOPBACK_PCM_NODELAY = 3,
    FACTORY_LOOPBACK_PACKET_NODELAY = 4,
    FACTORY_LOOPBACK_CODEC = 5,
    FACTORY_LOOPBACK_REALTIME = 6
} loopback_type;

typedef enum {
    FACTORY_SET_PARAM_OK            = 0,
    FACTORY_TEST_OFF,
    FACTORY_LOOPBACK_TEST,
    FACTORY_FORCEROUTE_TEST,
    FACTORY_RMS_TEST
} factory_set_param_return;

struct factory_manager {
    factory_mode_status mode;
    loopback_type loopback_mode;
    audio_devices_t out_device;
    audio_devices_t in_device;
    bool rms_test_enable;
    bool is_calibration_test;
    bool is_dspk_changed;
};

bool is_factory_mode(struct factory_manager *factory);
bool is_factory_loopback_mode(struct factory_manager *factory);
bool is_factory_rms_mode(struct factory_manager *factory);
bool is_factory_bt_realtime_loopback_mode(struct factory_manager *factory);

void factory_set_parameters(struct audio_device *adev, struct str_parms *parms);
void factory_deinit(struct factory_manager *factory);
struct factory_manager * factory_init(void);

#endif  // __EXYNOS_AUDIOHAL_FACTORY_H__
