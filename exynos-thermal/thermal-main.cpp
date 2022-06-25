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

#include <algorithm>
#include <cctype>

#include "exynos-thermal.h"

void copyConfFile(string src, string dst);

ThermalMain::ThermalMain(void) {
}

ThermalMain::ThermalMain(string& confPath) {
	this->confPath = confPath;
	initSensor();
	initDevice();
	printlog("InitDevice Passed!\n");
	confParse();
	printlog("ConfParse Passed!\n");
	runThreads();
	printlog("RunThreads Passed!\n");
}

ThermalMain::ThermalMain(string &confPath, string &envPath) : confPath(confPath), envPath(envPath) {
	printlog("Using Env Config!\n");
	envParse();
	printlog("EnvParse Passed!\n");
	if (!findConfFile(dynamicConfFile)) {
		copyConfFile(confPath, dynamicConfPath + dynamicConfFile);
	}
	confParse();
	printlog("ConfParse Passed!\n");
	initScenario();
	printlog("initScenario Passed!\n");
	runThreads();
	waitForthreads();
	printlog("RunThreads Passed!\n");
}
void copyConfFile(string src, string dst) {
	ifstream source(src.c_str(), std::ifstream::binary);
	ofstream dest(dst.c_str(), std::ofstream::binary);

	if (!source.is_open()) {
		printlog("Can't open : %s\n", src.c_str());
		return;
	}
	if (!dest.is_open()) {
		printlog("Can't open : %s\n", dst.c_str());
		return;
	}

	dest << source.rdbuf();

	source.close();
	dest.close();

	printlog("Conf file is copied!(%s -> %s)\n", src.c_str(), dst.c_str());
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

static int getString(ifstream &file, stringstream &line){
	string buf;
	getline(file, buf);
	trim(buf);

	if (buf.empty())
		return EOF;
	else {
		line.clear();
		line.str(buf);
	}

	return 0;
}

static void stringTokenizer(const string &str, vector<string> &tokens, const string & delimiters = "+")
{
	string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos) {
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		lastPos = str.find_first_not_of(delimiters, pos);
		pos = str.find_first_of(delimiters, lastPos);
	}
}

