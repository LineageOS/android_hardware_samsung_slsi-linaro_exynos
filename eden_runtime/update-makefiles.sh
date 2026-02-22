#!/bin/bash

options="-r vendor.samsung_slsi.hardware:vendor/samsung_slsi/hardware \
         -r android.hidl:system/libhidl/transport \
         -r android.hardware:hardware/interfaces"

outputs="vendor/samsung_slsi/hardware/eden_runtime/1.0/default"

#hidl-gen -L c++-headers -o $outputs $options vendor.samsung_slsi.hardware.eden_runtime@1.0;
#hidl-gen -Lmakefile $options vendor.samsung_slsi.hardware.eden_runtime@1.0;
#hidl-gen -Landroidbp $options vendor.samsung_slsi.hardware.eden_runtime@1.0;
hidl-gen -Lhash $options vendor.samsung_slsi.hardware.eden_runtime@1.0;

#hidl-gen -L androidbp-impl -o $outputs $options vendor.samsung_slsi.hardware.eden_runtime@1.0;
#hidl-gen -L c++-impl -o $outputs $options vendor.samsung_slsi.hardware.eden_runtime@1.0;
