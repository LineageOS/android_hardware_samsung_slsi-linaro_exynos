/*
 * Copyright (C) 2016, Samsung Electronics Co. LTD
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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera3DualInterface"
#include <cutils/log.h>
#include <list>

#include "ExynosCamera3DualInterface.h"
#include "ExynosCameraAutoTimer.h"

#ifdef SAMSUNG_TN_FEATURE
#include "SecCameraVendorTags.h"
#endif

#ifdef TIME_LOGGER_ENABLE
#include "ExynosCameraTimeLogger.h"
#endif

namespace android {

using namespace std;

/* Convert Id to the one for HAL. Refer to the enum CAMERA_ID in ExynosCameraSensorInfoBase.h  */
static int HAL_getCameraId(int id)
{
    int cameraId = id;
    switch (cameraId) {
    case CAMERA_REMAP_ID_TELE:
        cameraId = CAMERA_ID_BACK_1;
        break;
    case CAMERA_REMAP_ID_IRIS:
        cameraId = CAMERA_ID_SECURE;
        break;
    case CAMERA_REMAP_ID_DUAL:
        cameraId = CAMERA_ID_BACK_0;
        break;
    default:
        break;
    }

    return cameraId;
}

/* Revert Id to the one for interfacing in the dual hal interface structure  */
static int HAL_revertCameraId(int id)
{
    int cameraId = id;
    switch (cameraId) {
    case CAMERA_ID_SECURE:
        cameraId = CAMERA_REMAP_ID_IRIS;
        break;
    case CAMERA_ID_BACK_1:
        cameraId = CAMERA_REMAP_ID_TELE;
        break;
    default:
        break;
    }

    return cameraId;
}

static int HAL3_camera_device_open(const struct hw_module_t* module,
                                    const char *id,
                                    struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(id);
    enum CAMERA_STATE state;
    FILE *fp = NULL;
    int ret = 0;

    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    bool hasFlash = false;
    char flashFilePath[100] = {'\0',};

    cameraId = HAL_getCameraId(cameraId);

    /* Validation check */
    ALOGI("INFO(%s[%d]):camera(%d) in ======", __FUNCTION__, __LINE__, cameraId);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        if (cameraId < CAMERA_ID_HIDDEN_START || cameraId >= CAMERA_ID_HIDDEN_START + HAL_getNumberOfHiddenCameras()) {
            ALOGE("ERR(%s[%d]):Invalid camera ID %d", __FUNCTION__, __LINE__, cameraId);
            return -EINVAL;
        }
    }

    /* Check init thread state */
    if (g_thread) {
        ret = pthread_join(g_thread, NULL);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):pthread_join failed with error code %d",  __FUNCTION__, __LINE__, ret);
        }
        g_thread = 0;
    }

    /* Setting status and check current status */
    state = CAMERA_OPENED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s[%d]):camera(%d) state(%d) is INVALID", __FUNCTION__, __LINE__, cameraId, state);
        return -EUSERS;
    }

    /* Create camera device */
    if (g_cam_device3[cameraId]) {
        ALOGE("ERR(%s[%d]):returning existing camera ID(%d)", __FUNCTION__, __LINE__, cameraId);
        *device = (hw_device_t *)g_cam_device3[cameraId];
        goto done;
    }

    g_cam_device3[cameraId] = (camera3_device_t *)malloc(sizeof(camera3_device_t));
    if (!g_cam_device3[cameraId])
        return -ENOMEM;

    g_cam_openLock[cameraId].lock();
    g_cam_device3[cameraId]->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam_device3[cameraId]->common.version = CAMERA_DEVICE_API_VERSION_3_4;
    g_cam_device3[cameraId]->common.module  = const_cast<hw_module_t *>(module);
    g_cam_device3[cameraId]->common.close   = HAL3_camera_device_close;
    g_cam_device3[cameraId]->ops            = &camera_device3_ops;

    ALOGV("DEBUG(%s[%d]):open camera(%d)", __FUNCTION__, __LINE__, cameraId);
    g_cam_device3[cameraId]->priv = new ExynosCamera3(cameraId, &g_cam_info[cameraId]);
    *device = (hw_device_t *)g_cam_device3[cameraId];

    ALOGI("INFO(%s[%d]):camera(%d) out from new g_cam_device3[%d]->priv()",
        __FUNCTION__, __LINE__, cameraId, cameraId);

    g_cam_openLock[cameraId].unlock();

done:
    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();

    if (g_cam_info[cameraId]) {
        metadata = g_cam_info[cameraId];
        flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

        ALOGV("INFO(%s[%d]): cameraId(%d), flashAvailable.count(%d), flashAvailable.data.u8[0](%d)",
            __FUNCTION__, cameraId, flashAvailable.count, flashAvailable.data.u8[0]);

        if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
            hasFlash = true;
        } else {
            hasFlash = false;
        }
    }

    /* Turn off torch and update torch status */
    if(hasFlash && g_cam_torchEnabled[cameraId]) {
        if (cameraId == CAMERA_ID_BACK) {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
        } else {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
        }

        fp = fopen(flashFilePath, "w+");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):torch file open fail, ret(%d)", __FUNCTION__, __LINE__, fp);
        } else {
            fwrite("0", sizeof(char), 1, fp);
            fflush(fp);
            fclose(fp);

            g_cam_torchEnabled[cameraId] = false;
        }
    }

    g_callbacks->torch_mode_status_change(g_callbacks, id, TORCH_MODE_STATUS_NOT_AVAILABLE);

    ALOGI("INFO(%s[%d]):camera(%d) out =====", __FUNCTION__, __LINE__, cameraId);

    return 0;
}

static int HAL3_camera_device_close(struct hw_device_t* device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId;
    int ret = OK;
    enum CAMERA_STATE state;
    char camid[10];

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (device) {
        camera3_device_t *cam_device = (camera3_device_t *)device;
        cameraId = obj(cam_device)->getCameraId();

        ALOGV("DEBUG(%s[%d]):close camera(%d)", __FUNCTION__, __LINE__, cameraId);

        ret = obj(cam_device)->releaseDevice();
        if (ret) {
            ALOGE("ERR(%s[%d]):initialize error!!", __FUNCTION__, __LINE__);
            ret = BAD_VALUE;
        }

        state = CAMERA_CLOSED;
        if (check_camera_state(state, cameraId) == false) {
            ALOGE("ERR(%s[%d]):camera(%d) state(%d) is INVALID",
                __FUNCTION__, __LINE__, cameraId, state);
            return -1;
        }

        g_cam_openLock[cameraId].lock();
        ALOGV("INFO(%s[%d]):camera(%d) open locked..", __FUNCTION__, __LINE__, cameraId);
        g_cam_device3[cameraId] = NULL;
        g_cam_openLock[cameraId].unlock();
        ALOGV("INFO(%s[%d]):camera(%d) open unlocked..", __FUNCTION__, __LINE__, cameraId);

        delete static_cast<ExynosCamera3 *>(cam_device->priv);
        free(cam_device);

        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):close camera(%d)", __FUNCTION__, __LINE__, cameraId);
    }

    /* Update torch status */
    g_cam_torchEnabled[cameraId] = false;
    snprintf(camid, sizeof(camid), "%d\n", cameraId);
    g_callbacks->torch_mode_status_change(g_callbacks, camid,
        TORCH_MODE_STATUS_AVAILABLE_OFF);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_initialize(const struct camera3_device *dev,
                                        const camera3_callback_ops_t *callback_ops)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    g_cam_configLock[cameraId].lock();

    ALOGE("INFO(%s[%d]): dual cam_state[0](%d)", __FUNCTION__, __LINE__, cam_state[0]);

#ifdef DUAL_CAMERA_SUPPORTED
    if (cameraId != 0 && g_cam_device3[0] != NULL
        && cam_state[0] != CAMERA_NONE && cam_state[0] != CAMERA_CLOSED) {
        ret = obj(dev)->setDualMode(true);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):camera(%d) set dual mode fail, ret(%d)",
                __FUNCTION__, __LINE__, cameraId, ret);
        } else {
            ALOGI("INFO(%s[%d]):camera(%d) set dual mode)",
                __FUNCTION__, __LINE__, cameraId);
        }
    }