void ThermalMain::envParse(void) {
	ifstream envFile(envPath);
	string prop, value;
	string name;
	stringstream line;

	while (!envFile.eof()){
		if (getString(envFile, line))
			continue;

		line >> name;
		if (name[0] != '[')
			break;
		name.erase(name.end() - 1);
		name.erase(name.begin());

		getString(envFile, line);
		line >> prop;
		if (prop != "type")
			break;

		line >> value;
		if (value == "real"){
			// Real Sensor definition
			string nodePath;
			unsigned int samplingTime = 1000;

			while (!prop.empty()) {
				if (prop == "node_path") {
					line >> value;
					nodePath = value;
				}
				else if (prop == "sampling") {
					line >> value;
					samplingTime = atoi(value.c_str());
				}
				if (getString(envFile, line))
					break;
				line >> prop;
			}

			// Register Sensor to SensorThread and SensorMap
			Real *real = new Real(name, samplingTime, nodePath);
			if (real)
				SensorMap[name] = real;

			SensorThread *thread = SensorThreadMap[samplingTime];
			if (!thread) {
				thread = new SensorThread(samplingTime);
				SensorThreadMap[samplingTime] = thread;
			}
			thread->insert(real);
			real->setThread(thread);
			real->activate();
		}
		else if (value == "virtual") {
			unsigned int samplingTime = 1000;
			int setPoint = 0, setPointClr = 0, math = 0;
			vector<Sensor *> sensors;
			vector<int> offsets, weights;
			Sensor *sensor = NULL;

			getString(envFile, line);
			line >> prop;
			while (!prop.empty()) {
				if (prop == "sampling") {
					line >> value;
					samplingTime = atoi(value.c_str());
				}
				else if (prop == "trip_sensor") {
					line >> value;
					sensor = SensorMap[value];
				}
				else if (prop == "set_point") {
					line >> value;
					setPoint = atoi(value.c_str());
				}
				else if (prop == "set_point_clr") {
					line >> value;
					setPointClr = atoi(value.c_str());
				}
				else if (prop == "sensors") {
					vector<string> v;
					while (line >> value) {
						v.push_back(value);
					}

					for (unsigned long i = 0; i < v.size(); i++){
						sensors.push_back(SensorMap[v[i]]);
					}
				}
				else if (prop == "weights") {
					vector<string> v;
					while (line >> value) {
						v.push_back(value);
					}

					for (unsigned long i = 0; i < v.size(); i++){
						weights.push_back(atoi(v[i].c_str()));
					}
				}
				else if (prop == "offsets") {
					vector<string> v;
					while (line >> value) {
						v.push_back(value);
					}

					for (unsigned long i = 0; i < v.size(); i++){
						offsets.push_back(atoi(v[i].c_str()));
					}
				}
				else if (prop == "math") {
					line >> value;
					math = atoi(value.c_str());
				}

				if (getString(envFile, line))
					break;
				line >> prop;
			}
			SensorThread *thread = SensorThreadMap[samplingTime];
			if (!thread) {
				// create new thread for given sampling time
				thread = new SensorThread(samplingTime);
				SensorThreadMap[samplingTime] = thread;
			}

			// insert given monitor instance into Monitor Thread
			Virtual *virt = new Virtual(name, samplingTime, thread, setPoint, setPointClr, sensors, math, offsets, weights);
			SensorMap[name] = virt;
			thread->insert(virt);
			if (sensor)
				((Real*)sensor)->insertVirtualSensor(virt);
		}
		else if (value == "conf_file") {
			while (!prop.empty()) {
				if (prop == "file_name") {
					line >> value;
					dynamicConfFile = value;
				}
				if (getString(envFile, line))
					break;
				line >> prop;
			}
		}
		else {
			Device *device = NULL;
			string nodePath;
			vector<string> levelTable;
			string deviceType = value;
			string tempVal;
			string levelType = "integer";

			while (!prop.empty()) {
				if (prop == "node_path") {
					line >> value;
					nodePath = value;
				}
				else if (prop == "level_table") {
					while (line >> value) {
						if (value[0] == '\"') {
							tempVal = value;
							while (value[value.size() - 1] != '\"') {
								line >> value;
								tempVal = tempVal + ' ' + value;
							}
							value = tempVal;
							tempVal.clear();
							value.replace(0, 1, "");
							value.replace(value.size() - 1, 1, "");
							levelType = "string";
						}
						else if (value.find("0x") != string::npos)
							levelType = "hex";
						else if (value.find("0b") != string::npos)
							levelType = "bin";
						levelTable.push_back(value.c_str());
					}
				}
				if (getString(envFile, line))
					break;
				line >> prop;
			}

			if (deviceType == "charger")
				device = new ChargerDevice(name, nodePath, levelType, levelTable);
			else if (deviceType == "cpufreq")
				device = new CPUFreqDevice(name, nodePath, levelType, levelTable);
			else if (deviceType == "gpufreq")
				device = new GPUFreqDevice(name, nodePath, levelType, levelTable);
			else if (deviceType == "cpuhotplug")
				device = new CPUHotplugDevice(name, nodePath, levelType, levelTable);
			else if (deviceType == "camera")
				device = new CameraDevice(name, nodePath, levelType, levelTable);
			else if (deviceType == "backlight")
				device = new BacklightDevice(name, nodePath, levelType, levelTable);
			else if (deviceType == "report")
				device = new ReportDevice(name, nodePath, levelType, levelTable);

			if (device)
				DeviceMap[name] = device;
		}
	}
}

