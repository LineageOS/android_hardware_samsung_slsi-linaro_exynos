#
# Copyright (C) 2016-2019 ARM Limited. All rights reserved.
#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# HAL module implemenation, not prelinked.
# Binary located in hw/gralloc.<ro.product.board>.so
include $(CLEAR_VARS)

# Include makefile which exports Gralloc Major and Minor version numbers
include $(LOCAL_PATH)/../gralloc.version.mk

# Include platform specific makefiles
include $(if $(wildcard $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk), $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk,)

MALI_GRALLOC_VPU_LIBRARY_PATH?=\"/system/lib/\"
MALI_GRALLOC_GPU_LIBRARY_64_PATH1 := \"/vendor/lib64/egl/\"
MALI_GRALLOC_GPU_LIBRARY_64_PATH2 := \"/system/lib64/egl/\"
MALI_GRALLOC_DPU_LIBRARY_64_PATH := \"/vendor/lib64/hw/\"
MALI_GRALLOC_GPU_LIBRARY_32_PATH1 := \"/vendor/lib/egl/\"
MALI_GRALLOC_GPU_LIBRARY_32_PATH2 := \"/system/lib/egl/\"
MALI_GRALLOC_DPU_LIBRARY_32_PATH := \"/vendor/lib/hw/\"

# 2.0+ specific.
# If set to the default value of zero then build the allocator. Otherwise build the mapper.
# The top-level make file builds both.
GRALLOC_MAPPER?=0

#
# Static hardware defines
#
# These defines are used in case runtime detection does not find the
# user-space driver to read out hardware capabilities

# GPU support for AFBC 1.0
MALI_GPU_SUPPORT_AFBC_BASIC?=1
# GPU support for AFBC 1.1 block split
MALI_GPU_SUPPORT_AFBC_SPLITBLK?=0
# GPU support for AFBC 1.1 wide block
MALI_GPU_SUPPORT_AFBC_WIDEBLK?=0
# GPU support for AFBC 1.2 tiled headers
MALI_GPU_SUPPORT_AFBC_TILED_HEADERS?=0
# GPU support for writing AFBC YUV formats
MALI_GPU_SUPPORT_AFBC_YUV_WRITE?=0

#
# Software behaviour defines
#

# The following defines are used to override default behaviour of which heap is selected for allocations.
# The default is to pick system heap.
# The following two defines enable either DMA heap or compound page heap for when the usage has
# GRALLOC_USAGE_HW_FB or GRALLOC_USAGE_HW_COMPOSER set and GRALLOC_USAGE_HW_VIDEO_ENCODER is not set.
# These defines should not be enabled at the same time.
GRALLOC_USE_ION_DMA_HEAP?=0
GRALLOC_USE_ION_COMPOUND_PAGE_HEAP?=0

# fbdev bitdepth to use
GRALLOC_FB_BPP?=32
# When enabled, forces display framebuffer format to BGRA_8888
GRALLOC_FB_SWAP_RED_BLUE?=1
# Disables the framebuffer HAL device. When a hwc impl is available.
GRALLOC_DISABLE_FRAMEBUFFER_HAL?=1

# When enabled, buffers will never be allocated with AFBC
GRALLOC_ARM_NO_EXTERNAL_AFBC?=1
# Properly initializes an empty AFBC buffer
GRALLOC_INIT_AFBC?=0

ifeq ($(BOARD_USES_EXYNOS_AFBC_FEATURE), true)
GRALLOC_ARM_NO_EXTERNAL_AFBC:=0
GRALLOC_INIT_AFBC:=1
endif

# Minimum buffer dimensions in pixels when buffer will use AFBC
GRALLOC_DISP_W?=1080
GRALLOC_DISP_H?=1920
GRALLOC_AFBC_MIN_SIZE?=50

ifdef BOARD_GRALLOC_AFBC_DISP_W
GRALLOC_DISP_W:=$(BOARD_GRALLOC_AFBC_DISP_W)
endif
ifdef BOARD_GRALLOC_AFBC_DISP_H
GRALLOC_DISP_H:=$(BOARD_GRALLOC_AFBC_DISP_H)
endif
ifdef BOARD_GRALLOC_AFBC_MIN_SIZE
GRALLOC_AFBC_MIN_SIZE := $(BOARD_GRALLOC_AFBC_MIN_SIZE)
endif

