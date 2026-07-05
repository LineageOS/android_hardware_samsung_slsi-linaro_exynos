/*
 * Copyright (C) 2021 Samsung Electronics Co. Ltd.
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

#include <utility>
#include <unordered_map>
#include <android/binder_manager.h>
#include <android/binder_auto_utils.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <log/log.h>
#include <sys/mman.h>
#include <utils/Trace.h>
#include <vndk/hardware_buffer.h>
#include <android-base/properties.h>

#include <VendorVideoAPI.h>
#include <hardware/exynos/sbwcdecoder.h>
#include <aidl/vendor/samsung_slsi/hardware/SbwcDecompService/ISbwcDecompService.h>

#include "SBWCHelper.h"
#include "exynos_format.h"
#include "ExynosGraphicBufferCore.h"

using namespace aidl::vendor::samsung_slsi::hardware::SbwcDecompService;
using namespace vendor::graphics;

namespace SBWCHelper
{

static std::unordered_map<AHardwareBuffer*, AHardwareBuffer*> yuvToSbwc;
static std::unordered_map<AHardwareBuffer*, AHardwareBuffer*> sbwcToYuv;
static std::unordered_map<AHardwareBuffer*, int32_t> ref;

static buffer_handle_t lastSrc, lastDst;

static bool debugEnabled = android::base::GetBoolProperty("vendor.sbwchelper.debug.enabled", false);
static bool traceEnabled = android::base::GetBoolProperty("vendor.sbwchelper.trace.enabled", false);

/** Check format is 10bit or not
 *
 * @param[in] format exynos HAL format
 * @return boolean
 */
static bool is10Bit(uint32_t format);

/** Allocate YUV AHardwareBuffer used SBWC AHardwareBuffer's information
 *
 * @param[in] inSbwcAHB SBWC AHardwareBuffer
 * @param[out] outYuvAHB YUV AHardwareaBuffer
 * @return result of AHardwareBuffer_allocate()
 */
static int64_t allocAHB(AHardwareBuffer *inSbwcAHB, AHardwareBuffer **outYuvAHB);

/** Get attribute for SbwcDecompService
 *
 * @param[in] handle native handle used to set attribute
 * @return attributes
 */
static uint32_t getAttr(const native_handle_t *handle);

/** Request decompress to SbwcDecompService
 *
 * @param[in] inYuvAHB YUV AHardwareBuffer will be stored decompressed data
 * @param[in] inSbwcAHB SBWC AHardwareBuffer has source data
 * @return success or fail
 */
static bool requestDecompress(AHardwareBuffer *inYuvAHB, AHardwareBuffer *inSbwcAHB);

/** To check whether this buffer has real SBWC data
 * @param[in] buffer handle
 * @return whether SBWC or not
 */
static bool isRealSbwc(buffer_handle_t handle);

/** Get GraphicBuffer's id from ExynosGraphicBuffer
 *
 * @param[in] AHardwareBuffer will be used to get id
 * @return id
 */
static inline uint64_t getId(const AHardwareBuffer *inAHB)
{
	const native_handle_t *handle = AHardwareBuffer_getNativeHandle(inAHB);
	if (handle == nullptr)
	{
		ALOGE("[SBWC] %s: handle is null AHB: %p", __func__, inAHB);
		return -1;
	}
	return ExynosGraphicBufferMeta::get_buffer_id(handle);
}

//---------------------------------------------------------------------------------------
// Public functions
//---------------------------------------------------------------------------------------

bool isSbwcFormat(AHardwareBuffer *inAHB)
{
	uint32_t format;
	bool ret = false;

	if (inAHB == nullptr) {
		return false;
	}

	const native_handle_t *handle = AHardwareBuffer_getNativeHandle(inAHB);

	if (handle == nullptr) {
		ALOGD("[SBWC] %s: null handle AHB: %p", __func__, inAHB);
		return false;
	}

	format = ExynosGraphicBufferMeta::get_format(handle);

	if ((HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC <= format)
			&& (format <= HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80))
	{
		ret = true;
	}

	if (debugEnabled)
	{
		uint64_t id = getId(inAHB);
		ALOGD("[SBWC] %s: AHB: %p, handle: %p id: %" PRIu64 " format: 0x%" PRIX32 " ret: %s",
				__func__, inAHB, handle, id, format, (ret) ? "true" : "false");
	}

	return ret;
}

