#!/bin/bash

options="-r vendor.trustonic.tee:hardware/samsung_slsi/exynos/tee/hardware/interfaces/tee \
	 -r vendor.trustonic.teeregistry:hardware/samsung_slsi/exynos/tee/hardware/interfaces/teeregistry\
         -r android.hidl:system/libhidl/transport \
         -r android.hardware:hardware/interfaces"

outputs="hardware/samsung_slsi/exynos/tee/hardware/interfaces/"

hidl-gen -Lhash $options vendor.trustonic.tee@2.0;
hidl-gen -Lhash $options vendor.trustonic.tee.tui@2.0;
hidl-gen -Lhash $options vendor.trustonic.teeregistry@1.0;

