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

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <condition_variable>

#define ANDROID_LOG

#ifdef ANDROID_LOG
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <android/log.h>

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "echo-service", __VA_ARGS__)

#define printlog(...) \
{ \
if (debugLogEnable) \
	__android_log_print(ANDROID_LOG_VERBOSE, "echo-service", "[ExynosThermal] " __VA_ARGS__); \
}
#else
#include <stdio.h>

#define printlog(...) \
{ \
if (debugLogEnable) \
	printf("[ExynosThermal] ", __VA_ARGS__); \
}
#endif

using namespace std;

extern bool debugLogEnable;

class Algo;
class Virtual;
class Real;
class Device;
class Scenario;
class ThermalMain;

class ThermalThread;
template <typename T>
class _ThermalThread;
class SSThread;
class MonitorThread;
class ScenarioThread;

/* Sensor Class Definition */

class Sensor {
protected:
	string name;
	unsigned int samplingTime;
	ThermalThread *thread = NULL;
	map<string, vector<Algo *>> algoItems;
	bool activeStatus = false;
	int temperature = 0;
	string oldScen = "Default";
	string newScen = "Default";
public:
	Sensor(void) {}
	Sensor(string &, unsigned int);
	Sensor(string &, unsigned int, ThermalThread*);
	virtual ~Sensor(void) {}

	void traversItems(void);
	void activate(void);
	void deactivate(void);
	void deactivateOldScen(void);

	// Override functions
	virtual void updateTemp(void) = 0;

	// Get/Set functions
	int getTemp(void) { return temperature; }
	const string& getName(void) { return name; }
	bool getStatus(void) { return activeStatus; }
	unsigned int getSamplingTime(void) { return samplingTime; }
	const ThermalThread* getThermalThread(void) { return thread; }
	const vector<Algo*>& getAlgoItems(string &scenName) { return algoItems[scenName]; }
	void setCurScen(string &scenName) {	newScen = scenName;	}
	void setOldScenToNewScen(void) { oldScen = newScen; }

	void insertAlgoItems(Algo *algo, string &scenName) { algoItems[scenName].push_back(algo); }
	void setThread(ThermalThread* t) { thread = t; }
	void addPrefixScenName(string prefix);
	void destroyOldScens(void);
};

class Virtual : public Sensor {
private:
	const int triggerPoint = 0;
	const int clearPoint = 0;
	vector<Sensor *> sensors;
	__unused int math = 0;
	vector<int> offsets;
	vector<int> weights;
	int weightSum = 0;
	int offsetSum = 0;
public:
	Virtual(string &, unsigned int, ThermalThread*, int, int, vector<Sensor *> &, int);
	Virtual(string &, unsigned int, ThermalThread*, int, int, vector<Sensor *> &, int, vector<int> &, vector<int> &);

	// Override functions
	virtual void updateTemp(void);

	// Get/Set functions
	int getTriggerPoint(void) { return triggerPoint; }
	int getClearPoint(void) { return clearPoint; }
};

class Real : public Sensor {
protected:
	string nodePath;
	ifstream node;
	vector<Virtual *> virtualSensors;
	unsigned int activeVirtualSensor = 0;
public:
	Real(void) {}
	Real(string &, unsigned int);
	Real(string &, unsigned int, string &);

	// Override functions
	virtual void updateTemp(void);

	// Get/Set functions
	void updateVirtualSensors(void);
	void insertVirtualSensor(Virtual *);
};

class DummySensor : public Real {
private:
public:
	DummySensor(void) {}
	DummySensor(string&, unsigned int);
	void updateTemp(void);
	int getTemp(void);
};

/* Algorithm Class Definition */
class Algo {
protected:
	string name;
	unsigned int samplingTime;
	int triggerPoint;
	int clearPoint;
	unsigned int status = 0;
	bool activeStatus = false;
	Sensor* sensor;
	ThermalThread* thread;
	bool isDestroy = false;
	bool isNegative = false;
public:
	Algo(void) {}
	Algo(string &, unsigned int, int, int, string &, bool, Sensor *, ThermalThread *);
	virtual ~Algo(void) {}

	// Override functions
	virtual int decision(void) = 0;
	virtual void activate(void);
	virtual void deactivate(void);

	// Get/Set functions
	int getTriggerPoint(void) { return triggerPoint; }
	int getClearPoint(void) { return clearPoint; }
	bool getStatus(void) { return activeStatus; }
	const string& getName(void) { return name; }

	void setIsDestroy(void) { isDestroy = true; }
	bool getIsDestroy(void) { return isDestroy; }
	bool getIsNegative(void) { return isNegative; }
};

class SS : public Algo {
private:
	Device* device;
	int prevTemperature = 0;
	int devicePerfFloor = 0;
	int timeTick = 0;
	int timeTickTrigger = 0;
	int startLevel = 0;
public:
	SS(void) {}
	SS(string &, unsigned int, int, int, int, string &, bool, Sensor *, SSThread *, Device *, int, int);
	~SS(void) {}