#endif

    ret = obj(dev)->initilizeDevice(callback_ops);
    if (ret) {
        ALOGE("ERR(%s[%d]):initialize error!!", __FUNCTION__, __LINE__);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();

    ALOGV("DEBUG(%s):set callback ops - %p", __FUNCTION__, callback_ops);
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_configure_streams(const struct camera3_device *dev,
                                                camera3_stream_configuration_t *stream_list)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->configureStreams(stream_list);
    if (ret) {
        ALOGE("ERR(%s[%d]):configure_streams error!!", __FUNCTION__, __LINE__);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_register_stream_buffers(const struct camera3_device *dev,
                                                    const camera3_stream_buffer_set_t *buffer_set)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->registerStreamBuffers(buffer_set);
    if (ret) {
        ALOGE("ERR(%s[%d]):register_stream_buffers error!!", __FUNCTION__, __LINE__);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static const camera_metadata_t* HAL3_camera_device_construct_default_request_settings(
                                                                const struct camera3_device *dev,
                                                                int type)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    camera_metadata_t *request = NULL;
    status_t res;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    res = obj(dev)->construct_default_request_settings(&request, type);
    if (res) {
        ALOGE("ERR(%s[%d]):constructDefaultRequestSettings error!!", __FUNCTION__, __LINE__);
        g_cam_configLock[cameraId].unlock();
        return NULL;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return request;
}

static int HAL3_camera_device_process_capture_request(const struct camera3_device *dev,
                                                        camera3_capture_request_t *request)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGV("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->processCaptureRequest(request);
    if (ret) {
        ALOGE("ERR(%s[%d]):process_capture_request error(%d)!!", __FUNCTION__, __LINE__, ret);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGV("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_flush(const struct camera3_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = 0;
    uint32_t cameraId = obj(dev)->getCameraId();

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->flush();
    if (ret) {
        ALOGE("ERR(%s[%d]):flush error(%d)!!", __FUNCTION__, __LINE__, ret);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static void HAL3_camera_device_get_metadata_vendor_tag_ops(const struct camera3_device *dev,
                                                            vendor_tag_query_ops_t* ops)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (dev == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    if (ops == NULL)
        ALOGE("ERR(%s[%d]):ops is NULL", __FUNCTION__, __LINE__);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
}

static void HAL3_camera_device_dump(const struct camera3_device *dev, int fd)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (dev == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    if (fd < 0)
        ALOGE("ERR(%s[%d]):fd is Negative Value", __FUNCTION__, __LINE__);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
}

/***************************************************************************
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: static function to query the numner of cameras
 *
 * PARAMETERS : none
 *
 * RETURN     : the number of cameras pre-defined
 ***************************************************************************/
static int HAL_getNumberOfCameras()
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */
    int getNumOfCamera = sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
    ALOGV("DEBUG(%s[%d]):Number of cameras(%d)", __FUNCTION__, __LINE__, getNumOfCamera);
    return getNumOfCamera;
}

static int HAL_getNumberOfHiddenCameras()
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */
    int getNumOfHiddenCamera = sizeof(sCameraHiddenInfo) / sizeof(sCameraHiddenInfo[0]);
    ALOGV("DEBUG(%s[%d]):Number of Hidden Camera(%d)", __FUNCTION__, __LINE__, getNumOfHiddenCamera);
    return getNumOfHiddenCamera;
}

static int HAL_getCameraInfo(int camera_id, struct camera_info *info)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */
    status_t ret = NO_ERROR;

    int cameraId = HAL_getCameraId(camera_id);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        if (cameraId < CAMERA_ID_HIDDEN_START || cameraId >= CAMERA_ID_HIDDEN_START + HAL_getNumberOfHiddenCameras()) {
            ALOGE("ERR(%s[%d]):Invalid camera ID %d", __FUNCTION__, __LINE__, cameraId);
            return -ENODEV;
        }
    }

    /* set facing and orientation */
    if (cameraId >= CAMERA_ID_HIDDEN_START)
        memcpy(info, &sCameraHiddenInfo[cameraId - CAMERA_ID_HIDDEN_START], sizeof(CameraInfo));
    else
        memcpy(info, &sCameraInfo[cameraId], sizeof(CameraInfo));

    /* set device API version */
    info->device_version = CAMERA_DEVICE_API_VERSION_3_4;

    /* set camera_metadata_t if needed */
    if (info->device_version >= HARDWARE_DEVICE_API_VERSION(2, 0)) {
        if (g_cam_info[cameraId] == NULL) {
            ALOGV("DEBUG(%s[%d]):Return static information (%d)", __FUNCTION__, __LINE__, cameraId);
            ret = ExynosCamera3MetadataConverter::constructStaticInfo(cameraId, &g_cam_info[cameraId]);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]): static information is NULL", __FUNCTION__, __LINE__);
                return -EINVAL;
            }
            info->static_camera_characteristics = g_cam_info[cameraId];
        } else {
            ALOGV("DEBUG(%s[%d]):Reuse Return static information (%d)", __FUNCTION__, __LINE__, cameraId);
            info->static_camera_characteristics = g_cam_info[cameraId];
        }
    }

    /* set service arbitration (resource_cost, conflicting_devices, conflicting_devices_length */
    info->resource_cost = sCameraConfigInfo[cameraId].resource_cost;
    info->conflicting_devices = sCameraConfigInfo[cameraId].conflicting_devices;
    info->conflicting_devices_length = sCameraConfigInfo[cameraId].conflicting_devices_length;
    ALOGV("INFO(%s info->resource_cost = %d ", __FUNCTION__, info->resource_cost);
    if (info->conflicting_devices_length) {
        for (size_t i = 0; i < info->conflicting_devices_length; i++) {
            ALOGV("INFO(%s info->conflicting_devices = %s ", __FUNCTION__, info->conflicting_devices[i]);
        }
    } else {
        ALOGV("INFO(%s info->conflicting_devices_length is zero ", __FUNCTION__);
    }

    return NO_ERROR;
}

static int HAL_set_callbacks(const camera_module_callbacks_t *callbacks)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    if (callbacks == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    g_callbacks = callbacks;

    return OK;
}

#ifdef TIME_LOGGER_LAUNCH_ENABLE
extern ExynosCameraTimeLogger g_timeLoggerLaunch;
extern int g_timeLoggerLaunchIndex;
bool g_timeLoggerFirstSetParameter;
#endif
static int HAL_open_legacy(const struct hw_module_t* module, const char* id,
                          uint32_t halVersion, struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    int ret = 0;

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (module == NULL) {
        ALOGE("ERR(%s[%d]):module is NULL", __FUNCTION__, __LINE__);
        ret = -EINVAL;
    }

    if (id == NULL) {
        ALOGE("ERR(%s[%d]):id is NULL", __FUNCTION__, __LINE__);
        ret = -EINVAL;
    }

    if (device == NULL) {
        ALOGE("ERR(%s[%d]):device is NULL", __FUNCTION__, __LINE__);
        ret = -EINVAL;
    }

    if (halVersion == 0)
        ALOGE("ERR(%s[%d]):halVersion is Zero", __FUNCTION__, __LINE__);

    char *newId = "20";
    int cameraId = atoi(id);

#ifdef TIME_LOGGER_LAUNCH_ENABLE
    int loggerCameraId = (cameraId == CAMERA_REMAP_ID_DUAL) ? CAMERA_ID_BACK : cameraId;
    g_timeLoggerFirstSetParameter = false;
    if (g_timeLoggerLaunchIndex % TIME_LOGGER_LAUNCH_COUNT == 0)
        TIME_LOGGER_INIT_BASE(&g_timeLoggerLaunch, loggerCameraId);
    TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, loggerCameraId, ++g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_TOTAL_TIME, true);
    TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, loggerCameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_OPEN_START, true);
#endif

#ifdef USE_ONE_INTERFACE_FILE
    if (!ret)
#if 0
        if (cameraId == CAMERA_ID_BACK)
            return HAL_camera_device_dual_open(module, newId, device);
        else
#endif
            return HAL_camera_device_dual_open(module, id, device);
    else
        return ret;
#else
    return NO_ERROR;
#endif
}

static int HAL_set_torch_mode(const char* camera_id, bool enabled)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(camera_id);
    FILE *fp = NULL;
    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    int ret = 0;
    char flashFilePath[100] = {'\0',};

    cameraId = HAL_getCameraId(cameraId);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        if (cameraId < CAMERA_ID_HIDDEN_START || cameraId >= CAMERA_ID_HIDDEN_START + HAL_getNumberOfHiddenCameras()) {
            ALOGE("ERR(%s[%d]):Invalid camera ID %d", __FUNCTION__, __LINE__, cameraId);
            return -EINVAL;
        }
    }

    /* Check the android.flash.info.available */
    /* If this camera device does not support flash, It have to return -ENOSYS */
    metadata = g_cam_info[cameraId];
    flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

    if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
        ALOGV("DEBUG(%s[%d]): Flash metadata exist", __FUNCTION__, __LINE__);
    } else {
        ALOGE("ERR(%s[%d]): Can not find flash metadata", __FUNCTION__, __LINE__);
        return -ENOSYS;
    }

    ALOGI("INFO(%s[%d]): Current Camera State (state = %d)", __FUNCTION__, __LINE__, cam_state[cameraId]);

    /* Add the check the camera state that camera in use or not */
    if (cam_state[cameraId] > CAMERA_CLOSED) {
        ALOGE("ERR(%s[%d]): Camera Device is busy (state = %d)", __FUNCTION__, __LINE__, cam_state[cameraId]);
        g_callbacks->torch_mode_status_change(g_callbacks, camera_id, TORCH_MODE_STATUS_AVAILABLE_OFF);
        return -EBUSY;
    }

    /* Add the sysfs file read (sys/class/camera/flash/torch_flash) then set 0 or 1 */
    if (cameraId == CAMERA_ID_BACK) {
        snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
    } else {
        snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
    }

    fp = fopen(flashFilePath, "w+");
    if (fp == NULL) {
        ALOGE("ERR(%s[%d]):torch file open(%s) fail, ret(%d)",
            __FUNCTION__, __LINE__, flashFilePath, fp);
        return -ENOSYS;
    }

    if (enabled) {
        fwrite("1", sizeof(char), 1, fp);
    } else {
        fwrite("0", sizeof(char), 1, fp);
    }

    fflush(fp);

    ret = fclose(fp);
    if (ret != 0) {
        ALOGE("ERR(%s[%d]): file close failed(%d)", __FUNCTION__, __LINE__, ret);
    }

    if (enabled) {
        g_cam_torchEnabled[cameraId] = true;
        g_callbacks->torch_mode_status_change(g_callbacks,
            camera_id, TORCH_MODE_STATUS_AVAILABLE_ON);
    } else {
        g_cam_torchEnabled[cameraId] = false;
        g_callbacks->torch_mode_status_change(g_callbacks,
            camera_id, TORCH_MODE_STATUS_AVAILABLE_OFF);
    }

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void *init_func(__unused void *data)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    FILE *fp = NULL;
    char name[64];

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    fp = fopen(INIT_MODULE_PATH, "r");
    if (fp == NULL) {
        ALOGI("INFO(%s[%d]):module init file open fail, ret(%d)", __FUNCTION__, __LINE__, fp);
        return NULL;
    }

    if (fgets(name, sizeof(name), fp) == NULL) {
        ALOGI("INFO(%s[%d]):failed to read init sysfs", __FUNCTION__, __LINE__);
    }

    fclose(fp);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return NULL;
}

static int HAL_init()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = 0;

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    ret = pthread_create(&g_thread, NULL, init_func, NULL);
    if (ret) {
        ALOGE("ERR(%s[%d]):pthread_create failed with error code %d", __FUNCTION__, __LINE__, ret);
    }

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return OK;
}

