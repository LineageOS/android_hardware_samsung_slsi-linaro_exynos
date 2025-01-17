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

#ifndef __AUDIO_COMMON_INTERFACE_H__
#define __AUDIO_COMMON_INTERFACE_H__

/* General Function for Mode Selection */
bool isCPCallMode(struct audio_device *adev);
bool isAPCallMode(struct audio_device *adev);

/* General Function for Mixer Control */
int adev_mixer_ctl_get_number(struct audio_device *adev, char *name);

#endif // __AUDIO_COMMON_INTERFACE_H__