void ThermalMain::confParse(void) {
	ifstream confFile(dynamicConfPath + dynamicConfFile);
	string prop, value;
	string name;
	stringstream line;
	string scenName = "Default";
	map<string, Scenario *> parsedScenMap;

	while (!confFile.eof()){
		if (getString(confFile, line))
			continue;

		// Every items are started from name
		prop.clear();
		while (prop.empty()) {
			line >> prop;
			trim(prop);
		}

		if (prop == "SCEN") {
			line >> value;
			scenName = value;

			Scenario *scenario = parsedScenMap[scenName];
			if (!scenario) {
				scenario = new Scenario(scenName);
				parsedScenMap[scenName] = scenario;
			}
			getString(confFile, line);
			line >> name;
		}
		else if (prop == "]") {
			scenName = "Default";

			getString(confFile, line);
			line >> name;
		}
		else
			name = prop;

		if (name[0] != '[') {
			printlog("[CONF PARSE FAIL] Each item should be started with 'SCEN' or embraced with [name]\n");
			exit(0);
		}
		name.erase(name.end() - 1);
		name.erase(name.begin());

		// First property of item must define algo_type
		getString(confFile, line);
		line >> prop;
		if (prop != "algo_type") {
			printlog("[CONF PARSE FAIL] Each item's property should contain & start with 'algo_type'\n");
			exit(0);
		}

		line >> value;
		if (value == "monitor") {
			unsigned int samplingTime = 1000;
			vector<int> thresholds, thresholds_clr;
			Sensor *sensor = NULL;
			vector<vector<Device *>> device;
			vector<vector<int>> actionInfo;
			string threadKey;
			bool isNegative = false;

			getString(confFile, line);
			line >> prop;
			while (!prop.empty()) {
				if (prop == "sampling") {
					line >> value;
					samplingTime = atoi(value.c_str());
				}
				else if (prop == "sensor") {
					line >> value;
					sensor = SensorMap[value];
				}
				else if (prop == "thresholds") {
					while (line >> value) {
						thresholds.push_back(atoi(value.c_str()));
					}
				}
				else if (prop == "thresholds_clr") {
					while (line >> value) {
						thresholds_clr.push_back(atoi(value.c_str()));
					}
				}
				else if (prop == "actions") {
					// Parse the given string by space
					vector<string> v;
					while (line >> value)
						v.push_back(value);

					for (unsigned long i = 0; i < v.size(); i++){
						// Each string separeted by space, check "+" delimiter
						vector<string> tokens;
						stringTokenizer(v[i], tokens, "+");
						vector<Device*> *dv = new vector<Device*>;
						device.push_back(*dv);

						for (unsigned long j = 0; j < tokens.size(); j++) {
							// Insert the seperated tokens in one threshold
							device[i].push_back(DeviceMap.find(tokens[j])->second);
						}
					}
				}
				else if (prop == "action_info") {
					// Each string separeted by space, check "+" delimiter
					vector<string> v;
					while (line >> value)
						v.push_back(value);

					for (unsigned long i = 0; i < v.size(); i++){
						// Each string separeted by space, check "+" delimiter
						vector<string> tokens;
						stringTokenizer(v[i], tokens, "+");
						vector<int> *dv = new vector<int>;
						actionInfo.push_back(*dv);

						for (unsigned long j = 0; j < tokens.size(); j++) {
							// Insert the seperated tokens in one threshold
							actionInfo[i].push_back(atoi(tokens[j].c_str()));
						}
					}
				}
				else if (prop == "lowactive") {
					line >> value;
					if (value == "true")
						isNegative = true;
					else
						isNegative = false;
				}

				if (getString(confFile, line))
					break;
				else {
					line >> prop;
					if (prop == "]")
						break;
				}
			}

			threadKey = std::to_string(samplingTime) + scenName + "MONITOR";
			MonitorThread *thread = MonitorThreadMap[threadKey];
			if (!thread) {
				// create new thread for given sampling time
				thread = new MonitorThread(samplingTime);
				MonitorThreadMap[threadKey] = thread;
			}
			if (sensor == NULL || thresholds.empty() || thresholds_clr.empty() || device.empty() || actionInfo.empty()) {
				printlog("[CONF PARSE FAIL] There is some missing property for %s\n", name.c_str());
				exit(0);
			}

			// insert given monitor instance into Monitor Thread
			if (sensor) {
				Monitor *monitor = new Monitor(name, samplingTime, thresholds[0], thresholds_clr[0], scenName, isNegative, sensor, thread, device, actionInfo, thresholds, thresholds_clr);
				thread->insert(monitor);
			}
		}
		else if (value == "ss") {
			unsigned int samplingTime = 1000;
			int setPoint = 0, setPointClr = 0, devicePerfFloor = INT_MAX, timeTick = 0;
			int startLevel = 0;
			Sensor *sensor = NULL;
			Device *device = NULL;
			string threadKey;
			bool isNegative = false;

			getString(confFile, line);
			line >> prop;
			while (!prop.empty()) {
				if (prop == "sampling") {
					line >> value;
					samplingTime = atoi(value.c_str());
				}
				else if (prop == "sensor") {
					line >> value;
					sensor = SensorMap[value];
				}
				else if (prop == "device") {
					line >> value;
					device = DeviceMap[value];
				}
				else if (prop == "set_point") {
					line >> value;
					setPoint = atoi(value.c_str());
				}
				else if (prop == "set_point_clr") {
					line >> value;
					setPointClr = atoi(value.c_str());
				}
				else if (prop == "device_perf_floor") {
					line >> value;
					devicePerfFloor = atoi(value.c_str());
				}
				else if (prop == "time_tick_trigger") {
					line >> value;
					timeTick = atoi(value.c_str());
				}
				else if (prop == "start_level") {
					line >> value;
					startLevel = atoi(value.c_str());
				}
				else if (prop == "lowactive") {
					line >> value;
					if (value == "true")
						isNegative = true;
					else
						isNegative = false;
				}
				if (getString(confFile, line))
					break;
				else {
					line >> prop;
					if (prop == "]")
						break;
				}
			}

			threadKey = std::to_string(samplingTime) + scenName + "SS";
			SSThread *thread = SSThreadMap[threadKey];
			if (!thread) {
				// create new thread for given sampling time
				thread = new SSThread(samplingTime);
				SSThreadMap[threadKey] = thread;
			}

			if (sensor == NULL || device == NULL || setPoint == 0 || setPointClr == 0) {
				printlog("[CONF PARSE FAIL] There is some missing property for %s\n", name.c_str());
				exit(0);
			}
			// insert given monitor instance into Monitor Thread
			if (sensor) {
				SS *ss = new SS(name, samplingTime, setPoint, setPointClr, startLevel, scenName, isNegative, sensor, thread, device, devicePerfFloor, timeTick);
				thread->insert(ss);
			}

		}
		else {
			printlog("[CONF PARSE FAIL] Algo type should be 'monitor' or 'SS'\n");
			exit(0);
		}
	}
	ScenarioMap.clear();
	ScenarioMap.insert(parsedScenMap.begin(), parsedScenMap.end());
}
void ThermalMain::initScenario(void) {
	string thermalPath = "/data/vendor/thermal/";
	ofstream thermalFile;

	ScenarioThreadVal = new ScenarioThread;

	ScenarioThreadVal->setDynamicConfPath(thermalPath + "exynos-thermal.dconf");
	thermalFile.open(thermalPath + "exynos-thermal.dconf");
	thermalFile << "PARSED";
	thermalFile.close();

	ScenarioThreadVal->setScenPath(thermalPath + "exynos-thermal.scen");
	thermalFile.open(thermalPath + "exynos-thermal.scen");
	thermalFile << "Default";
	thermalFile.close();

	ScenarioThreadVal->setMainThread(this);

	for (auto it = SensorThreadMap.begin(); it != SensorThreadMap.end(); it++) {
		it->second->setScenarioThread(ScenarioThreadVal);
	}
}
bool ThermalMain::findConfFile(string filePath) {
	ifstream confFile;

	confFile.open(dynamicConfPath+filePath);

	if (!confFile || !confFile.is_open())
		return false;
	else
		return true;
}
void ThermalMain::dynamicConfParse(string dconfFile) {
	dynamicConfFile = dconfFile;
	confParse();
	runThreads();
}
void ThermalMain::destroyOldUnusings(void) {
	for (auto it = SensorThreadMap.begin(); it != SensorThreadMap.end(); it++) {
		it->second->destroyOldScens();
	}
	for (auto it = SSThreadMap.begin(); it != SSThreadMap.end();) {
		if (it->second->destroyOldItems()) {
			it->second->wakeupThread();
			it->second->activate();
			if (runThreadMap[it->first]->joinable())
				runThreadMap[it->first]->join();
			delete runThreadMap[it->first];
			runThreadMap.erase(it->first);
			delete it->second;
			SSThreadMap.erase(it++);
		}
		else {
			++it;
		}
	}
	for (auto it = MonitorThreadMap.begin(); it != MonitorThreadMap.end();) {
		if (it->second->destroyOldItems()){
			it->second->wakeupThread();
			it->second->activate();
			if (runThreadMap[it->first]->joinable())
				runThreadMap[it->first]->join();
			delete runThreadMap[it->first];
			runThreadMap.erase(it->first);
			delete it->second;
			MonitorThreadMap.erase(it++);
		}
		else {
			++it;
		}
	}

}
void ThermalMain::initSensor(void) {
	// H/W Sensor initialization
	string name[] = {
		"tmu_little",
		"tmu_big",
		"tmu_gpu",
		"tmu_isp",
		"therm_soc",
		"therm_bat"
	};
	string nodePath[] = {
		"/sys/class/thermal/thermal_zone1/temp",
		"/sys/class/thermal/thermal_zone0/temp",
		"/sys/class/thermal/thermal_zone2/temp",
		"/sys/class/thermal/thermal_zone3/temp",
		"/sys/class/hwmon/hwmon0/temp1_input",
		"/sys/class/hwmon/hwmon1/temp1_input"
	};

	for (int i = 0; i < 6; i++) {
		Sensor *s = new Real(name[i], 1000, nodePath[i]);
		SensorMap[name[i]] = s;
	}

	// Register Sensor to SensorThread
	for (auto it = SensorMap.begin(); it != SensorMap.end(); it++) {
		Sensor *s = it->second;
		SensorThread *thread = SensorThreadMap[s->getSamplingTime()];
		if (!thread) {
			thread = new SensorThread(s->getSamplingTime());
			SensorThreadMap[s->getSamplingTime()] = thread;
		}
		thread->insert(s);
		s->setThread(thread);
		s->activate();
	}
}