# Vsync backend(not used)
GRALLOC_VSYNC_BACKEND?=default

GRALLOC_USE_ION_DMABUF_SYNC?=1

MALI_GRALLOC_DISABLE_ALOGV?=1

# Set 1 to require importBuffer before locking raw buffers
GRALLOC_IMPORT_BUFFER_REQUIRED?=0

# Used for WFD on exynos3830
ifeq ($(TARGET_SOC_BASE), exynos3830)
GRALLOC_SCALER_WFD:=1
else
GRALLOC_SCALER_WFD:=0
endif

# Disable AFBC on secure buffers for exynos9830 based SOC
ifeq ($(TARGET_SOC_BASE), exynos9830)
GRALLOC_SECURE_BUFFER_NO_AFBC:=1
else
GRALLOC_SECURE_BUFFER_NO_AFBC:=0
endif

# DPU support RGBA1010102 AFBC
ifeq ($(TARGET_SOC), exynos2100)
GRALLOC_DPU_SUPPORT_1010102_AFBC:=1
else
GRALLOC_DPU_SUPPORT_1010102_AFBC:=0
endif

# Disable ION debug handle import on 7885
ifneq ($(TARGET_SOC), exynos7885)
GRALLOC_ION_HANDLE_IMPORT:=1
else
GRALLOC_ION_HANDLE_IMPORT:=0
endif

GRALLOC_USE_ASHMEM_METADATA:=0

GRALLOC_HFR_BATCH_SIZE?=8

