/*
 * Copyright (C) 2013 The Android Open Source Project
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

#include <hardware/fb.h>

#include "gralloc_helper.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_reference.h"
#include "mali_gralloc_bufferaccess.h"

/*****************************************************************************/
struct gralloc_context_t {
    gralloc1_device_t  device;
    /* our private data here */
};

typedef struct mali_gralloc_func
{
	gralloc1_function_descriptor_t desc;
	gralloc1_function_pointer_t func;
} mali_gralloc_func;

/*****************************************************************************/

int fb_device_open(const hw_module_t* module, const char* name, hw_device_t** device);

static int gralloc_close(struct hw_device_t *dev)
{
    gralloc_context_t* ctx = reinterpret_cast<gralloc_context_t*>(dev);
    if (ctx) {
        /* TODO: keep a list of all buffer_handle_t created, and free them
         * all here.
         */
        free(ctx);
    }
    return 0;
}

static void mali_gralloc_dump(gralloc1_device_t* device,
	uint32_t* outSize, char* outBuffer)
{
	GRALLOC_UNUSED(device);
	GRALLOC_UNUSED(outSize);
	GRALLOC_UNUSED(outBuffer);
}

static int32_t mali_gralloc_create_descriptor(gralloc1_device_t* device,
	gralloc1_buffer_descriptor_t* outDescriptor)
{
	int ret = 0;
	ret = mali_gralloc_create_descriptor_internal(outDescriptor);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_destroy_descriptor(gralloc1_device_t* device,
	gralloc1_buffer_descriptor_t descriptor)
{
	int ret = 0;
	ret = mali_gralloc_destroy_descriptor_internal(descriptor);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_consumer_usage(gralloc1_device_t* device,
	gralloc1_buffer_descriptor_t descriptor, /*uint64_t */gralloc1_consumer_usage_t usage)
{
	int ret = 0;
	ret = mali_gralloc_set_consumerusage_internal(descriptor, usage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_dimensions(gralloc1_device_t* device,
	gralloc1_buffer_descriptor_t descriptor, uint32_t width, uint32_t height)
{
	int ret = 0;
	ret = mali_gralloc_set_dimensions_internal(descriptor, width, height);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_format(gralloc1_device_t* device,
	gralloc1_buffer_descriptor_t descriptor, /*int32_t*/ android_pixel_format_t format)
{
	int ret = 0;
	ret = mali_gralloc_set_format_internal(descriptor, format);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_set_producer_usage(gralloc1_device_t* device,
	gralloc1_buffer_descriptor_t descriptor, /*uint64_t */gralloc1_producer_usage_t usage)
{
	int ret = 0;
	ret = mali_gralloc_set_producerusage_internal(descriptor, usage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_backing_store(gralloc1_device_t* device,
	buffer_handle_t buffer, gralloc1_backing_store_t* outStore)
{
	int ret = 0;
	ret = mali_gralloc_get_backing_store_internal(buffer, outStore);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_consumer_usage(gralloc1_device_t* device,
	buffer_handle_t buffer, uint64_t* /*gralloc1_consumer_usage_t*/ outUsage)
{
	int ret = 0;
	ret = mali_gralloc_get_consumer_usage_internal(buffer, outUsage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_dimensions(gralloc1_device_t* device,
	buffer_handle_t buffer, uint32_t* outWidth, uint32_t* outHeight)
{
	int ret = 0;
	ret = mali_gralloc_get_dimensions_internal(buffer, outWidth, outHeight);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_format(gralloc1_device_t* device,
	buffer_handle_t buffer, int32_t* outFormat)
{
	int ret = 0;
	ret = mali_gralloc_get_format_internal(buffer, outFormat);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_producer_usage(gralloc1_device_t* device,
	buffer_handle_t buffer, uint64_t* /*gralloc1_producer_usage_t*/ outUsage)
{
	int ret = 0;
	ret = mali_gralloc_get_producer_usage_internal(buffer, outUsage);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc_get_stride(gralloc1_device_t* device,
	buffer_handle_t buffer, uint32_t* outStride)
{
	GRALLOC_UNUSED(device);

	int stride;

	if (mali_gralloc_query_getstride(buffer, &stride) < 0)
	{
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	*outStride = (uint32_t)stride;

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_allocate(gralloc1_device_t* device,
	uint32_t numDescriptors, const gralloc1_buffer_descriptor_t* descriptors,
	buffer_handle_t* outBuffers)
{
	mali_gralloc_module* m;
	m = reinterpret_cast<private_module_t*>(device->common.module);
	int format, width, height;
	uint64_t usage;
	uint32_t layer_count;

	if (numDescriptors != 1)
	{
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(*descriptors);
	format = bufDescriptor->hal_format;
	usage = bufDescriptor->producer_usage | bufDescriptor->consumer_usage;
	width = bufDescriptor->width;
	height = bufDescriptor->height;
	layer_count = bufDescriptor->layerCount;

#if GRALLOC_FB_SWAP_RED_BLUE == 1
	/* match the framebuffer format */
	if (usage & GRALLOC1_CONSUMER_USAGE_CLIENT_TARGET)
	{
#ifdef GRALLOC_16_BITS
		format = HAL_PIXEL_FORMAT_RGB_565;
#else
		format = HAL_PIXEL_FORMAT_BGRA_8888;
#endif
	}
#endif

	if(mali_gralloc_buffer_allocate(m, format, bufDescriptor->consumer_usage, bufDescriptor->producer_usage, width, height, layer_count, outBuffers) < 0)
	{
		ALOGE("Failed to allocate buffer.");
		return GRALLOC1_ERROR_NO_RESOURCES;
	}
	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_retain(gralloc1_device_t* device,
	buffer_handle_t buffer)
{
	mali_gralloc_module* m;
	m = reinterpret_cast<private_module_t*>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (mali_gralloc_reference_retain(m, buffer) < 0)
	{
		return GRALLOC1_ERROR_NO_RESOURCES;
	}

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc_release(gralloc1_device_t* device,
	buffer_handle_t buffer)
{
	mali_gralloc_module* m;
	m = reinterpret_cast<private_module_t*>(device->common.module);

	if (mali_gralloc_reference_release(m, buffer, true) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc1_get_num_flex_planes(gralloc1_device_t* device,
	buffer_handle_t buffer, uint32_t* outNumPlanes)
{
	mali_gralloc_module* m;
	m = reinterpret_cast<private_module_t*>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}
	if (mali_gralloc_get_num_flex_planes(m, buffer, outNumPlanes) < 0)
	{
		return GRALLOC1_ERROR_UNSUPPORTED;
	}
	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc1_lock_async(gralloc1_device_t* device,
	buffer_handle_t buffer,
	uint64_t /*gralloc1_producer_usage_t*/ producerUsage,
	uint64_t /*gralloc1_consumer_usage_t*/ consumerUsage,
	const gralloc1_rect_t* accessRegion, void** outData,
	int32_t acquireFence)
{
	mali_gralloc_module* m;
	m = reinterpret_cast<private_module_t*>(device->common.module);
	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}
	if (!((producerUsage | consumerUsage) & (GRALLOC1_USAGE_SW_READ_MASK | GRALLOC1_USAGE_SW_WRITE_MASK)))
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if (mali_gralloc_lock_async(m, buffer, producerUsage | consumerUsage, accessRegion->left, accessRegion->top,
		accessRegion->width, accessRegion->height, outData, acquireFence) < 0)
	{
		return GRALLOC1_ERROR_UNSUPPORTED;
	}
	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc1_lock_flex_async(gralloc1_device_t* device,
	buffer_handle_t buffer,
	uint64_t /*gralloc1_producer_usage_t*/ producerUsage,
	uint64_t /*gralloc1_consumer_usage_t*/ consumerUsage,
	const gralloc1_rect_t* accessRegion,
	struct android_flex_layout* outFlexLayout, int32_t acquireFence)
{
	mali_gralloc_module* m;
	m = reinterpret_cast<private_module_t*>(device->common.module);

	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	if (!((producerUsage | consumerUsage) & (GRALLOC1_USAGE_SW_READ_MASK | GRALLOC1_USAGE_SW_WRITE_MASK)))
	{
		return GRALLOC1_ERROR_BAD_VALUE;
	}

	if (mali_gralloc_lock_flex_async(m, buffer, producerUsage | consumerUsage, accessRegion->left, accessRegion->top,
						accessRegion->width, accessRegion->height, outFlexLayout, acquireFence) < 0)
	{
		return GRALLOC1_ERROR_UNSUPPORTED;
	}
	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc1_unlock_async(gralloc1_device_t* device,
	buffer_handle_t buffer, int32_t* outReleaseFence)
{
	mali_gralloc_module* m;
	m = reinterpret_cast<private_module_t*>(device->common.module);
	if (private_handle_t::validate(buffer) < 0)
	{
		return GRALLOC1_ERROR_BAD_HANDLE;
	}

	mali_gralloc_unlock_async(m, buffer, outReleaseFence);
	return GRALLOC1_ERROR_NONE;
}

static int32_t mali_gralloc1_set_layer_count(gralloc1_device_t* device,
		gralloc1_buffer_descriptor_t descriptor,
		uint32_t layerCount)
{
	int ret = 0;
	ret = mali_gralloc_set_layer_count_internal(descriptor, layerCount);
	GRALLOC_UNUSED(device);
	return ret;
}

static int32_t mali_gralloc1_get_layer_count(gralloc1_device_t* device, buffer_handle_t buffer, uint32_t* outLayerCount)
{
	int ret = 0;
	ret = mali_gralloc_get_layer_count_internal(buffer, outLayerCount);
	GRALLOC_UNUSED(device);
	return ret;
}

static const mali_gralloc_func mali_gralloc_func_list[] =
{
	{GRALLOC1_FUNCTION_DUMP, (gralloc1_function_pointer_t)mali_gralloc_dump},
	{GRALLOC1_FUNCTION_CREATE_DESCRIPTOR, (gralloc1_function_pointer_t)mali_gralloc_create_descriptor},
	{GRALLOC1_FUNCTION_DESTROY_DESCRIPTOR, (gralloc1_function_pointer_t)mali_gralloc_destroy_descriptor},
	{GRALLOC1_FUNCTION_SET_CONSUMER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_set_consumer_usage},
	{GRALLOC1_FUNCTION_SET_DIMENSIONS, (gralloc1_function_pointer_t)mali_gralloc_set_dimensions},
	{GRALLOC1_FUNCTION_SET_FORMAT, (gralloc1_function_pointer_t)mali_gralloc_set_format},
	{GRALLOC1_FUNCTION_SET_PRODUCER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_set_producer_usage},
	{GRALLOC1_FUNCTION_GET_BACKING_STORE, (gralloc1_function_pointer_t)mali_gralloc_get_backing_store},
	{GRALLOC1_FUNCTION_GET_CONSUMER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_get_consumer_usage},
	{GRALLOC1_FUNCTION_GET_DIMENSIONS, (gralloc1_function_pointer_t)mali_gralloc_get_dimensions},
	{GRALLOC1_FUNCTION_GET_FORMAT, (gralloc1_function_pointer_t)mali_gralloc_get_format},
	{GRALLOC1_FUNCTION_GET_PRODUCER_USAGE, (gralloc1_function_pointer_t)mali_gralloc_get_producer_usage},
	{GRALLOC1_FUNCTION_GET_STRIDE, (gralloc1_function_pointer_t)mali_gralloc_get_stride},
	{GRALLOC1_FUNCTION_ALLOCATE, (gralloc1_function_pointer_t)mali_gralloc_allocate},
	{GRALLOC1_FUNCTION_RETAIN, (gralloc1_function_pointer_t)mali_gralloc_retain},
	{GRALLOC1_FUNCTION_RELEASE, (gralloc1_function_pointer_t)mali_gralloc_release},
	{GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES, (gralloc1_function_pointer_t)mali_gralloc1_get_num_flex_planes},
	{GRALLOC1_FUNCTION_LOCK, (gralloc1_function_pointer_t)mali_gralloc1_lock_async},
	{GRALLOC1_FUNCTION_LOCK_FLEX, (gralloc1_function_pointer_t)mali_gralloc1_lock_flex_async},
	{GRALLOC1_FUNCTION_UNLOCK, (gralloc1_function_pointer_t)mali_gralloc1_unlock_async},
	{GRALLOC1_FUNCTION_SET_LAYER_COUNT, (gralloc1_function_pointer_t)mali_gralloc1_set_layer_count},
	{GRALLOC1_FUNCTION_GET_LAYER_COUNT, (gralloc1_function_pointer_t)mali_gralloc1_get_layer_count},

	/* GRALLOC1_FUNCTION_INVALID has to be the last descriptor on the list. */
	{GRALLOC1_FUNCTION_INVALID, NULL}
};

static void mali_gralloc_getCapabilities(gralloc1_device_t *dev, uint32_t *outCount, int32_t *outCapabilities)
{
	GRALLOC_UNUSED(dev);

	if (outCount != NULL)
		*outCount = 1;

	if (outCapabilities != NULL)
	{
		*(outCapabilities++) = GRALLOC1_CAPABILITY_LAYERED_BUFFERS;
	}
}

static gralloc1_function_pointer_t mali_gralloc_getFunction(gralloc1_device_t *dev, int32_t descriptor)
{
	GRALLOC_UNUSED(dev);
	uint32_t pos = 0;

	while (mali_gralloc_func_list[pos].desc != GRALLOC1_FUNCTION_INVALID)
	{
		if (mali_gralloc_func_list[pos].desc == descriptor)
		{
			return mali_gralloc_func_list[pos].func;
		}
		pos++;
	}

	return NULL;
}

int mali_gralloc_device_open(hw_module_t const* module, const char* name, hw_device_t** device)
{
	int status = -EINVAL;
	if (!strncmp(name, GRALLOC_HARDWARE_MODULE_ID, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		gralloc_context_t *dev;
		dev = (gralloc_context_t*)malloc(sizeof(*dev));

		/* initialize our state here */
		memset(dev, 0, sizeof(*dev));

		/* initialize the procs */
		dev->device.common.tag = HARDWARE_DEVICE_TAG;
		dev->device.common.version = 0;
		dev->device.common.module = const_cast<hw_module_t*>(module);
		dev->device.common.close = gralloc_close;

		dev->device.getCapabilities = mali_gralloc_getCapabilities;
		dev->device.getFunction = mali_gralloc_getFunction;

		private_module_t *p = reinterpret_cast<private_module_t*>(dev->device.common.module);
		if (p->ionfd == -1)
			p->ionfd = exynos_ion_open();

		*device = &dev->device.common;
		status = 0;
	}
	return status;
}

int mali_gralloc_module_device_open(const hw_module_t* module, const char* name,
                        hw_device_t** device)
{
    int status = -EINVAL;
	if (!strncmp(name, GRALLOC_HARDWARE_MODULE_ID, MALI_GRALLOC_HARDWARE_MAX_STR_LEN))
	{
		status = mali_gralloc_device_open(module, name, device);
	}
	else if(!strncmp(name, GRALLOC_HARDWARE_FB0, MALI_GRALLOC_HARDWARE_MAX_STR_LEN) )
	{
        status = fb_device_open(module, name, device);
    }
    return status;
}

/*****************************************************************************/
static struct hw_module_methods_t mali_gralloc_module_methods = {
	mali_gralloc_module_device_open
};

// There is one global instance of the module

struct private_module_t HAL_MODULE_INFO_SYM = {
// gralloc1_module_t
.base = {
	// hw_module_t
	.common = {
		.tag = HARDWARE_MODULE_TAG,
		.module_api_version = GRALLOC_MODULE_API_VERSION_1_0,
		.hal_api_version = 0,
		.id = GRALLOC_HARDWARE_MODULE_ID,
		.name = "Graphics Memory Allocator Module",
		.author = "ARM Ltd.",
		.methods = &mali_gralloc_module_methods,
	},
},

.framebuffer = 0,
.flags = 0,
.numBuffers = 0,
.bufferMask = 0,
.lock = PTHREAD_MUTEX_INITIALIZER,
.currentBuffer = 0,
.ionfd = -1,
};
