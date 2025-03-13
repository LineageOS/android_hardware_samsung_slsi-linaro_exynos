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
#include <cutils/properties.h>
bool debugLogEnable = true;

int main(int argc, char *argv[])
{
	string confPath = "/vendor/etc/exynos-thermal.conf";
	string envPath = "/vendor/etc/exynos-thermal.env";
	char carrier_property[PROPERTY_VALUE_MAX] = {0};
	char device_property[PROPERTY_VALUE_MAX] = {0};
	char real_config_path[50] = {0};

	if (property_get("ro.boot.device", device_property, "") > 0) {
		if (strlen(device_property) > 0) {
			if(property_get("ro.carrier", carrier_property, "") > 0) {
				if (strlen(carrier_property) > 0) {
					memset(real_config_path, 0, 50);
					sprintf(real_config_path,"/vendor/exynos-thermal-%s-%s.conf", device_property, carrier_property);
					if (access(real_config_path, 0) == 0)
						confPath = real_config_path;
				}
			} else {
				memset(real_config_path, 0, 50);
				sprintf(real_config_path,"/vendor/exynos-thermal-%s.conf", device_property);
				if (access(real_config_path, 0) == 0)
					confPath = real_config_path;
			}
		}
	}

	if (argc == 2) {
		string arg(argv[1]);
		if (arg == "nolog")
			debugLogEnable = false;
	}

	ThermalMain(confPath, envPath);

	return 0;
}
