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

#ifndef __ODM_AUDIOHAL_TABLE_H__
#define __ODM_AUDIOHAL_TABLE_H__

/*
 * This header includes ODM specific table and variable for AudioHAL and AudioProxy
 */
/* Audio Streams Table for readable log messages */
char * stream_table[ASTREAM_CNT];

/**
 ** Audio Usage Table for readable log messages
 **/
extern char * usage_table[AUSAGE_CNT];

/**
 ** Usage Path(AP/CP to Codec) Configuration based on Audio Usage
 **/
extern char * usage_path_table[AUSAGE_CNT];

/**
 ** Device Path(Codec to Device) Configuration based on Audio Input/Output Device
 **/
extern char * device_table[DEVICE_CNT];

/**
 ** Sampling Rate Modifier Configuration based on Audio Input/Output Device
 **/
extern char * modifier_table[MODIFIER_MAX];

/**
 ** Offload Message Table for readable log messages
 **/
extern char * offload_msg_table[OFFLOAD_MSG_MAX];

/**
 ** Audio log
 **/
extern char * audiomode_table[AUDIO_MODE_CNT];
extern char * callmode_table[CALL_MODE_CNT];
#endif  // __ODM_AUDIOHAL_TABLE_H__