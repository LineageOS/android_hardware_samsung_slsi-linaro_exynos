/*
**
** Copyright 2013, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraDualInterface"
#include <cutils/log.h>
#include <list>

#include "ExynosCameraDualInterface.h"
#include "ExynosCameraAutoTimer.h"

namespace android {

using namespace std;

static int HAL_camera_device_open(
        const struct hw_module_t* module,
        const char *id,
        struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(id);

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

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);
    if (cameraId < 0 || cameraId >= EXYNOS_CAMERA_INTERNAL_NUM) {
        ALOGE("ERR(%s):Invalid camera ID %s", __FUNCTION__, id);
        return -EINVAL;
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

    g_cam_device[cameraId]->common.close   = HAL_camera_device_dual_close;

    g_cam_openLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

done:
    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();
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

    ret = obj(dev)->startPreview();
    ALOGI("INFO(%s[%d]):camera(%d) out from startPreview()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_previewLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

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
        __unused struct camera_device *dev,
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
    ALOGI("INFO(%s[%d]):camera(%d) out from release()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_openLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

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

static int HAL_getNumberOfCameras()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
}

static int HAL_set_callbacks(__unused const camera_module_callbacks_t *callbacks)
{
    ALOGW("WRN(%s): not supported API level 1.0", __FUNCTION__);
    return 0;
}

static int HAL_getCameraInfo(int cameraId, struct camera_info *info)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %d", __FUNCTION__, cameraId);
        return -EINVAL;
    }

    memcpy(info, &sCameraInfo[cameraId], sizeof(CameraInfo));
    info->device_version = HARDWARE_DEVICE_API_VERSION(1, 0);

    return NO_ERROR;
}

static int HAL_open_legacy(__unused const struct hw_module_t* module, __unused const char* id, __unused uint32_t halVersion, __unused struct hw_device_t** device)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return NO_ERROR;
}

static void HAL_get_vendor_tag_ops(__unused vendor_tag_ops_t* ops)
{
    ALOGV("INFO(%s):", __FUNCTION__);
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

    // wide
    if (g_threadInterface[cameraId] != NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] != NULL, assert!!!!",
                    __FUNCTION__, __LINE__, cameraId);
    }

    g_threadInterface[cameraId] = new ExynosCameraThreadInterface(module, cameraId);
    g_threadInterface[cameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_IMMEDIATE);

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
    if (isDualCamera(cameraId) == true) {
        int teleCameraId = getTeleCameraId(cameraId);

        cameraIdList.push_back(teleCameraId);

        if (g_threadInterface[teleCameraId] != NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_threadInterface[%d] != NULL, assert!!!!",
                __FUNCTION__, __LINE__, teleCameraId);
        }

        g_threadInterface[teleCameraId] = new ExynosCameraThreadInterface(module, teleCameraId);
        g_threadInterface[teleCameraId]->setRunMode(ExynosCameraThreadInterface::RUN_MODE_IMMEDIATE);

        if (g_callbackHooker[teleCameraId] != NULL) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] != NULL, assert!!!!",
                        __FUNCTION__, __LINE__, teleCameraId);
        }

        // callback hooking on tele camera.
        g_callbackHooker[teleCameraId] = new ExynosCameraCallbackHooker(teleCameraId);
        g_callbackHooker[teleCameraId]->setHooking(true);

        ret = g_threadInterface[teleCameraId]->open();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface[%d]->open() fail", __FUNCTION__, __LINE__, teleCameraId);
            goto done;
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

        cameraIdList.push_back(cameraId);

        ret = g_threadInterface[cameraId]->close();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface(%d).close() fail", __FUNCTION__, __LINE__, cameraId);
        }

        // tele
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);

        cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->close();
            funcRet = ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface(%d).close() fail", __FUNCTION__, __LINE__, teleCameraId);
            }
        }

        /*
         * reset pip / dual camera mode status.
         * it will check open on next time.
         */
        if (getOpenedCameraHalCnt() == 0) {
            setPipMode(false);
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

            delete g_threadInterface[cmdCameraId];
            g_threadInterface[cmdCameraId] = NULL;
        }

        if (g_callbackHooker[cmdCameraId] != NULL) {
            delete g_callbackHooker[cmdCameraId];
            g_callbackHooker[cmdCameraId] = NULL;
        }
    }

    /* clear dual mode */
    setAutoDualMode(false);

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

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_set_preview_window(dev, buf);
        if (ret != 0) {
            ALOGW("WARN(%s[%d]):HAL_camera_device_set_preview_window(%d) fail", __FUNCTION__, __LINE__, cameraId);
            /* return ret; */
        }

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);

            /* this is set with NULL*/
            ret = HAL_camera_device_set_preview_window(g_cam_device[teleCameraId], NULL);
            if (ret != 0) {
                ALOGW("WARN(%s[%d]):HAL_camera_device_set_preview_window(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                /* return ret; */
            }
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
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

    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        if (isDualCamera(cameraId) == true) {
            // wide
            if (g_callbackHooker[cameraId] == NULL) {
                android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] == NULL, assert!!!!",
                    __FUNCTION__, __LINE__, cameraId);
            }

            g_callbackHooker[cameraId]->setCallbacks(g_cam_device[cameraId], notify_cb, data_cb, data_cb_timestamp, get_memory, user);

            // tele
            int teleCameraId = getTeleCameraId(cameraId);

            if (g_callbackHooker[teleCameraId] == NULL) {
                android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):g_callbackHooker[%d] == NULL, assert!!!!",
                    __FUNCTION__, __LINE__, teleCameraId);
            }

            g_callbackHooker[teleCameraId]->setCallbacks(g_cam_device[teleCameraId], notify_cb, data_cb, data_cb_timestamp, get_memory, user);
        } else {
            HAL_camera_device_set_callbacks(dev, notify_cb, data_cb, data_cb_timestamp, get_memory, user);
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static void HAL_camera_device_dual_enable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        HAL_camera_device_enable_msg_type(dev, msg_type);

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            HAL_camera_device_enable_msg_type(g_cam_device[teleCameraId], msg_type);
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static void HAL_camera_device_dual_disable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        HAL_camera_device_disable_msg_type(dev, msg_type);

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            HAL_camera_device_disable_msg_type(g_cam_device[teleCameraId], msg_type);
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

static int HAL_camera_device_dual_msg_type_enabled(
        struct camera_device *dev,
        int32_t msg_type)
{
    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_msg_type_enabled(dev, msg_type);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_msg_type_enabled(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);

            /* this is set with NULL*/
            ret = HAL_camera_device_msg_type_enabled(g_cam_device[teleCameraId], msg_type);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_msg_type_enabled(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
    }

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


    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        cameraIdList.push_back(cameraId);

        ret = g_threadInterface[cameraId]->startPreview();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface[%d]->startPreview()", __FUNCTION__, __LINE__, cameraId);
            goto done;
        }

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->startPreview();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->startPreview()", __FUNCTION__, __LINE__, teleCameraId);
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

        cameraIdList.push_back(cameraId);

        ret = g_threadInterface[cameraId]->stopPreview();
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface[%d]->stopPreview() fail", __FUNCTION__, __LINE__, cameraId);
        }

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->stopPreview();
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->stopPreview() fail", __FUNCTION__, __LINE__, teleCameraId);
            }
        }
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

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_store_meta_data_in_buffers(g_cam_device[teleCameraId], enable);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_store_meta_data_in_buffers(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
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

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_auto_focus(dev);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_auto_focus(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_auto_focus(g_cam_device[teleCameraId]);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_auto_focus(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static int HAL_camera_device_dual_cancel_auto_focus(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_cancel_auto_focus(dev);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_cancel_auto_focus(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_cancel_auto_focus(g_cam_device[teleCameraId]);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_cancel_auto_focus(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
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

        cameraIdList.push_back(cameraId);

        /*
         * take_picture is happen only in wide camera
         * callback will block in ExynosCameraCallbackHooker
         * And slave camera's takepicture should be called first.
         * The msg_type can be changed as soon as master camera's callback was finished.
         * So slave camera's pictureThread can be affected by this.
         */

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->takePicture();
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->takePicture()", __FUNCTION__, __LINE__, teleCameraId);
                goto done;
            }
        }

        ret = g_threadInterface[cameraId]->takePicture();
        funcRet |= ret;
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface[%d]->takePicture()", __FUNCTION__, __LINE__, cameraId);
            goto done;
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

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return funcRet;
}

static int HAL_camera_device_dual_cancel_picture(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        /*
         * cancel_picture is happen only in wide camera
         * callback will block in ExynosCameraCallbackHooker
         */
        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_cancel_picture(g_cam_device[teleCameraId]);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_cancel_picture(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }

    ret = HAL_camera_device_cancel_picture(dev);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_cancel_picture(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static int HAL_camera_device_dual_set_parameters(
        struct camera_device *dev,
        const char *parms)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    int funcRet = 0;
    list<int> cameraIdList;

    if (dev) {
        String8 str(parms);
        CameraParameters newWideParams(str);
        int cameraId = obj(dev)->getCameraId();

        cameraIdList.push_back(cameraId);
        {
            if (isDualCamera(cameraId) == true) {
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
            }

            ret = g_threadInterface[cameraId]->setParameters(newWideParams.flatten());
            funcRet |= ret;
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->setParameters()", __FUNCTION__, __LINE__, cameraId);
                goto done;
            }
        }

        if (isDualCamera(cameraId) == true) {
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

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

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
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();
        ret = HAL_camera_device_send_command(dev, cmd, arg1, arg2);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):HAL_camera_device_send_command(%d) fail", __FUNCTION__, __LINE__, cameraId);
            return ret;
        }

        if (isDualCamera(cameraId) == true) {
            /* we must put same ratio's preview and picture size, according to wide camera */

            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_send_command(g_cam_device[teleCameraId], cmd, arg1, arg2);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_send_command(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

static void HAL_camera_device_dual_release(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;
    list<int> cameraIdList;

    if (dev) {
        int cameraId = obj(dev)->getCameraId();

        cameraIdList.push_back(cameraId);

        ret = g_threadInterface[cameraId]->release();
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):g_threadInterface[%d]->release() fail", __FUNCTION__, __LINE__, cameraId);
        }

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);

            cameraIdList.push_back(teleCameraId);

            ret = g_threadInterface[teleCameraId]->release();
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):g_threadInterface[%d]->release() fail", __FUNCTION__, __LINE__, teleCameraId);
            }
        }
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

        if (isDualCamera(cameraId) == true) {
            int teleCameraId = getTeleCameraId(cameraId);
            ret = HAL_camera_device_dump(g_cam_device[teleCameraId], fd);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]):HAL_camera_device_dump(%d) fail", __FUNCTION__, __LINE__, teleCameraId);
                return ret;
            }
        }
    }

    EXYNOS_CAMERA_DUAL_CAMERA_IF_DEBUG_LOG("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

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
    enum API_TYPE apiType = API_TYPE_OPEN;

    status_t ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

#ifdef USE_THREAD_ON_DUAL_CAMERA_OPEN
    ret = m_runThread(apiType);
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
    enum API_TYPE apiType = API_TYPE_SET_PARAMETERS;

    status_t ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

#ifdef USE_THREAD_ON_DUAL_CAMERA_SET_PARAMETERS
    ret = m_runThread(apiType, params);
#else
    ret = HAL_camera_device_set_parameters(m_cameraDevice, params);
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_set_parameters() fail", __FUNCTION__, __LINE__);
    }
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::startPreview(void)
{
    enum API_TYPE apiType = API_TYPE_START_PREVIEW;

    status_t ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

    ret = m_runThread(API_TYPE_START_PREVIEW);

    return ret;
}

