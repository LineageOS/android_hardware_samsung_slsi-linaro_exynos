#!/bin/bash

OFI_2_1_FOLDER="./../../ofi/2.1"
ORCA_FOLDER="./../../ofi/source/orca/src"
OUT_FOLDER="./../../../../../out/target/product/${PRODUCT_NAME}"

echo "Sync with cv headers and libraries"
rsync -rptgoDhvL --delete "$ORCA_FOLDER/cv/include/" "./cv/include"

echo "Sync with graph compiler headers and libraries"
rsync -azvh --delete "$ORCA_FOLDER/gc/graph_compiler/include/ofi_gc_if.hpp" "./graph_compiler/include/ofi_gc_if.hpp"
rsync -azvh --delete "$ORCA_FOLDER/gc/graph_compiler/include/ofi_gc_api.hpp" "./graph_compiler/include/ofi_gc_api.hpp"
rsync -azvh --delete "$ORCA_FOLDER/gc/graph_compiler/include/graphGen_debuger.hpp" "./graph_compiler/include/graphGen_debuger.hpp"

echo "Sync with klm headers and libraries"
rsync -rptgoDhvL --delete "$ORCA_FOLDER/klm/include/ofi_kernel_desc.h" "./klm/include/ofi_kernel_desc.h"
rsync -azvh --delete "$ORCA_FOLDER/klm/include/ofi_kernel_lib.h" "./klm/include/ofi_kernel_lib.h"
rsync -azvh --delete "$ORCA_FOLDER/klm/include/ofi_kernel_lib_manager_if.h" "./klm/include/ofi_kernel_lib_manager_if.h"

echo "Sync with rt headers and libraries"
rsync -azvh --delete "$ORCA_FOLDER/rt/source/dsp_nn/include/ofi_api-public.h" "./rt/include/ofi_api-public.h"
rsync -azvh --delete "$ORCA_FOLDER/rt/source/dsp_nn/include/ofi_memory_manager.h" "./rt/include/ofi_memory_manager.h"
rsync -azvh --delete "$ORCA_FOLDER/rt/source/dsp_nn/memory_manager/ofi_mm_memory-public.h" "./rt/memory_manager/ofi_mm_memory-public.h"

echo "Sync to ofi/2.1/ folder"
rsync -azvh --delete "$ORCA_FOLDER/rt/source/dsp_nn/include/ofi_api-public.h" "$OFI_2_1_FOLDER/include/ofi_api-public.h"
rsync -azvh --delete "$ORCA_FOLDER/rt/source/dsp_nn/include/ofi_api.h" "$OFI_2_1_FOLDER/include/ofi_api.h"
rsync -azvh --delete "$ORCA_FOLDER/rt/source/dsp_nn/memory_manager/ofi_mm_memory-public.h" "$OFI_2_1_FOLDER/include/ofi_mm_memory-public.h"
rsync -azvh --delete "$ORCA_FOLDER/rt/source/common/include/ofi_os_hal.h" "$OFI_2_1_FOLDER/include/ofi_os_hal.h"

rsync -azvh --delete "$OUT_FOLDER/vendor/lib/libofi_seva_vendor.so" "$OFI_2_1_FOLDER/lib/libofi_seva_vendor.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib64/libofi_seva_vendor.so" "$OFI_2_1_FOLDER/lib64/libofi_seva_vendor.so"

rsync -azvh --delete "$OUT_FOLDER/vendor/lib/libinference_engine_vendor.so" "$OFI_2_1_FOLDER/lib/libinference_engine_vendor.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib64/libinference_engine_vendor.so" "$OFI_2_1_FOLDER/lib64/libinference_engine_vendor.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib/libofi_plugin_vendor.so" "$OFI_2_1_FOLDER/lib/libofi_plugin_vendor.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib64/libofi_plugin_vendor.so" "$OFI_2_1_FOLDER/lib64/libofi_plugin_vendor.so"

rsync -azvh --delete "$OUT_FOLDER/vendor/lib/libofi_klm_vendor.so" "$OFI_2_1_FOLDER/lib/libofi_klm_vendor.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib64/libofi_klm_vendor.so" "$OFI_2_1_FOLDER/lib64/libofi_klm_vendor.so"

rsync -azvh --delete "$OUT_FOLDER/vendor/lib/libofi_rt_framework_user_vendor.so" "$OFI_2_1_FOLDER/lib/libofi_rt_framework_user_vendor.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib64/libofi_rt_framework_user_vendor.so" "$OFI_2_1_FOLDER/lib64/libofi_rt_framework_user_vendor.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib/libofi_kernels_cpu.so" "$OFI_2_1_FOLDER/lib/libofi_kernels_cpu.so"
rsync -azvh --delete "$OUT_FOLDER/vendor/lib64/libofi_kernels_cpu.so" "$OFI_2_1_FOLDER/lib64/libofi_kernels_cpu.so"