#ifdef USE_ONE_INTERFACE_FILE
static int HAL_camera_device_open(
        const struct hw_module_t* module,
        const char *id,
        struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(id);
    FILE *fp = NULL;

    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    bool hasFlash = false;
    char flashFilePath[100] = {'\0',};

#ifdef BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA
    if (cameraId == 0) {
        return HAL_ext_camera_device_open_wrapper(module, id, device);
    }
#endif

#ifdef BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA
    if (cameraId == 1) {
        return HAL_ext_camera_device_open_wrapper(module, id, device);
    }
#endif

#if (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA)
#else
    enum CAMERA_STATE state;
    int ret = 0;

    cameraId = HAL_getCameraId(cameraId);

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        if (cameraId < CAMERA_ID_HIDDEN_START || cameraId >= CAMERA_ID_HIDDEN_START + HAL_getNumberOfHiddenCameras()) {
            ALOGE("ERR(%s[%d]):Invalid camera ID %d", __FUNCTION__, __LINE__, cameraId);
            return -EINVAL;
        }
    }

    /* Check init thread state */
    if (g_thread) {
        ret = pthread_join(g_thread, NULL);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):pthread_join failed with error code %d",  __FUNCTION__, __LINE__, ret);
        }
        g_thread = 0;
    }

    state = CAMERA_OPENED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID", __FUNCTION__, cameraId, state);
        return -1;
    }

    if (g_cam_device[cameraId]) {
        ALOGE("DEBUG(%s):returning existing camera ID %s", __FUNCTION__, id);
        *device = (hw_device_t *)g_cam_device[cameraId];
        goto done;
    }

    g_cam_device[cameraId] = (camera_device_t *)malloc(sizeof(camera_device_t));
    if (!g_cam_device[cameraId])
        return -ENOMEM;

    g_cam_openLock[cameraId].lock();
    g_cam_device[cameraId]->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam_device[cameraId]->common.version = 1;
    g_cam_device[cameraId]->common.module  = const_cast<hw_module_t *>(module);
    g_cam_device[cameraId]->common.close   = HAL_camera_device_dual_close;

    g_cam_device[cameraId]->ops = &camera_device_ops;

    ALOGD("DEBUG(%s):open camera %s", __FUNCTION__, id);
#ifdef USE_DUAL_CAMERA
    g_cam_device[cameraId]->priv = new ExynosCamera(cameraId, g_cam_device[cameraId], getAutoDualMode());
#else
    g_cam_device[cameraId]->priv = new ExynosCamera(cameraId, g_cam_device[cameraId]);
#endif
    *device = (hw_device_t *)g_cam_device[cameraId];
    ALOGI("INFO(%s[%d]):camera(%d) out from new g_cam_device[%d]->priv()",
        __FUNCTION__, __LINE__, cameraId, cameraId);

    g_cam_openLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

done:
    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();

    if (g_cam_info[cameraId]) {
        metadata = g_cam_info[cameraId];
        flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

        ALOGV("INFO(%s[%d]): cameraId(%d), flashAvailable.count(%d), flashAvailable.data.u8[0](%d)",
            __FUNCTION__, cameraId, flashAvailable.count, flashAvailable.data.u8[0]);

        if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
            hasFlash = true;
        } else {
            hasFlash = false;
        }
    }

    if(hasFlash && g_cam_torchEnabled[cameraId]) {
        if (cameraId == CAMERA_ID_BACK) {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
        } else {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
        }

        fp = fopen(flashFilePath, "w+");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):torch file open fail, ret(%d)", __FUNCTION__, __LINE__, fp);
        } else {
            fwrite("0", sizeof(char), 1, fp);
            fflush(fp);
            fclose(fp);

            g_cam_torchEnabled[cameraId] = false;
            g_callbacks->torch_mode_status_change(g_callbacks, id, TORCH_MODE_STATUS_AVAILABLE_OFF);
        }
    }

    g_callbacks->torch_mode_status_change(g_callbacks, id, TORCH_MODE_STATUS_NOT_AVAILABLE);

    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
#endif /* (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA) */

    return 0;
}

static int HAL_camera_device_close(struct hw_device_t* device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId;
    enum CAMERA_STATE state;

#if (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA)
#else
    ALOGI("INFO(%s[%d]): in", __FUNCTION__, __LINE__);

    if (device) {
        camera_device_t *cam_device = (camera_device_t *)device;
        cameraId = obj(cam_device)->getCameraId();

        ALOGI("INFO(%s[%d]):camera(%d)", __FUNCTION__, __LINE__, cameraId);

        state = CAMERA_CLOSED;
        if (check_camera_state(state, cameraId) == false) {
            ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
                __FUNCTION__, cameraId, state);
            return -1;
        }

        g_cam_openLock[cameraId].lock();
        ALOGI("INFO(%s[%d]):camera(%d) locked..", __FUNCTION__, __LINE__, cameraId);
        g_cam_device[cameraId] = NULL;
        g_cam_openLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

        delete static_cast<ExynosCamera *>(cam_device->priv);
        free(cam_device);

        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d)", __FUNCTION__, __LINE__, cameraId);
    }

    ALOGI("INFO(%s[%d]): out", __FUNCTION__, __LINE__);
#endif /* (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA) */

    return 0;
}

static int HAL_camera_device_set_preview_window(
        struct camera_device *dev,
        struct preview_stream_ops *buf)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    static int ret;
    uint32_t cameraId = obj(dev)->getCameraId();

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);
    ret = obj(dev)->setPreviewWindow(buf);
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
    return ret;
}

static void HAL_camera_device_set_callbacks(struct camera_device *dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->setCallbacks(notify_cb, data_cb, data_cb_timestamp,
                           get_memory,
                           user);
}

static void HAL_camera_device_enable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->enableMsgType(msg_type);
}

static void HAL_camera_device_disable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->disableMsgType(msg_type);
}

static int HAL_camera_device_msg_type_enabled(
        struct camera_device *dev,
        int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->msgTypeEnabled(msg_type);
}

static int HAL_camera_device_start_preview(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    static int ret;
    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_PREVIEW;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
            __FUNCTION__, cameraId, state);
        return -1;
    }

    g_cam_previewLock[cameraId].lock();

#ifdef DUAL_CAMERA_SUPPORTED
    if (cameraId != 0 && g_cam_device[0] != NULL
        && cam_state[0] != CAMERA_NONE && cam_state[0] != CAMERA_CLOSED) {
#ifdef CAMERA_GED_FEATURE
        ret = obj(dev)->setDualMode(true);
#else
#ifdef SAMSUNG_QUICK_SWITCH
        ret = obj(dev)->setDualMode(!obj(dev)->getQuickSwitchFlag());
#endif
#endif
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):camera(%d) set dual mode fail, ret(%d)",
                __FUNCTION__, __LINE__, cameraId, ret);
        } else {
            ALOGI("INFO(%s[%d]):camera(%d) set dual mode)",
                __FUNCTION__, __LINE__, cameraId);
        }
    }
#endif

    ret = obj(dev)->startPreview();
    ALOGV("INFO(%s[%d]):camera(%d) out from startPreview()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_previewLock[cameraId].unlock();

    ALOGV("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    if (ret == OK) {
        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d) out (startPreview succeeded)",
            __FUNCTION__, __LINE__, cameraId);
    } else {
        ALOGI("INFO(%s[%d]):camera(%d) out (startPreview FAILED)",
            __FUNCTION__, __LINE__, cameraId);
    }
    return ret;
}

static void HAL_camera_device_stop_preview(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);
/* HACK : If camera in recording state, */
/*        CameraService have to call the stop_recording before the stop_preview */
#if 1
    if (cam_state[cameraId] == CAMERA_RECORDING) {
        ALOGE("ERR(%s[%d]):camera(%d) in RECORDING RUNNING state ---- INVALID ----",
            __FUNCTION__, __LINE__, cameraId);
        ALOGE("ERR(%s[%d]):camera(%d) The stop_recording must be called "
            "before the stop_preview  ---- INVALID ----",
            __FUNCTION__, __LINE__,  cameraId);
        HAL_camera_device_stop_recording(dev);
        ALOGE("ERR(%s[%d]):cameraId=%d out from stop_recording  ---- INVALID ----",
            __FUNCTION__, __LINE__,  cameraId);

        for (int i=0; i<30; i++) {
            ALOGE("ERR(%s[%d]):camera(%d) The stop_recording must be called "
                "before the stop_preview  ---- INVALID ----",
                __FUNCTION__, __LINE__,  cameraId);
        }
        ALOGE("ERR(%s[%d]):camera(%d) sleep 500ms for ---- INVALID ---- state",
            __FUNCTION__, __LINE__,  cameraId);
        usleep(500000); /* to notify, sleep 500ms */
    }
#endif
    state = CAMERA_PREVIEWSTOPPED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID", __FUNCTION__, cameraId, state);
        return;
    }

    g_cam_previewLock[cameraId].lock();

    obj(dev)->stopPreview();
    ALOGI("INFO(%s[%d]):camera(%d) out from stopPreview()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_previewLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
}

static int HAL_camera_device_preview_enabled(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->previewEnabled();
}

static int HAL_camera_device_store_meta_data_in_buffers(
        struct camera_device *dev,
        int enable)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->storeMetaDataInBuffers(enable);
}

static int HAL_camera_device_start_recording(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    static int ret;
    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_RECORDING;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
            __FUNCTION__, cameraId, state);
        return -1;
    }

    g_cam_recordingLock[cameraId].lock();

    ret = obj(dev)->startRecording();
    ALOGI("INFO(%s[%d]):camera(%d) out from startRecording()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_recordingLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    if (ret == OK) {
        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d) out (startRecording succeeded)",
            __FUNCTION__, __LINE__, cameraId);
    } else {
        ALOGI("INFO(%s[%d]):camera(%d) out (startRecording FAILED)",
            __FUNCTION__, __LINE__, cameraId);
    }
    return ret;
}