bool ContainsSbwcData(AHardwareBuffer *inAHB)
{
	uint32_t format;
	bool ret = false;

	if (inAHB == nullptr) {
		return false;
	}

	const native_handle_t *handle = AHardwareBuffer_getNativeHandle(inAHB);

	if (handle == nullptr) {
		ALOGD("[SBWC] %s: null handle AHB: %p", __func__, inAHB);
		return false;
	}

	format = ExynosGraphicBufferMeta::get_format(handle);

	if (((HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC <= format)
			&& (format <= HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80))
		&& isRealSbwc(handle))
	{
		ret = true;
	}

	if (debugEnabled)
	{
		uint64_t id = getId(inAHB);
		ALOGD("[SBWC] %s: AHB: %p, handle: %p id: %" PRIu64 " format: 0x%" PRIX32 " ret: %s",
				__func__, inAHB, handle, id, format, (ret) ? "true" : "false");
	}

	return ret;
}

bool newYuvAHB(AHardwareBuffer *inSbwcAHB, AHardwareBuffer **outYuvAHB)
{
	AHardwareBuffer *yuvAHB = nullptr;
	int64_t result = android::NO_ERROR;

	if (traceEnabled)
	{
		ATRACE_CALL();
	}

	if (inSbwcAHB == nullptr || (!isSbwcFormat(inSbwcAHB)))
	{
		ALOGE("[SBWC] %s: Invalid value \"%s\" %s:%d",
				__func__,
				(inSbwcAHB == nullptr) ? "Null pointer" : "No SBWC",
				__FILE__,
				__LINE__);
		return false;
	}

	if (sbwcToYuv.count(inSbwcAHB) > 0)
	{
		AHardwareBuffer *yuvAHB = sbwcToYuv.at(inSbwcAHB);

		*outYuvAHB = yuvAHB;

		ref.at(yuvAHB) += 1;

		if (debugEnabled)
		{
			ALOGD("[SBWC] %s: Increase ref counter AHB: %p ref: %" PRId32 "", __func__, yuvAHB, ref.at(yuvAHB));
		}

		AHardwareBuffer_acquire(inSbwcAHB);

		return true;
	}

	// Allocate AHB per every request now
	// To do: Make more efficiently
	result = allocAHB(inSbwcAHB, &yuvAHB);

	if (result != android::NO_ERROR)
	{
		ALOGE("[SBWC] %s: \"YUV allocation failed\" %s:%d",
				__func__, __FILE__, __LINE__);
		return false;
	}

	yuvToSbwc.insert({yuvAHB, inSbwcAHB});
	sbwcToYuv.insert({inSbwcAHB, yuvAHB});

	ref.insert({yuvAHB, 1});

	*outYuvAHB = yuvAHB;

	if (debugEnabled)
	{
		const native_handle_t *handle = AHardwareBuffer_getNativeHandle(inSbwcAHB);
		uint32_t format = ExynosGraphicBufferMeta::get_format(handle);

		ALOGD("[SBWC] %s: inSbwcAHB: %p format: 0x%" PRIX32 "",
				__func__, inSbwcAHB, format);

		handle = AHardwareBuffer_getNativeHandle(*outYuvAHB);
		format = ExynosGraphicBufferMeta::get_format(handle);

		ALOGD("[SBWC] %s: outYuvAHB: %p format: 0x%" PRIX32 "",
				__func__, yuvAHB, format);
	}

	AHardwareBuffer_acquire(inSbwcAHB);

	return true;
}

bool decompress(AHardwareBuffer *inSbwcAHB)
{
	if (traceEnabled)
	{
		ATRACE_CALL();
	}

	if (inSbwcAHB == nullptr)
	{
		ALOGE("[SBWC] %s: Invalid value \"%s\" %s:%d",
				__func__,
				"Null pointer",
				__FILE__,
				__LINE__);
		return false;
	}

	if (debugEnabled)
	{
		ALOGD("[SBWC] %s: inSbwcAHB: %p", __func__, inSbwcAHB);
	}

	if (sbwcToYuv.count(inSbwcAHB) <= 0)
	{
		ALOGE("[SBWC] %s: Invalid value \"Not registered SBWC AHB\" AHB: %p %s:%d",
				__func__, inSbwcAHB, __FILE__, __LINE__);
		return false;
	}

	AHardwareBuffer *yuvAHB = sbwcToYuv.at(inSbwcAHB);

	return requestDecompress(yuvAHB, inSbwcAHB);
}