	// Override functions
	virtual int decision(void);
	virtual void activate(void);
	virtual void deactivate(void);
};

class Monitor : public Algo {
private:
	vector<int> thresholds;
	vector<vector<Device *>> device;
	vector<vector<int>> actionInfo;
	vector<int> clrThresholds;
public:
	Monitor(void) {}
	Monitor(string &, unsigned int, int, int, string &, bool, Sensor *, MonitorThread *, vector<vector<Device *>> &, vector<vector<int>> &, vector<int> &, vector<int> &);
	~Monitor(void) {}

	// Override functions
	virtual int decision(void);
	virtual void deactivate(void);
};

/* Device Class Definition */
class Device {
public:

protected:
	string name;
	string nodePath;
	ofstream node;
	map<string, int> request;
	int currentStatus = 0;
	mutex lock;
	bool lowestMode = true;
	int devicePerfFloor = INT_MAX;
	vector<string> leveltoValue;
	string levelType = "integer";
	int getLowestStatus(void);
public:
	Device(void) {}
	Device(string &);
	Device(string &, string &);
	Device(string &, string &, vector<string> &);
	Device(string &, string &, string &, vector<string> &);
	virtual ~Device(void) {}

	int deviceControl(const string &, int);

	// Override functions
	virtual void statusApply(void);

	// Get/Set functions
	unsigned int getLevelCount(void) { return leveltoValue.size(); }
	void setLeveltoValue(vector<string> &v) { leveltoValue = v; }
	const string& getValue(int lv) { return leveltoValue[lv]; }
	int getCurrentStatus(void) { return currentStatus; }

	const string& getName(void) { return name;}
};

class CameraDevice : public Device {
public:
	CameraDevice(void) {}
	CameraDevice(string &name) : Device(name) {}
	CameraDevice(string &name, string &nodePath) : Device(name, nodePath) {}
	CameraDevice(string &name, string &nodePath, vector<string> &leveltoValue) : Device(name, nodePath, leveltoValue)  {}
	CameraDevice(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : Device(name, nodePath, levelType, leveltoValue)  {}
	void statusApply(void);
};

class CPUHotplugDevice : public Device {
public:
	CPUHotplugDevice(void) {}
	CPUHotplugDevice(string &name) : Device(name) {}
	CPUHotplugDevice(string &name, string &nodePath) : Device(name, nodePath) {}
	CPUHotplugDevice(string &name, string &nodePath, vector<string> &leveltoValue) : Device(name, nodePath, leveltoValue)  {}
	CPUHotplugDevice(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : Device(name, nodePath, levelType, leveltoValue)  {}
};

class BacklightDevice : public Device {
public:
	BacklightDevice(void) {}
	BacklightDevice(string &name) : Device(name) {}
	BacklightDevice(string &name, string &nodePath) : Device(name, nodePath) {}
	BacklightDevice(string &name, string &nodePath, vector<string> &leveltoValue) : Device(name, nodePath, leveltoValue)  {}
	BacklightDevice(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : Device(name, nodePath, levelType, leveltoValue)  {}
};

class CPUFreqDevice : public Device {
public:
	CPUFreqDevice(void) {}
	CPUFreqDevice(string &name) : Device(name) {}
	CPUFreqDevice(string &name, string &nodePath) : Device(name, nodePath) {}
	CPUFreqDevice(string &name, string &nodePath, vector<string> &leveltoValue) : Device(name, nodePath, leveltoValue)  {}
	CPUFreqDevice(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : Device(name, nodePath, levelType, leveltoValue)  {}
	void statusApply(void);
};

class GPUFreqDevice : public Device {
public:
	GPUFreqDevice(void) {}
	GPUFreqDevice(string &name) : Device(name) {}
	GPUFreqDevice(string &name, string &nodePath) : Device(name, nodePath) {}
	GPUFreqDevice(string &name, string &nodePath, vector<string> &leveltoValue) : Device(name, nodePath, leveltoValue)  {}
	GPUFreqDevice(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : Device(name, nodePath, levelType, leveltoValue)  {}
};

class ChargerDevice : public Device {
public:
	ChargerDevice(void) {}
	ChargerDevice(string &name) : Device(name) {}
	ChargerDevice(string &name, string &nodePath) : Device(name, nodePath) {}
	ChargerDevice(string &name, string &nodePath, vector<string> &leveltoValue) : Device(name, nodePath, leveltoValue)  {}
	ChargerDevice(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : Device(name, nodePath, levelType, leveltoValue)  {}
};

class ReportDevice : public Device {
public:
	ReportDevice(void) {}
	ReportDevice(string &name) : Device(name) {}
	ReportDevice(string &name, string &nodePath) : Device(name, nodePath) {}
	ReportDevice(string &name, string &nodePath, vector<string> &leveltoValue) : Device(name, nodePath, leveltoValue)  {}
	ReportDevice(string &name, string &nodePath, string &levelType, vector<string> &leveltoValue) : Device(name, nodePath, levelType, leveltoValue)  {}
	void statusApply(void);
};

class DummyDevice : public Device {
public:
	DummyDevice(void) {}
	DummyDevice(string &name) : Device(name) {}
	void statusApply(void);
};