static void HAL_camera_device_stop_recording(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_RECORDINGSTOPPED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID", __FUNCTION__, cameraId, state);
        return;
    }

    g_cam_recordingLock[cameraId].lock();

    obj(dev)->stopRecording();
    ALOGI("INFO(%s[%d]):camera(%d) out from stopRecording()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_recordingLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
}

static int HAL_camera_device_recording_enabled(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->recordingEnabled();
}

static void HAL_camera_device_release_recording_frame(struct camera_device *dev,
                                const void *opaque)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */

    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->releaseRecordingFrame(opaque);
}

static int HAL_camera_device_auto_focus(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->autoFocus();
}

static int HAL_camera_device_cancel_auto_focus(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->cancelAutoFocus();
}

static int HAL_camera_device_take_picture(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->takePicture();
}

static int HAL_camera_device_cancel_picture(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->cancelPicture();
}

static int HAL_camera_device_set_parameters(
        struct camera_device *dev,
        const char *parms)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    String8 str(parms);
    CameraParameters p(str);
    return obj(dev)->setParameters(p);
}

char *HAL_camera_device_get_parameters(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    String8 str;

/* HACK : to avoid compile error */
#if (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA)
    ALOGE("ERR(%s[%d]):invalid opertion on external camera", __FUNCTION__, __LINE__);
#else
    CameraParameters parms = obj(dev)->getParameters();
    str = parms.flatten();
#endif
    return strdup(str.c_str());
}

static void HAL_camera_device_put_parameters(
        struct camera_device *dev,
        char *parms)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    free(parms);
}

static int HAL_camera_device_send_command(
        struct camera_device *dev,
        int32_t cmd,
        int32_t arg1,
        int32_t arg2)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->sendCommand(cmd, arg1, arg2);
}

static void HAL_camera_device_release(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_RELEASED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
            __FUNCTION__, cameraId, state);
        return;
    }

    g_cam_openLock[cameraId].lock();

    obj(dev)->release();
    ALOGV("INFO(%s[%d]):camera(%d) out from release()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_openLock[cameraId].unlock();

    ALOGV("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
}

static int HAL_camera_device_dump(struct camera_device *dev, int fd)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->dump(fd);
}
#endif

static void HAL_get_vendor_tag_ops(__unused vendor_tag_ops_t* ops)
{
    ALOGV("INFO(%s):", __FUNCTION__);

#ifdef SAMSUNG_TN_FEATURE
    SecCameraVendorTags::Ops = ops;

    ops->get_all_tags = SecCameraVendorTags::get_ext_all_tags;
    ops->get_tag_count = SecCameraVendorTags::get_ext_tag_count;
    ops->get_tag_type = SecCameraVendorTags::get_ext_tag_type;
    ops->get_tag_name = SecCameraVendorTags::get_ext_tag_name;
    ops->get_section_name = SecCameraVendorTags::get_ext_section_name;
    ops->reserved[0] = NULL;
#else
    ALOGW("WARN(%s[%d]):empty operation", __FUNCTION__, __LINE__);
#endif
}

/*
 * from here
 * Dual Camera Interface
 */
static int HAL_camera_device_dual_open(
        const struct hw_module_t* module,
        const char *id,
        struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;

    int cameraId = atoi(id);
    list<int> cameraIdList;

    if (cameraId == CAMERA_REMAP_ID_DUAL)
        setAutoDualMode(true);
    else 
        setAutoDualMode(false);

    cameraId = HAL_getCameraId(cameraId);

    /* for dual camera. it open 2 HAL object on open time.
     * if app want PIP, app will open another HAL.
     * then, the HAL(opened for dual camera) will crush with HAL(open for PIP).
     *
     * so, if the opened camera HAL object is bigger than 1.
     *     this code close the HAL(opened for dual camera)
     */
    ALOGD("DEBUG(%s[%d]):cameraId(%d):getOpenedCameraHalCnt() : %d, dualMode(%d)",
        __FUNCTION__, __LINE__,  cameraId, getOpenedCameraHalCnt(), getAutoDualMode());

    /*
     * case1 : back camera opened (check)
     *         opened cnt : 2 (camera0, camera2)
     * case2 : front camera opened (not check)
     *         opened cnt : 1 (camera1)
     * case3 : nothing camera opened (not check)
     *         opened cnt : 0 (-)
     */
    if (2 <= getOpenedCameraHalCnt()) {
        int mainCameraId = -1;
        int subCameraId = -1;

        getDualCameraId(&mainCameraId, &subCameraId);

        /* If it's PIP scenario, forcely close the slave camera */
        if (mainCameraId != cameraId) {
            int teleCameraId = subCameraId;

            if (g_threadInterface[teleCameraId] != NULL) {
                ret = g_threadInterface[teleCameraId]->checkApiDone();
                if (ret != NO_ERROR) {
                    ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, teleCameraId);
                }
            }

            ALOGD("DEBUG(%s[%d]):cameraId(%d):try to close teleCameraId : %d for PIP",
                __FUNCTION__, __LINE__,  cameraId, teleCameraId);

            enum CAMERA_STATE state = CAMERA_PREVIEW;

            if (check_camera_state(state, teleCameraId) == true) {
                HAL_camera_device_stop_preview(g_cam_device[teleCameraId]);
            }

            ret = HAL_camera_device_close(g_threadInterface[teleCameraId]->m_hw_device);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_close(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
            }

            delete g_threadInterface[teleCameraId];
            g_threadInterface[teleCameraId] = NULL;

            delete g_callbackHooker[teleCameraId];
            g_callbackHooker[teleCameraId] = NULL;

            setPipMode(true);
        } else {
            setPipMode(false);
        }
    }

    if (isDualCamera(cameraId) == true) {
        Mutex::Autolock lock(g_threadInterfaceLock);

        /* Dual Camera Entry */
        int teleCameraId = getTeleCameraId(cameraId);

        // wide
        if (g_threadInterface[cameraId] != NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] != NULL, assert!!!!",
                    __FUNCTION__, __LINE__, cameraId);
        }

        g_threadInterface[cameraId] = new ExynosCameraThreadInterface(module, cameraId);
        g_threadInterface[cameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_NO_THREAD);

        if (g_callbackHooker[cameraId] != NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] != NULL, assert!!!!",
                    __FUNCTION__, __LINE__, cameraId);
        }

        // no callback hooking
        g_callbackHooker[cameraId] = new ExynosCameraCallbackHooker(cameraId);
        g_callbackHooker[cameraId]->setHooking(false);

        cameraIdList.push_back(cameraId);

        ret = g_threadInterface[cameraId]->open();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface[%d]->open() fail", __FUNCTION__, __LINE__, cameraId);
            goto done;
        }

        // tele
        cameraIdList.push_back(teleCameraId);

        if (g_threadInterface[teleCameraId] != NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_threadInterface[%d] != NULL, assert!!!!",
                    __FUNCTION__, __LINE__, teleCameraId);
        }

        g_threadInterface[teleCameraId] = new ExynosCameraThreadInterface(module, teleCameraId);
        g_threadInterface[teleCameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_BATCH);

        if (g_callbackHooker[teleCameraId] != NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] != NULL, assert!!!!",
                    __FUNCTION__, __LINE__, teleCameraId);
        }

        // callback hooking on tele camera.
        g_callbackHooker[teleCameraId] = new ExynosCameraCallbackHooker(teleCameraId);
        g_callbackHooker[teleCameraId]->setHooking(false);

        ret = g_threadInterface[teleCameraId]->open();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface[%d]->open() fail", __FUNCTION__, __LINE__, teleCameraId);
            goto done;
        }

done:

        for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
            int cmdCameraId = *it;

            if (g_threadInterface[cmdCameraId] != NULL) {
                ret = g_threadInterface[cmdCameraId]->checkApiDone();
                funcRet |= ret;
                if (ret != NO_ERROR) {
                    ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                }
            }
        }

        if (funcRet == NO_ERROR) {
            if (g_threadInterface[cameraId] != NULL)
                *device = g_threadInterface[cameraId]->m_hw_device;
        } else {
            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    HAL_camera_device_close(g_threadInterface[cmdCameraId]->m_hw_device);
                    delete g_threadInterface[cmdCameraId];
                    g_threadInterface[cmdCameraId] = NULL;
                }

                if (g_callbackHooker[cmdCameraId] != NULL) {
                    delete g_callbackHooker[cmdCameraId];
                    g_callbackHooker[cmdCameraId] = NULL;
                }
            }
        }
    } else {
        /* Not Dual Camera Entry */
        funcRet = HAL_camera_device_open(module, id, device);
    }

#ifdef TIME_LOGGER_LAUNCH_ENABLE
    int mainCameraId = -1;
    int subCameraId = -1;

    getDualCameraId(&mainCameraId, &subCameraId);
    if ((isDualCamera(cameraId) == true && cameraId == mainCameraId) ||
           isDualCamera(cameraId) == false) {
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_OPEN_START, false);
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_OPEN_END, true);
    }
#endif

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
};