// To do: Deferred free
bool freeYuvAHB(AHardwareBuffer **inYuvAHB)
{
	AHardwareBuffer *sbwcAHB;

	if (traceEnabled)
	{
		ATRACE_CALL();
	}

	if (inYuvAHB == nullptr || *inYuvAHB == nullptr)
	{
		ALOGE("[SBWC] %s: Invalid value \"%s\" %s:%d",
				__func__,
				(inYuvAHB == nullptr) || (*inYuvAHB == nullptr) ? "Null pointer" : "No YUV",
				__FILE__,
				__LINE__);
		return false;
	}

	if ((yuvToSbwc.count(*inYuvAHB) == 0) || (ref.at(*inYuvAHB) <= 0))
	{
		ALOGE("[SBWC] %s: Invalid value \"Not registered YUV AHB\" %s:%d",
				__func__, __FILE__, __LINE__);
		return false;
	}
	// Multiple referenced
	else if (ref.at(*inYuvAHB) > 1)
	{
		ref.at(*inYuvAHB) -= 1;

		if (debugEnabled)
		{
			ALOGD("[SBWC] %s: Decrease ref counter AHB: %p ref: %" PRId32 "", __func__, *inYuvAHB, ref.at(*inYuvAHB));
		}

		sbwcAHB = yuvToSbwc.at(*inYuvAHB);
		AHardwareBuffer_release(sbwcAHB);

		return true;
	}

	sbwcAHB = yuvToSbwc.at(*inYuvAHB);

	sbwcToYuv.erase(sbwcAHB);
	yuvToSbwc.erase(*inYuvAHB);

	if (debugEnabled)
	{
		ALOGD("[SBWC] %s: Deleted SBWC AHB: %p",
				__func__, sbwcAHB);

		ALOGD("[SBWC] %s: Deleted YUV AHB: %p",
				__func__, *inYuvAHB);
	}

	AHardwareBuffer_release(*inYuvAHB);
	AHardwareBuffer_release(sbwcAHB);

	*inYuvAHB = nullptr;

	if ((sbwcToYuv.size() == 0) && (yuvToSbwc.size() == 0)) {
		lastSrc = lastDst = 0;
	}

	return true;
}

uint32_t getByteStride(uint32_t format, uint32_t alloc_width, [[maybe_unused]] uint32_t plane)
{
	uint32_t bytesPerPixel = 0;

	switch (format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
			bytesPerPixel = 1;
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
			bytesPerPixel = 2;
			break;
	}

	uint32_t unaligned_stride = alloc_width * bytesPerPixel;
	constexpr uint32_t byteAlign = 0xff;
	uint32_t aligned_stride = (unaligned_stride + byteAlign) & ~byteAlign;

	return aligned_stride;
}

//---------------------------------------------------------------------------------------
// DEPRECATED
//---------------------------------------------------------------------------------------

AHardwareBuffer *newYuvAHB(AHardwareBuffer *sbwcAHB)
{
	AHardwareBuffer *yuvAHB = nullptr;
	int64_t result = android::NO_ERROR;

	if (sbwcAHB == nullptr || (!isSbwcFormat(sbwcAHB)))
	{
		ALOGE("[SBWC] %s: Invalid value \"%s\" %s:%d",
				__func__,
				(sbwcAHB == nullptr) ? "Null pointer" : "No SBWC",
				__FILE__,
				__LINE__);
		return nullptr;
	}

	if (sbwcToYuv.count(sbwcAHB) > 0)
	{
		AHardwareBuffer *yuvAHB = sbwcToYuv.at(sbwcAHB);

		ref.at(yuvAHB) += 1;

		if (debugEnabled)
		{
			ALOGD("[SBWC] %s: Increase ref counter AHB: %p ref: %" PRId32 "", __func__, yuvAHB, ref.at(yuvAHB));
		}

		AHardwareBuffer_acquire(sbwcAHB);

		return yuvAHB;
	}

	// Allocate AHB per every request now
	// To do: Make more efficiently
	result = allocAHB(sbwcAHB, &yuvAHB);

	if (result != android::NO_ERROR)
	{
		ALOGE("[SBWC] %s: \"YUV allocation failed\" %s:%d",
				__func__, __FILE__, __LINE__);
		return nullptr;
	}

	yuvToSbwc.insert({yuvAHB, sbwcAHB});
	sbwcToYuv.insert({sbwcAHB, yuvAHB});

	ref.insert({yuvAHB, 1});

	if (debugEnabled)
	{
		const native_handle_t *handle = AHardwareBuffer_getNativeHandle(sbwcAHB);
		uint32_t format = ExynosGraphicBufferMeta::get_format(handle);

		ALOGD("[SBWC] %s: inSbwcAHB: %p format: 0x%" PRIX32 "",
				__func__, sbwcAHB, format);

		handle = AHardwareBuffer_getNativeHandle(yuvAHB);
		format = ExynosGraphicBufferMeta::get_format(handle);

		ALOGD("[SBWC] %s: outYuvAHB: %p format: 0x%" PRIX32 "",
				__func__, yuvAHB, format);
	}

	AHardwareBuffer_acquire(sbwcAHB);

	return yuvAHB;
}