void ThermalMain::initDevice(void) {
	// Device initialization
	string name1 = "charger";
	string name2 = "little_freq";
	string name3 = "big_freq";
	string name4 = "gpu_freq";
	string name5 = "cpu4_hotplug";
	string name6 = "cpu5_hotplug";
	string name7 = "cpu6_hotplug";
	string name8 = "cpu7_hotplug";
	string name9 = "camera";
	string name10 = "backlight";
	string name11 = "report";

	string node1 = "/sys/class/power_supply/battery/thermal_limit";
	string node2 = "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";
	string node3 = "/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq";
	string node4 = "/sys/devices/platform/11500000.mali/dvfs_max_lock";
	string node5 = "/sys/devices/system/cpu/cpu4/online";
	string node6 = "/sys/devices/system/cpu/cpu5/online";
	string node7 = "/sys/devices/system/cpu/cpu6/online";
	string node8 = "/sys/devices/system/cpu/cpu7/online";
	string node9 = "/sys/devices/platform/14490000.fimc_is/debug/fixed_sensor_fps";
	string node10 = "/sys/class/panel/panel/max_brightness";
	string node11 = "/data/exynos-thermal.log";

	vector<string> t2 = { "1638000", "1534000", "1456000", "1326000", "1222000", "1118000", "1053000", "910000", "806000", "702000", "598000", "403000" };
	vector<string> t3 = { "2314000", "2210000", "2184000", "2080000", "1976000", "1898000", "1768000", "1664000", "1508000", "1456000", "1352000", "1248000", "1144000", "1040000", "936000" };
	vector<string> t4 = { "1053", "949", "839", "764", "683", "572", "546", "455", "385", "338", "260" };

	Device *d1 = new ChargerDevice(name1, node1);
	Device *d2 = new CPUFreqDevice(name2, node2);
	Device *d3 = new CPUFreqDevice(name3, node3);
	Device *d4 = new GPUFreqDevice(name4, node4);
	Device *d5 = new CPUHotplugDevice(name5, node5);
	Device *d6 = new CPUHotplugDevice(name6, node6);
	Device *d7 = new CPUHotplugDevice(name7, node7);
	Device *d8 = new CPUHotplugDevice(name8, node8);
	Device *d9 = new CameraDevice(name9, node9);
	Device *d10 = new BacklightDevice(name10, node10);
	Device *d11 = new ReportDevice(name11, node11);

	d2->setLeveltoValue(t2);
	d3->setLeveltoValue(t3);
	d4->setLeveltoValue(t4);

	DeviceMap[name1] = d1;
	DeviceMap[name2] = d2;
	DeviceMap[name3] = d3;
	DeviceMap[name4] = d4;
	DeviceMap[name5] = d5;
	DeviceMap[name6] = d6;
	DeviceMap[name7] = d7;
	DeviceMap[name8] = d8;
	DeviceMap[name9] = d9;
	DeviceMap[name10] = d10;
	DeviceMap[name11] = d11;
}