static int HAL_camera_device_dual_close(struct hw_device_t* device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;

    if (device) {
        camera_device_t *cam_device = (camera_device_t *)device;

        // wide
        int cameraId = obj(cam_device)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->close();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface(%d).close() fail", __FUNCTION__, __LINE__, cameraId);
            }

            // tele
            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->close();
            funcRet = ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface(%d).close() fail", __FUNCTION__, __LINE__, teleCameraId);
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }

                    delete g_threadInterface[cmdCameraId];
                    g_threadInterface[cmdCameraId] = NULL;
                }

                if (g_callbackHooker[cmdCameraId] != NULL) {
                    delete g_callbackHooker[cmdCameraId];
                    g_callbackHooker[cmdCameraId] = NULL;
                }

            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_close(device);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
    }

    /* clear dual mode */
    setAutoDualMode(false);

    /*
     * reset pip / dual camera mode status.
     * it will check open on next time.
     */
    if (getOpenedCameraHalCnt() == 0) {
        setPipMode(false);
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static int HAL_camera_device_dual_set_preview_window(
        struct camera_device *dev,
        struct preview_stream_ops *buf)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->setPreviewWindow(buf);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->setPreviewWindow()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }


            cameraIdList.push_back(teleCameraId);

            /* NULL setting */
            ret = g_threadInterface[teleCameraId]->setPreviewWindow(NULL);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->setPreviewWindow()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_set_preview_window(dev, buf);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static void HAL_camera_device_dual_set_callbacks(
        struct camera_device *dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->setCallbacks()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }


            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->setCallbacks()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            HAL_camera_device_set_callbacks(dev, notify_cb, data_cb, data_cb_timestamp, get_memory, user);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static void HAL_camera_device_dual_enable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->enableMsgType(msg_type);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->enableMsgType()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->enableMsgType(msg_type);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->enableMsgType()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            HAL_camera_device_enable_msg_type(dev, msg_type);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static void HAL_camera_device_dual_disable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->disableMsgType(msg_type);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->disableMsgType()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }


            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->disableMsgType(msg_type);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->disableMsgType()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            HAL_camera_device_disable_msg_type(dev, msg_type);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static int HAL_camera_device_dual_msg_type_enabled(
        struct camera_device *dev,
        int32_t msg_type)
{
    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev)
        ret = HAL_camera_device_msg_type_enabled(dev, msg_type);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static int HAL_camera_device_dual_start_preview(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();
#ifdef TIME_LOGGER_LAUNCH_ENABLE
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_FIRST_PARAMETER_END, false);
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_START_PREVIEW_START, true);
#endif
        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            /* wait for previous api done for master/slave to prevent startPreview -> stopPreview -> startPreview */
            if (g_threadInterface[cameraId] != NULL) {
                ret = g_threadInterface[cameraId]->checkApiDone();
                funcRet |= ret;
                if (ret != NO_ERROR) {
                    ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cameraId);
                }
            }

            if (g_threadInterface[teleCameraId] != NULL) {
                ret = g_threadInterface[teleCameraId]->checkApiDone();
                funcRet |= ret;
                if (ret != NO_ERROR) {
                    ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, teleCameraId);
                }
            }

            /* resetting thread mode */
            g_threadInterface[cameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_NO_THREAD);
            g_threadInterface[teleCameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_BATCH);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->startPreview();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->startPreview() failed", __FUNCTION__, __LINE__, cameraId);
                g_threadInterface[teleCameraId]->flush();
                goto done;
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->startPreview();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->startPreview() failed", __FUNCTION__, __LINE__, teleCameraId);
                g_threadInterface[cameraId]->stopPreview();
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }

            if (cameraId >= 0) {
                if (teleCameraId >= 0 && g_threadInterface[teleCameraId] != NULL)
                    g_threadInterface[teleCameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_IMMEDIATE);

                if (g_threadInterface[cameraId] != NULL)
                    g_threadInterface[cameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_IMMEDIATE);
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_start_preview(dev);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static void HAL_camera_device_dual_stop_preview(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    list<int> cameraIdList;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->stopPreview();
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->stopPreview() fail", __FUNCTION__, __LINE__, cameraId);
            }


            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->stopPreview();
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->stopPreview() fail", __FUNCTION__, __LINE__, teleCameraId);
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            HAL_camera_device_stop_preview(dev);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static int HAL_camera_device_dual_preview_enabled(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_preview_enabled(dev);

        /* tele camera follow wide camera */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_preview_enabled(g_cam_device[teleCameraId]);
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static int HAL_camera_device_dual_store_meta_data_in_buffers(
        struct camera_device *dev,
        int enable)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_store_meta_data_in_buffers(dev, enable);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_store_meta_data_in_buffers(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }

        /* tele camera follow wide camera */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_store_meta_data_in_buffers(g_cam_device[teleCameraId], enable);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_store_meta_data_in_buffers(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static int HAL_camera_device_dual_start_recording(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_start_recording(dev);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_start_recording(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }

        /* recording is happen only in wide camera */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_start_recording(g_cam_device[teleCameraId]);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_start_recording(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static void HAL_camera_device_dual_stop_recording(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        HAL_camera_device_stop_recording(dev);

        /* recording is happen only in wide camera */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            HAL_camera_device_stop_recording(g_cam_device[teleCameraId]);
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static int HAL_camera_device_dual_recording_enabled(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_recording_enabled(dev);

        /* recording is happen only in wide camera */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_recording_enabled(g_cam_device[teleCameraId]);
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static void HAL_camera_device_dual_release_recording_frame(
        struct camera_device *dev,
        const void *opaque)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        HAL_camera_device_release_recording_frame(dev, opaque);

        /* recording is happen only in wide camera */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            HAL_camera_device_release_recording_frame(g_cam_device[teleCameraId], opaque);
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static int HAL_camera_device_dual_auto_focus(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->autoFocus();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->autoFocus()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->autoFocus();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->autoFocus()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_auto_focus(dev);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static int HAL_camera_device_dual_cancel_auto_focus(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->cancelAutoFocus();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->cancelAutoFocus()", __FUNCTION__, __LINE__, cameraId);
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->cancelAutoFocus();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->cancelAutoFocus()", __FUNCTION__, __LINE__, teleCameraId);
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_cancel_auto_focus(dev);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static int HAL_camera_device_dual_take_picture(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->takePicture();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->takePicture()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->takePicture();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->takePicture()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_take_picture(dev);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static int HAL_camera_device_dual_cancel_picture(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->cancelPicture();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->cancelPicture()", __FUNCTION__, __LINE__, cameraId);
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->cancelPicture();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->cancelPicture()", __FUNCTION__, __LINE__, teleCameraId);
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_cancel_picture(dev);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static int HAL_camera_device_dual_set_parameters(
        struct camera_device *dev,
        const char *parms)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

#ifdef TIME_LOGGER_LAUNCH_ENABLE
    if (g_timeLoggerFirstSetParameter == false) {
        int cameraId = obj(dev)->getCameraId();
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_OPEN_END, false);
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_FIRST_PARAMETER_START, true);
    }
#endif

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;

    if (dev) {
        String8 str(parms);
        CameraParameters newWideParams(str);
        int cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);
            {
                /*
                 * if dual_mode(PIP), disable dual camera,
                 * else enable dual camera (but it need to check inside isDualCamera())
                 */
                int newPipMode = newWideParams.getInt("dual_mode");

                if (newPipMode == 1) {
                    setPipMode(true);
                    newWideParams.set("dual_camera_mode", 0);
                } else {
                    setPipMode(false);
                    newWideParams.set("dual_camera_mode", 1);
                }

                ret = g_threadInterface[cameraId]->setParameters(newWideParams.flatten());
                funcRet |= ret;
                if (ret != NO_ERROR) {
                    ALOGE("ERR(%s[%d]):g_threadInterface[%d]->setParameters()", __FUNCTION__, __LINE__, cameraId);
                    goto done;
                }

                /* we must put same ratio's preview and picture size, according to wide camera */

                int teleCameraId = getTeleCameraId(cameraId);

                cameraIdList.push_back(teleCameraId);

                CameraParameters wideParams(str);
                CameraParameters teleParams(str);

                int widePreviewW, widePreviewH = 0;
                int telePreviewW, telePreviewH = 0;

                int widePictureW, widePictureH = 0;
                int telePictureW, telePictureH = 0;

                wideParams.getPreviewSize(&widePreviewW, &widePreviewH);
                teleParams.getPreviewSize(&telePreviewW, &telePreviewH);

                wideParams.getPictureSize(&widePictureW, &widePictureH);
                teleParams.getPictureSize(&telePictureW, &telePictureH);

                float widePreviewRatio = (float)widePreviewW / (float)widePreviewH;
                float telePreviewRatio = (float)telePreviewW / (float)telePreviewH;

                float widePictureRatio = (float)widePictureW / (float)widePictureH;
                float telePictureRatio = (float)telePictureW / (float)telePictureH;

                EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): widePreviewRatio(%f) = widePreviewW(%d) / widePreviewH(%d)", __FUNCTION__, __LINE__, widePreviewRatio, widePreviewW, widePreviewH);
                EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): telePreviewRatio(%f) = telePreviewW(%d) / telePreviewH(%d)", __FUNCTION__, __LINE__, telePreviewRatio, telePreviewW, telePreviewH);

                EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): widePictureRatio(%f) = widePictureW(%d) / widePictureH(%d)", __FUNCTION__, __LINE__, widePictureRatio, widePictureW, widePictureH);
                EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): telePictureRatio(%f) = telePictureW(%d) / telePictureH(%d)", __FUNCTION__, __LINE__, telePictureRatio, telePictureW, telePictureH);

                bool flagFound = false;

                if(widePreviewRatio != telePreviewRatio) {
                    Vector<Size> sizes;
                    teleParams.getSupportedPreviewSizes(sizes);
                    int w, h = 0;
                    flagFound = false;

                    for (unsigned i = 0; i < sizes.size(); i++) {
                        if (((float)sizes[i].width / (float)sizes[i].height) == widePreviewRatio) {
                            w = sizes[i].width;
                            h = sizes[i].height;

                            flagFound = true;
                            break;
                        }
                    }

                    if (flagFound == false) {
                        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):cannot find wide(%f-ratio) preview size on tele(%f-ratio), assert!!!!",
                                __FUNCTION__, __LINE__, widePreviewRatio, telePreviewRatio);
                    }

                    teleParams.setPreviewSize(w, h);
                }

                if(widePictureRatio != telePictureRatio) {
                    Vector<Size> sizes;
                    teleParams.getSupportedPictureSizes(sizes);
                    int w, h = 0;
                    flagFound = false;

                    for (unsigned i = 0; i < sizes.size(); i++) {
                        if (((float)sizes[i].width / (float)sizes[i].height) == widePictureRatio) {
                            w = sizes[i].width;
                            h = sizes[i].height;

                            flagFound = true;
                            break;
                        }
                    }

                    if (flagFound == false) {
                        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):cannot find wide(%f-ratio) picture size on tele(%f-ratio), assert!!!!",
                                __FUNCTION__, __LINE__, widePreviewRatio, telePreviewRatio);
                    }

                    teleParams.setPictureSize(w, h);
                }

                {
                    teleParams.set("dual_camera_mode", 1);

                    ret = g_threadInterface[teleCameraId]->setParameters(teleParams.flatten());
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->setParameters()", __FUNCTION__, __LINE__, teleCameraId);
                        goto done;
                    }
                }
            }