status_t ExynosCameraThreadInterface::stopPreview(void)
{
    enum API_TYPE apiType = API_TYPE_STOP_PREVIEW;

    status_t ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

#ifdef USE_THREAD_ON_DUAL_CAMERA_STOP_PREVIEW
    ret = m_runThread(apiType);
#else
    HAL_camera_device_stop_preview(m_cameraDevice);
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::takePicture(void)
{
    enum API_TYPE apiType = API_TYPE_TAKE_PICTURE;

    status_t ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

#ifdef USE_THREAD_ON_DUAL_CAMERA_TAKE_PICTURE
    ret = m_runThread(apiType);
#else
    ret = HAL_camera_device_take_picture(m_cameraDevice);
    if (ret != 0) {
        CLOGE("ERR(%s[%d]):HAL_camera_device_take_picture() fail", __FUNCTION__, __LINE__);
    }
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::release(void)
{
    enum API_TYPE apiType = API_TYPE_RELEASE;

    status_t ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

#ifdef USE_THREAD_ON_DUAL_CAMERA_API_TYPE_RELEASE
    ret = m_runThread(apiType);
#else
    HAL_camera_device_release(m_cameraDevice);
#endif

    return ret;
}

status_t ExynosCameraThreadInterface::close(void)
{
    enum API_TYPE apiType = API_TYPE_CLOSE;

    status_t ret = m_waitThread(apiType);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_waitThread(%s) fail", __FUNCTION__, __LINE__, m_apiType2Str(apiType));
        return ret;
    }

#ifdef USE_THREAD_ON_DUAL_CAMERA_CLOSE
    ret = m_runThread(apiType);
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
#if 1
        // we must wait, when release and close
        if (m_lastApiType == API_TYPE_RELEASE ||
            m_lastApiType == API_TYPE_CLOSE) {
            flagRealCheck = true;
        }
#else
        flagRealCheck = true;
#endif
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

status_t ExynosCameraThreadInterface::m_runThread(enum API_TYPE apiType, const char *params)
{
    status_t ret = NO_ERROR;

    if (params != NULL)
        m_params.setTo(params);

    // push on every time
    m_pushApiQ(apiType);

    switch (m_runMode) {
    case RUN_MODE_IMMEDIATE:
        ret = m_apiThread->run();
        break;
    case RUN_MODE_BATCH:
        if (apiType == API_TYPE_OPEN ||
            apiType == API_TYPE_START_PREVIEW ||
            apiType == API_TYPE_STOP_PREVIEW ||
            apiType == API_TYPE_TAKE_PICTURE) {
            // it start to trigger.
            ret = m_apiThread->run();
        } else if (apiType == API_TYPE_RELEASE ||
                   apiType == API_TYPE_CLOSE) {
            // we flush except release and close
            m_skipWithoutApiQ(apiType);

            ret = m_apiThread->run();
        } else {
            DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("just push(%s) on Q", m_apiType2Str(apiType));
        }

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
        CLOGE("ERR(%s[%d]):m_apiThread->join(%d) fail", __FUNCTION__, __LINE__, apiType);
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

    enum API_TYPE apiType = m_popApiQ();

    if (apiType == API_TYPE_BASE) {
        CLOGD("DEBUG(%s[%d]):cameraId(%d) apiType is API_TYPE_BASE. so, skip thread running",
            __FUNCTION__, __LINE__, m_cameraId);
        goto done;
    }

    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):cameraId(%d) api(%s) start",
        __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType));

    switch (apiType) {
    case API_TYPE_OPEN:
        snprintf(strCameraId, sizeof(strCameraId), "%d", m_cameraId);

        ret = HAL_camera_device_open(m_module, strCameraId, &m_hw_device);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_open(%d) fail", __FUNCTION__, __LINE__, m_cameraId);
        } else {
            // g_cam_device is alloced in HAL_camera_device_open
            m_cameraDevice = g_cam_device[m_cameraId];
        }
        break;
    case API_TYPE_SET_PARAMETERS:
        ret = HAL_camera_device_set_parameters(m_cameraDevice, m_params);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):HAL_camera_device_set_parameters() fail", __FUNCTION__, __LINE__);
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
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid apiType(%d), assert!!!!",
            __FUNCTION__, __LINE__, apiType);
        break;
    }

    m_lastApiType = apiType;
    m_lastApiRet = ret;

    DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):cameraId(%d) api(%s) end",
        __FUNCTION__, __LINE__, m_cameraId, m_apiType2Str(apiType));

    if (m_runMode == RUN_MODE_BATCH) {
        if (/* m_lastApiRet == NO_ERROR && */
            m_sizeOfApiQ() != 0) {
            // repeat api operation.
            boolRet = true;
        } else {
            // TODO : error callback
        }
    }

