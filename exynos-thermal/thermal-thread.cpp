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

#include <chrono>
#include <typeinfo>
#include "exynos-thermal.h"

/* ThermalThread wrapper class implementation*/
ThermalThread::~ThermalThread(void) {}

/* thermalThread base class implementation */
template <typename T>
thermalThread<T>::thermalThread(void) {
	lock_guard<mutex> lg(flag); // Prepare condition_variable for thread controlling
}
template <typename T>
thermalThread<T>::thermalThread(int samplingTime) : samplingTime(samplingTime), numActive(0) {
	lock_guard<mutex> lg(flag); // Prepare condition_variable for thread controlling
}
template <typename T>
thermalThread<T>::~thermalThread(void) {}

template <typename T>
void thermalThread<T>::activate(void) {
	lock_guard<mutex> lg(mtx);

	numActive++;
	if (numActive == 1)
		cond.notify_one();
}
template <typename T>
void thermalThread<T>::deactivate(void) {
	lock_guard<mutex> lg(mtx);

	if (numActive > 0)
		numActive--;
}
/* SSThread implemenation */
SSThread::SSThread(void) {}
SSThread::SSThread(int samplingTime) : thermalThread(samplingTime) {}
SSThread::~SSThread(void) {
	vector<SS *>().swap(items);
}

bool SSThread::destroyOldItems(void) {
	vector<SS *> tempItems;
	unsigned int itemCnt = 0;
	for (SS *ss : items) {
		if (ss->getIsDestroy())
			delete ss;
		else {
			itemCnt++;
			tempItems.push_back(ss);
		}
	}
	items.clear();
	items.assign(tempItems.begin(), tempItems.end());
	vector<SS *>().swap(tempItems);
	if (itemCnt == 0) {
		destroyFlag = true;
		return true;
	}
	return false;
}
void SSThread::operator()(void) {
	unique_lock<mutex> lock(flag);
	unique_lock<mutex> sleeplock(sleepFlag);
	while (true) {
		if (destroyFlag == false && numActive == 0)
			cond.wait(lock); // If there has no active SS Item, goto wait
		else if (destroyFlag) {
			return;
		}
		// Travers all SS node
		for (SS *ss : items) {
			// If active status of SS is true
			if (ss->getStatus()) {
				ss->decision();
			}
		}
		condSleep.wait_for(sleeplock, chrono::milliseconds(samplingTime));
	}
}

/* MonitorThread implemenation */
MonitorThread::MonitorThread(void) {}
MonitorThread::MonitorThread(int samplingTime) : thermalThread(samplingTime) {}
MonitorThread::~MonitorThread(void) {
	vector<Monitor *>().swap(items);
}

bool MonitorThread::destroyOldItems(void) {
	vector<Monitor *> tempItems;
	unsigned int itemCnt = 0;
	for (Monitor *monitor : items) {
		if (monitor->getIsDestroy())
			delete monitor;
		else {
			itemCnt++;
			tempItems.push_back(monitor);
		}
	}
	items.clear();
	items.assign(tempItems.begin(), tempItems.end());
	vector<Monitor *>().swap(tempItems);
	if (itemCnt == 0) {
		destroyFlag = true;
		return true;
	}
	return false;
}
void MonitorThread::operator()(void) {
	unique_lock<mutex> lock(flag);
	unique_lock<mutex> sleeplock(sleepFlag);
	while (true) {
		if (destroyFlag == false && numActive == 0)
			cond.wait(lock); // If there has no active SS Item, goto wait
		else if (destroyFlag) {
			return;
		}
		// Travers all monitor node
		for (Monitor *monitor : items) {
			// If active status of monitor is true
			if (monitor->getStatus()) {
				monitor->decision();
			}
		}
		condSleep.wait_for(sleeplock, chrono::milliseconds(samplingTime));
	}
}

/* SensorThread implementation */
SensorThread::SensorThread(void) {}
SensorThread::SensorThread(int samplingTime) : thermalThread(samplingTime) {}
SensorThread::~SensorThread(void) {}

void SensorThread::destroyOldScens(void) {
	for (Sensor *sensor : items) {
		sensor->destroyOldScens();
	}
}
void SensorThread::sendSignalToSensorThread(string scenName, bool isSetOld) {
	if (isSetOld) {
		for (Sensor *sensor : items) {
			sensor->addPrefixScenName("OLDSCEN_");
		}
		scenName = "OLDSCEN_" + scenName;
	}
	newScen = scenName;
	needWait = false;
	conds[0].notify_one();

}
void SensorThread::sendSignalToSensorThread(bool signal) {

	if (signal == false) {
		needWait = false;
		conds[1].notify_one();
	}

	isScenChanging = signal;
}
void SensorThread::waitForScenThread(int step) {
	unique_lock<mutex> lock(flags[step]);
	scenarioThread->sendSignalToScenThreads();

	if (needWait)
		(conds[step]).wait(lock);

}
void SensorThread::traverseSensors(bool setNewScen) {
	for (Sensor *sensor : items) {
		if (setNewScen)
			sensor->setCurScen(newScen);

		if (sensor->getStatus()) {
			sensor->updateTemp();
			sensor->traversItems();
			// If this sensor is Real Sensor(Dummy) check virtual sensor status
			if (typeid(*sensor) == typeid(Real))
				((Real*)sensor)->updateVirtualSensors();
		}
	}
}
void SensorThread::changingScen(void) {
	// 1. wating for all sensor thead stop
	needWait = true;
	waitForScenThread(0);

	// 2. scen thread will wakeup this point
	needWait = true;
	// 3. new scen applying
	traverseSensors(true);

	// 4. waiting for all sesnsor thread applying new scen
	waitForScenThread(1);

	// 5. scen thread will wakeup this point
	// 6. old scen clear
	for (Sensor *sensor : items) {
		sensor->deactivateOldScen();
		sensor->setOldScenToNewScen();
	}

	// 7. send the signal to scen thread
	scenarioThread->sendSignalToScenThreads();
	printlog("ScenChange for sensor Thread is done [%s]\n", newScen.c_str());
}
void SensorThread::operator()(void) {
	unique_lock<mutex> lock(flag);
	unique_lock<mutex> sleeplock(sleepFlag);
	while (true) {
		if (numActive == 0)
			cond.wait(lock); // If there has no active Sensor Item, goto wait

		if (isScenChanging)
			changingScen();

		// Travers all sensor node
		traverseSensors(false);

		condSleep.wait_for(sleeplock, chrono::milliseconds(samplingTime));
	}
}