done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_set_parameters(dev, parms);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

#ifdef TIME_LOGGER_LAUNCH_ENABLE
    if (g_timeLoggerFirstSetParameter == false) {
        int cameraId = obj(dev)->getCameraId();
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_FIRST_PARAMETER_START, false);
        TIME_LOGGER_UPDATE_BASE(&g_timeLoggerLaunch, cameraId, g_timeLoggerLaunchIndex, 0, DURATION, LAUNCH_FIRST_PARAMETER_END, true);
        g_timeLoggerFirstSetParameter = true;
    }
#endif

    return funcRet;
}

char *HAL_camera_device_dual_get_parameters(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    char *ret = NULL;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_get_parameters(dev);

        /* return only wide camera's parameters */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_get_parameters(g_cam_device[teleCameraId]);
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static void HAL_camera_device_dual_put_parameters(
        struct camera_device *dev,
        char *parms)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        HAL_camera_device_put_parameters(dev, parms);

        /* put_paramters only on wide */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            HAL_camera_device_put_parameters(g_cam_device[teleCameraId], parms);
        }
        */
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static int HAL_camera_device_dual_send_command(
        struct camera_device *dev,
        int32_t cmd,
        int32_t arg1,
        int32_t arg2)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;
    int cameraId = -1;

    if (dev) {
        cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->sendCommand(cmd, arg1, arg2);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->sendCommand()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->sendCommand(cmd, arg1, arg2);
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->sendCommand()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    funcRet |= ret;
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            funcRet = HAL_camera_device_send_command(dev, cmd, arg1, arg2);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static void HAL_camera_device_dual_release(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    list<int> cameraIdList;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            Mutex::Autolock lock(g_threadInterfaceLock);

            /* Dual Camera Entry */
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(cameraId);

            ret = g_threadInterface[cameraId]->release();
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->release() fail", __FUNCTION__, __LINE__, cameraId);
            }

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->release();
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->release() fail", __FUNCTION__, __LINE__, teleCameraId);
            }

done:

            for (list<int>::iterator it = cameraIdList.begin(); it != cameraIdList.end(); it++) {
                int cmdCameraId = *it;

                if (g_threadInterface[cmdCameraId] != NULL) {
                    ret = g_threadInterface[cmdCameraId]->checkApiDone();
                    if (ret != NO_ERROR) {
                        ALOGE("ERR(%s[%d]):g_threadInterface[%d]->checkApiDone() fail", __FUNCTION__, __LINE__, cmdCameraId);
                    }
                }
            }
        } else {
            /* Not Dual Camera Entry */
            HAL_camera_device_release(dev);
        }
    } else {
        ALOGE("ERR(%s[%d]): dev = NULL", __FUNCTION__, __LINE__);
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static int HAL_camera_device_dual_dump(struct camera_device *dev, int fd)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_dump(dev, fd);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_dump(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }

        /* skip dump on tele */
        /*
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_dump(g_cam_device[teleCameraId], fd);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_dump(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }*/
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

/* #define DEBUG_USE_THREAD_ON_DUAL_CAMERA */
#ifdef DEBUG_USE_THREAD_ON_DUAL_CAMERA
#define DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG CLOGD
#else
#define DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG CLOGV
#endif

ExynosCameraThreadInterface::ExynosCameraThreadInterface()
{
    android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid call, assert!!!!",
        __FUNCTION__, __LINE__);
}

ExynosCameraThreadInterface::ExynosCameraThreadInterface(const struct hw_module_t* module, int cameraId)
{
    ALOGD("DEBUG(%s[%d]):ExynosCameraThreadInterface(%d) constructor", __FUNCTION__, __LINE__, cameraId);

    m_hw_device = NULL;

    m_module = module;
    m_cameraId = cameraId;
    snprintf(m_name,  sizeof(m_name), "ExynosCameraThreadInterface_%d", m_cameraId);

    m_cameraDevice = NULL;

#ifdef USE_THREAD_ON_DUAL_CAMERA_INTERFACE
    m_runMode = RUN_MODE_IMMEDIATE;
#else
    m_runMode = RUN_MODE_NO_THREAD;
#endif

    m_apiThread = new cameraThreadInterface(this,
                                            &ExynosCameraThreadInterface::m_mainThreadFunc,
                                            "ExynosCameraThreadInterface",
                                            PRIORITY_URGENT_DISPLAY);

    m_lastApiType = API_TYPE_BASE;
    m_lastApiRet = NO_ERROR;
}

ExynosCameraThreadInterface::~ExynosCameraThreadInterface()
{
    CLOGD("DEBUG(%s[%d]):ExynosCameraThreadInterface() destructor", __FUNCTION__, __LINE__);

    m_clearApiQ();
}

void ExynosCameraThreadInterface::setRunMode(enum RUM_MODE runMode)
{
    Mutex::Autolock lock(m_apiQLock);

    CLOGD("DEBUG(%s[%d]):m_runMode is changed(%s -> %s)",
        __FUNCTION__, __LINE__, m_runMode2Str(m_runMode), m_runMode2Str(runMode));

    m_runMode = runMode;
}

ExynosCameraThreadInterface::RUM_MODE ExynosCameraThreadInterface::getRunMode(void)
{
    Mutex::Autolock lock(m_apiQLock);

    return m_runMode;
}

status_t ExynosCameraThreadInterface::open(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_OPEN
    enum API_TYPE apiType = API_TYPE_OPEN;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        char strCameraId[3];
        snprintf(strCameraId, sizeof(strCameraId), "%d", m_cameraId);

        ret = HAL_camera_device_open(m_module, strCameraId, &m_hw_device);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_open(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        } else {
            // g_cam_device is alloced in HAL_camera_device_open
            m_cameraDevice = g_cam_device[m_cameraId];
        }
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    char strCameraId[3];
    snprintf(strCameraId, sizeof(strCameraId), "%d", m_cameraId);

    ret = HAL_camera_device_open(m_module, strCameraId, &m_hw_device);
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_open(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
    } else {
        // g_cam_device is alloced in HAL_camera_device_open
        m_cameraDevice = g_cam_device[m_cameraId];
    }
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::setParameters(const char *params)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_SET_PARAMETERS
    enum API_TYPE apiType = API_TYPE_SET_PARAMETERS;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_set_parameters(m_cameraDevice, params);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        apiPacel->setParams(params);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_set_parameters(m_cameraDevice, params);
#endif
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_set_parameters() fail", __FUNCTION__, __LINE__);
    }

    return ret;
}

status_t ExynosCameraThreadInterface::setPreviewWindow(struct preview_stream_ops *buf)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_SET_PREVIEW_WINDOW
    enum API_TYPE apiType = API_TYPE_SET_PREVIEW_WINDOW;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_set_preview_window(m_cameraDevice, buf);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        apiPacel->setPreviewBuf(buf);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_set_preview_window(m_cameraDevice, buf);
#endif
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_set_window() fail", __FUNCTION__, __LINE__);
    }

    return ret;
}

status_t ExynosCameraThreadInterface::setCallbacks(camera_notify_callback notifyCb,
               camera_data_callback dataCb,
               camera_data_timestamp_callback dataCbTimestamp,
               camera_request_memory getMemory,
               void* user)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_SET_CALLBACKS
    enum API_TYPE apiType = API_TYPE_SET_CALLBACKS;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        if (g_callbackHooker[m_cameraId] == NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] == NULL, assert!!!!",
                    __FUNCTION__, __LINE__, m_cameraId);
        }

        g_callbackHooker[m_cameraId]->setCallbacks(g_cam_device[m_cameraId], notifyCb, dataCb, dataCbTimestamp, getMemory, user);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        callback_t callback;
        callback.notifyCb = notifyCb;
        callback.dataCb = dataCb;
        callback.dataCbTimestamp = dataCbTimestamp;
        callback.getMemory = getMemory;
        callback.user = user;
        apiPacel->setCallbacks(&callback);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    if (g_callbackHooker[m_cameraId] == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] == NULL, assert!!!!",
                __FUNCTION__, __LINE__, m_cameraId);
    }

    g_callbackHooker[m_cameraId]->setCallbacks(g_cam_device[m_cameraId], notifyCb, dataCb, dataCbTimestamp, getMemory, user);
#endif
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):g_callbackHooker->setCallbacks fail", __FUNCTION__, __LINE__);
    }

    return ret;
}

status_t ExynosCameraThreadInterface::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_SEND_COMMAND
    enum API_TYPE apiType = API_TYPE_SEND_COMMAND;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail(%d)", __FUNCTION__, __LINE__, m_apiType2Str(apiType), ret);
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_send_command(m_cameraDevice, cmd, arg1, arg2);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        command_t command;
        command.cmd  = cmd;
        command.arg1 = arg1;
        command.arg2 = arg2;
        apiPacel->setCommand(&command);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]): m_runThread(%d) fail(%d)", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_send_command(m_cameraDevice, cmd, arg1, arg2);
#endif
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_send_command(%d) fail(%d)", __FUNCTION__, __LINE__, m_cameraId, ret);
    }

    return ret;
}