bool freeYuvAHB(AHardwareBuffer *yuvAHB)
{
	AHardwareBuffer *sbwcAHB;

	if (yuvAHB == nullptr)
	{
		ALOGE("[SBWC] %s: Invalid value \"%s\" %s:%d",
				__func__,
				(yuvAHB == nullptr) || (yuvAHB == nullptr) ? "Null pointer" : "No YUV",
				__FILE__,
				__LINE__);
		return false;
	}

	if ((yuvToSbwc.count(yuvAHB) == 0) || (ref.at(yuvAHB) <= 0))
	{
		ALOGE("[SBWC] %s: Invalid value \"Not registered YUV AHB: %p\" %s:%d",
				__func__, yuvAHB, __FILE__, __LINE__);
		return false;
	}
	// Multiple referenced
	else if (ref.at(yuvAHB) > 1)
	{
		ref.at(yuvAHB) -= 1;

		if (debugEnabled)
		{
			ALOGD("[SBWC] %s: Decrease ref counter AHB: %p ref: %" PRId32 "", __func__, yuvAHB, ref.at(yuvAHB));
		}

		sbwcAHB = yuvToSbwc.at(yuvAHB);
		AHardwareBuffer_release(sbwcAHB);

		return true;
	}

	sbwcAHB = yuvToSbwc.at(yuvAHB);

	sbwcToYuv.erase(sbwcAHB);
	yuvToSbwc.erase(yuvAHB);

	if (debugEnabled)
	{
		ALOGD("[SBWC] %s: Deleted SBWC AHB: %p", __func__, sbwcAHB);

		ALOGD("[SBWC] %s: Deleted YUV AHB: %p", __func__, yuvAHB);
	}

	AHardwareBuffer_release(yuvAHB);
	AHardwareBuffer_release(sbwcAHB);

	if ((sbwcToYuv.size() == 0) && (yuvToSbwc.size() == 0)) {
		lastSrc = lastDst = 0;
	}

	return true;
}

//---------------------------------------------------------------------------------------
// Private functions
//---------------------------------------------------------------------------------------

static bool isRealSbwc(buffer_handle_t handle)
{
	int metaDataFd = ExynosGraphicBufferMeta::get_video_metadata_fd(handle);
	int nPixelFormat = 0;

	if (metaDataFd < 0)
		return 0;

	ExynosVideoMeta *metaData = static_cast<ExynosVideoMeta*>(mmap(0,
		sizeof(*metaData), PROT_READ | PROT_WRITE, MAP_SHARED, metaDataFd, 0));

	if (!metaData) {
		ALOGE("[SBWC] Failed to mmap to ExynosVideoMeta");
		return -ENOMEM;
	}

	nPixelFormat = metaData->nPixelFormat;

	munmap(metaData, sizeof(*metaData));

	return !nPixelFormat;
}

static bool is10Bit(uint32_t format)
{
	switch(format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80:
			return true;
		default:
			return false;
        }
}

