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

#include <stdlib.h>

#include "exynos-thermal.h"

/* Sensor base class Implemantation */
Sensor::Sensor(string &name, unsigned int samplingTime, ThermalThread* thread) : name(name), samplingTime(samplingTime), thread(thread) {}
Sensor::Sensor(string &name, unsigned int samplingTime) : name(name), samplingTime(samplingTime) {}

void Sensor::destroyOldScens(void) {
	map<string, vector<Algo *>> tempItems;

	for (auto it = algoItems.begin(); it != algoItems.end(); it++){
		if (it->first.find("OLDSCEN_") != string::npos) {
			for (Algo * algo : it->second) {
				algo->setIsDestroy();
			}
		}
		else
			tempItems[it->first] = it->second;
	}
	algoItems.clear();
	algoItems.insert(tempItems.begin(), tempItems.end());
	tempItems.clear();
}
void Sensor::addPrefixScenName(string prefix){
	map<string, vector<Algo *>> tempItems;

	for (auto it = algoItems.begin(); it != algoItems.end(); it++) {
		tempItems[prefix + it->first] = it->second;
	}
	algoItems.clear();
	algoItems.insert(tempItems.begin(), tempItems.end());
	tempItems.clear();
}

void Sensor::traversItems(void) {
	for (Algo *algo : algoItems[newScen]) {
		if (!algo)
			return;

		if (algo->getStatus() == false) {
			if (!algo->getIsNegative()) {
				if (temperature >= algo->getTriggerPoint()) {
					printlog("Sensor [%s] activate Algo [%s], scen [%s]\n", name.c_str(), algo->getName().c_str(), newScen.c_str());
					algo->activate();
				}
			}
			else if (algo->getIsNegative()) {
				if (-temperature >= algo->getTriggerPoint()) {
					printlog("Sensor [%s] activate Algo [%s], scen [%s]\n", name.c_str(), algo->getName().c_str(), newScen.c_str());
					algo->activate();
				}
			}
		}
	}
}

void Sensor::activate(void) {
	if (activeStatus)
		return;

	activeStatus = true;
	thread->activate();
}

void Sensor::deactivate(void) {
	if (!activeStatus)
		return;

	activeStatus = false;
	thread->deactivate();
}
void Sensor::deactivateOldScen(void) {
	for (Algo *algo : algoItems[oldScen]) {
		if (!algo)
			return;

		if (algo->getStatus() == true){
			printlog("Sensor [%s] deactivate Algo [%s], scen [%s]\n", name.c_str(), algo->getName().c_str(), oldScen.c_str());
			algo->deactivate();
		}
	}
}

/* Real Sensor Implementation */
Real::Real(string &name, unsigned int samplingTime) : Sensor(name, samplingTime) {}
Real::Real(string &name, unsigned int samplingTime, string &nodePath) : Sensor(name, samplingTime), nodePath(nodePath) {
	node.open(nodePath);
	if (!node.is_open()) printlog("Can't open node: %s\n", nodePath.c_str());

	node >> unitbuf;
}

void Real::updateTemp(void) {
	string buf;

	if (node.is_open()) {
		node.seekg(0, node.beg);
		node >> buf;
		temperature = atoi(buf.c_str());
	} else {
		temperature = 0;
	}
	printlog("Sensor [%s], Scen [%s] temp(%d)\n", this->name.c_str(), this->newScen.c_str(), temperature);
}

void Real::updateVirtualSensors(void) {
	if (activeVirtualSensor == virtualSensors.size())
		return;

	for (Virtual *v : virtualSensors){
		if (v->getStatus() == false && temperature > v->getTriggerPoint()) {
			v->activate();
			activeVirtualSensor++;
		}
		else if (v->getStatus() == true && temperature < v->getClearPoint()) {
			v->deactivate();
			activeVirtualSensor--;
		}
	}
}

void Real::insertVirtualSensor(Virtual *virt) {
	virtualSensors.push_back(virt);
}

/* Virtual Sensor Implementation */
Virtual::Virtual(string &name, unsigned int samplingTime, ThermalThread* thread, int triggerPoint, int clearPoint, vector<Sensor *> &sensors, int math) :
Sensor(name, samplingTime, thread), triggerPoint(triggerPoint), clearPoint(clearPoint), sensors(sensors), math(math) {}
Virtual::Virtual(string &name, unsigned int samplingTime, ThermalThread* thread, int triggerPoint, int clearPoint, vector<Sensor *> &sensors, int math, vector<int> &offsets, vector<int> &weights) :
Sensor(name, samplingTime, thread), triggerPoint(triggerPoint), clearPoint(clearPoint), sensors(sensors), math(math), offsets(offsets), weights(weights) {
	offsetSum = 0;
	weightSum = 0;
	for (int i : offsets)
		offsetSum += i;
	offsetSum *= 100;
	for (int i : weights)
		weightSum += i;
}

void Virtual::updateTemp(void) {
	unsigned long i;

	for (i = 0; i < sensors.size(); i++) {
		temperature += sensors[i]->getTemp() * weights[i];
	}

	temperature = (temperature + offsetSum) / weightSum;
	printlog("Sensor[%s] temp(%d)\n", this->name.c_str(), temperature);

	return;
}

/* Dummy Sensor Implementation */
DummySensor::DummySensor(string &name, unsigned int samplingTime) : Real(name, samplingTime) {}

void DummySensor::updateTemp(void) {
	temperature = (rand() % 101) * 1000;
}