 /* Scenario Class Definition */
class Scenario {
protected:
	string name;
	bool curScen = false;

public:
	Scenario(void) {}
	Scenario(string name) : name(name) { if (name == "Default") curScen = true; }
	virtual ~Scenario(void) {}

	void setCurScen(bool flag) { curScen = flag; }
	const string& getName(void) { return name; }
};


/* Thread Class Definition */
// ThermalThread class is just wrapper class
class ThermalThread {
public:
	virtual ~ThermalThread(void) = 0;
	virtual void activate(void) = 0;
	virtual void deactivate(void) = 0;
	virtual void operator() (void) = 0;
};

template <typename T>
class thermalThread : public ThermalThread {
protected:
	int samplingTime = 1000;
	int numActive = 0;
	vector<T *> items;
	mutex flag, mtx, sleepFlag;
	condition_variable cond, condSleep;
	bool isGenerated = false;
	bool destroyFlag = false;

public:
	thermalThread();
	thermalThread(int);
	virtual ~thermalThread() = 0;

	// Implement in thermalThread
	void activate(void);
	void deactivate(void);
	void insert(T *item) { items.push_back(item); }

	void setIsGenerated(bool _isGen) { isGenerated = _isGen; }
	bool getIsGenerated(void) { return isGenerated; }
	void wakeupThread(void) { condSleep.notify_one(); }

	// Thread main function is implemented in each derived classes
	virtual void operator() (void) = 0;
};

class SSThread : public thermalThread<SS> {
public:
	SSThread(void);
	SSThread(int);
	~SSThread(void);

	virtual void operator()(void);
	bool destroyOldItems(void);
};

class MonitorThread : public thermalThread<Monitor> {
public:
	MonitorThread(void);
	MonitorThread(int);
	~MonitorThread(void);

	virtual void operator() (void);
	bool destroyOldItems(void);
};

class SensorThread : public thermalThread<Sensor> {
protected:
	mutex flags[2];
	condition_variable conds[2];
	ScenarioThread* scenarioThread = NULL;
	bool isScenChanging = false;
	string newScen;
	bool needWait = false;
public:
	SensorThread(void);
	SensorThread(int);
	~SensorThread(void);

	virtual void operator() (void);
	void changingScen(void);
	void setScenarioThread(ScenarioThread* _scenarioThread) { scenarioThread = _scenarioThread; }
	void sendSignalToSensorThread(string signal, bool isSetOld);
	void sendSignalToSensorThread(bool signal);
	void waitForScenThread(int step);
	void destroyOldScens(void);
	void traverseSensors(bool setNewScen);
};

class ScenarioThread : public thermalThread<Scenario> {
protected:
	int sensorThreadsWaitCnt = 0;
	mutex numSensorThreadLock;
	string dynamicConfFile = "PARSED";
	string dconfPath;
	string scenPath;
	ThermalMain* mainThread = NULL;
	string curScen = "Default";
	string newScen = "Default";

	void writeToFile(string fpath, string val);
public:
	ScenarioThread(void);
	ScenarioThread(bool);
	~ScenarioThread(void);

	virtual void operator() (void);
	void waitForAllSensorThreads(void);
	void sendSignalToScenThreads(void);
	string checkStringInFile(string path);

	void scenUpdateWithSensorThread(void);
	void setDynamicConfPath(string path) { dconfPath = path; }
	void setScenPath(string path) { scenPath = path; }
	void setMainThread(ThermalMain* t) { mainThread = t; }
	void scenUpdateWithSensorThread(bool);
};

class ThermalMain {
private:
	string confPath;
	string envPath;
	string dynamicConfPath = "/data/vendor/thermal/config/";
	string dynamicConfFile = "exynos-thermal_default.conf";
	map<string, SSThread *> SSThreadMap;
	map<string, MonitorThread *> MonitorThreadMap;
	map<int, SensorThread *> SensorThreadMap;
	map<string, Device *> DeviceMap;
	map<string, Sensor *> SensorMap;
	ScenarioThread* ScenarioThreadVal = NULL;
	map<string, Scenario *> ScenarioMap;

	map<string, thread *> runThreadMap;

	void confParse(void);
	void initSensor(void);
	void initDevice(void);
	void runThreads(void);
	void envParse(void);
	void dconfFind(void);
	void parseList(string, vector<string> &);
	void initScenario(void);
	void waitForthreads(void);

public:
	ThermalMain();
	ThermalMain(string &);
	ThermalMain(string &, string &);

	void markConfFile(void);
	void dynamicConfParse(string dconfFile);
	void destroyOldUnusings(void);

	void insertThread(string name, thread *t) { runThreadMap[name] = t; }
	const map<int, SensorThread *>& getSensorThreads(void) { return SensorThreadMap; }
	const map<string, Scenario *>& getScenarioMap(void) { return ScenarioMap; }
	bool findConfFile(string filePath);
};