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

#ifndef __SECRIL_INTERFACE_H__
#define __SECRIL_INTERFACE_H__

#include <system/audio.h>

int  SecRilOpen();
int  SecRilClose();
int  SecRilDuosInit();
int  SecRilCheckConnection();
int  SecRilSetVoiceSolution();
int  SecRilSetExtraVolume(bool on);
int  SecRilSetWbamr(int on);
int  SecRilsetEmergencyMode(bool on);
int  SecRilSetScoSolution(bool echoCancle, int sampleRate);
int  SecRilSetVoiceVolume(int device, int volume, float fvolume);
int  SecRilSetVoicePath(int mode, int device);
int  SecRilSetTxMute(bool state);
int  SecRilSetRxMute(bool state);
int  SecRilSetAudioMode(int mode, bool state);
int  SecRilSetDualMic(bool state);
int  SecRilSetLoopback(int loopbackMode, int rx_device, int tx_device);
int  SecRilSetCurrentModem(int curModem);
int  SecRilRegisterCallback(int rilState, int* callback);
int  SecRilSetRealCallStatus(bool on);
int  SecRilSetVoLTEState(int state);
void SecRilSetCallFowardingMode(bool callFwd);
void SecRilDump(int fd __unused);
void SecRilCheckMultiSim();
int  SecRilSetSoundClkMode(int mode);
void SecRilSetUSBMicState(bool state);
int SecRilSetHACMode(bool state);
int SecRilSetTTYmode(int ttymode);

#endif /* __SECRIL_INTERFACE_H__ */
