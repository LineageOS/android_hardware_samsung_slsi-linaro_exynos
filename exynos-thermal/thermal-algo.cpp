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

#include "exynos-thermal.h"

/* Algoritham base class Implementation */
Algo::Algo(string &name, unsigned int samplingTime, int _triggerPoint, int _clearPoint, string &scenName, bool isNegative, Sensor *sensor, ThermalThread *thread) :
name(name), samplingTime(samplingTime), sensor(sensor), thread(thread), isNegative(isNegative) {
	activeStatus = false;
	sensor->insertAlgoItems(this, scenName);

	if (isNegative) {
		triggerPoint = -_triggerPoint;
		clearPoint = -_clearPoint;
	}
	else {
		triggerPoint = _triggerPoint;
		clearPoint = _clearPoint;
	}

}

void Algo::activate(void) {
	activeStatus = true;
	thread->activate();
}

void Algo::deactivate(void) {
	activeStatus = false;
	thread->deactivate();
}

/* Monitor Implemenation */
Monitor::Monitor(string &name, unsigned int samplingTime, int triggerPoint, int clearPoint, string &scenName, bool isNegative, Sensor *sensor, MonitorThread *thread, vector<vector<Device *>> &device, vector<vector<int>> &actionInfo, vector<int> &_thresholds, vector<int> &_clrThresholds) :
Algo(name, samplingTime, triggerPoint, clearPoint, scenName, isNegative, sensor, thread), device(device), actionInfo(actionInfo) {
	if (isNegative) {
		for (int t : _thresholds)
			thresholds.push_back(-t);
		for (int t : _clrThresholds)
			clrThresholds.push_back(-t);
	}
	else {
		thresholds.resize(_thresholds.size());
		std::copy(_thresholds.begin(), _thresholds.end(), thresholds.begin());
		clrThresholds.resize(_clrThresholds.size());
		std::copy(_clrThresholds.begin(), _clrThresholds.end(), clrThresholds.begin());
	}
}
int Monitor::decision(void){
	int temp = 0;
	unsigned int newStatus = 0;

	if (isNegative)
		temp = -(sensor->getTemp());
	else
		temp = sensor->getTemp();

	if (status == 0 || (temp >= thresholds[status] && status < thresholds.size())) {
		// If the temperature is higher than next status, determine new status from thresholds
		for (int t : thresholds) {
			if (t <= temp) {
				newStatus++;
				continue;
			}
			break;
		}
	}
	else if (temp < clrThresholds[status - 1]) {
		// If the status cannot go to upper, determine new status from clrThresholds
		for (int t : clrThresholds) {
			if (t <= temp) {
				newStatus++;
				continue;
			}
			break;
		}
	}
	else
		newStatus = status;

	printlog("[ExynosThermal] Monitor [%s] temp(%d) status(%d -> %d)\n", this->name.c_str(), temp, status, newStatus);

	// If need to apply new status
	if (newStatus != status) {
		if (newStatus == 0)
			deactivate();
		else {
			// Release previous limit
			if (status > 0) {
				for (Device *d : device[status - 1])
					d->deviceControl(name, 0);
			}
			// Request new limit
			if (newStatus > 0) {
				int i = 0;
				for (Device *d : device[newStatus - 1])
					d->deviceControl(name, actionInfo[newStatus - 1][i++]);
			}
			// Apply final status of devicies
			if (status > 0) {
				for (Device *d : device[status - 1])
					d->statusApply();
			}
			if (newStatus > 0) {
				for (Device *d : device[newStatus - 1])
					d->statusApply();
			}
			status = newStatus;
		}
	}

	return 0;
}
void Monitor::deactivate(void) {
	if (status > 0) {
		for (Device *d : device[status - 1])
			d->deviceControl(name, 0);

		for (Device *d : device[status - 1])
			d->statusApply();

		activeStatus = false;
		status = 0;
		thread->deactivate();
	}
}

/* SS Implemenation */
SS::SS(string &name, unsigned int samplingTime, int triggerPoint, int clearPoint, int startLevel, string &scenName, bool isNegative, Sensor* sensor, SSThread *thread, Device *device, int devicePerfFloor = 0, int timeTickTrigger = 0) :
Algo(name, samplingTime, triggerPoint, clearPoint, scenName, isNegative, sensor, thread), device(device), devicePerfFloor(devicePerfFloor), timeTickTrigger(timeTickTrigger), startLevel(startLevel) {}
int SS::decision(void){
	int temp = 0;
	int nextStatus, nextValue, tempDiff, tickTriggered = 0;

	if (isNegative)
		temp = -(sensor->getTemp());
	else
		temp = sensor->getTemp();

	// If temperature is lower then clearPoint, release the thermal restriction.
	if (temp < clearPoint){
		printlog("[ExynosThermal] SS [%s] temp(%d->%d) status(%d -> 0)\n", this->name.c_str(), prevTemperature, temp, status);
		deactivate();
		return 0;
	}

	/*
	 * If status move 0 to upper, calculate tempDiff from triggerPoint.
	 * If not, calculate the temparture different from prevTemperature.
	 */
	if (status == 0)
		tempDiff = (temp / 1000) - (triggerPoint / 1000) + 1;
	else
		tempDiff = (temp / 1000) - (prevTemperature / 1000);

	// If the SS algorithm uses timeTickTrigger, update and apply time tick counter
	if (timeTickTrigger != 0) {
		if (tempDiff == 0 && device->getCurrentStatus() <= devicePerfFloor) {
			// If there has no temperature change, update tickTrigger
			if (++timeTick == timeTickTrigger)
				tickTriggered = 1;
		}
		else timeTick = 0;
	}

	nextStatus = status + tempDiff + tickTriggered;
	if (nextStatus < 0) nextStatus = 0;

	if (nextStatus != status) {
		status = nextStatus;
		// Get the raw value from Device
		nextStatus += startLevel - 1;
		if (nextStatus >= device->getLevelCount())
			nextStatus = device->getLevelCount() - 1;

		// Apply new raw value to device
		nextValue = nextStatus < devicePerfFloor ? nextStatus : devicePerfFloor;
		printlog("[ExynosThermal] SS [%s] temp(%d->%d) status(%d)\n", this->name.c_str(), prevTemperature, temp, nextValue);

		device->deviceControl(name, nextValue);
		device->statusApply();
		timeTick = 0;
	}

	// Update previous temp
	prevTemperature = temp;

	return 0;
}

void SS::activate(void) {
	activeStatus = true;
	thread->activate();
}

void SS::deactivate(void) {
	timeTick = status = 0;
	device->deviceControl(name, 0);
	device->statusApply();
	activeStatus = false;
	thread->deactivate();
}