void ThermalMain::runThreads(void) {
	thread *t;
	for (auto it = SensorThreadMap.begin(); it != SensorThreadMap.end(); it++) {
		if (!it->second->getIsGenerated()) {
			t = new thread(ref(*(it->second)));
			runThreadMap[to_string(it->first)] = t;
			it->second->setIsGenerated(true);
		}
	}
	for (auto it = SSThreadMap.begin(); it != SSThreadMap.end(); it++) {
		if (!it->second->getIsGenerated()) {
			t = new thread(ref(*(it->second)));
			runThreadMap[it->first] = t;
			it->second->setIsGenerated(true);
		}
	}
	for (auto it = MonitorThreadMap.begin(); it != MonitorThreadMap.end(); it++) {
		if (!it->second->getIsGenerated()) {
			t = new thread(ref(*(it->second)));
			runThreadMap[it->first] = t;
			it->second->setIsGenerated(true);
		}
	}
	if (!ScenarioThreadVal->getIsGenerated()) {
		t = new thread(ref(*(ScenarioThreadVal)));
		runThreadMap["ScenarioThread"] = t;
		ScenarioThreadVal->setIsGenerated(true);
	}
}
void ThermalMain::waitForthreads(void) {
	for (auto it = runThreadMap.begin(); it != runThreadMap.end(); it++){
		it->second->join();
	}
}