ifeq ($(GRALLOC_USE_ION_DMA_HEAP), 1)
ifeq ($(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP), 1)
$(error GRALLOC_USE_ION_DMA_HEAP and GRALLOC_USE_ION_COMPOUND_PAGE_HEAP can't be enabled at the same time)
endif
endif

LOCAL_C_INCLUDES := $(MALI_LOCAL_PATH) $(MALI_DDK_INCLUDES) \
    $(TOP)/hardware/samsung_slsi-linaro/exynos/include

# General compilation flags
LOCAL_CFLAGS := -Werror -DLOG_TAG=\"gralloc\"

# ALOGV flag
LOCAL_CFLAGS += -DLOG_NDEBUG=$(MALI_GRALLOC_DISABLE_ALOGV)

# Static hw flags
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_BASIC=$(MALI_GPU_SUPPORT_AFBC_BASIC)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_SPLITBLK=$(MALI_GPU_SUPPORT_AFBC_SPLITBLK)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_WIDEBLK=$(MALI_GPU_SUPPORT_AFBC_WIDEBLK)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_TILED_HEADERS=$(MALI_GPU_SUPPORT_AFBC_TILED_HEADERS)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_YUV_WRITE=$(MALI_GPU_SUPPORT_AFBC_YUV_WRITE)

LOCAL_CFLAGS_arm64 += -DMALI_GRALLOC_GPU_LIBRARY_PATH1=$(MALI_GRALLOC_GPU_LIBRARY_64_PATH1)
LOCAL_CFLAGS_arm64 += -DMALI_GRALLOC_GPU_LIBRARY_PATH2=$(MALI_GRALLOC_GPU_LIBRARY_64_PATH2)
LOCAL_CFLAGS_arm64 += -DMALI_GRALLOC_DPU_LIBRARY_PATH=$(MALI_GRALLOC_DPU_LIBRARY_64_PATH)
LOCAL_CFLAGS_arm += -DMALI_GRALLOC_GPU_LIBRARY_PATH1=$(MALI_GRALLOC_GPU_LIBRARY_32_PATH1)
LOCAL_CFLAGS_arm += -DMALI_GRALLOC_GPU_LIBRARY_PATH2=$(MALI_GRALLOC_GPU_LIBRARY_32_PATH2)
LOCAL_CFLAGS_arm += -DMALI_GRALLOC_DPU_LIBRARY_PATH=$(MALI_GRALLOC_DPU_LIBRARY_32_PATH)
LOCAL_CFLAGS += -DMALI_GRALLOC_VPU_LIBRARY_PATH=$(MALI_GRALLOC_VPU_LIBRARY_PATH)

# Software behaviour flags
LOCAL_CFLAGS += -DGRALLOC_VERSION_MAJOR=1
LOCAL_CFLAGS += -DHIDL_ALLOCATOR_VERSION_SCALED=$(HIDL_ALLOCATOR_VERSION_SCALED)
LOCAL_CFLAGS += -DHIDL_MAPPER_VERSION_SCALED=$(HIDL_MAPPER_VERSION_SCALED)
LOCAL_CFLAGS += -DHIDL_COMMON_VERSION_SCALED=$(HIDL_COMMON_VERSION_SCALED)
LOCAL_CFLAGS += -DHIDL_IALLOCATOR_NAMESPACE=$(HIDL_IALLOCATOR_NAMESPACE)
LOCAL_CFLAGS += -DHIDL_IMAPPER_NAMESPACE=$(HIDL_IMAPPER_NAMESPACE)
LOCAL_CFLAGS += -DHIDL_COMMON_NAMESPACE=$(HIDL_COMMON_NAMESPACE)
LOCAL_CFLAGS += -DGRALLOC_DISP_W=$(GRALLOC_DISP_W)
LOCAL_CFLAGS += -DGRALLOC_DISP_H=$(GRALLOC_DISP_H)
LOCAL_CFLAGS += -DGRALLOC_AFBC_MIN_SIZE=$(GRALLOC_AFBC_MIN_SIZE)
LOCAL_CFLAGS += -DDISABLE_FRAMEBUFFER_HAL=$(GRALLOC_DISABLE_FRAMEBUFFER_HAL)
LOCAL_CFLAGS += -DGRALLOC_USE_ION_DMA_HEAP=$(GRALLOC_USE_ION_DMA_HEAP)
LOCAL_CFLAGS += -DGRALLOC_USE_ION_COMPOUND_PAGE_HEAP=$(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP)
LOCAL_CFLAGS += -DGRALLOC_INIT_AFBC=$(GRALLOC_INIT_AFBC)
LOCAL_CFLAGS += -DGRALLOC_FB_BPP=$(GRALLOC_FB_BPP)
LOCAL_CFLAGS += -DGRALLOC_FB_SWAP_RED_BLUE=$(GRALLOC_FB_SWAP_RED_BLUE)
LOCAL_CFLAGS += -DGRALLOC_ARM_NO_EXTERNAL_AFBC=$(GRALLOC_ARM_NO_EXTERNAL_AFBC)
LOCAL_CFLAGS += -DGRALLOC_LIBRARY_BUILD=1
LOCAL_CFLAGS += -DGRALLOC_USE_ION_DMABUF_SYNC=$(GRALLOC_USE_ION_DMABUF_SYNC)
LOCAL_CFLAGS += -DGRALLOC_IMPORT_BUFFER_REQUIRED=$(GRALLOC_IMPORT_BUFFER_REQUIRED)
LOCAL_CFLAGS += -DGRALLOC_SCALER_WFD=$(GRALLOC_SCALER_WFD)
LOCAL_CFLAGS += -DGRALLOC_HFR_BATCH_SIZE=$(GRALLOC_HFR_BATCH_SIZE)
LOCAL_CFLAGS += -DGRALLOC_USE_ASHMEM_METADATA=$(GRALLOC_USE_ASHMEM_METADATA)
LOCAL_CFLAGS += -DGRALLOC_ION_HANDLE_IMPORT=$(GRALLOC_ION_HANDLE_IMPORT)
LOCAL_CFLAGS += -DGRALLOC_DPU_SUPPORT_1010102_AFBC=$(GRALLOC_DPU_SUPPORT_1010102_AFBC)
LOCAL_CFLAGS += -DGRALLOC_SECURE_BUFFER_NO_AFBC=$(GRALLOC_SECURE_BUFFER_NO_AFBC)

ifeq ($(TARGET_SOC_BASE), exynos7885)
LOCAL_CFLAGS += -DGRALLOC_7885_WA
endif

ifeq ($(BOARD_USES_ALIGN_RESTRICTION), true)
LOCAL_CFLAGS += -DGRALLOC_MSCL_ALIGN_RESTRICTION
endif

ifeq ($(BOARD_EXYNOS_S10B_FORMAT_ALIGN), 64)
LOCAL_CFLAGS += -DBOARD_EXYNOS_S10B_FORMAT_ALIGN=$(BOARD_EXYNOS_S10B_FORMAT_ALIGN)
else
LOCAL_CFLAGS += -DBOARD_EXYNOS_S10B_FORMAT_ALIGN=16
endif

ifeq ($(BOARD_USES_GRALLOC_ION_SYNC), true)
LOCAL_CFLAGS += -DGRALLOC_ION_SYNC
endif

# HEVC Encoder H/W restriction
ifneq ($(filter exynos7885 exynos7872,$(TARGET_SOC_BASE)),)
LOCAL_CFLAGS += -DEXYNOS_CHROMA_VSTRIDE_ALIGN -DCHROMA_VALIGN=$(BOARD_MFC_CHROMA_VALIGN)
endif

ifneq ($(BOARD_USES_EXYNOS_DATASPACE_FEATURE), true)
LOCAL_CFLAGS += -DGRALLOC_FORCE_BT601
endif

LOCAL_SHARED_LIBRARIES := libhardware liblog libcutils libGLESv1_CM libion_exynos libsync libutils

LOCAL_SHARED_LIBRARIES += libnativewindow
LOCAL_STATIC_LIBRARIES := libarect
LOCAL_HEADER_LIBRARIES := libnativebase_headers

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
ifeq ($(TARGET_SOC),)
    LOCAL_MODULE := gralloc.default
else
    LOCAL_MODULE := gralloc.$(TARGET_SOC)
endif

LOCAL_MODULE_OWNER := arm
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

LOCAL_SRC_FILES := \
    gralloc_buffer_priv.cpp \
    mali_gralloc_bufferaccess.cpp \
    mali_gralloc_bufferallocation.cpp \
    mali_gralloc_bufferdescriptor.cpp \
    mali_gralloc_ion.cpp \
    mali_gralloc_formats.cpp \
    mali_gralloc_reference.cpp \
    mali_gralloc_debug.cpp \
    format_info.cpp

LOCAL_SRC_FILES += mali_gralloc_module.cpp \
                   framebuffer_device.cpp \
                   gralloc_vsync_${GRALLOC_VSYNC_BACKEND}.cpp \
                   mali_gralloc_public_interface.cpp \
                   mali_gralloc_private_interface.cpp
LOCAL_MODULE_OWNER := arm

include $(BUILD_SHARED_LIBRARY)

######### GrallocMapperCamera ############

include $(CLEAR_VARS)

# HAL module implemenation, not prelinked.
# Binary located in hw/gralloc.<ro.product.board>.so
include $(CLEAR_VARS)

# Include makefile which exports Gralloc Major and Minor version numbers
include $(LOCAL_PATH)/../gralloc.version.mk

# Include platform specific makefiles
include $(if $(wildcard $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk), $(LOCAL_PATH)/Android.$(TARGET_BOARD_PLATFORM).mk,)

MALI_GRALLOC_VPU_LIBRARY_PATH?=\"/system/lib/\"
MALI_GRALLOC_GPU_LIBRARY_64_PATH1 := \"/vendor/lib64/egl/\"
MALI_GRALLOC_GPU_LIBRARY_64_PATH2 := \"/system/lib64/egl/\"
MALI_GRALLOC_DPU_LIBRARY_64_PATH := \"/vendor/lib64/hw/\"
MALI_GRALLOC_GPU_LIBRARY_32_PATH1 := \"/vendor/lib/egl/\"
MALI_GRALLOC_GPU_LIBRARY_32_PATH2 := \"/system/lib/egl/\"
MALI_GRALLOC_DPU_LIBRARY_32_PATH := \"/vendor/lib/hw/\"

# 2.0+ specific.
# If set to the default value of zero then build the allocator. Otherwise build the mapper.
# The top-level make file builds both.
GRALLOC_MAPPER?=0

#
# Static hardware defines
#
# These defines are used in case runtime detection does not find the
# user-space driver to read out hardware capabilities

# GPU support for AFBC 1.0
MALI_GPU_SUPPORT_AFBC_BASIC?=1
# GPU support for AFBC 1.1 block split
MALI_GPU_SUPPORT_AFBC_SPLITBLK?=0
# GPU support for AFBC 1.1 wide block
MALI_GPU_SUPPORT_AFBC_WIDEBLK?=0
# GPU support for AFBC 1.2 tiled headers
MALI_GPU_SUPPORT_AFBC_TILED_HEADERS?=0
# GPU support for writing AFBC YUV formats
MALI_GPU_SUPPORT_AFBC_YUV_WRITE?=0

#
# Software behaviour defines
#

# The following defines are used to override default behaviour of which heap is selected for allocations.
# The default is to pick system heap.
# The following two defines enable either DMA heap or compound page heap for when the usage has
# GRALLOC_USAGE_HW_FB or GRALLOC_USAGE_HW_COMPOSER set and GRALLOC_USAGE_HW_VIDEO_ENCODER is not set.
# These defines should not be enabled at the same time.
GRALLOC_USE_ION_DMA_HEAP?=0
GRALLOC_USE_ION_COMPOUND_PAGE_HEAP?=0

# fbdev bitdepth to use
GRALLOC_FB_BPP?=32
# When enabled, forces display framebuffer format to BGRA_8888
GRALLOC_FB_SWAP_RED_BLUE?=1
# Disables the framebuffer HAL device. When a hwc impl is available.
GRALLOC_DISABLE_FRAMEBUFFER_HAL?=1

# When enabled, buffers will never be allocated with AFBC
GRALLOC_ARM_NO_EXTERNAL_AFBC?=1
# Properly initializes an empty AFBC buffer
GRALLOC_INIT_AFBC?=0

ifeq ($(BOARD_USES_EXYNOS_AFBC_FEATURE), true)
GRALLOC_ARM_NO_EXTERNAL_AFBC:=0
GRALLOC_INIT_AFBC:=1
endif

# Minimum buffer dimensions in pixels when buffer will use AFBC
GRALLOC_DISP_W?=1080
GRALLOC_DISP_H?=1920
GRALLOC_AFBC_MIN_SIZE?=75

ifdef BOARD_GRALLOC_AFBC_DISP_W
GRALLOC_DISP_W:=$(BOARD_GRALLOC_AFBC_DISP_W)
endif
ifdef BOARD_GRALLOC_AFBC_DISP_H
GRALLOC_DISP_H:=$(BOARD_GRALLOC_AFBC_DISP_H)
endif
ifdef BOARD_GRALLOC_AFBC_MIN_SIZE
GRALLOC_AFBC_MIN_SIZE := $(BOARD_GRALLOC_AFBC_MIN_SIZE)
endif

# Vsync backend(not used)
GRALLOC_VSYNC_BACKEND?=default

GRALLOC_USE_ION_DMABUF_SYNC?=1

MALI_GRALLOC_DISABLE_ALOGV?=1

# Set 1 to require importBuffer before locking raw buffers
GRALLOC_IMPORT_BUFFER_REQUIRED?=0

# Used for WFD on exynos3830
ifeq ($(TARGET_SOC_BASE), exynos3830)
GRALLOC_SCALER_WFD:=1
else
GRALLOC_SCALER_WFD:=0
endif

# Disable ION debug handle import on 7885
ifneq ($(TARGET_SOC), exynos7885)
GRALLOC_ION_HANDLE_IMPORT:=1
else
GRALLOC_ION_HANDLE_IMPORT:=0
endif

GRALLOC_USE_ASHMEM_METADATA:=0

GRALLOC_HFR_BATCH_SIZE?=8

ifeq ($(GRALLOC_USE_ION_DMA_HEAP), 1)
ifeq ($(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP), 1)
$(error GRALLOC_USE_ION_DMA_HEAP and GRALLOC_USE_ION_COMPOUND_PAGE_HEAP can't be enabled at the same time)
endif
endif

LOCAL_C_INCLUDES := $(MALI_LOCAL_PATH) $(MALI_DDK_INCLUDES) \
    $(TOP)/hardware/samsung_slsi-linaro/exynos/include

# General compilation flags
LOCAL_CFLAGS := -Werror -DLOG_TAG=\"gralloc\"

# ALOGV flag
LOCAL_CFLAGS += -DLOG_NDEBUG=$(MALI_GRALLOC_DISABLE_ALOGV)

# Static hw flags
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_BASIC=$(MALI_GPU_SUPPORT_AFBC_BASIC)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_SPLITBLK=$(MALI_GPU_SUPPORT_AFBC_SPLITBLK)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_WIDEBLK=$(MALI_GPU_SUPPORT_AFBC_WIDEBLK)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_TILED_HEADERS=$(MALI_GPU_SUPPORT_AFBC_TILED_HEADERS)
LOCAL_CFLAGS += -DMALI_GPU_SUPPORT_AFBC_YUV_WRITE=$(MALI_GPU_SUPPORT_AFBC_YUV_WRITE)

LOCAL_CFLAGS_arm64 += -DMALI_GRALLOC_GPU_LIBRARY_PATH1=$(MALI_GRALLOC_GPU_LIBRARY_64_PATH1)
LOCAL_CFLAGS_arm64 += -DMALI_GRALLOC_GPU_LIBRARY_PATH2=$(MALI_GRALLOC_GPU_LIBRARY_64_PATH2)
LOCAL_CFLAGS_arm64 += -DMALI_GRALLOC_DPU_LIBRARY_PATH=$(MALI_GRALLOC_DPU_LIBRARY_64_PATH)
LOCAL_CFLAGS_arm += -DMALI_GRALLOC_GPU_LIBRARY_PATH1=$(MALI_GRALLOC_GPU_LIBRARY_32_PATH1)
LOCAL_CFLAGS_arm += -DMALI_GRALLOC_GPU_LIBRARY_PATH2=$(MALI_GRALLOC_GPU_LIBRARY_32_PATH2)
LOCAL_CFLAGS_arm += -DMALI_GRALLOC_DPU_LIBRARY_PATH=$(MALI_GRALLOC_DPU_LIBRARY_32_PATH)
LOCAL_CFLAGS += -DMALI_GRALLOC_VPU_LIBRARY_PATH=$(MALI_GRALLOC_VPU_LIBRARY_PATH)

# Software behaviour flags
LOCAL_CFLAGS += -DGRALLOC_VERSION_MAJOR=1
LOCAL_CFLAGS += -DHIDL_ALLOCATOR_VERSION_SCALED=$(HIDL_ALLOCATOR_VERSION_SCALED)
LOCAL_CFLAGS += -DHIDL_MAPPER_VERSION_SCALED=$(HIDL_MAPPER_VERSION_SCALED)
LOCAL_CFLAGS += -DHIDL_COMMON_VERSION_SCALED=$(HIDL_COMMON_VERSION_SCALED)
LOCAL_CFLAGS += -DHIDL_IALLOCATOR_NAMESPACE=$(HIDL_IALLOCATOR_NAMESPACE)
LOCAL_CFLAGS += -DHIDL_IMAPPER_NAMESPACE=$(HIDL_IMAPPER_NAMESPACE)
LOCAL_CFLAGS += -DHIDL_COMMON_NAMESPACE=$(HIDL_COMMON_NAMESPACE)
LOCAL_CFLAGS += -DGRALLOC_DISP_W=$(GRALLOC_DISP_W)
LOCAL_CFLAGS += -DGRALLOC_DISP_H=$(GRALLOC_DISP_H)
LOCAL_CFLAGS += -DGRALLOC_AFBC_MIN_SIZE=$(GRALLOC_AFBC_MIN_SIZE)
LOCAL_CFLAGS += -DDISABLE_FRAMEBUFFER_HAL=$(GRALLOC_DISABLE_FRAMEBUFFER_HAL)
LOCAL_CFLAGS += -DGRALLOC_USE_ION_DMA_HEAP=$(GRALLOC_USE_ION_DMA_HEAP)
LOCAL_CFLAGS += -DGRALLOC_USE_ION_COMPOUND_PAGE_HEAP=$(GRALLOC_USE_ION_COMPOUND_PAGE_HEAP)
LOCAL_CFLAGS += -DGRALLOC_INIT_AFBC=$(GRALLOC_INIT_AFBC)
LOCAL_CFLAGS += -DGRALLOC_FB_BPP=$(GRALLOC_FB_BPP)
LOCAL_CFLAGS += -DGRALLOC_FB_SWAP_RED_BLUE=$(GRALLOC_FB_SWAP_RED_BLUE)
LOCAL_CFLAGS += -DGRALLOC_ARM_NO_EXTERNAL_AFBC=$(GRALLOC_ARM_NO_EXTERNAL_AFBC)
LOCAL_CFLAGS += -DGRALLOC_LIBRARY_BUILD=1
LOCAL_CFLAGS += -DGRALLOC_USE_ION_DMABUF_SYNC=$(GRALLOC_USE_ION_DMABUF_SYNC)
LOCAL_CFLAGS += -DGRALLOC_IMPORT_BUFFER_REQUIRED=$(GRALLOC_IMPORT_BUFFER_REQUIRED)
LOCAL_CFLAGS += -DGRALLOC_SCALER_WFD=$(GRALLOC_SCALER_WFD)
LOCAL_CFLAGS += -DGRALLOC_HFR_BATCH_SIZE=$(GRALLOC_HFR_BATCH_SIZE)
LOCAL_CFLAGS += -DGRALLOC_USE_ASHMEM_METADATA=$(GRALLOC_USE_ASHMEM_METADATA)
LOCAL_CFLAGS += -DGRALLOC_ION_HANDLE_IMPORT=$(GRALLOC_ION_HANDLE_IMPORT)

ifeq ($(TARGET_SOC_BASE), exynos7885)
LOCAL_CFLAGS += -DGRALLOC_7885_WA
endif

ifeq ($(BOARD_USES_ALIGN_RESTRICTION), true)
LOCAL_CFLAGS += -DGRALLOC_MSCL_ALIGN_RESTRICTION
endif

ifeq ($(BOARD_EXYNOS_S10B_FORMAT_ALIGN), 64)
LOCAL_CFLAGS += -DBOARD_EXYNOS_S10B_FORMAT_ALIGN=$(BOARD_EXYNOS_S10B_FORMAT_ALIGN)
else
LOCAL_CFLAGS += -DBOARD_EXYNOS_S10B_FORMAT_ALIGN=16
endif

ifeq ($(BOARD_USES_GRALLOC_ION_SYNC), true)
LOCAL_CFLAGS += -DGRALLOC_ION_SYNC
endif

# HEVC Encoder H/W restriction
ifneq ($(filter exynos7885 exynos7872,$(TARGET_SOC_BASE)),)
LOCAL_CFLAGS += -DEXYNOS_CHROMA_VSTRIDE_ALIGN -DCHROMA_VALIGN=$(BOARD_MFC_CHROMA_VALIGN)
endif

# Kernel 3.18 ion allocation is different comapred to higer kernel version
ifneq ($(filter exynos7870 exynos7570,$(TARGET_SOC_BASE)),)
LOCAL_CFLAGS += -DGRALLOC_7570_7870_WA
endif

ifneq ($(BOARD_USES_EXYNOS_DATASPACE_FEATURE), true)
LOCAL_CFLAGS += -DGRALLOC_FORCE_BT601
endif

LOCAL_SHARED_LIBRARIES := libhardware liblog libcutils libGLESv1_CM libion_exynos libsync libutils

LOCAL_SHARED_LIBRARIES += android.hardware.graphics.mapper@2.0

LOCAL_SHARED_LIBRARIES += libnativewindow
LOCAL_STATIC_LIBRARIES := libarect
LOCAL_HEADER_LIBRARIES := libnativebase_headers

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64

LOCAL_MODULE := libGrallocMapperCamera

LOCAL_MODULE_OWNER := samsung_arm
LOCAL_PROPRIETARY_MODULE := true

LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

#GRALLOC_INIT_AFBC := 0

LOCAL_SRC_FILES := \
	mali_gralloc_bufferaccess.cpp \
	GrallocMapperCamera.cpp \
	format_info.cpp \
	mali_gralloc_ion.cpp \
	mali_gralloc_formats.cpp \
	mali_gralloc_bufferallocation.cpp \
	gralloc_buffer_priv.cpp \
	mali_gralloc_debug.cpp

include $(BUILD_SHARED_LIBRARY)