/* ScenarioThread implementation */
ScenarioThread::ScenarioThread(void) {}
ScenarioThread::~ScenarioThread(void) {}


void ScenarioThread::waitForAllSensorThreads(void) {
	unique_lock<mutex> lock(flag);

	if (sensorThreadsWaitCnt > 0)
		cond.wait(lock);
}
void ScenarioThread::sendSignalToScenThreads(void) {
	unique_lock<mutex> lock(numSensorThreadLock);

	sensorThreadsWaitCnt--;
	if (sensorThreadsWaitCnt == 0)
		cond.notify_one();
}
void ScenarioThread::scenUpdateWithSensorThread(bool isSetOld) {
	map<int, SensorThread *> sensorThreads = mainThread->getSensorThreads();
	map<string, Scenario *> scenarioMap = mainThread->getScenarioMap();
	printlog("Changing Scenario Start (%s->%s)\n", curScen.c_str(), newScen.c_str());
	// send scen changing signal
	for (auto it = sensorThreads.begin(); it != sensorThreads.end(); it++) {
		sensorThreadsWaitCnt++;
		it->second->sendSignalToSensorThread(true);
		it->second->wakeupThread();
	}

	// wait for all sensor thread stop
	waitForAllSensorThreads();

	// send new scen info
	for (auto it = sensorThreads.begin(); it != sensorThreads.end(); it++) {
		sensorThreadsWaitCnt++;
		it->second->sendSignalToSensorThread(newScen, isSetOld);
	}

	// wait for all sensor thread stop
	waitForAllSensorThreads();

	// send signal to old scen clear
	for (auto it = sensorThreads.begin(); it != sensorThreads.end(); it++) {
		sensorThreadsWaitCnt++;
		it->second->sendSignalToSensorThread(false);
	}

	// wait for all sensor thread done
	waitForAllSensorThreads();

	scenarioMap[curScen]->setCurScen(false);
	scenarioMap[newScen]->setCurScen(true);
	curScen = newScen;

	printlog("All scenChangeDone to %s\n", curScen.c_str());
}
string ScenarioThread::checkStringInFile(string path) {
	ifstream readFile;
	string checkString;

	readFile.open(path);
	if (!readFile) {
		checkString = "ENOFILE";
	}
	else {
		readFile.seekg(0, readFile.beg);
		readFile >> checkString;
	}
	readFile.close();
	return checkString;
}
void ScenarioThread::writeToFile(string fPath, string val) {
	ofstream outFile;
	outFile.open(fPath, ios::out);
	outFile << val;
	outFile.close();
}
void ScenarioThread::operator()(void) {
	string checkString;
	map<string, Scenario *> scenarioMap;

	while (true) {
		if (curScen == newScen) {
			checkString = checkStringInFile(scenPath);
			if (checkString == "ENOFILE") {
				printlog("No scen file\n");
				writeToFile(scenPath, curScen);
			}
			else {
				scenarioMap = mainThread->getScenarioMap();
				if (!scenarioMap[checkString]) {
					printlog("There is no scen name: %s\n", checkString.c_str());
					writeToFile(scenPath, curScen);
				}
				else {
					newScen = checkString;
				}
			}
		}
		else {
			scenUpdateWithSensorThread(false);
		}

		if (dynamicConfFile == "PARSED") {
			checkString = checkStringInFile(dconfPath);
			if (checkString == "ENOFILE") {
				printlog("No conf file\n");
				writeToFile(dconfPath, "PARSED");
			}
			else {
				if (!mainThread->findConfFile(checkString) && checkString != "PARSED") {
					printlog("There is no conf file: %s\n", checkString.c_str());
					writeToFile(dconfPath, "PARSED");
				}
				else {
					dynamicConfFile = checkString;
				}
			}
		}
		else {
			printlog("Start to apply new CONF file\n")
			// 1. Make all scenario 'old' by adding "OLDSCEN_" prefix.
			// Current Scenario 'OLDSCEN_curScen'
			scenUpdateWithSensorThread(true);

			// 2. Parse new conf file, create instance/threads, run threads.
			mainThread->dynamicConfParse(dynamicConfFile);

			// 3. Change curScen from 'OLDSCEN_curScen' to 'curScen'
			scenarioMap = mainThread->getScenarioMap();
			if (!scenarioMap[curScen]) {
				printlog("There is no scen name in new conf file: %s\n", checkString.c_str());
				newScen = curScen = "Default";
				writeToFile(scenPath, curScen);
			}
			scenUpdateWithSensorThread(false);
			// 4. Destroy unusing instances, threads.
			mainThread->destroyOldUnusings();

			dynamicConfFile = "PARSED";
			writeToFile(dconfPath, "PARSED");
			printlog("Appling new CONF file is completed\n");
		}
		this_thread::sleep_for(chrono::milliseconds(1000));
	}
}