static int64_t allocAHB(AHardwareBuffer *inSbwcAHB, AHardwareBuffer **outYuvAHB)
{
	const native_handle_t *handle = AHardwareBuffer_getNativeHandle(inSbwcAHB);
	AHardwareBuffer_Desc desc;

	desc.width = ExynosGraphicBufferMeta::get_width(handle);
	desc.height = ExynosGraphicBufferMeta::get_height(handle);
	desc.layers = 1;
	desc.rfu0 = 0;
	desc.rfu1 = 0;
	desc.usage = ExynosGraphicBufferMeta::get_usage(handle);
	desc.format = AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420;

	switch (ExynosGraphicBufferMeta::get_format(handle)) {
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
			desc.format = HAL_PIXEL_FORMAT_YCBCR_P010; // 0x36
	}

	/*
	 * AHardwareBuffer allocation allowed YUV format as AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420 only
	 * So we need to set 10bit flag to allocate 10bit format
	 */
	desc.usage |= is10Bit(ExynosGraphicBufferMeta::get_format(handle)) ? SBWC_REQUEST_10BIT : 0;

	return AHardwareBuffer_allocate(&desc, outYuvAHB);
}

static uint32_t getAttr(const native_handle_t *handle)
{
	uint32_t attr = 0;
	uint64_t usage = ExynosGraphicBufferMeta::get_usage(handle);

	// To do: Use function from libexynosgraphicbuffer_core instead magic number
	if (usage & (1ULL << 14))
	{
		attr |= SBWCDECODER_ATTR_SECURE_BUFFER;
	}

	return attr;
}

static bool requestDecompress(AHardwareBuffer *inYuvAHB, AHardwareBuffer *inSbwcAHB)
{
	const native_handle_t *yuvHandle = AHardwareBuffer_getNativeHandle(inYuvAHB);
	const native_handle_t *sbwcHandle = AHardwareBuffer_getNativeHandle(inSbwcAHB);

	static std::shared_ptr<ISbwcDecompService> sbwcDecompService = nullptr; 

	if ((lastSrc == yuvHandle) && (lastDst == sbwcHandle)) {
		if (debugEnabled) {
			ALOGD("[SBWC] Skip decompress because same request");
		}
		return true;
	}

	if (sbwcDecompService == nullptr)
	{
		const std::string instance = std::string() + ISbwcDecompService::descriptor + "/default";
		sbwcDecompService = ISbwcDecompService::fromBinder(ndk::SpAIBinder(AServiceManager_waitForService(instance.c_str())));
		if (sbwcDecompService == nullptr)
		{
			ALOGE("[SBWC] %s: \"SbwcDecompService getting failed\" %s:%d",
					__func__, __FILE__, __LINE__);
			return false;
		}
	}

    aidl::android::hardware::common::NativeHandle sbwcAidlHandle =
            ::android::dupToAidl(sbwcHandle);

    aidl::android::hardware::common::NativeHandle yuvAidlHandle =
            ::android::dupToAidl(yuvHandle);

	int32_t attr = getAttr(yuvHandle);
	int32_t result = android::NO_ERROR;
	ndk::ScopedAStatus status = sbwcDecompService->decode(sbwcAidlHandle, yuvAidlHandle, attr, &result);

	if (!status.isOk() || result != android::NO_ERROR)
	{
		ALOGE("[SBWC] %s: \"SbwcDecompService decompression failed\" %s:%d",
					__func__, __FILE__, __LINE__);
		lastSrc = lastDst = 0;
		return false;
	}

	lastSrc = yuvHandle;
	lastDst = sbwcHandle;

	if (debugEnabled)
	{
		const native_handle_t *handle = AHardwareBuffer_getNativeHandle(inSbwcAHB);
		uint32_t format = ExynosGraphicBufferMeta::get_format(handle);

		uint64_t yuvId;
		uint64_t sbwcId;

		yuvId = getId(inYuvAHB);
		sbwcId = getId(inSbwcAHB);

		ALOGD("[SBWC] %s: inSbwcAHB: %p handle: %p format: 0x%" PRIX32 " id: %" PRIu64 "",
				__func__, inSbwcAHB, sbwcHandle, format, sbwcId);

		handle = AHardwareBuffer_getNativeHandle(inYuvAHB);
		format = ExynosGraphicBufferMeta::get_format(handle);

		ALOGD("[SBWC] %s: outYuvAHB: %p handle: %p format: 0x%" PRIX32 " id: %" PRIu64 "",
				__func__, inYuvAHB, yuvHandle, format, yuvId);

		ALOGD("[SBWC] %s: result: %s", __func__, (result == android::NO_ERROR) ? "success" : "failed" );
	}

	return true;
}

} // namespace SBWCHelper
