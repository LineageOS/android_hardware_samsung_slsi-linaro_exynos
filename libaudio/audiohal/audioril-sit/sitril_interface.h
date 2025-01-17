/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef __SITRIL_INTERFACE_H__
#define __SITRIL_INTERFACE_H__


#include <system/audio.h>

int  SitRilOpen();
int  SitRilClose();
int  SitRilDuosInit();
int  SitRilCheckConnection();
int  SitRilSetVoiceSolution();
int  SitRilSetExtraVolume(bool on);
int  SitRilSetWbamr(int on);
int  SitRilsetEmergencyMode(bool on);
int  SitRilSetScoSolution(bool echoCancle, int sampleRate);
int  SitRilSetVoiceVolume(int device, int volume, float fvolume);
int  SitRilSetVoicePath(int mode, int device);
int  SitRilSetTxMute(bool state);
int  SitRilSetRxMute(bool state);
int  SitRilSetAudioMode(int mode, bool state);
int  SitRilSetDualMic(bool state);
int  SitRilSetLoopback(int loopbackMode, int rx_device, int tx_device);
int  SitRilSetCurrentModem(int curModem);
int  SitRilRegisterCallback(int rilState, int* callback);
int  SitRilSetRealCallStatus(bool on);
int  SitRilSetVoLTEState(int state);
void SitRilSetCallFowardingMode(bool callFwd);
void SitRilDump(int fd __unused);
void SitRilCheckMultiSim();
int  SitRilSetSoundClkMode(int mode);
void SitRilSetUSBMicState(bool state);
int SitRilSetHacModeState(bool state);
int SitRilSetTTYMode(int ttymode);

#endif /* __SITRIL_INTERFACE_H__ */
