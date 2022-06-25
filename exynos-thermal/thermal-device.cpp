/*
* Copyright (C) 2018 The Android Open Source Project
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

#include <iomanip>
#include "exynos-thermal.h"

/* Generic Device Implemenation */
Device::Device(string &name) : name(name) {}
Device::Device(string &name, string &nodePath) : name(name), nodePath(nodePath) {
	node.open(nodePath);
	if (!node.is_open()) printlog("[ExynosThermal] Device [%s] open failed\n", name.c_str());
	node << unitbuf; // Flush the write buffer every write
}
Device::Device(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : name(name), nodePath(nodePath), leveltoValue(leveltoValue), levelType(levelType) {
	node.open(nodePath);
	if (!node.is_open()) printlog("[ExynosThermal] Device [%s] open failed\n", name.c_str());
	node << unitbuf; // Flush the write buffer every write
}

int Device::getLowestStatus(void) {
	int i = 0;

	for (auto it = request.begin(); it != request.end(); it++) {
		if (it->second > i){
			i = it->second;
		}
	}
	return i;
}

int Device::deviceControl(const string &algoName, int action) {
	lock_guard<mutex> LG(lock);

	request[algoName] = action;
	return 0;
}
void Device::statusApply(void) {
	lock_guard<mutex> LG(lock);
	int i = 0;
	string writeVal;
	int writeInt;

	if (lowestMode == true)
		i = getLowestStatus();

	if (!leveltoValue.empty())
		writeVal = leveltoValue[i];
	else {
		printlog("[ExynosThermal] %s: Level Table is Empty\n", __func__);
		return;
	}
	if (currentStatus != i) {
		node.seekp(0, node.beg);
		if (levelType == "string") {
			node << writeVal;
		}
		else if (levelType == "hex") {
			writeInt = strtoul(writeVal.replace(0, 2, "").c_str(), NULL, 16);
			node << std::hex << writeInt;
		}
		else if (levelType == "integer") {
			writeInt = atoi(writeVal.c_str());
			node << writeInt;
		}

		currentStatus = i;
	}
}

/* Camera Device Implementation */
void CameraDevice::statusApply(void) {
	lock_guard<mutex> LG(lock);
	int i = 0;
	string writeVal;

	if (lowestMode == true)
		i = getLowestStatus();

	if (!leveltoValue.empty())
		writeVal = leveltoValue[i];
	else {
		printlog("[ExynosThermal] %s: Level Table is Empty\n", __func__);
		return;
	}

	if (currentStatus == 0){
		ofstream en("/sys/devices/platform/150c0000.exynos_is/debug/en_fixed_sensor_fps");
		en << unitbuf << "1";
	}
	if (i == 0){
		ofstream en("/sys/devices/platform/150c0000.exynos_is/debug/en_fixed_sensor_fps");
		en << unitbuf << "0";
	}

	if (currentStatus != i) {
		node.seekp(0, node.beg);
		node << writeVal;
		currentStatus = i;
	}
}

/* CPU Hotplug Device Implementation */

/* Backlight Brightness Device Implementation */

/* CPU Freq Device Implementation */
void CPUFreqDevice::statusApply(void) {
	lock_guard<mutex> LG(lock);
	int i = 0;
	int writeInt = 0;

	if (lowestMode == true)
		i = getLowestStatus();
	if (!leveltoValue.empty())
		writeInt = atoi(leveltoValue[i].c_str());
	else {
		printlog("[ExynosThermal] %s: Level Table is Empty\n", __func__);
		return;
	}
	if (currentStatus != i) {
		node.seekp(0, node.beg);
		node.write((const char *)&writeInt, sizeof(writeInt));
		currentStatus = i;
	}
}
/* GPU Freq Device Implemenation */

/* Charger Device Implementation */

/* Report Device Implementation */
void ReportDevice::statusApply(void) {
	lock_guard<mutex> LG(lock);
	time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());

	for (auto it = request.begin(); it != request.end(); it++) {
		if (it->second == 1) {
			node << put_time(localtime(&now), "%F %T") << " : [" << it->first << "] Over Thresholds, currenct Status(" << it->second << ")" << endl;
			it->second = 2;
		}
		else if (it->second == 0) {
			node << put_time(localtime(&now), "%F %T") << " : [" << it->first << "] Under Thresholds Clear" << endl;
			it->second = 2;
		}
	}
}

/* Dummy Device Implementation */
void DummyDevice::statusApply(void) {
	lock_guard<mutex> LG(lock);

	int i = getLowestStatus();
	if (currentStatus != i) {
		currentStatus = i;
	}
}