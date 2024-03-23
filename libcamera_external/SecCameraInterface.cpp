/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /*!
 * \file      SecCameraInterface.cpp
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#ifndef ANDROID_HARDWARE_SECCAMERAINTERFACE_CPP
#define ANDROID_HARDWARE_SECCAMERAINTERFACE_CPP

#define LOG_NDEBUG 0
#define LOG_TAG "SecCameraHardware"

#include "SecCameraInterface.h"
#include "SecCameraHardware.h"

namespace android {

const static CameraInfo sCameraInfo[] = SEC_CAMERA_INFO;

/* Close this device */

static camera_device_t *g_cam_device;

static inline SecCameraHardware *obj(struct camera_device *dev)
{
    return reinterpret_cast<SecCameraHardware *>(dev->priv);
}

static int HAL_camera_device_close(struct hw_device_t *device)
{
    if (!g_cam_device)
        ALOGI("camera device already closed");

    if (device) {
        camera_device_t *cam_device = (camera_device_t *)device;
        ALOGI("camera device closed %d", obj(cam_device)->getCameraId());
        delete static_cast<SecCameraHardware *>(cam_device->priv);
        free(cam_device);
        g_cam_device = 0;
    }

    return 0;
}

/* Set the preview_stream_ops to which preview frames are sent */
static int HAL_camera_device_set_preview_window(struct camera_device *dev,
                                                struct preview_stream_ops *buf)
{
    HALDBG("%s", __FUNCTION__);
    return obj(dev)->setPreviewWindow(buf);
}

/* Set the notification and data callbacks */
static void HAL_camera_device_set_callbacks(struct camera_device *dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    HALDBG("%s", __FUNCTION__);
    obj(dev)->setCallbacks(notify_cb, data_cb, data_cb_timestamp,
                           get_memory,
                           user);
}

/*
 * The following three functions all take a msg_type, which is a bitmask of
 * the messages defined in include/ui/Camera.h
 */

/*
 * Enable a message, or set of messages.
 */
static void HAL_camera_device_enable_msg_type(struct camera_device *dev, int32_t msg_type)
{
    HALDBG("%s", __FUNCTION__);
    obj(dev)->enableMsgType(msg_type);
}

/**
 * Disable a message, or a set of messages.
 *
 * Once received a call to disableMsgType(CAMERA_MSG_VIDEO_FRAME), camera
 * HAL should not rely on its client to call releaseRecordingFrame() to
 * release video recording frames sent out by the cameral HAL before and
 * after the disableMsgType(CAMERA_MSG_VIDEO_FRAME) call. Camera HAL
 * clients must not modify/access any video recording frame after calling
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME).
 */
static void HAL_camera_device_disable_msg_type(struct camera_device *dev, int32_t msg_type)
{
    HALDBG("%s", __FUNCTION__);
    obj(dev)->disableMsgType(msg_type);
}

/*
 * Query whether a message, or a set of messages, is enabled.  Note that
 * this is operates as an AND, if any of the messages queried are off, this
 * will return false.
 */
static int HAL_camera_device_msg_type_enabled(struct camera_device *dev, int32_t msg_type)
{
    HALDBG("%s", __FUNCTION__);
    return obj(dev)->msgTypeEnabled(msg_type);
}

/*
 * Start preview mode.
 */
static int HAL_camera_device_start_preview(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->startPreview();
}

/*
 * Stop a previously started preview.
 */
static void HAL_camera_device_stop_preview(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    obj(dev)->stopPreview();
}

/*
 * Returns true if preview is enabled.
 */
static int HAL_camera_device_preview_enabled(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->previewEnabled();
}

/*
 * Request the camera HAL to store meta data or real YUV data in the video
 * buffers sent out via CAMERA_MSG_VIDEO_FRAME for a recording session. If
 * it is not called, the default camera HAL behavior is to store real YUV
 * data in the video buffers.
 *
 * This method should be called before startRecording() in order to be
 * effective.
 *
 * If meta data is stored in the video buffers, it is up to the receiver of
 * the video buffers to interpret the contents and to find the actual frame
 * data with the help of the meta data in the buffer. How this is done is
 * outside of the scope of this method.
 *
 * Some camera HALs may not support storing meta data in the video buffers,
 * but all camera HALs should support storing real YUV data in the video
 * buffers. If the camera HAL does not support storing the meta data in the
 * video buffers when it is requested to do do, INVALID_OPERATION must be
 * returned. It is very useful for the camera HAL to pass meta data rather
 * than the actual frame data directly to the video encoder, since the
 * amount of the uncompressed frame data can be very large if video size is
 * large.
 *
 * @param enable if true to instruct the camera HAL to store
 *      meta data in the video buffers; false to instruct
 *      the camera HAL to store real YUV data in the video
 *      buffers.
 *
 * @return OK on success.
 */