status_t ExynosCameraThreadInterface::enableMsgType(int32_t msg)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_ENABLE_MSG_TYPE
    enum API_TYPE apiType = API_TYPE_ENABLE_MSG_TYPE;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        HAL_camera_device_enable_msg_type(m_cameraDevice, msg);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        apiPacel->setMsgType(msg);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    HAL_camera_device_enable_msg_type(m_cameraDevice, msg);
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::disableMsgType(int32_t msg)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_DISABLE_MSG_TYPE
    enum API_TYPE apiType = API_TYPE_DISABLE_MSG_TYPE;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        HAL_camera_device_disable_msg_type(m_cameraDevice, msg);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        apiPacel->setMsgType(msg);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    HAL_camera_device_disable_msg_type(m_cameraDevice, msg);
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::autoFocus(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_AUTO_FOCUS
    enum API_TYPE apiType = API_TYPE_AUTO_FOCUS;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_auto_focus(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_auto_focus(m_cameraDevice);
#endif
    if (ret != 0) {
        ALOGE("ERR(%s[%d]):HAL_camera_device_auto_focus(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        return ret;
    }

    return ret;
}

status_t ExynosCameraThreadInterface::cancelAutoFocus(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_CANCEL_AUTO_FOCUS
    enum API_TYPE apiType = API_TYPE_CANCEL_AUTO_FOCUS;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_cancel_auto_focus(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_cancel_auto_focus(m_cameraDevice);
#endif
    if (ret != 0) {
        ALOGE("ERR(%s[%d]):HAL_camera_device_cancel_auto_focus(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        return ret;
    }

    return ret;
}

status_t ExynosCameraThreadInterface::startPreview(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_START_PREVIEW
    enum API_TYPE apiType = API_TYPE_START_PREVIEW;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_start_preview(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
    }
#else
    ret = HAL_camera_device_start_preview(m_cameraDevice);
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::stopPreview(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_STOP_PREVIEW
    enum API_TYPE apiType = API_TYPE_STOP_PREVIEW;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        HAL_camera_device_stop_preview(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    HAL_camera_device_stop_preview(m_cameraDevice);
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::takePicture(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_TAKE_PICTURE
    enum API_TYPE apiType = API_TYPE_TAKE_PICTURE;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_take_picture(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_take_picture(m_cameraDevice);
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_take_picture() fail", __FUNCTION__, __LINE__);
    }
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::cancelPicture(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_CANCEL_PICTURE
    enum API_TYPE apiType = API_TYPE_CANCEL_PICTURE;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_cancel_picture(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_cancel_picture(m_cameraDevice);
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_take_picture() fail", __FUNCTION__, __LINE__);
    }
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::release(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_RELEASE
    enum API_TYPE apiType = API_TYPE_RELEASE;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        HAL_camera_device_release(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    HAL_camera_device_release(m_cameraDevice);
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::close(void)
{
    status_t ret = NO_ERROR;
#ifdef USE_THREAD_ON_DUAL_CAMERA_CLOSE
    enum API_TYPE apiType = API_TYPE_CLOSE;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_close(m_hw_device);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_close() fail", __FUNCTION__, __LINE__);
        } else {
            m_hw_device = NULL;

            // g_cam_device is free in HAL_camera_device_close
            m_cameraDevice = g_cam_device[m_cameraId];
        }
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_close(m_hw_device);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_close() fail", __FUNCTION__, __LINE__);
    } else {
        m_hw_device = NULL;

        // g_cam_device is free in HAL_camera_device_close
        m_cameraDevice = g_cam_device[m_cameraId];
    }
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::checkApiDone(void)
{
    status_t ret = NO_ERROR;
    enum API_TYPE apiType = API_TYPE_CHECK_API_DONE;
    bool flagRealCheck = false;

    switch (m_runMode) {
    case RUN_MODE_IMMEDIATE:
        flagRealCheck = true;
        break;
    case RUN_MODE_BATCH:
        // It will trigger all apiQ, when start_preview
        if (m_lastApiType == API_TYPE_START_PREVIEW) {
            flagRealCheck = true;
        }
        break;
    case RUN_MODE_NO_THREAD:
        flagRealCheck = true;
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid m_runMode(%s), assert!!!!",
            __FUNCTION__, __LINE__, m_runMode2Str(m_runMode));
        break;
    }

    if (flagRealCheck == true) {
        ret = m_waitThread(apiType);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        }

        ret |= m_lastApiRet;
    }

    return ret;
}

status_t ExynosCameraThreadInterface::flush()
{
    status_t ret = NO_ERROR;

#ifdef USE_THREAD_ON_DUAL_CAMERA_START_PREVIEW
    enum API_TYPE apiType = API_TYPE_FLUSH;

    ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    if (getRunMode() == RUN_MODE_NO_THREAD) {
        ret = HAL_camera_device_start_preview(m_cameraDevice);
    } else {
        ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = new ExynosCameraThreadAPIPacel(apiType);
        ret = m_runThread(apiPacel);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_runThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
    }
#else
    ret = HAL_camera_device_start_preview(m_cameraDevice);
#endif
    return ret;
}


status_t ExynosCameraThreadInterface::m_runThread(ExynosCameraThreadAPIPacelSP_sptr_t apiPacel)
{
    status_t ret = NO_ERROR;

    if (apiPacel == NULL) {
        CLOGE("DEBUG(%s[%d]):cameraId(%d) apiPacel is null. so, skip thread running",
            __FUNCTION__, __LINE__, m_cameraId);
        return INVALID_OPERATION;
    }

    // push on every time
    m_pushApiQ(apiPacel);

    enum API_TYPE apiType = apiPacel->getApiType();

    switch (m_runMode) {
    case RUN_MODE_IMMEDIATE:
        ret = m_apiThread->run();
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):m_apiThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
        }
        break;
    case RUN_MODE_BATCH:
        if (apiType == API_TYPE_START_PREVIEW ||
            apiType == API_TYPE_FLUSH) {
            // it start to trigger.
            ret = m_apiThread->run();
        } else if (apiType == API_TYPE_RELEASE ||
                   apiType == API_TYPE_CLOSE) {
            // we flush except release and close
            m_skipWithoutApiQ(apiType);

            ret = m_apiThread->run();
            if (ret != 0) {
                CLOGE("ERR(%s[%d]):m_apiThread(%d) fail", __FUNCTION__, __LINE__, m_cameraId, ret);
            }
        } else {
            DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("just push(%s) on Q", m_apiType2Str(apiType));
        }

        ret = m_waitThread(apiType);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
            return ret;
        }
        ret = m_lastApiRet;
        break;
    case RUN_MODE_NO_THREAD:
        m_mainThreadFunc();
        ret = m_lastApiRet;
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid m_runMode(%s), assert!!!!",
            __FUNCTION__, __LINE__, m_runMode2Str(m_runMode));
        break;
    }

    return ret;
}

status_t ExynosCameraThreadInterface::m_waitThread(enum API_TYPE apiType)
{
    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):before m_waitThread(%s)",
        __FUNCTION__, __LINE__, m_apiType2Str(apiType));

    status_t ret = m_apiThread->join();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_apiThread->join(%d) fail(%d)", __FUNCTION__, __LINE__, apiType, ret);
    }

    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):after  m_waitThread(%s)",
        __FUNCTION__, __LINE__, m_apiType2Str(apiType));

    return ret;
}

bool ExynosCameraThreadInterface::m_mainThreadFunc(void)
{
    status_t ret = NO_ERROR;
    bool     boolRet = false;

    char strCameraId[3];
    enum API_TYPE apiType;
    command_t *command;
    callback_t *callback;

    ExynosCameraThreadAPIPacelSP_sptr_t apiPacel = m_popApiQ();
    if (apiPacel == NULL) {
        CLOGE("DEBUG(%s[%d]):cameraId(%d) apiPacel is null. so, skip thread running",
            __FUNCTION__, __LINE__, m_cameraId);
        goto done;
    }

    apiType = apiPacel->getApiType();
    if (apiType == API_TYPE_BASE) {
        CLOGD("DEBUG(%s[%d]):cameraId(%d) apiType is API_TYPE_BASE. so, skip thread running apiType(%d)",
            __FUNCTION__, __LINE__, m_cameraId, apiType);
        goto done;
    }

    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):cameraId(%d) api(%s) start",
        __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType));

    if (m_cameraDevice == NULL && apiType != API_TYPE_OPEN) {
        CLOGD("DEBUG(%s[%d]):cameraId(%d) m_cameraDevice == NULL. so, skip thread running apiType(%s)",
            __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType));
        goto done;
    }

    switch (apiType) {
    case API_TYPE_OPEN:
        snprintf(strCameraId, sizeof(strCameraId), "%d", HAL_revertCameraId(m_cameraId));
        ret = HAL_camera_device_open(m_module, strCameraId, &m_hw_device);

        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_open(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        } else {
            // g_cam_device is alloced in HAL_camera_device_open
            m_cameraDevice = g_cam_device[m_cameraId];
        }
        break;
    case API_TYPE_SET_PARAMETERS:
        ret = HAL_camera_device_set_parameters(m_cameraDevice, apiPacel->getParams());
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_set_parameters() fail", __FUNCTION__, __LINE__);
        }
        break;
    case API_TYPE_SET_PREVIEW_WINDOW:
        ret = HAL_camera_device_set_preview_window(m_cameraDevice, apiPacel->getPreviewBuf());
        break;
    case API_TYPE_SET_CALLBACKS:
        if (g_callbackHooker[m_cameraId] == NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] == NULL, assert!!!!",
                    __FUNCTION__, __LINE__, m_cameraId);
        }

        callback = apiPacel->getCallback();
        g_callbackHooker[m_cameraId]->setCallbacks(g_cam_device[m_cameraId],
                callback->notifyCb, callback->dataCb, callback->dataCbTimestamp,
                callback->getMemory, callback->user);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):g_callbackHooker->setCallbacks fail", __FUNCTION__, __LINE__);
        }
        break;
    case API_TYPE_SEND_COMMAND:
        command = apiPacel->getCommand();
        ret = HAL_camera_device_send_command(m_cameraDevice, command->cmd, command->arg1, command->arg2);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_send_command(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        }

        DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):cameraId(%d) api(%s) sendCommand(%d, %d, %d)",
                __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType), command->cmd, command->arg1, command->arg2);
        break;
    case API_TYPE_ENABLE_MSG_TYPE:
        HAL_camera_device_enable_msg_type(m_cameraDevice, apiPacel->getMsgType());

        DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):cameraId(%d) api(%s) msgType(0x%x)",
                __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType), apiPacel->getMsgType());
        break;
    case API_TYPE_DISABLE_MSG_TYPE:
        HAL_camera_device_disable_msg_type(m_cameraDevice, apiPacel->getMsgType());

        DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):cameraId(%d) api(%s) msgType(0x%x)",
                __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType), apiPacel->getMsgType());
        break;
    case API_TYPE_AUTO_FOCUS:
        ret = HAL_camera_device_auto_focus(m_cameraDevice);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_auto_focus(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        }
        break;
    case API_TYPE_CANCEL_AUTO_FOCUS:
        ret = HAL_camera_device_cancel_auto_focus(m_cameraDevice);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_cancel_auto_focus(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        }
        break;
    case API_TYPE_START_PREVIEW:
        ret = HAL_camera_device_start_preview(m_cameraDevice);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_start_preview() fail", __FUNCTION__, __LINE__);
        }
        break;
    case API_TYPE_STOP_PREVIEW:
        HAL_camera_device_stop_preview(m_cameraDevice);
        break;
    case API_TYPE_TAKE_PICTURE:
        ret = HAL_camera_device_take_picture(m_cameraDevice);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_take_picture() fail", __FUNCTION__, __LINE__);
        }
        break;
    case API_TYPE_CANCEL_PICTURE:
        ret = HAL_camera_device_cancel_picture(m_cameraDevice);
        if (ret != 0) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_cancel_picture() fail", __FUNCTION__, __LINE__);
        }
        break;
    case API_TYPE_RELEASE:
        HAL_camera_device_release(m_cameraDevice);
        break;
    case API_TYPE_CLOSE:
        ret = HAL_camera_device_close(m_hw_device);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_close() fail", __FUNCTION__, __LINE__);
        } else {
            m_hw_device = NULL;

            // g_cam_device is free in HAL_camera_device_close
            m_cameraDevice = g_cam_device[m_cameraId];
        }

        break;
    case API_TYPE_FLUSH:
        CLOGD("DEBUG(%s[%d]):cameraId(%d) api(%s)",__FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType));
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid apiType(%d), assert!!!!",
            __FUNCTION__, __LINE__, apiType);
        break;
    }

    m_lastApiType = apiType;
    m_lastApiRet = ret;

    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):cameraId(%d) api(%s) end",
        __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType));

