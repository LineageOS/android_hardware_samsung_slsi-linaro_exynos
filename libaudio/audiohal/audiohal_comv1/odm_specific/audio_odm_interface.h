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

#ifndef __AUDIO_ODM_INTERFACE_H__
#define __AUDIO_ODM_INTERFACE_H__

/* ODM Function for Customer Feature and Parameter */
void odm_out_set_parameters(struct audio_device *adev, struct stream_out *out, struct str_parms *parms);
void odm_adev_set_parameters(struct audio_device *adev, struct str_parms *parms);
void odm_adev_get_parameters(struct audio_device *adev, struct str_parms *query);

/* FM Radio specific function */
bool isFMRadioOn(struct audio_device *adev);
bool fmradio_need_skip_routing(struct audio_device *adev, struct stream_in *in, audio_usage_type usage_type);
void fmradio_set_mixer_for_route(struct audio_device *adev, audio_usage new_playback_ausage);
bool fmradio_check_input_stream(struct audio_device *adev, audio_source_t source, audio_devices_t devices);
int fmradio_open_input_stream(struct audio_device *adev, struct stream_in *in);

/* ODM Function for Offload */
void update_offload_effect(struct audio_device *adev, int type);

#endif  // __AUDIO_ODM_INTERFACE_H__