static int HAL_camera_device_store_meta_data_in_buffers(struct camera_device *dev, int enable)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->storeMetaDataInBuffers(enable);
}

/*
 * Start record mode. When a record image is available, a
 * CAMERA_MSG_VIDEO_FRAME message is sent with the corresponding
 * frame. Every record frame must be released by a camera HAL client via
 * releaseRecordingFrame() before the client calls
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME). After the client calls
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
 * responsibility to manage the life-cycle of the video recording frames,
 * and the client must not modify/access any video recording frames.
 */
static int HAL_camera_device_start_recording(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->startRecording();
}

/*
 * Stop a previously started recording.
 */
static void HAL_camera_device_stop_recording(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    obj(dev)->stopRecording();
}

/*
 * Returns true if recording is enabled.
 */
static int HAL_camera_device_recording_enabled(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->recordingEnabled();
}

/*
 * Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
 *
 * It is camera HAL client's responsibility to release video recording
 * frames sent out by the camera HAL before the camera HAL receives a call
 * to disableMsgType(CAMERA_MSG_VIDEO_FRAME). After it receives the call to
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
 * responsibility to manage the life-cycle of the video recording frames.
 */
static void HAL_camera_device_release_recording_frame(struct camera_device *dev,
                                const void *opaque)
{
    HALDBG("%s", __FUNCTION__);

    obj(dev)->releaseRecordingFrame(opaque);
}

/*
 * Start auto focus, the notification callback routine is called with
 * CAMERA_MSG_FOCUS once when focusing is complete. autoFocus() will be
 * called again if another auto focus is needed.
 */
static int HAL_camera_device_auto_focus(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->autoFocus();
}

/*
 * Cancels auto-focus function. If the auto-focus is still in progress,
 * this function will cancel it. Whether the auto-focus is in progress or
 * not, this function will return the focus position to the default.  If
 * the camera does not support auto-focus, this is a no-op.
 */
static int HAL_camera_device_cancel_auto_focus(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->cancelAutoFocus();
}

/*
 * Take a picture.
 */
static int HAL_camera_device_take_picture(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->takePicture();
}

/*
 * Cancel a picture that was started with takePicture. Calling this method
 * when no picture is being taken is a no-op.
 */
static int HAL_camera_device_cancel_picture(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);
    return obj(dev)->cancelPicture();
}

/*
 * Set the camera parameters. This returns BAD_VALUE if any parameter is
 * invalid or not supported.
 */
static int HAL_camera_device_set_parameters(struct camera_device *dev,
                                            const char *parms)
{
    HALDBG("%s", __FUNCTION__);
    String8 str(parms);
    CameraParameters p(str);
    return obj(dev)->setParameters(p);
}

/* Return the camera parameters. */
char *HAL_camera_device_get_parameters(struct camera_device *dev)
{

    HALDBG("%s", __FUNCTION__);

    String8 str;
    CameraParameters parms = obj(dev)->getParameters();
    str = parms.flatten();
    return strdup(str.c_str());
}

static void HAL_camera_device_put_parameters(struct camera_device *dev, char *parms)
{
    HALDBG("%s", __FUNCTION__);
    free(parms);
}

/*
 * Send command to camera driver.
 */
static int HAL_camera_device_send_command(struct camera_device *dev,
                    int32_t cmd, int32_t arg1, int32_t arg2)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->sendCommand(cmd, arg1, arg2);
}

/*
 * Release the hardware resources owned by this object.  Note that this is
 * *not* done in the destructor.
 */
static void HAL_camera_device_release(struct camera_device *dev)
{
    HALDBG("%s", __FUNCTION__);

    obj(dev)->release();
}

/*
 * Dump state of the camera hardware
 */
static int HAL_camera_device_dump(struct camera_device *dev, int fd)
{
    HALDBG("%s", __FUNCTION__);

    return obj(dev)->dump(fd);
}


static int HAL_getNumberOfCameras(void)
{
    int num = sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
    ALOGV("%s: camera num %d", __FUNCTION__, num);
    return num;
}