done:
    /* run once */
    return boolRet;
}

void ExynosCameraThreadInterface::m_pushApiQ(enum API_TYPE apiType)
{
    Mutex::Autolock lock(m_apiQLock);

    int qSize = m_apiQ.size();

    if (qSize < EXYNOS_CAMERA_THREAD_API_Q_SIZE) {
        DEBUG_USE_THREAD_ON_DUAL_CAMERA_LOG("DEBUG(%s[%d]):m_apiQ.size is (%d). so, push apiType(%s)",
            __FUNCTION__, __LINE__, qSize, m_apiType2Str(apiType));
    } else {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):im_apiQ.size(%d) is too big, when puth apiType(%s). EXYNOS_CAMERA_THREAD_API_Q_SIZE(%d), assert!!!!",
            __FUNCTION__, __LINE__, qSize, m_apiType2Str(apiType), EXYNOS_CAMERA_THREAD_API_Q_SIZE);
    }

    m_apiQ.push_back(apiType);
}

ExynosCameraThreadInterface::API_TYPE ExynosCameraThreadInterface::m_popApiQ(void)
{
    Mutex::Autolock lock(m_apiQLock);

    enum API_TYPE apiType = API_TYPE_BASE;

    if (m_apiQ.empty()) {
        CLOGD("m_apiQ is empty");
        return apiType;
    }

    List<enum API_TYPE>::iterator r;

    r = m_apiQ.begin()++;
    apiType = *r;
    m_apiQ.erase(r);

    return apiType;
}

void ExynosCameraThreadInterface::m_skipApiQ(enum API_TYPE apiType)
{
    Mutex::Autolock lock(m_apiQLock);

    enum API_TYPE curApiType = API_TYPE_BASE;

    if (m_apiQ.empty()) {
        CLOGD("m_apiQ is empty");
        return;
    }

    List<enum API_TYPE>::iterator r;

    r = m_apiQ.begin()++;

    do {
        curApiType = *r;

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

    List<enum API_TYPE>::iterator r;

    r = m_apiQ.begin()++;

    do {
        curApiType = *r;

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

    List<enum API_TYPE>::iterator r;

    r = m_apiQ.begin()++;

    do {
        curApiType = *r;

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
    case API_TYPE_START_PREVIEW:
        str = "API_TYPE_START_PREVIEW";
        break;
    case API_TYPE_STOP_PREVIEW:
        str = "API_TYPE_STOP_PREVIEW";
        break;
    case API_TYPE_TAKE_PICTURE:
        str = "API_TYPE_TAKE_PICTURE";
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