done:
    if (m_runMode == RUN_MODE_BATCH) {
        if (/* m_lastApiRet == NO_ERROR && */
            m_sizeOfApiQ() != 0) {
            // repeat api operation.
            boolRet = true;
        } else {
            // TODO : error callback
        }
    }

    /* run once */
    return boolRet;
}

void ExynosCameraThreadInterface::m_pushApiQ(ExynosCameraThreadAPIPacelSP_sptr_t apiPacel)
{
    Mutex::Autolock lock(m_apiQLock);

    int qSize = m_apiQ.size();
    enum API_TYPE apiType = apiPacel->getApiType();

    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):m_apiQ.size is (%d). so, push apiType(%s)",
            __FUNCTION__, __LINE__, qSize, m_apiType2Str(apiType));

    m_apiQ.push_back(apiPacel);
}

ExynosCameraThreadAPIPacelSP_sptr_t ExynosCameraThreadInterface::m_popApiQ(void)
{
    Mutex::Autolock lock(m_apiQLock);

    ExynosCameraThreadAPIPacelSP_sptr_t apiPacel;

    enum API_TYPE apiType = API_TYPE_BASE;

    if (m_apiQ.empty()) {
        CLOGD("m_apiQ is empty");
        return NULL;
    }

    List<ExynosCameraThreadAPIPacelSP_sptr_t>::iterator r;

    r = m_apiQ.begin()++;
    apiPacel = *r;
    m_apiQ.erase(r);

    return apiPacel;
}

void ExynosCameraThreadInterface::m_skipApiQ(enum API_TYPE apiType)
{
    Mutex::Autolock lock(m_apiQLock);

    enum API_TYPE curApiType = API_TYPE_BASE;

    if (m_apiQ.empty()) {
        CLOGD("m_apiQ is empty");
        return;
    }

    List<ExynosCameraThreadAPIPacelSP_sptr_t>::iterator r;

    r = m_apiQ.begin()++;

    do {
        curApiType = (*r)->getApiType();

        if (curApiType == apiType) {
            DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("curApiType(%s) == apiType(%s). so, skip",
                m_apiType2Str(curApiType), m_apiType2Str(apiType));
            r = m_apiQ.erase(r);
        } else {
            r++;
        }
    } while (r != m_apiQ.end());
}

void ExynosCameraThreadInterface::m_skipApiQ(enum API_TYPE apiType1, enum API_TYPE apiType2)
{
    Mutex::Autolock lock(m_apiQLock);

    enum API_TYPE curApiType = API_TYPE_BASE;

    if (m_apiQ.empty()) {
        CLOGD("m_apiQ is empty");
        return;
    }

    List<ExynosCameraThreadAPIPacelSP_sptr_t>::iterator r;

    r = m_apiQ.begin()++;

    do {
        curApiType = (*r)->getApiType();

        if (curApiType == apiType1) {
            DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("curApiType(%s) == apiType1(%s). so, skip",
                m_apiType2Str(curApiType), m_apiType2Str(apiType1));
            r = m_apiQ.erase(r);
        }

        if (curApiType == apiType2) {
            DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("curApiType(%s) == apiType2(%s). so, skip",
                m_apiType2Str(curApiType), m_apiType2Str(apiType2));
            r = m_apiQ.erase(r);
        } else {
            r++;
        }
    } while (r != m_apiQ.end());
}

void ExynosCameraThreadInterface::m_skipWithoutApiQ(enum API_TYPE apiType)
{
    Mutex::Autolock lock(m_apiQLock);

    enum API_TYPE curApiType = API_TYPE_BASE;

    if (m_apiQ.empty()) {
        CLOGD("m_apiQ is empty");
        return;
    }

    List<ExynosCameraThreadAPIPacelSP_sptr_t>::iterator r;

    r = m_apiQ.begin()++;

    do {
        curApiType = (*r)->getApiType();

        if (curApiType != apiType) {
            DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("curApiType(%s) != apiType(%s). so, erase",
                m_apiType2Str(curApiType), m_apiType2Str(apiType));
            r = m_apiQ.erase(r);
        } else {
            r++;
        }
    } while (r != m_apiQ.end());
}

int ExynosCameraThreadInterface::m_sizeOfApiQ(void)
{
    Mutex::Autolock lock(m_apiQLock);

    return m_apiQ.size();
}

void ExynosCameraThreadInterface::m_clearApiQ(void)
{
    Mutex::Autolock lock(m_apiQLock);
    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("m_clearApiQ");

    m_apiQ.clear();
}

char *ExynosCameraThreadInterface::m_apiType2Str(enum API_TYPE apiType)
{
    char *str = NULL;

    switch (apiType) {
    case API_TYPE_BASE:
        str = "API_TYPE_BASE";
        break;
    case API_TYPE_OPEN:
        str = "API_TYPE_OPEN";
        break;
    case API_TYPE_SET_PARAMETERS:
        str = "API_TYPE_SET_PARAMETERS";
        break;
    case API_TYPE_SET_PREVIEW_WINDOW:
        str = "API_TYPE_SET_PREVIEW_WINDOW";
        break;
    case API_TYPE_SET_CALLBACKS:
        str = "API_TYPE_SET_CALLBACKS";
        break;
    case API_TYPE_SEND_COMMAND:
        str = "API_TYPE_SEND_COMMAND";
        break;
    case API_TYPE_ENABLE_MSG_TYPE:
        str = "API_TYPE_ENABLE_MSG_TYPE";
        break;
    case API_TYPE_DISABLE_MSG_TYPE:
        str = "API_TYPE_DISABLE_MSG_TYPE";
        break;
    case API_TYPE_AUTO_FOCUS:
        str = "API_TYPE_AUTO_FOCUS";
        break;
    case API_TYPE_CANCEL_AUTO_FOCUS:
        str = "API_TYPE_CANCEL_AUTO_FOCUS";
        break;
    case API_TYPE_START_PREVIEW:
        str = "API_TYPE_START_PREVIEW";
        break;
    case API_TYPE_STOP_PREVIEW:
        str = "API_TYPE_STOP_PREVIEW";
        break;
    case API_TYPE_TAKE_PICTURE:
        str = "API_TYPE_TAKE_PICTURE";
        break;
    case API_TYPE_CANCEL_PICTURE:
        str = "API_TYPE_CANCEL_PICTURE";
        break;
    case API_TYPE_RELEASE:
        str = "API_TYPE_RELEASE";
        break;
    case API_TYPE_CLOSE:
        str = "API_TYPE_CLOSE";
        break;
    case API_TYPE_CHECK_API_DONE:
        str = "API_TYPE_CHECK_API_DONE";
        break;
    case API_TYPE_FLUSH:
        str = "API_TYPE_FLUSH";
        break;
    case API_TYPE_MAX:
        str = "API_TYPE_MAX";
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid apiType(%d), assert!!!!",
                    __FUNCTION__, __LINE__, apiType);
        break;
    }

    return str;
}

char *ExynosCameraThreadInterface::m_runMode2Str(enum RUM_MODE runMode)
{
    char *str = NULL;

    switch (runMode) {
    case RUN_MODE_BASE:
        str = "RUN_MODE_BASE";
        break;
    case RUN_MODE_IMMEDIATE:
        str = "RUN_MODE_IMMEDIATE";
        break;
    case RUN_MODE_BATCH:
        str = "RUN_MODE_BATCH";
        break;
    case RUN_MODE_NO_THREAD:
        str = "RUN_MODE_NO_THREAD";
        break;
    case RUN_MODE_MAX:
        str = "RUN_MODE_MAX";
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Invalid runMode(%d), assert!!!!",
                    __FUNCTION__, __LINE__, runMode);
        break;
    }

    return str;
}

}; /* namespace android */