static int HAL_getCameraInfo(int cameraId, struct camera_info *cameraInfo)
{
    if (CC_UNLIKELY(cameraId < 0)) {
        ALOGE("%s: camera id %d", __FUNCTION__, cameraId);
        return BAD_VALUE;
    }

    memcpy(cameraInfo, &sCameraInfo[cameraId], sizeof(CameraInfo));
    return 0;
}

#define SET_METHOD(m) m : HAL_camera_device_##m

static camera_device_ops_t camera_device_ops = {
        SET_METHOD(set_preview_window),
        SET_METHOD(set_callbacks),
        SET_METHOD(enable_msg_type),
        SET_METHOD(disable_msg_type),
        SET_METHOD(msg_type_enabled),
        SET_METHOD(start_preview),
        SET_METHOD(stop_preview),
        SET_METHOD(preview_enabled),
        SET_METHOD(store_meta_data_in_buffers),
        SET_METHOD(start_recording),
        SET_METHOD(stop_recording),
        SET_METHOD(recording_enabled),
        SET_METHOD(release_recording_frame),
        SET_METHOD(auto_focus),
        SET_METHOD(cancel_auto_focus),
        SET_METHOD(take_picture),
        SET_METHOD(cancel_picture),
        SET_METHOD(set_parameters),
        SET_METHOD(get_parameters),
        SET_METHOD(put_parameters),
        SET_METHOD(send_command),
        SET_METHOD(release),
        SET_METHOD(dump),
};

#undef SET_METHOD

static int HAL_ext_camera_device_open(const struct hw_module_t* module,
                                  const char *id,
                                  struct hw_device_t **device)
{
    ALOGD("camera_device_open: E");

    int cameraId = atoi(id);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        if (cameraId != 99) {
            ALOGE("camera_device_open: invalid camera ID %s", id);
            return -EINVAL;
        } else {
            cameraId = 2;
        }
    }

    if (g_cam_device) {
        if (obj(g_cam_device)->getCameraId() == cameraId) {
            ALOGD("returning existing camera ID %s", id);
            goto done;
        } else {
            ALOGE("camera_device_open: error, cannot open camera %d. camera %d is already running!",
                    cameraId, obj(g_cam_device)->getCameraId());
            return -ENOSYS;
        }
    }

    g_cam_device = (camera_device_t *)malloc(sizeof(camera_device_t));
    if (!g_cam_device) {
        ALOGE("camera_device_open: error, fail to get memory");
        return -ENOMEM;
    }

    memset(g_cam_device, 0, sizeof(camera_device_t));
    g_cam_device->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam_device->common.version = 1;
    g_cam_device->common.module  = const_cast<hw_module_t *>(module);
    g_cam_device->common.close   = HAL_camera_device_close;

    g_cam_device->ops = &camera_device_ops;

    g_cam_device->priv = new SecCameraHardware(cameraId, g_cam_device);
    if (!obj(g_cam_device)->mInitialized) {
        ALOGE("Instance is not created");
        if (g_cam_device->priv) {
            delete static_cast<SecCameraHardware *>(g_cam_device->priv);
            free(g_cam_device);
            g_cam_device = 0;
        }
        return -ENOSYS;
    }

done:
    *device = (hw_device_t *)g_cam_device;
    ALOGI("opened camera %s (%p)", id, *device);
    return 0;
}

int HAL_ext_camera_device_open_wrapper(const struct hw_module_t* module,
                                  const char *id,
                                  struct hw_device_t **device)
{
    return HAL_ext_camera_device_open(module, id, device);
}

static hw_module_methods_t ext_camera_module_methods = {
            open : HAL_ext_camera_device_open
};

/* merge to libcamera_common */
#if 0
extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
      common : {
          tag           : HARDWARE_MODULE_TAG,
          version_major : 1,
          version_minor : 0,
          id            : CAMERA_HARDWARE_MODULE_ID,
          name          : "Px camera HAL",
          author        : "Samsung Corporation",
          methods       : &ext_camera_module_methods,
          dso           : NULL,
          reserved      : {0}
      },
      get_number_of_cameras : HAL_getNumberOfCameras,
      get_camera_info       : HAL_getCameraInfo,
      set_callbacks         : NULL,
#if (TARGET_ANDROID_VER_MAJ >= 4 && TARGET_ANDROID_VER_MIN >= 4)
      get_vendor_tag_ops    : NULL
#endif
    };
}
#endif

}; /* namespace android */

#endif /* ANDROID_HARDWARE_SECCAMERAINTERFACE_CPP */
