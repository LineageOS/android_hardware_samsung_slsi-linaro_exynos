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
 * \file      ISecCameraHardware.cpp
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#ifndef ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP
#define ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP

#define LOG_NDEBUG 0
#define LOG_NFPS 1
#define LOG_NPERFORMANCE 1

#define LOG_TAG "ISecCameraHardware"

#include <ISecCameraHardware.h>

#if VENDOR_FEATURE
#define SMART_THREAD
#endif
#define SMART_READ_INFO
#ifdef SMART_READ_INFO
#define AUTO_PARAM_CALLBACK
#endif

#define PRE_AF_FAIL 5
#define PRE_AF_SUCCESS	6

#define REC_CAPTURE_WIDTH	2304
#define REC_CAPTURE_HEIGHT	1728

namespace android {

ISecCameraHardware::ISecCameraHardware(int cameraId, camera_device_t *dev)
    : mCameraId(cameraId),
    mParameters(),
    mFlagANWindowRegister(false),
    mPreviewHeap(NULL),
    mPostviewHeap(NULL),
    mPostviewHeapTmp(NULL),
    mRawHeap(NULL),
    mRecordingHeap(NULL),
    mJpegHeap(NULL),
    mHDRHeap(NULL),
    mYUVHeap(NULL),
    mPreviewFormat(CAM_PIXEL_FORMAT_YUV420SP),
    mPictureFormat(CAM_PIXEL_FORMAT_JPEG),
    mFliteFormat(CAM_PIXEL_FORMAT_YUV420SP),
    mMirror(false),
    mNeedSizeChange(false),
    mFastCaptureCalled(false),
    mRecordingTrace(false),
    mMsgEnabled(0),
    mGetMemoryCb(0),
    mPreviewWindow(NULL),
    mNotifyCb(0),
    mDataCb(0),
    mDataCbTimestamp(0),
    mCallbackCookie(0),
    mDisablePostview(false),
    mSamsungApp(false),
    mAntibanding60Hz(-1),

    mHalDevice(dev)
{
    if (mCameraId == CAMERA_FACING_BACK) {
        mZoomSupport = IsZoomSupported();
        mEnableDZoom = mZoomSupport ? IsAPZoomSupported() : false;
        mFastCaptureSupport = IsFastCaptureSupportedOnRear();

        mFLiteSize.width = backFLiteSizes[0].width;
        mFLiteSize.height = backFLiteSizes[0].height;
        mFLiteCaptureSize = mFLiteSize;

        mPreviewSize.width = backPreviewSizes[0].width;
        mPreviewSize.height = backPreviewSizes[0].height;
        mOrgPreviewSize = mPreviewSize;

        mPictureSize.width = backPictureSizes[0].width;
        mPictureSize.height = backPictureSizes[0].height;

        mThumbnailSize.width = backThumbSizes[0].width;
        mThumbnailSize.height = backThumbSizes[0].height;

        mVideoSize.width = backRecordingSizes[0].width;
        mVideoSize.height = backRecordingSizes[0].height;
    } else {
        mZoomSupport = IsZoomSupported();
        mEnableDZoom = mZoomSupport ? IsAPZoomSupported() : false;
        mFastCaptureSupport = IsFastCaptureSupportedOnFront();

        mFLiteSize.width = frontFLiteSizes[0].width;
        mFLiteSize.height = frontFLiteSizes[0].height;
        mFLiteCaptureSize = mFLiteSize;

        mPreviewSize.width = frontPreviewSizes[0].width;
        mPreviewSize.height = frontPreviewSizes[0].height;
        mOrgPreviewSize = mPreviewSize;

        mPictureSize.width = frontPictureSizes[0].width;
        mPictureSize.height = frontPictureSizes[0].height;

        mThumbnailSize.width = frontThumbSizes[0].width;
        mThumbnailSize.height = frontThumbSizes[0].height;

        mVideoSize.width = frontRecordingSizes[0].width;
        mVideoSize.height = frontRecordingSizes[0].height;
    }

#ifndef FCJUNG
    mFactoryPuntRangeData.min = -1;
    mFactoryPuntRangeData.max = -1;
    mFactoryPuntRangeData.num = -1;
    mFactoryZoomRangeCheckData.min = -1;
    mFactoryZoomRangeCheckData.max = -1;
    mFactoryZoomSlopeCheckData.min = -1;
    mFactoryZoomSlopeCheckData.max = -1;
    mFactoryOISRangeData.x_min = -1;
    mFactoryOISRangeData.x_max = -1;
    mFactoryOISRangeData.y_min = -1;
    mFactoryOISRangeData.y_max = -1;
    mFactoryOISRangeData.x_gain = -1;
    mFactoryOISRangeData.peak_x = -1;
    mFactoryOISRangeData.peak_y = -1;
    mFactoryVibRangeData.x_min = -1;
    mFactoryVibRangeData.x_max = -1;
    mFactoryVibRangeData.y_min = -1;
    mFactoryVibRangeData.y_max = -1;
    mFactoryVibRangeData.peak_x = -1;
    mFactoryVibRangeData.peak_y = -1;
    mFactoryGyroRangeData.x_min = -1;
    mFactoryGyroRangeData.x_max = -1;
    mFactoryGyroRangeData.y_min = -1;
    mFactoryGyroRangeData.y_max = -1;
    mFactoryAFScanLimit.min = -1;
    mFactoryAFScanLimit.max = -1;
    mFactoryAFScanRange.min = -1;
    mFactoryAFScanRange.max = -1;
    mFactoryIRISRange.min = -1;
    mFactoryIRISRange.max = -1;
    mFactoryGainLiveViewRange.min = -1;
    mFactoryGainLiveViewRange.max = -1;
    mFactoryShCloseSpeedTime.x = -1;
    mFactoryShCloseSpeedTime.y = -1;
    mFactoryCaptureGainRange.min = -1;
    mFactoryCaptureGainRange.max = -1;
    mFactoryAFDiffCheck.min = -1;
    mFactoryAFDiffCheck.max = -1;
    mFactoryFlashRange.x = -1;
    mFactoryFlashRange.y = -1;
    mFactoryWBRangeData.in_rg = -1;
    mFactoryWBRangeData.in_bg = -1;
    mFactoryWBRangeData.out_rg = -1;
    mFactoryWBRangeData.out_bg = -1;
    mFactoryAFLEDRangeData.start_x = -1;
    mFactoryAFLEDRangeData.end_x = -1;
    mFactoryAFLEDRangeData.start_y = -1;
    mFactoryAFLEDRangeData.end_y = -1;
    mFactoryAFLEDlvlimit.min = -1;
    mFactoryAFLEDlvlimit.max = -1;
    mFactoryTiltScanLimitData.min = -1;
    mFactoryTiltScanLimitData.max = -1;
    mFactoryTiltAFRangeData.min = -1;
    mFactoryTiltAFRangeData.max = -1;
    mFactoryTiltDiffRangeData.min = -1;
    mFactoryTiltDiffRangeData.max = -1;
    mFactoryIRCheckRGainData.min = -1;
    mFactoryIRCheckRGainData.max = -1;
    mFactoryIRCheckBGainData.min = -1;
    mFactoryIRCheckBGainData.max = -1;
    mFactorySysMode = 0;
    mFactoryNumCnt = 0;
#endif /* FCJUNG */

    mRawSize = mPreviewSize;
	mPostviewSize = mPreviewSize;
	mCaptureMode = RUNNING_MODE_SINGLE;

#if FRONT_ZSL
    rawImageMem = NULL;
    mFullPreviewHeap = NULL;
#endif

    mPreviewWindowSize.width = mPreviewWindowSize.height = 0;

    mFrameRate = 0;
    mFps = 0;
    mJpegQuality= 100;
    mSceneMode = (cam_scene_mode)sceneModes[0].val;
    mFlashMode = (cam_flash_mode)flashModes[0].val;
    if (mCameraId == CAMERA_FACING_BACK)
        mFocusMode = (cam_focus_mode)backFocusModes[0].val;
    else
        mFocusMode = (cam_focus_mode)frontFocusModes[0].val;

    mFirmwareMode = CAM_FW_MODE_NONE;
    mVtMode = CAM_VT_MODE_NONE;
    mMovieMode = false;
#ifdef DEBUG_PREVIEW_NO_FRAME /* 130221.DSLIM Delete me in a week*/
    mFactoryMode = false;
#endif
    mDtpMode = false;

    mMaxFrameRate = 30000;
    mDropFrameCount = 3;
    mbFirst_preview_started = false;

#if IS_FW_DEBUG
    if (mCameraId == CAMERA_FACING_FRONT || mCameraId == FD_SERVICE_CAMERA_ID)
        mPrintDebugEnabled = false;
#endif
	mIonCameraClient = -1;
	mPictureFrameSize = 0;
	mFullPreviewFrameSize = 0;
	CLEAR(mAntiBanding);
	mAutoFocusExit = false;
	mPreviewRunning = false;
	mPreviewThreadType = PREVIEW_THREAD_NONE;
	mRecordingRunning = false;
	mAutoFocusRunning = false;
	mPictureRunning = false;
	mRecordSrcIndex = -1;
	CLEAR(mRecordSrcSlot);
	mRecordDstIndex = -1;
	CLEAR(mRecordFrameAvailable);
	mRecordFrameAvailableCnt = 0;
	mFlagFirstFrameDump = false;
	mPostRecordIndex = -1;
	mRecordTimestamp = 0;
	mLastRecordTimestamp = 0;
	mPostRecordExit = false;
	mPreviewInitialized = false;
	mPreviewHeapFd = -1;
	mRecordHeapFd = -1;
	mPostviewHeapFd = -1;
	mPostviewHeapTmpFd = -1;
	CLEAR(mFrameMetadata);
	CLEAR(mFaces);
	mZSLindex = -1;
	mFullPreviewRunning = false;
	mPreviewFrameSize = 0;
    mPreviewFrameOffset = 0;
	mRecordingFrameSize = 0;
	mRawFrameSize = 0;
	mPostviewFrameSize = 0;
	mFirstStart = 0;
	mTimerSet = 0;
	mZoomParamSet = 1;
	mZoomCurrLevel = 0;
	mZoomSetVal = 0;
	mZoomStatus = 0;
	mLensStatus = 0;
	mZoomStatusBak = 0;
	mLensChecked = 0;
    mLensStatus = 0;

	mCameraPower = true;
#ifdef APPLY_ESD
	isResetedByESD = false;
#endif

	roi_x_pos = 0;
	roi_y_pos = 0;
	roi_width = 0;
	roi_height = 0;

	mBurstShotExit = false;

#ifdef ZOOM_CTRL_THREAD
       zoom_cmd = 0;
       old_zoom_cmd = 100;
#endif

}

ISecCameraHardware::~ISecCameraHardware()
{
    if (mPreviewHeap) {
        mPreviewHeap->release(mPreviewHeap);
        mPreviewHeap = 0;
        mPreviewHeapFd = -1;
    }

    if (mRecordingHeap) {
        mRecordingHeap->release(mRecordingHeap);
        mRecordingHeap = 0;
    }

    if (mRawHeap != NULL) {
        mRawHeap->release(mRawHeap);
        mRawHeap = 0;
    }

    if (mHDRHeap) {
        mHDRHeap->release(mHDRHeap);
        mHDRHeap = NULL;
    }

#ifndef RCJUNG
    if (mYUVHeap) {
        mYUVHeap->release(mYUVHeap);
        mYUVHeap = NULL;
    }
#endif

    if (mJpegHeap) {
        mJpegHeap->release(mJpegHeap);
        mJpegHeap = 0;
    }

    if (mPostviewHeap) {
        mPostviewHeap->release(mPostviewHeap);
        mPostviewHeap = 0;
    }

    if (mPostviewHeapTmp) {
        mPostviewHeapTmp->release(mPostviewHeapTmp);
        mPostviewHeapTmp = 0;
    }

    if (0 < mIonCameraClient)
        ion_client_destroy(mIonCameraClient);
    mIonCameraClient = -1;
}

#ifdef BURST_SHOT_SUPPORT
ISecCameraHardware::BurstShot::BurstShot()
{
    mLimitByte = 300*1024*1024;

    memset(mItem, 0, sizeof(mItem));
    init();
}

ISecCameraHardware::BurstShot::~BurstShot()
{
    release();
}

bool ISecCameraHardware::BurstShot::init()
{
    Mutex::Autolock lock(mBurstLock);

    ALOGD("BURSTSHOT BurstShot::init() ");

    mUsedByte = 0;
    mTotalIndex = 0;

    mCountMax = 0;
    mHead = 1;
    mTail = 0;
    mAttrib = 0;

    memset(mItem, 0, sizeof(mItem));
#ifdef USE_CONTEXTUAL_FILE_NAME
	freeContextualFileName();
	setContextualState(false);
#endif

	strcpy(mPath, "/data/media/0/DCIM/Camera");

    mInitialize = true;
    return true;
}

void ISecCameraHardware::BurstShot::release()
{
    Mutex::Autolock lock(mBurstLock);

	ALOGD("BURSTSHOT MEM: burst release.");
	for (int i=0;i<MAX_BURST_COUNT;i++) {
		if ( mItem[i].type == CAMERA_BURST_MEMORY_ION ) {  // ION memory
			burst_item *b_item = &mItem[i];
			if (b_item->size > 0) {
				int ret = 0;

				ret = ion_unmap(b_item->virt, b_item->size);
				if (ret < 0)
					ALOGE("ERR(%s):ion_unmap(%p, %d) fail", __FUNCTION__, b_item->virt, b_item->size);
				ion_free(b_item->fd);

				b_item->size = 0;
			}
			if (b_item->ion > 0) {
				ion_client_destroy(b_item->ion);
				b_item->ion = -1;
			}
		}
	}
}

bool ISecCameraHardware::BurstShot::isEmpty()
{
    return ( (mTail+1)%MAX_BURST_COUNT == mHead );
}

bool ISecCameraHardware::BurstShot::isLimit(int size)
{
    Mutex::Autolock lock(mBurstLock);

    if ( mLimitByte < mUsedByte+size )
        return false;

    int head = (mHead+1)%MAX_BURST_COUNT ;
    if ( head == mTail )
        return false;

    return true;
}

bool ISecCameraHardware::BurstShot::isStartLimit(void)
{
    Mutex::Autolock lock(mBurstLock);

    if ( (mLimitByte - mUsedByte) < (10 * 1024 * 1024) )
        return false;

    return true;
}

uint8_t* ISecCameraHardware::BurstShot::getAddress(burst_item* item)
{
    if ( item == NULL ) {
		ALOGE("BURSTSHOT: item address is NULL");
        return NULL;
	}

    if ( item->type == CAMERA_BURST_MEMORY_ION ) {
		ALOGD("BURSTSHOT: nativeSaveJpegPicture: buf (%p)", item->virt);
        return (uint8_t*)(item->virt);
	}

	return  NULL;
}

bool ISecCameraHardware::BurstShot::allocMemBurst(ion_client ionClient, ExynosBuffer *buf, int index, bool flagCache)
{
    if (ionClient == 0) {
        ALOGE("ERR(%s): ionClient is zero (%d)", __func__, ionClient);
        return false;
    }

    if (buf->size.extS[index] != 0) {
        /* HACK: For non-cacheable */
        buf->fd.extFd[index] = ion_alloc(ionClient, buf->size.extS[index], 0, ION_HEAP_SYSTEM_MASK, 0);
        if (buf->fd.extFd[index] <= 0) {
            ALOGE("ERR(%s): ion_alloc(%d, %d) failed", __func__, index, buf->size.extS[index]);
            buf->fd.extFd[index] = -1;
            freeMemBurst(buf, index);
            return false;
        }

        buf->virt.extP[index] = (char *)ion_map(buf->fd.extFd[index], buf->size.extS[index], 0);
        if ((buf->virt.extP[index] == (char *)MAP_FAILED) || (buf->virt.extP[index] == NULL)) {
            ALOGE("ERR(%s): ion_map(%d) failed", __func__, buf->size.extS[index]);
            buf->virt.extP[index] = NULL;
            freeMemBurst(buf, index);
            return false;
        }
    }

    return true;
}

void ISecCameraHardware::BurstShot::freeMemBurst(ExynosBuffer *buf, int index)
{
	ALOGD("BURSTSHOT MEM: freeMemBurst free item. buf->fd.extFd[%d] = %d", index, buf->fd.extFd[index]);
	if (0 < buf->fd.extFd[index]) {
		ALOGD("BURSTSHOT MEM: freeMemBurst free item2.");
		if (buf->virt.extP[index] != NULL) {
			ALOGD("BURSTSHOT MEM: freeMemBurst free item3.");
			int ret = 0;

            ret = ion_unmap(buf->virt.extP[index], buf->size.extS[index]);
            if (ret < 0)
                ALOGE("ERR(%s):ion_unmap(%p, %d) fail", __FUNCTION__, buf->virt.extP[index], buf->size.extS[index]);
			else
				ALOGD("BURSTSHOT MEM: memory unmmaped normaly");
        } else {
			ALOGE("BURSTSHOT MEM: freeMemBurst: buf->virt.extP[%p] is NULL", buf->virt.extP[index]);
		}
        ion_free(buf->fd.extFd[index]);
    } else {
		ALOGE("BURSTSHOT MEM: freeMemBurst: buf->fd.extFd[%d] = %d", index, buf->fd.extFd[index]);
	}

    buf->fd.extFd[index] = -1;
    buf->virt.extP[index] = NULL;
    buf->size.extS[index] = 0;
}

uint8_t* ISecCameraHardware::BurstShot::malloc(int size, bool cached)
{
	uint8_t *ptr;
	int mIonBurst = ion_client_create();
	ExynosBuffer nullBuf;
	ExynosBuffer burst_mem;

	burst_mem = nullBuf;
	burst_mem.size.extS[0] = size;
	if (allocMemBurst(mIonBurst, &burst_mem, 0, 1 << 1) == false) {
		ALOGE("BURSTSHOT: ERR(%s): malloc: mIonBurst allocMemBurst fail", __func__);
		ion_client_destroy(mIonBurst);
		return NULL;
	} else {
		ALOGV("BURSTSHOT MEM: DEBUG(%s): malloc: mIonBurst allocMem adr(%p), size(%d), ion(%d)", __FUNCTION__,
				burst_mem.virt.extP[0], burst_mem.size.extS[0], mIonBurst);
		memset(burst_mem.virt.extP[0], 0, burst_mem.size.extS[0]);
	}

	memset(&mJpegION, 0, sizeof(mJpegION));
	mJpegION.size = burst_mem.size.extS[0];
	mJpegION.type = CAMERA_BURST_MEMORY_ION;
	mJpegION.ion = mIonBurst;
	mJpegION.fd = burst_mem.fd.extFd[0];
	mJpegION.virt = (uint8_t*)burst_mem.virt.extP[0];

	ALOGD("BURSTSHOT MEM: malloc: mJpegION.size = %d", mJpegION.size);
	ALOGD("BURSTSHOT MEM: malloc: mJpegION->virt = %p", mJpegION.virt);

	ptr = (uint8_t *)mJpegION.virt;

	{
		Mutex::Autolock lock(mBurstLock);
		mUsedByte += mJpegION.size;
		ALOGD("BURSTSHOT: alloc mUsedByte = %d", mUsedByte);
	}
	return ptr;
}

bool ISecCameraHardware::BurstShot::free(burst_item *item)
{
	if ( item->type == CAMERA_BURST_MEMORY_ION ) {
		ALOGD("BURSTSHOT: free burstshot item. index = %d, size = %d", item->ix, item->size);
		if (item->size > 0) {
//			ALOGD("BURSTSHOT: free burstshot item.");
			int ret = 0;

            ret = ion_unmap(item->virt, item->size);
            if (ret < 0)
                ALOGE("ERR(%s):ion_unmap(%p, %d) fail", __FUNCTION__, item->virt, item->size);
			else {
//				ALOGD("BURSTSHOT MEM: memory unmmaped normaly");
			}
			ion_free(item->fd);
		} else {
			ALOGE("BURSTSHOT MEM: free - wrong size: item->size = %d", item->size);
		}

        if (item->ion > 0) {
            ion_client_destroy(item->ion);
            item->ion = -1;
        }
    }

    Mutex::Autolock lock(mBurstLock);
    mUsedByte -= item->size;
	ALOGD("BURSTSHOT: free mUsedByte = %d", mUsedByte);
	item->size = 0;

    return true;
}

bool ISecCameraHardware::BurstShot::push(int cnt)
{
    Mutex::Autolock lock(mBurstLock);

    if ( mLimitByte < mUsedByte  )
        return false;

    int head = (mHead+1)%MAX_BURST_COUNT ;
    if ( head == mTail )
        return false;

    if (cnt == 0) {
        srand((unsigned int)time(NULL));
        mRandNum = rand();
    }

    mJpegION.frame_num = cnt + 1;
    mJpegION.group = mRandNum;

    mItem[mHead] = mJpegION;
    mItem[mHead].ix = (++mTotalIndex)%1000;
	mItem[mHead].ion = mJpegION.ion;
	mItem[mHead].fd = mJpegION.fd;
	mItem[mHead].virt = mJpegION.virt;

	ALOGD("BURSTSHOT MEM: mItem[%d] = %p", mHead, &(mItem[mHead]));

    mHead = head;

    return true;
}

bool ISecCameraHardware::BurstShot::pop(burst_item *item)
{
    Mutex::Autolock lock(mBurstLock);

    int tail = (mTail+1)%MAX_BURST_COUNT;
    if ( tail == mHead )
        return false;

    mTail = tail;
    *item = mItem[mTail];

	ALOGE("BURSTSHOT MEM: pop: mTail = %d, mItem[mTail] = %p", mTail, &(mItem[tail]));

	memset(&mItem[mTail], 0, sizeof(burst_item));
    return true;
}

void ISecCameraHardware::BurstShot::DumpState(void)
{
    ALOGD("BURSTSHOT DumpState ------------------------------------------");
    ALOGD("BURSTSHOT	mHead=%d, mTail=%d ", mHead,  mTail);
    ALOGD("BURSTSHOT	mUsedByte=%d MaxByte:%d", mUsedByte, mLimitByte );
    ALOGD("BURSTSHOT	mAttrib=0x%08x", mAttrib);
    ALOGD("BURSTSHOT DumpState  ------------------------------------------");
}

void ISecCameraHardware::BurstShot::getFileName(char* buffer, int i, int group_num)
{
    ALOGV("getFileName %d, %d", i, group_num);

    time_t rawtime;
    struct tm *timeinfo;
    char date_time[20];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime((char *)date_time, 20, "%Y%m%d_%H%M%S", timeinfo);

#ifdef USE_CONTEXTUAL_FILE_NAME
	ALOGD("mContextualstate %d, %s", mContextualstate, getContextualFileName());
	if ( getMode() & BURST_ATTRIB_BEST_MODE ) {
		if ( getContextualState() )
			sprintf(buffer, "%s/%s_%s_%d_bestshot.jpg%c", getPath(), date_time, getContextualFileName(), i, NULL);
		else
			sprintf(buffer, "%s/%s_%d_bestshot.jpg%c", getPath(), date_time, i, NULL);
	} else {
		if ( getContextualState() )
			sprintf(buffer, "%s/%s_%s_%d.jpg", getPath(), date_time, getContextualFileName(), i);
		else
			sprintf(buffer, "%s/%s_%d.jpg", getPath(), date_time, i);
	}
	ALOGD("getFileName~~~~ %d, %s/%s", i, buffer,  getContextualFileName());
#else
	if ( getMode() & BURST_ATTRIB_BEST_MODE )
		sprintf(buffer, "%s/%s_%d_bestshot.jpg%c", getPath(), date_time, i, NULL);
	else
		sprintf(buffer, "%s/%s_%d.jpg", getPath(), date_time, i);

	ALOGD("getFileName~~~~ %d, %s", i, buffer);
#endif
}

#ifdef USE_CONTEXTUAL_FILE_NAME
char* ISecCameraHardware::BurstShot::getContextualFileName(void)
{
    ALOGD("BurstShot::getContextualFileName");
    return mContextualFilename;
}

void ISecCameraHardware::BurstShot::setContextualFileName(char* strName)
{
    setContextualState(true);
    freeContextualFileName();
    memcpy(mContextualFilename, strName, strlen(strName));
    ALOGD("BurstShot::setContextualFileName: %s, %d", mContextualFilename, strlen(mContextualFilename));
}
#endif
#endif

bool ISecCameraHardware::init()
{
    mPreviewRunning = false;
    mPreviewThreadType = PREVIEW_THREAD_NONE;
    mFullPreviewRunning = false; /* for FRONT_ZSL */
    mPreviewInitialized = false;
#ifdef DEBUG_PREVIEW_CALLBACK
    mPreviewCbStarted = false;
#endif
    mRecordingRunning = false;
    mPictureRunning = false;
	mPictureStart = false;
	mCaptureStarted = false;
	mCancelCapture = false;
    mAutoFocusRunning = false;
    mSelfShotFDReading = false;
    mAutoFocusExit = false;
	mDualCapture = false;
#ifdef BURST_SHOT_SUPPORT
	mBurstWriteRunning = false;
	mEnableStrCb = true;
#endif
    mFaceDetectionStatus = V4L2_FACE_DETECTION_OFF;

    /* for NSM Mode */
    mbGetFDinfo = false;
    /* end NSM Mode */

    if (mEnableDZoom) {
        /* Thread for zoom */
        mPreviewZoomThread = new CameraThread(this, &ISecCameraHardware::previewZoomThread, "previewZoomThread");
        mPostRecordThread = new CameraThread(this, &ISecCameraHardware::postRecordThread, "postRecordThread");
        mPreviewThread = mRecordingThread = NULL;
#ifdef CHANGED_PREVIEW_SUPPORT
        mChangedPreviewThread = new CameraThread(this, &ISecCameraHardware::changedPreviewThread, "changedPreviewThread");
#endif
    } else {
        mPreviewThread = new CameraThread(this, &ISecCameraHardware::previewThread, "previewThread");
        mRecordingThread = new CameraThread(this, &ISecCameraHardware::recordingThread, "recordingThread");
        mPreviewZoomThread = mPostRecordThread = NULL;
    }

    mPictureThread = new CameraThread(this, &ISecCameraHardware::pictureThread, "pictureThread");
#if FRONT_ZSL
    mZSLPictureThread = new CameraThread(this, &ISecCameraHardware::zslpictureThread, "zslpictureThread");
#endif

#ifdef BURST_SHOT_SUPPORT
    mBurstPictureThread = new CameraThread(this, &ISecCameraHardware::burstPictureThread);
    mBurstWriteThread = new CameraThread(this, &ISecCameraHardware::burstWriteThread);
#endif
	if (mCameraId == CAMERA_FACING_BACK) {
        mAutoFocusThread = new CameraThread(this, &ISecCameraHardware::autoFocusThread, "autoFocusThread");
        mAutoFocusThread->run("autoFocusThread", PRIORITY_DEFAULT);
#ifdef SMART_THREAD
        mSmartThread = new CameraThread(this, &ISecCameraHardware::smartThread);
        mSmartThread->run("smartThread", PRIORITY_DEFAULT);
#endif
#ifdef ZOOM_CTRL_THREAD
        mZoomCtrlThread = new CameraThread(this, &ISecCameraHardware::ZoomCtrlThread);
        mZoomCtrlThread->run("ZoomCtrlThread", PRIORITY_DEFAULT);
#endif

        mHDRPictureThread = new CameraThread(this, &ISecCameraHardware::HDRPictureThread);
        mAEBPictureThread = new CameraThread(this, &ISecCameraHardware::AEBPictureThread);
        mBlinkPictureThread = new CameraThread(this, &ISecCameraHardware::BlinkPictureThread);
        mRecordingPictureThread = new CameraThread(this, &ISecCameraHardware::RecordingPictureThread);
		mDumpPictureThread = new CameraThread(this, &ISecCameraHardware::dumpPictureThread);

#if VENDOR_FEATURE
		mShutterThread = new CameraThread(this, &ISecCameraHardware::shutterThread);
		mShutterThread->run("shutterThread", PRIORITY_URGENT_DISPLAY);
#endif
    }

#if IS_FW_DEBUG
    if (!mPrintDebugEnabled &&
        (mCameraId == CAMERA_FACING_FRONT || mCameraId == FD_SERVICE_CAMERA_ID)) {
        mPrevOffset = 0;
        mCurrOffset = 0;
        mPrevWp = 0;
        mCurrWp = 0;
        mDebugVaddr = 0;

        int ret = nativeGetDebugAddr(&mDebugVaddr);
        if (ret < 0) {
            ALOGE("ERR(%s):Fail on SecCamera->getDebugAddr()", __func__);
            mPrintDebugEnabled = false;
        } else {
            ALOGD("mDebugVaddr = 0x%x", mDebugVaddr);
            mPrintDebugEnabled = true;
#if IS_FW_DEBUG_THREAD
            mStopDebugging = false;
            mDebugThread = new DebugThread(this);
            mDebugThread->run("debugThread", PRIORITY_DEFAULT);
#endif
        }
    }
#endif

    mIonCameraClient = ion_client_create();
    if (mIonCameraClient < 0) {
        ALOGE("ERR(%s):ion_client_create() fail", __func__);
        mIonCameraClient = -1;
    }

    for (int i = 0; i < REC_BUF_CNT; i++) {
        for (int j = 0; j < REC_PLANE_CNT; j++) {
            mRecordDstHeap[i][j] = NULL;
            mRecordDstHeapFd[i][j] = -1;
        }
    }

    mInitRecSrcQ();
    mInitRecDstBuf();

    return true;
}

void ISecCameraHardware::initDefaultParameters()
{
    /* Preview */
    mParameters.setPreviewSize(mPreviewSize.width, mPreviewSize.height);

    if (mCameraId == CAMERA_FACING_BACK) {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                SecCameraParameters::createSizesStr(backPreviewSizes, ARRAY_SIZE(backPreviewSizes)).c_str());

        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, B_KEY_PREVIEW_FPS_RANGE_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, B_KEY_SUPPORTED_PREVIEW_FPS_RANGE_VALUE);

        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, B_KEY_SUPPORTED_PREVIEW_FRAME_RATES_VALUE);
        mParameters.setPreviewFrameRate(B_KEY_PREVIEW_FRAME_RATE_VALUE);

        mParameters.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, B_KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                SecCameraParameters::createSizesStr(frontPreviewSizes, ARRAY_SIZE(frontPreviewSizes)).c_str());

        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, F_KEY_PREVIEW_FPS_RANGE_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, F_KEY_SUPPORTED_PREVIEW_FPS_RANGE_VALUE);

        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, F_KEY_SUPPORTED_PREVIEW_FRAME_RATES_VALUE);
        mParameters.setPreviewFrameRate(F_KEY_PREVIEW_FRAME_RATE_VALUE);

        mParameters.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, F_KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_VALUE);
    }

    mParameters.setPreviewFormat(previewPixelFormats[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
        SecCameraParameters::createValuesStr(previewPixelFormats, ARRAY_SIZE(previewPixelFormats)).c_str());

    /* Picture */
    mParameters.setPictureSize(mPictureSize.width, mPictureSize.height);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, mThumbnailSize.width);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, mThumbnailSize.height);

    if (mCameraId == CAMERA_FACING_BACK) {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                SecCameraParameters::createSizesStr(backPictureSizes, ARRAY_SIZE(backPictureSizes)).c_str());

        mParameters.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
                SecCameraParameters::createSizesStr(backThumbSizes, ARRAY_SIZE(backThumbSizes)).c_str());
    } else {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                SecCameraParameters::createSizesStr(frontPictureSizes, ARRAY_SIZE(frontPictureSizes)).c_str());

        mParameters.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
                SecCameraParameters::createSizesStr(frontThumbSizes, ARRAY_SIZE(frontThumbSizes)).c_str());
    }

    mParameters.setPictureFormat(picturePixelFormats[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
        SecCameraParameters::createValuesStr(picturePixelFormats, ARRAY_SIZE(picturePixelFormats)).c_str());

	mParameters.set(CameraParameters::KEY_JPEG_QUALITY, 1);
	mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, 100);

    mParameters.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    /* Video */
    mParameters.setVideoSize(mVideoSize.width, mVideoSize.height);
    if (mCameraId == CAMERA_FACING_BACK) {
        mParameters.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
            SecCameraParameters::createSizesStr(backRecordingSizes, ARRAY_SIZE(backRecordingSizes)).c_str());
        mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, B_KEY_VIDEO_STABILIZATION_SUPPORTED_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
                SecCameraParameters::createSizesStr(frontRecordingSizes, ARRAY_SIZE(frontRecordingSizes)).c_str());
        mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, F_KEY_VIDEO_STABILIZATION_SUPPORTED_VALUE);
    }

    mParameters.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, KEY_VIDEO_SNAPSHOT_SUPPORTED_VALUE);

    /* UI settings */
    mParameters.set(CameraParameters::KEY_WHITE_BALANCE, whiteBalances[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE,
        SecCameraParameters::createValuesStr(whiteBalances, ARRAY_SIZE(whiteBalances)).c_str());

    mParameters.set(CameraParameters::KEY_EFFECT, effects[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_EFFECTS,
       SecCameraParameters::createValuesStr(effects, ARRAY_SIZE(effects)).c_str());

    if (mCameraId == CAMERA_FACING_BACK) {
        mParameters.set(CameraParameters::KEY_SCENE_MODE, sceneModes[0].desc);
        mParameters.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,
                SecCameraParameters::createValuesStr(sceneModes, ARRAY_SIZE(sceneModes)).c_str());

        if (IsFlashSupported()) {
            mParameters.set(CameraParameters::KEY_FLASH_MODE, flashModes[0].desc);
            mParameters.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES,
                    SecCameraParameters::createValuesStr(flashModes, ARRAY_SIZE(flashModes)).c_str());
        }

        mParameters.set(CameraParameters::KEY_FOCUS_MODE, backFocusModes[0].desc);
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES, B_KEY_NORMAL_FOCUS_DISTANCES_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
                SecCameraParameters::createValuesStr(backFocusModes, ARRAY_SIZE(backFocusModes)).c_str());

        if (IsAutoFocusSupported()) {
            /* FOCUS AREAS supported.
             * MAX_NUM_FOCUS_AREAS > 0 : supported
             * MAX_NUM_FOCUS_AREAS = 0 : not supported
             *
             * KEY_FOCUS_AREAS = "left,top,right,bottom,weight"
             */
            mParameters.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, "1");
            mParameters.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
        }
    } else {
        mParameters.set(CameraParameters::KEY_FOCUS_MODE, frontFocusModes[0].desc);
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES, F_KEY_FOCUS_DISTANCES_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
                SecCameraParameters::createValuesStr(frontFocusModes, ARRAY_SIZE(frontFocusModes)).c_str());
    }

    /* Face Detect */
    if (mCameraId == CAMERA_FACING_BACK) {
        //mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, B_KEY_MAX_NUM_DETECTED_FACES_HW_VALUE);
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, "1");	/* FOR HAL TEST */
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, B_KEY_MAX_NUM_DETECTED_FACES_SW_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, F_KEY_MAX_NUM_DETECTED_FACES_HW_VALUE);
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, F_KEY_MAX_NUM_DETECTED_FACES_SW_VALUE);
    }


    /* Zoom */
    if (mZoomSupport) {
        mParameters.set(CameraParameters::KEY_ZOOM, 0);
        mParameters.set(CameraParameters::KEY_MAX_ZOOM, 30);
        /* If zoom is supported, set zoom ratios as value of KEY_ZOOM_RATIOS here.
         * EX) mParameters.set(CameraParameters::KEY_ZOOM_RATIOS, "100,102,104");
         */
        mParameters.set(CameraParameters::KEY_ZOOM_RATIOS,
                "100,102,104,109,111,113,119,121,124,131,"
                "134,138,146,150,155,159,165,170,182,189,"
                "200,213,222,232,243,255,283,300,319,364,400");
        mParameters.set(CameraParameters::KEY_ZOOM_SUPPORTED, B_KEY_ZOOM_SUPPORTED_VALUE);
        mParameters.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, B_KEY_SMOOTH_ZOOM_SUPPORTED_VALUE);
    }

    mParameters.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS, "3");		/* FOR HAL TEST 0 -> 3 */

    mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, 0);
    mParameters.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, 6);
    mParameters.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, -6);
    mParameters.setFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, 0.33);

    /* AE, AWB Lock */
    if (mCameraId == CAMERA_FACING_BACK) {
        mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, B_KEY_AUTO_EXPOSURE_LOCK_SUPPORTED_VALUE);
        mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, B_KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, F_KEY_AUTO_EXPOSURE_LOCK_SUPPORTED_VALUE);
        mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, F_KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED_VALUE);
    }

    /* AntiBanding */
    // chooseAntiBandingFrequency();
    if (mCameraId == CAMERA_FACING_BACK) {
        mParameters.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING,
                SecCameraParameters::createValuesStr(antibandings, ARRAY_SIZE(antibandings)).c_str());
        mParameters.set(CameraParameters::KEY_ANTIBANDING, antibandings[3].desc);
    }

    ALOGV("initDefaultParameters EX: %s", mParameters.flatten().c_str());
}

status_t ISecCameraHardware::setPreviewWindow(preview_stream_ops *w)
{
    mPreviewWindow = w;

    if (CC_UNLIKELY(!w)) {
        mPreviewWindowSize.width = mPreviewWindowSize.height = 0;
        ALOGE("setPreviewWindow: NULL Surface!");
        return OK;
    }

    int halPixelFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_FULL;

    if (mMovieMode)
        halPixelFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP;

    ALOGD("DEBUG(%s): size(%d/%d)", __FUNCTION__, mPreviewSize.width, mPreviewSize.height);
    /* YV12 */
    ALOGV("setPreviewWindow: halPixelFormat = %s",
        halPixelFormat == HAL_PIXEL_FORMAT_YV12 ? "YV12" :
        halPixelFormat == HAL_PIXEL_FORMAT_YCrCb_420_SP ? "NV21" :
        halPixelFormat == HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP ? "NV21M" :
        halPixelFormat == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL ? "NV21 FULL" :
        "Others");

    mPreviewWindowSize = mPreviewSize;
    ALOGD("DEBUG(%s): setPreviewWindow window Size width=%d height=%d", __FUNCTION__, mPreviewWindowSize.width, mPreviewWindowSize.height);
    if (nativeCreateSurface(mPreviewWindowSize.width, mPreviewWindowSize.height, halPixelFormat) == false) {
        ALOGE("setPreviewWindow: error, nativeCreateSurface");
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

#if IS_FW_DEBUG
void ISecCameraHardware::printDebugFirmware()
{
    if (!mPrintDebugEnabled) {
        ALOGD("printDebugFirmware mPrintDebugEnabled is false..");
        return;
    }

    mIs_debug_ctrl.write_point = *((unsigned int *)(mDebugVaddr + FIMC_IS_FW_DEBUG_REGION_SIZE));
    mIs_debug_ctrl.assert_flag = *((unsigned int *)(mDebugVaddr + FIMC_IS_FW_DEBUG_REGION_SIZE + 4));
    mIs_debug_ctrl.pabort_flag = *((unsigned int *)(mDebugVaddr + FIMC_IS_FW_DEBUG_REGION_SIZE + 8));
    mIs_debug_ctrl.dabort_flag = *((unsigned int *)(mDebugVaddr + FIMC_IS_FW_DEBUG_REGION_SIZE + 12));

    mCurrWp = mIs_debug_ctrl.write_point;
    mCurrOffset = mCurrWp - FIMC_IS_FW_DEBUG_REGION_ADDR;

    if (mCurrWp != mPrevWp) {
		unsigned int LP, CP;
        *(char *)(mDebugVaddr + mCurrOffset - 1) = '\0';
        LP = CP = mDebugVaddr + mPrevOffset;

        if (mCurrWp < mPrevWp) {
            *(char *)(mDebugVaddr + FIMC_IS_FW_DEBUG_REGION_SIZE - 1) = '\0';
            while (CP < (mDebugVaddr + FIMC_IS_FW_DEBUG_REGION_SIZE) && *(char *)CP != 0) {
                while(*(char *)CP != '\n' && *(char *)CP != 0)
                    CP++;
                *(char *)CP = NULL;
                if (*(char *)LP != 0)
                    LOGD_IS("%s", (char *)LP);
                LP = ++CP;
            }
            LP = CP = mDebugVaddr;
        }

        while (CP < (mDebugVaddr + mCurrOffset) && *(char *)CP != 0) {
            while(*(char *)CP != '\n' && *(char *)CP != 0)
                CP++;
            *(char *)CP = NULL;
            if (*(char *)LP != 0)
                LOGD_IS("%s", (char *)LP);
            LP = ++CP;
        }
    }

    mPrevWp = mIs_debug_ctrl.write_point;
    mPrevOffset = mPrevWp - FIMC_IS_FW_DEBUG_REGION_ADDR;

}

#if IS_FW_DEBUG_THREAD
bool ISecCameraHardware::debugThread()
{
    mDebugLock.lock();
    mDebugCondition.waitRelative(mDebugLock, 2000000000);
    if (mStopDebugging) {
        mDebugLock.unlock();
        return false;
    }

    printDebugFirmware();

    mDebugLock.unlock();

    return true;

}
#endif
#endif

status_t ISecCameraHardware::startPreview()
{
    ALOGD("startPreview E");

    mSensorSize.width = 0;
    mSensorSize.height = 0;

    LOG_PERFORMANCE_START(1);

    Mutex::Autolock lock(mLock);

    if (mPictureRunning) {
        ALOGW("startPreview: warning, picture is not completed yet");
        if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) ||
            (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME)) {
            /* upper layer can access the mmaped memory if raw or postview message is enabled
            But the data will be changed after preview is started */
            ALOGE("startPreview: error, picture data is not transferred yet");
            return INVALID_OPERATION;
        }
    }

    status_t ret;
    if (mEnableDZoom)
        ret = nativeStartPreviewZoom();
    else
        ret = nativeStartPreview();

    if (ret != NO_ERROR) {
        ALOGE("startPreview: error, nativeStartPreview");
#if IS_FW_DEBUG
        if (mCameraId == CAMERA_FACING_FRONT || mCameraId == FD_SERVICE_CAMERA_ID)
            printDebugFirmware();
#endif
        return NO_INIT;
    }

    mFlagFirstFrameDump = false;
    if (mEnableDZoom) {
#ifdef CHANGED_PREVIEW_SUPPORT
        int FaceMode = getFaceDetection();
        if(mPASMMode !=	MODE_BEAUTY_SHOT && FaceMode != V4L2_FACE_DETECTION_SMILE_SHOT) {
            ret = mPreviewZoomThread->run("previewZoomThread", PRIORITY_URGENT_DISPLAY);
            mPreviewThreadType = PREVIEW_THREAD_NORMAL;
        }
        else {
            ret = mChangedPreviewThread->run("changedPreviewThread", PRIORITY_URGENT_DISPLAY);
            mPreviewThreadType = PREVIEW_THREAD_CHANGED;
        }
#else
        ret = mPreviewZoomThread->run("previewZoomThread", PRIORITY_URGENT_DISPLAY);
        mPreviewThreadType = PREVIEW_THREAD_NORMAL;
#endif
    }
    else {
        ret = mPreviewThread->run("previewThread", PRIORITY_URGENT_DISPLAY);
        mPreviewThreadType = PREVIEW_THREAD_NORMAL;		
    }

    if (ret != NO_ERROR) {
        ALOGE("startPreview: error, Not starting preview");
        return UNKNOWN_ERROR;
    }

#if FRONT_ZSL
    if (mSamsungApp && !mMovieMode && mCameraId == CAMERA_FACING_FRONT) {
        if (nativeStartFullPreview() != NO_ERROR) {
            ALOGE("startPreview: error, nativeStartPreview");
            return NO_INIT;
        }

        if (mZSLPictureThread->run("zslpictureThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
            ALOGE("startPreview: error, Not starting preview");
            return UNKNOWN_ERROR;
        }

        mFullPreviewRunning = true;
    }
#endif
	mPreviewRunning = true;

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("startPreview X");
    return NO_ERROR;
}

bool ISecCameraHardware::previewThread()
{
    int index = nativeGetPreview();
    if (CC_UNLIKELY(index < 0)) {
		if (mFastCaptureCalled) {
			return false;
		}
        ALOGE("previewThread: error, nativeGetPreview");
#if IS_FW_DEBUG
        if (mCameraId == CAMERA_FACING_FRONT || mCameraId == FD_SERVICE_CAMERA_ID) {
            dump_is_fw_log("streamOn", (uint8_t *)mDebugVaddr, (uint32_t)FIMC_IS_FW_DEBUG_REGION_SIZE);
            printDebugFirmware();
        }
#endif
        if (!mPreviewThread->exitRequested()) {
#ifdef DEBUG_PREVIEW_NO_FRAME
            if (!mFactoryMode)
                LOG_FATAL("nativeGetPreview: failed to get a frame!");
            else
                mNotifyCb(CAMERA_MSG_ERROR, 2000, 0, mCallbackCookie);
#else
            if (!recordingEnabled())
                mNotifyCb(CAMERA_MSG_ERROR, 2000, 0, mCallbackCookie);
            else {
                ALOGI("previewThread: X");
                return false;
            }
#endif
            ALOGI("previewThread: X, after callback");
        }
        return false;
    }else if (mPreviewThread->exitRequested()) {
        return false;
    }

#ifdef DUMP_LAST_PREVIEW_FRAME
    mLastIndex = index;
#endif

    mLock.lock();

    if (mDropFrameCount > 0) {
        mDropFrameCount--;
        mLock.unlock();
        nativeReleasePreviewFrame(index);
        return true;
    }

    mLock.unlock();

    /* Notify the client of a new frame. */
    if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
#ifdef DEBUG_PREVIEW_CALLBACK
        if (!mPreviewCbStarted) {
            mPreviewCbStarted = true;
            ALOGD("preview callback started");
        }
#endif
        mDataCb(CAMERA_MSG_PREVIEW_FRAME, mPreviewHeap, index, NULL, mCallbackCookie);
    }

    /* Display a new frame */
    if (CC_LIKELY(mFlagANWindowRegister)) {
        bool ret = nativeFlushSurface(mPreviewWindowSize.width, mPreviewWindowSize.height, mPreviewFrameSize, index);
        if (CC_UNLIKELY(!ret))
            ALOGE("previewThread: error, nativeFlushSurface");
    }

    mLock.lock();
    if (mFirstStart == 0)
        mFirstStart = 1;
    mLock.unlock();

#if DUMP_FILE
    static int i = 0;
    if (++i % 15 == 0) {
        dumpToFile(mPreviewHeap->data + (mPreviewFrameSize*index), mPreviewFrameSize, "/data/preview.yuv");
        i = 0;
    }
#endif

    /* Release the frame */
    int err = nativeReleasePreviewFrame(index);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("previewThread: error, nativeReleasePreviewFrame");
        return false;
    }

#if defined(SEC_USES_TVOUT) && defined(SUSPEND_ENABLE)
    nativeTvoutSuspendCall();
#endif

    /* prevent a frame rate from getting higher than the max value */
    mPreviewThread->calcFrameWaitTime(mMaxFrameRate);

    return true;
}

bool ISecCameraHardware::previewZoomThread()
{
    int index = nativeGetPreview();
    int err = -1;

    if (CC_UNLIKELY(index < 0)) {
		if (mFastCaptureCalled) {
			return false;
		}
        ALOGE("previewZoomThread: error, nativeGetPreview in %s", recordingEnabled() ? "recording" : "preview");
#ifndef APPLY_ESD
        if (!mPreviewZoomThread->exitRequested()) {
#ifdef DEBUG_PREVIEW_NO_FRAME
            if (!mFactoryMode) {
                LOG_FATAL("nativeGetPreview: failed to get a frame!");
            } else {
                mNotifyCb(CAMERA_MSG_ERROR, 2000, 0, mCallbackCookie);
                ALOGI("previewZoomThread: X, after callback");
            }
#else
            mNotifyCb(CAMERA_MSG_ERROR, 2000, 0, mCallbackCookie);
            ALOGI("previewZoomThread: X, after callback");
#endif
        }
#endif	/* #ifndef APPLY_ESD */

#if VENDOR_FEATURE
#ifdef APPLY_ESD
        if (!mFactoryNumCnt) { /*14.02.15. heechul, this notiCb makes factory test NG */
            ALOGE("previewZoomThread: error, Before CAMERA_MSG_ERROR. ESD");
            mNotifyCb(CAMERA_MSG_ERROR, CAMERA_ERROR_PREVIEWFRAME_TIMEOUT, 0, mCallbackCookie);
            ALOGE("previewZoomThread: error, After CAMERA_MSG_ERROR. ESD");
        }
#endif	/* #ifdef APPLY_ESD */
#endif
        return false;
    } else if (mPreviewZoomThread->exitRequested()) {
        return false;
    }

#ifdef DUMP_LAST_PREVIEW_FRAME
    mLastIndex = index;
#endif

    mPostRecordIndex = index;

    mLock.lock();
    if (mDropFrameCount > 0) {
        mDropFrameCount--;
        mLock.unlock();
        nativeReleasePreviewFrame(index);
        return true;
    }
    mLock.unlock();

    /* first frame dump to jpeg */
    if (mFlagFirstFrameDump == true) {
        memcpy(mPictureBuf.virt.extP[0], mFliteNode.buffer[index].virt.extP[0], mPictureBuf.size.extS[0]);
        nativeMakeJpegDump();
        mFlagFirstFrameDump = false;
    }

    /* when recording mode, push frame of dq from FLite */
    if (mRecordingRunning) {
        static int skip_cnt;
        bool skip_frame_set;
        int videoSlotIndex = getRecSrcBufSlotIndex();
        if (videoSlotIndex < 0) {
            ALOGE("ERROR(%s): videoSlotIndex is -1", __func__);
        } else {
            if ( (mFrameRate == 15000) && (skip_cnt++ % 2))
                skip_frame_set = true;
            else
                skip_frame_set = false;

            if(!skip_frame_set) {
                mRecordSrcSlot[videoSlotIndex].buf = &(mFliteNode.buffer[index]);
                mRecordSrcSlot[videoSlotIndex].timestamp = mRecordTimestamp;
                /* ALOGV("DEBUG(%s): recording src(%d) adr %p, timestamp %lld", __func__, videoSlotIndex,
                    (mRecordSrcSlot[videoSlotIndex].buf)->virt.p, mRecordSrcSlot[videoSlotIndex].timestamp); */
                mPushRecSrcQ(&mRecordSrcSlot[videoSlotIndex]);
                mPostRecordCondition.signal();
            }
        }
    }

    mPreviewRunning = true;

    if(mFps == 120) {
        static int nDisplayDropCnt = 0;
        if(nDisplayDropCnt > 0) {
            nDisplayDropCnt--;
			/* Release the frame */
            err = nativeReleasePreviewFrame(index);
            if (CC_UNLIKELY(err < 0)) {
                ALOGE("previewZoomThread: error, nativeReleasePreviewFrame");
                return false;
            }
            return true;
        }
        else {
            nDisplayDropCnt = 1;
        }
    }
    /* Display a new frame */
    if (CC_LIKELY(mFlagANWindowRegister)) {
        bool ret = nativeFlushSurface(mPreviewWindowSize.width, mPreviewWindowSize.height, mPreviewFrameSize, index);
        if (CC_UNLIKELY(!ret))
            ALOGE("previewThread: error, nativeFlushSurface");
    } else {
        /* if not register ANativeWindow, just prepare callback buffer on CAMERA_MSG_PREVIEW_FRAME */
        if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
            if (nativePreviewCallback(index, NULL) < 0)
                ALOGE("ERROR(%s): nativePreviewCallback failed", __func__);
    }

    /* Notify the client of a new frame. */
    if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
#ifdef DEBUG_PREVIEW_CALLBACK
        if (!mPreviewCbStarted) {
            mPreviewCbStarted = true;
            ALOGD("preview callback started");
        }
#endif
        mDataCb(CAMERA_MSG_PREVIEW_FRAME, mPreviewHeap, index, NULL, mCallbackCookie);
    }

#if DUMP_FILE
    static int i = 0;
    if (++i % 15 == 0) {
        dumpToFile(mPreviewHeap->data + (mPreviewFrameSize*index), mPreviewFrameSize, "/data/preview.yuv");
        i = 0;
    }
#endif

    /* Release the frame */
    err = nativeReleasePreviewFrame(index);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("previewZoomThread: error, nativeReleasePreviewFrame");
        return false;
    }

    return true;
}

#ifdef CHANGED_PREVIEW_SUPPORT
bool ISecCameraHardware::changedPreviewThread()
{
    int index = nativeGetPreview();
    int err = -1;

    if (CC_UNLIKELY(index < 0)) {
		if (mFastCaptureCalled) {
			return false;
		}
        ALOGE("changedPreviewThread: error, nativeGetPreview in %s", recordingEnabled() ? "recording" : "preview");
        if (!mChangedPreviewThread->exitRequested()) {
#ifdef DEBUG_PREVIEW_NO_FRAME
            if (!mFactoryMode) {
                LOG_FATAL("nativeGetPreview: failed to get a frame!");
            } else {
                mNotifyCb(CAMERA_MSG_ERROR, 2000, 0, mCallbackCookie);
                ALOGI("changedPreviewThread: X, after callback");
            }
#else
            mNotifyCb(CAMERA_MSG_ERROR, 2000, 0, mCallbackCookie);
            ALOGI("changedPreviewThread: X, after callback");
#endif
        }
        return false;
    } else if (mChangedPreviewThread->exitRequested()) {
        return false;
    }
#ifdef DUMP_LAST_PREVIEW_FRAME
    mLastIndex = index;
#endif

    mLock.lock();
    if (mDropFrameCount > 0) {
        mDropFrameCount--;
        mLock.unlock();
        nativeReleasePreviewFrame(index);
        return true;
    }
    mLock.unlock();

    /* first frame dump to jpeg */
    if (mFlagFirstFrameDump == true) {
        memcpy(mPictureBuf.virt.extP[0], mFliteNode.buffer[index].virt.extP[0], mPictureBuf.size.extS[0]);
        nativeMakeJpegDump();
        mFlagFirstFrameDump = false;
    }

    /* Convert frame format from YV12 to NV21  */
    nativeCSCPreview(index, CAMERA_HEAP_PREVIEW);
    /* Notify the client of a new frame. */
    if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
#ifdef DEBUG_PREVIEW_CALLBACK
        if (!mPreviewCbStarted) {
            mPreviewCbStarted = true;
            ALOGD("preview callback started");
        }
#endif
        mDataCb(CAMERA_MSG_PREVIEW_FRAME, mPreviewHeap, index, NULL, mCallbackCookie);
    }
    /* Display a new frame */
    if (CC_LIKELY(mFlagANWindowRegister)) {
        bool ret = beautyLiveFlushSurface(mPreviewWindowSize.width, mPreviewWindowSize.height, mPreviewFrameSize, index);
        if (CC_UNLIKELY(!ret))
            ALOGE("changedPreviewThread: error, nativeFlushSurface");
    } else {
        /* if not register ANativeWindow, just prepare callback buffer on CAMERA_MSG_PREVIEW_FRAME */
        if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
            if (nativePreviewCallback(index, NULL) < 0)
                ALOGE("ERROR(%s): nativePreviewCallback failed", __func__);
    }
    mPreviewRunning = true;

#if DUMP_FILE
    static int i = 0;
    if (++i % 15 == 0) {
        dumpToFile(mPreviewHeap->data + (mPreviewFrameSize*index), mPreviewFrameSize, "/data/preview.yuv");
        i = 0;
    }
#endif

    /* Release the frame */
    err = nativeReleasePreviewFrame(index);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("changedPreviewThread: error, nativeReleasePreviewFrame");
        return false;
    }
    return true;
}
#endif

void ISecCameraHardware::stopPreview()
{
	ALOGD("stopPreview E");

	/*
	 * try count to wait for stopping previewThread
	 * maximum wait time = 30 * 10ms
	 */
	int waitForCnt = 600; // 30 -->. 600 (300ms --> 6sec) because, Polling timeout is 5 sec.

	LOG_PERFORMANCE_START(1);

	{
		Mutex::Autolock lock(mLock);
		if (!mPreviewRunning) {
			ALOGW("stopPreview: warning, preview has been stopped");
			return;
		}
	}

    if(mPASMMode == MODE_PANORAMA)
        disableMsgType(CAMERA_MSG_PREVIEW_FRAME);

	if(!mFactorySysMode)
		nativeDestroySurface();
	/* don't hold the lock while waiting for the thread to quit */
#if FRONT_ZSL
	if (mFullPreviewRunning) {
		mZSLPictureThread->requestExitAndWait();
		nativeForceStopFullPreview();
		nativeStopFullPreview();
		mFullPreviewRunning = false;
	}
#endif
	if (mEnableDZoom) {
#ifdef CHANGED_PREVIEW_SUPPORT
		if(mPreviewThreadType == PREVIEW_THREAD_NORMAL) {
			mPreviewZoomThread->requestExit();
			/* if previewThread can't finish, wait for 300ms */
			while (waitForCnt > 0 && mPreviewZoomThread->getTid() >= 0) {
				usleep(10000);
				waitForCnt--;
				if(waitForCnt == 0)
					ALOGD("waitForCnt is ZERO");
			}
		}
		else {
			mChangedPreviewThread->requestExit();
			/* if mChangedPreviewThread can't finish, wait for 25ms */
			while (waitForCnt > 0 && mChangedPreviewThread->getTid() >= 0) {
				usleep(10000);
				waitForCnt--;
			}
		}
#else
		mPreviewZoomThread->requestExit();
		/* if previewThread can't finish, wait for 25ms */
		while (waitForCnt > 0 && mPreviewZoomThread->getTid() >= 0) {
			usleep(10000);
			waitForCnt--;
		}
#endif
	} else {
		mPreviewThread->requestExitAndWait();
	}

	mPreviewThreadType = PREVIEW_THREAD_NONE;
#ifdef DUMP_LAST_PREVIEW_FRAME
	uint32_t offset = mPreviewFrameSize * mLastIndex;
	dumpToFile(mPreviewHeap->base() + offset, mPreviewFrameSize, "/data/preview-last.dump");
#endif

	Mutex::Autolock lock(mLock);

	nativeStopPreview();

	if (mEnableDZoom == true && waitForCnt > 0)
		mPreviewZoomThread->requestExitAndWait();

	mPreviewRunning = false;
	mPreviewInitialized = false;
#ifdef DEBUG_PREVIEW_CALLBACK
	mPreviewCbStarted = false;
#endif
	mDropFrameCount = INITIAL_REAR_SKIP_FRAME;
	LOG_PERFORMANCE_END(1, "total");

	if (mPreviewHeap) {
		mPreviewHeap->release(mPreviewHeap);
		mPreviewHeap = 0;
		mPreviewHeapFd = -1;
	}

	if (mRecordingHeap) {
		mRecordingHeap->release(mRecordingHeap);
		mRecordingHeap = 0;
	}

	if (mRawHeap != NULL) {
		mRawHeap->release(mRawHeap);
		mRawHeap = 0;
	}

	if (mJpegHeap) {
		mJpegHeap->release(mJpegHeap);
		mJpegHeap = 0;
	}

	if (mHDRHeap) {
		mHDRHeap->release(mHDRHeap);
		mHDRHeap = 0;
	}

	if (mPostviewHeap) {
		mPostviewHeap->release(mPostviewHeap);
		mPostviewHeap = 0;
	}

	if (mPostviewHeapTmp) {
		mPostviewHeapTmp->release(mPostviewHeapTmp);
		mPostviewHeapTmp = 0;
	}

	ALOGD("stopPreview X");
}

status_t ISecCameraHardware::storeMetaDataInBuffers(bool enable)
{
    ALOGV("%s", __FUNCTION__);

    if (!enable) {
        ALOGE("Non-m_frameMetadata buffer mode is not supported!");
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ISecCameraHardware::startRecording()
{
    ALOGD("startRecording E");

    Mutex::Autolock lock(mLock);

    status_t ret;
    mLastRecordTimestamp = 0;
#if FRONT_ZSL
    if (mFullPreviewRunning) {
        nativeForceStopFullPreview();
        mZSLPictureThread->requestExitAndWait();
        nativeStopFullPreview();
        mFullPreviewRunning = false;
    }
#endif

    if (mEnableDZoom) {
        ret = nativeStartRecordingZoom();
    } else {
        ret = nativeStartRecording();
    }

    if (CC_UNLIKELY(ret != NO_ERROR)) {
        ALOGE("startRecording X: error, nativeStartRecording");
        return UNKNOWN_ERROR;
    }

    if (mEnableDZoom) {
        mPostRecordExit = false;
        ret = mPostRecordThread->run("postRecordThread", PRIORITY_URGENT_DISPLAY);
    } else
        ret = mRecordingThread->run("recordingThread", PRIORITY_URGENT_DISPLAY);

    if (CC_UNLIKELY(ret != NO_ERROR)) {
        mRecordingTrace = true;
        ALOGE("startRecording: error %d, Not starting recording", ret);
        return ret;
    }

    mRecordingRunning = true;

    ALOGD("startRecording X");
    return NO_ERROR;
}

bool ISecCameraHardware::recordingThread()
{
    return true;
}

bool ISecCameraHardware::postRecordThread()
{
    mPostRecordLock.lock();
    mPostRecordCondition.wait(mPostRecordLock);
    mPostRecordLock.unlock();

    if (mSizeOfRecSrcQ() == 0) {
        ALOGW("WARN(%s): mSizeOfRecSrcQ size is zero", __func__);
    } else {
        rec_src_buf_t srcBuf;

        while (mSizeOfRecSrcQ() > 0) {
			int index;

            /* get dst video buf index */
            index = getRecDstBufIndex();
            if (index < 0) {
                ALOGW("WARN(%s): getRecDstBufIndex(%d) sleep and continue, skip frame buffer", __func__, index);
                usleep(13000);
                continue;
            }

            /* get src video buf */
            if (mPopRecSrcQ(&srcBuf) == false) {
                ALOGW("WARN(%s): mPopRecSrcQ(%d) failed", __func__, index);
                return false;
            }

            /* ALOGV("DEBUG(%s): SrcBuf(%d, %d, %lld), Dst idx(%d)", __func__,
                srcBuf.buf->fd.extFd[0], srcBuf.buf->fd.extFd[1], srcBuf.timestamp, index); */

            /* Notify the client of a new frame. */
            if (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME) {
				bool ret;
                /* csc from flite to video MHB and callback */
                ret = nativeCSCRecording(&srcBuf, index);
                if (ret == false) {
                    ALOGE("ERROR(%s): nativeCSCRecording failed.. SrcBuf(%d, %d, %lld), Dst idx(%d)", __func__,
                        srcBuf.buf->fd.extFd[0], srcBuf.buf->fd.extFd[1], srcBuf.timestamp, index);
                    setAvailDstBufIndex(index);
                    return false;
                } else {
                    if (0L < srcBuf.timestamp && mLastRecordTimestamp < srcBuf.timestamp) {
                        mDataCbTimestamp(srcBuf.timestamp, CAMERA_MSG_VIDEO_FRAME,
                                         mRecordingHeap, index, mCallbackCookie);
                        mLastRecordTimestamp = srcBuf.timestamp;
                        LOG_RECORD_TRACE("callback returned");
                    } else {
                        ALOGW("WARN(%s): timestamp(%lld) invaild - last timestamp(%lld) systemtime(%lld)",
                            __func__, srcBuf.timestamp, mLastRecordTimestamp, systemTime(SYSTEM_TIME_MONOTONIC));
                        setAvailDstBufIndex(index);
                    }
                }
            }
        }
    }
    LOG_RECORD_TRACE("X");
    return true;
}

void ISecCameraHardware::stopRecording()
{
    ALOGD("stopRecording E");

    Mutex::Autolock lock(mLock);
    if (!mRecordingRunning) {
        ALOGW("stopRecording: warning, recording has been stopped");
        return;
    }

    /* We request thread to exit. Don't wait. */
    if (mEnableDZoom) {
        mPostRecordExit = true;

        /* Change calling order of requestExit...() and signal
         * if you want to change requestExit() to requestExitAndWait().
         */
        mPostRecordThread->requestExit();
        mPostRecordCondition.signal();
        nativeStopRecording();
    } else {
        mRecordingThread->requestExit();
        nativeStopRecording();
    }

    mRecordingRunning = false;

    ALOGD("stopRecording X");
}

void ISecCameraHardware::releaseRecordingFrame(const void *opaque)
{
    status_t ret = NO_ERROR;
    bool found = false;
    int i;

    /* We does not release frames recorded any longer
     * if this function is called after stopRecording().
     */
    if (mEnableDZoom)
        ret = mPostRecordThread->exitRequested();
    else
        ret = mRecordingThread->exitRequested();

    if (CC_UNLIKELY(ret)) {
        ALOGW("releaseRecordingFrame: warning, we do not release any more!!");
        return;
    }

    {
        Mutex::Autolock lock(mLock);
        if (CC_UNLIKELY(!mRecordingRunning)) {
            ALOGW("releaseRecordingFrame: warning, recording is not running");
            return;
        }
    }

    struct addrs *addrs = (struct addrs *)mRecordingHeap->data;

    /* find MHB handler to match */
    if (addrs) {
        for (i = 0; i < REC_BUF_CNT; i++) {
            if ((char *)(&(addrs[i].type)) == (char *)opaque) {
                found = true;
                break;
            }
        }
    }

    mRecordDstLock.lock();
    if (found) {
        mRecordFrameAvailableCnt++;
        /* ALOGV("DEBUG(%s): found index[%d] FDy(%d), FDcbcr(%d) availableCount(%d)", __func__,
            i, addrs[i].fd_y, addrs[i].fd_cbcr, mRecordFrameAvailableCnt); */
        mRecordFrameAvailable[i] = true;
    } else
        ALOGE("ERR(%s):no matched index(%p)", __func__, (char *)opaque);

    if (mRecordFrameAvailableCnt > REC_BUF_CNT) {
        ALOGW("WARN(%s): mRecordFrameAvailableCnt is more than REC_BUF!!", __func__);
        mRecordFrameAvailableCnt = REC_BUF_CNT;
    }
    mRecordDstLock.unlock();
}

#ifdef ZOOM_CTRL_THREAD
bool ISecCameraHardware::ZoomCtrlThread()
{
	static int lens_checked;
	static int zoom_change;
	int get_zoom_read;
	int opti_zoom_level, opti_zoom_status;
	int get_lens_status;

	if (mZoomParamSet) {

	if (mPreviewRunning && !mPictureRunning && !mPictureStart) {
		nativeGetParameters(CAM_CID_ZOOM, &get_zoom_read);

		opti_zoom_level = get_zoom_read & 0x0F;
		opti_zoom_status = (get_zoom_read >> 4) & 0x07;

		if (opti_zoom_level != mZoomCurrLevel) {
			zoom_change = 1;
			mZoomCurrLevel = opti_zoom_level;
		}

		if (opti_zoom_status != mZoomStatus) {
			mZoomStatus = opti_zoom_status;
			if (!mZoomStatus) {
				zoom_change = 1;
			}
		}

		mLock.lock();
		mParameters.set("curr_zoom_level", mZoomCurrLevel);
		mParameters.set(CameraParameters::KEY_ZOOM_LENS_STATUS, mZoomStatus);
		mLock.unlock();

		if ( (mZoomStatusBak&0x06) != (mZoomStatus&0x6) ) {
			ALOGD("Smart LensStatus------- : %02x  ->  %02x (%d)   Level:%d", mZoomStatusBak, mZoomStatus, mZoomStatus&0x4, mZoomCurrLevel);
			mZoomStatusBak = mZoomStatus;
			if ( mZoomStatus&0x6 )
				zoom_change = 1;
		}


		if (!mLensChecked) {
			nativeGetParameters(CAM_CID_LENS_STATUS, &get_lens_status);
			if (get_lens_status == 3 || get_lens_status == 2) {
				lens_checked = 1;
				zoom_change = 1;
				mLensChecked = 1;
				ALOGD("Smart LensStatus : Lens %s error", (get_lens_status == 3 ? "open" : "close"));
			}
			ALOGV("Smart LensStatus : %d", get_lens_status);
		}

		if (!mPictureStart && (mMsgEnabled & CAMERA_MSG_ZOOM_STEP_NOTIFY)) {
			mLensStatus = mZoomStatus | (lens_checked << 3);
			lens_checked = 0;
			if(zoom_change == 1) {
				mNotifyCb(CAMERA_MSG_ZOOM_STEP_NOTIFY, mLensStatus, mZoomCurrLevel, mCallbackCookie);
				ALOGD("ZoomCtrlThread: callback  zoom level : %d, status : %02x, checked : %x02x, zoom_cmd:%d", mZoomCurrLevel, mLensStatus, lens_checked, zoom_cmd);
				zoom_change = 0;
			}

			if(zoom_cmd != 100) {
				if((zoom_cmd == V4L2_OPTICAL_ZOOM_TELE_START || zoom_cmd == V4L2_OPTICAL_ZOOM_WIDE_START ||zoom_cmd == V4L2_OPTICAL_ZOOM_SLOW_TELE_START 
					|| zoom_cmd == V4L2_OPTICAL_ZOOM_SLOW_WIDE_START	|| zoom_cmd == V4L2_OPTICAL_ZOOM_PINCH_START 
					|| zoom_cmd == V4L2_OPTICAL_ZOOM_PINCH_STOP)  && mLensStatus == 0) {
					if( ((zoom_cmd == V4L2_OPTICAL_ZOOM_TELE_START || zoom_cmd == V4L2_OPTICAL_ZOOM_SLOW_TELE_START) && mLensStatus == 0 && mZoomCurrLevel == 15) 
						||((zoom_cmd == V4L2_OPTICAL_ZOOM_WIDE_START || zoom_cmd == V4L2_OPTICAL_ZOOM_SLOW_WIDE_START) && mLensStatus == 0 && mZoomCurrLevel == 0)	) {
						ALOGD("ZoomCtrlThread: zoom_cmd:%d, mLensStatus:%d, mZoomCurrLevel:%d", zoom_cmd, mLensStatus, mZoomCurrLevel);
						ALOGD("ZoomCtrlThread: CAM_CID_OPTICAL_ZOOM_CTRL STOP 3");
						nativeSetParameters(CAM_CID_OPTICAL_ZOOM_CTRL, V4L2_OPTICAL_ZOOM_STOP);
						old_zoom_cmd = zoom_cmd;
						zoom_cmd = 100;
					} else {
						ALOGD("ZoomCtrlThread: CAM_CID_OPTICAL_ZOOM_CTRL zoom_cmd:%d, old_zoom_cmd:%d", zoom_cmd, old_zoom_cmd);
						if(zoom_cmd != V4L2_OPTICAL_ZOOM_STOP && old_zoom_cmd != V4L2_OPTICAL_ZOOM_STOP && zoom_cmd != 100 && old_zoom_cmd != 100) {
							ALOGD("ZoomCtrlThread: CAM_CID_OPTICAL_ZOOM_CTRL STOP 4");
							nativeSetParameters(CAM_CID_OPTICAL_ZOOM_CTRL, V4L2_OPTICAL_ZOOM_STOP);
							old_zoom_cmd = V4L2_OPTICAL_ZOOM_STOP;
						} else {
							ALOGD("ZoomCtrlThread: CAM_CID_OPTICAL_ZOOM_CTRL 4 zoom_cmd:%d", zoom_cmd);
							nativeSetParameters(CAM_CID_OPTICAL_ZOOM_CTRL, zoom_cmd);
							old_zoom_cmd = zoom_cmd;
							zoom_cmd = 100;
						}
					}
				} else if(zoom_cmd == V4L2_OPTICAL_ZOOM_STOP && mLensStatus == 1) {
					ALOGD("ZoomCtrlThread: CAM_CID_OPTICAL_ZOOM_CTRL STOP");
					nativeSetParameters(CAM_CID_OPTICAL_ZOOM_CTRL, zoom_cmd);
					old_zoom_cmd = zoom_cmd;
					zoom_cmd = 100;
				} else if(zoom_cmd == V4L2_OPTICAL_ZOOM_STOP && mLensStatus == 0 && (mZoomCurrLevel == 0 || mZoomCurrLevel == 15)) { 
					ALOGD("ZoomCtrlThread: CAM_CID_OPTICAL_ZOOM_CTRL STOP 2");
					nativeSetParameters(CAM_CID_OPTICAL_ZOOM_CTRL, zoom_cmd);
					old_zoom_cmd = zoom_cmd;
					zoom_cmd = 100;
				}
			}
		}
	}
	}

	usleep(10000);

	ALOGV("ZoomCtrlThread X");

	return true;
}
#endif

bool ISecCameraHardware::shutterThread()
{
	int read_val = 0;
	int err;

	ALOGD("ShutterThread: E");
	if (mCameraPower == false)
		return false;
	err = nativeGetNotiParameters(CAM_CID_NOTIFICATION, &read_val);
	ALOGD("ShutterThread: read_val = %d", read_val);
	if (err < 0) {
		ALOGD("ShutterThread: Sound interrupt wakeup by timeout(35sec). NO ERROR");
	} else {
		if (mCameraPower == false) {
			ALOGD("ShutterThread: Forced wakeup sound interrupt. NO ERROR.");
		} else {
#ifdef BURST_SHOT_SUPPORT
			if (mCaptureMode == RUNNING_MODE_BURST) {
				mBurstSoundCount++;
				if (mBurstSoundCount == 1) {
					ALOGD("BURSTSHOT: shutterThread: wakeup burstPictureThread()");
					mBurstShotCondition.signal();
				}

				if (mBurstSoundCount > mBurstSoundMaxCount) {
					mBurstSoundMaxCount = mBurstSoundCount;
					ALOGD("shutterThread: mBurstSoundMaxCount = %d", mBurstSoundMaxCount);
				}
			}
            ALOGE("BURSTSHOT ----- : CAMERA_MSG_SHUTTER  %d/%d", mBurstSoundCount, mBurstSoundMaxCount);
#endif
			if (mMsgEnabled & CAMERA_MSG_SHUTTER) {
				ALOGD("ShutterThread: Sound interrupt issued. Play shutter sound.");
				mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
			} else {
				ALOGE("ShutterThread: Sound interrupt issued but CAMERA_MSG_SHUTTER is disabled.");
			}
		}
	}

	if (mCameraPower == false)
		return false;
	else
		return true;
}

#ifdef SMART_THREAD
bool ISecCameraHardware::smartThread()
{
    static unsigned int thread_cnt, time_setting;
//    int err;
    int get_lens_status;


//    int strobe_charge, strobe_up_down;
//    float now_AV, now_TV, now_EV, now_EXP;
//    int now_SceneMode, now_SceneSubMode, now_FDnum, now_OTstatus, now_LV;
    int smart_self_shot_fd_info1 = 0, smart_self_shot_fd_info2 = 0;
    int fd_info_top = 0, fd_info_left = 0, fd_info_right = 0, fd_info_bottom = 0;

    /* for NSM Mode*/
    int nsm_fd_update = 0;
    /* end NSM Mode */

//    ALOGV("smartThread E");
    if (mCameraId == CAMERA_FACING_FRONT)
        return true;

    thread_cnt++;

    //Mutex::Autolock lock(mLock);

#if 0
//LOW_TEMPERATURE_LOCK_SUPPORT
    if(nativeCheckTimeUnLockFreq()) {
        ALOGD("LOW_TEMPERATURE_LOCK_SUPPORT : UNLOCK");
        nativeUnlockCurrCpuFreq();
        nativeUnlockBusFreq();
        nativeUnlockCpuMax();

        mTempLockChecking = false;
    }
#endif
    if(mBFactoryStopSmartThread) {
        ALOGD("pass smartThread for factory test");
        usleep(50000);
        return true;
    }

    if (mPreviewRunning && !mPictureRunning && !mPictureStart) {
		static int zoom_change;
		static int lens_checked;
#ifdef AUTO_PARAM_CALLBACK
        int autoparam_update = 0;
#endif
#ifdef SMART_READ_INFO
        int smart_read1 = 0, smart_read2 = 0;
#endif

        if (mZoomParamSet) {
            int get_zoom_read;
            int opti_zoom_level, opti_zoom_status;
#ifndef SMART_READ_INFO
            nativeGetParameters(CAM_CID_OPTICAL_ZOOM_CTRL, &get_zoom_read);
#else
            nativeGetParameters(CAM_CID_ZOOM, &get_zoom_read);
#endif
            opti_zoom_level = get_zoom_read & 0x1F;
            opti_zoom_status = (get_zoom_read >> 5) & 0x01;

            if (opti_zoom_level != mZoomCurrLevel) {
                zoom_change = 1;
                mZoomCurrLevel = opti_zoom_level;
            }

            if (opti_zoom_status != mZoomStatus) {
                mZoomStatus = opti_zoom_status;
                if (!mZoomStatus) {
                    zoom_change = 1;
                }
            }

            mLock.lock();
            mParameters.set("curr_zoom_level", mZoomCurrLevel);
            mParameters.set(CameraParameters::KEY_ZOOM_LENS_STATUS, mZoomStatus);
            mLock.unlock();

            //ALOGV("zoom level: %d, status: %d", mZoomCurrLevel, mZoomStatus);
        }

        mLock.lock();
        if (!(thread_cnt % 9600) || mFirstStart == 1) {
            time_setting = 1;
        }
        mLock.unlock();

        if (!(thread_cnt % 10)) {
#ifndef SMART_READ_INFO
#if 0
            nativeGetParameters(CAM_CID_FLASH, &get_flash_read);

            strobe_charge = get_flash_read & 0xFF;
            strobe_up_down = (get_flash_read >> 8) & 0xFF;

            mLock.lock();
            mParameters.set(CameraParameters::KEY_FLASH_CHARGING, strobe_charge);
            mParameters.set(CameraParameters::KEY_FLASH_STANDBY,
                    strobe_up_down ? CameraParameters::FLASH_STANDBY_ON
                    : CameraParameters::FLASH_STANDBY_OFF);
            mLock.unlock();

            ALOGV("strobe charge: %d, up_down: %d", strobe_charge, strobe_up_down);

            now_AV = nativeGetAV();
            now_TV = nativeGetTV();
            now_LV = nativeGetLV();
            now_SV = nativeGetSV();
            now_SceneMode = nativeGetSceneMode();
            now_SceneSubMode = nativeGetSceneSubMode();
            now_FDnum = nativeGetFDnumber();
            now_OTstatus = nativeGetOTstatus();
            now_WC = nativegetWarningCondition();

            now_AV = now_AV / 65536;
            now_TV = now_TV / 65536;
            now_SV = now_SV / 65536;
            now_EV = now_AV + now_TV;

            //now_AV = sqrt(pow(2,now_AV));
            now_AV = findfnumbervalue((int)(now_AV*10));

            //now_TV = 1/(pow(2,now_TV))*1000;
            now_TV = findShutterSpeedIndex((int)(now_TV*10));

            now_EV = findEVindex(now_EV);

            now_SV = pow(2,now_SV) / 0.32;
            now_ISO = findISOIndex((int)now_SV);

            ALOGV("AV : %d,  TV : %d,  EV : %d FDnum : %d OT : %d LV : %d ISO : %d", (int)now_AV, (int)now_TV, (int)now_EV, now_FDnum, now_OTstatus, now_LV, now_ISO);

            if (now_EV > 6)
                now_EV = 6.000;
            else if (now_EV < -6)
                now_EV = -6.000;

            ALOGV("Smart AV : %d,  TV : %d,  EV : %d FDnum : %d OT : %d LV : %d ISO : %d", (int)now_AV, (int)now_TV, (int)now_EV, now_FDnum, now_OTstatus, now_LV, now_ISO);
            ALOGV("Smart Scene : %d,  SceneSub : %d,  Flash charge : %d Flash standby : %d", (int)now_SceneMode, (int)now_SceneSubMode, (int)strobe_charge, (int)strobe_up_down);

            mLock.lock();
            mParameters.setAutoParameter((int)now_AV, (int)now_TV, (int)now_EV, now_SceneMode, now_SceneSubMode, now_FDnum, now_OTstatus, now_LV, now_ISO/*, now_WC*/);
            mLock.unlock();
#endif
#else	//#ifndef SMART_READ_INFO
            nativeGetParameters(CAM_CID_SMART_READ1, &smart_read1);
            nativeGetParameters(CAM_CID_SMART_READ2, &smart_read2);

            /* for NSM Mode */
            if (mbGetFDinfo && (mPASMMode == MODE_SMART_SUGGEST)) {
                int fd_info;
                char str[32];
                char buf[10] = {0,};

                buf[0] = (smart_read2 >> 2) & 0x1F;	// scene main : 2~6
                buf[1] = smart_read2 & 0x03;	// scene sub : 0~1
                sprintf(str, "0x%02X%02X", buf[0], buf[1]);

                mParameters.set(SecCameraParameters::KEY_NSM_SCENE_DETECT, str);

                nativeGetParameters(CAM_CID_NSM_FD_INFO, &fd_info);
                mParameters.set(SecCameraParameters::KEY_NSM_FD_INFO, fd_info);

                ALOGD("@@@ smartThread : NSM [SCENE DETECT = %s] [FD_INFO = 0x%08x]", str, fd_info);

                nsm_fd_update = 1;
                mLock.lock();
                mbGetFDinfo = false;
                mLock.unlock();
            }
            /* end NSM Mode */

#ifndef AUTO_PARAM_CALLBACK  /* callback AutoParameter */
            now_EV = (smart_read1 & 0xF) - 6;
            now_FDnum = (smart_read1 >> 4) & 0x1;
            now_OTstatus = (smart_read1 >> 5) & 0x3;
            strobe_charge = (smart_read1 >> 7) & 0x1;
            strobe_up_down = (smart_read1 >> 8) & 0x1;
            now_LV = (smart_read1 >> 9) & 0xF;
            // now_ISO = calculate_ISO((smart_read1 >> 13) & 0x7);
            now_AV = ((smart_read1 >> 16) & 0xF) + (((smart_read1 >> 20) & 0xF) * 10);
            now_TV = ((smart_read1 >> 24) & 0x2F) + 1;
            now_EXP = (smart_read1 >> 30) & 0x1;

            now_SceneSubMode = smart_read2 & 0x3;
            now_SceneMode = (smart_read2 >> 2) & 0x1F;
            now_SceneSubMode = (now_SceneSubMode == 0 ? 0 : (now_SceneSubMode + 0x12));

            mLock.lock();
            mParameters.set(CameraParameters::KEY_FLASH_CHARGING, strobe_charge);
            mParameters.set(CameraParameters::KEY_FLASH_STANDBY,
                    strobe_up_down ? CameraParameters::FLASH_STANDBY_ON
                    : CameraParameters::FLASH_STANDBY_OFF);
            mLock.unlock();

            ALOGV("Smart strobe charge: %d, up_down: %d", strobe_charge, strobe_up_down);
            ALOGV("Smart AV : %d,  TV : %d,  EV : %d, EXP : %d ", (int)now_AV, (int)now_TV, (int)now_EV, now_EXP);
            ALOGV("Smart FDnum : %d, OT : %d, LV : %d, ISO : %d", now_FDnum, now_OTstatus, now_LV, now_ISO);
            ALOGV("Smart Scene : %d,  Sub : %d", (int)now_SceneMode, (int)now_SceneSubMode);

            mLock.lock();
            mParameters.setAutoParameter((int)now_AV, (int)now_TV, (int)now_EV, now_SceneMode, now_SceneSubMode, now_FDnum, now_OTstatus, now_LV, now_ISO/*, now_WC*/);
            mLock.unlock();
#else	//#ifndef AUTO_PARAM_CALLBACK  /* callback AutoParameter */
            autoparam_update = 1;
#if 0
            /* F number and Max aperture value is using AV value */
            if (((smart_read1 >> 16) & 0xFF) < 0x31 )  // ISP issue - occurred with the F-num greater than 1 byte at Zoom 8~10.
                mExifFnum = (((smart_read1 >> 16) & 0xF) + (((smart_read1 >> 20) & 0xF) * 10)+ 160) * 10 ;
            else
                mExifFnum = (((smart_read1 >> 16) & 0xF) + (((smart_read1 >> 20) & 0xF) * 10)) * 10;
            //ALOGV("integ = %d, minor = %d, mExifFnum = %d", (smart_read1 >> 16) & 0xF, ((smart_read1 >> 20) & 0xF), mExifFnum);
#else
            mExifFnum = ((smart_read1 >> 8) & 0xFF) * 10;
            //ALOGD("mExifFnum = %d", mExifFnum);
#endif
            mExifShutterSpeed = (smart_read1 >> 24) & 0x3F;
            //ALOGV("Real-time shutter speed = %d", mExifShutterSpeed);
#endif	//#ifndef AUTO_PARAM_CALLBACK  /* callback AutoParameter */
#endif  /* SMART_READ_INFO */

            if (!mLensChecked) {
                nativeGetParameters(CAM_CID_LENS_STATUS, &get_lens_status);

                if (get_lens_status == 3 || get_lens_status == 2) {
                    lens_checked = 1;
                    zoom_change = 1;
                    mLensChecked = 1;
                    ALOGD("Smart LensStatus : Lens %s error", (get_lens_status == 3 ? "open" : "close"));
                }
                //ALOGV("Smart LensStatus : %d", get_lens_status);
            }

            /* burst shot free buffer info */
            smart_read2 = (smart_read2 & ~0x80000000) | ((mBurstShot.isStartLimit() << 31) & 0x80000000);
        }

#ifdef AUTO_PARAM_CALLBACK  /* callback AutoParameter */
        if (!mPictureStart && autoparam_update && (mMsgEnabled & CAMERA_MSG_AUTO_PARAMETERS_NOTIFY)) {
            autoparam_update = 0;

            /* for NSM Mode */
            if (nsm_fd_update) {
                smart_read2 |= 0x40000;  // FD status : 18
                nsm_fd_update = 0;
                ALOGD("@@@ smartThread : NSM [FD STATUS UPDATE smart_read2 = 0x%08x]", smart_read2);
            }
            /* end NSM Mode */

            ALOGD("SmartThread: callback  auto param : %08x, %08x", smart_read1, smart_read2);
            mNotifyCb(CAMERA_MSG_AUTO_PARAMETERS_NOTIFY, smart_read1, smart_read2, mCallbackCookie);
        }
#endif

#if 0
        if (zoom_change && (mMsgEnabled & CAMERA_MSG_ZOOM_STEP_NOTIFY)) {
            zoom_change = 0;
            mLensStatus = mZoomStatus | (lens_checked << 3);
            lens_checked = 0;
            ALOGD("smartThread: callback  zoom level: %d  lens status: %x", mZoomCurrLevel, mLensStatus);
            mNotifyCb(CAMERA_MSG_ZOOM_STEP_NOTIFY, mLensStatus, mZoomCurrLevel, mCallbackCookie);
        }	
#endif

        if ( (mZoomStatusBak&0x06) != (mZoomStatus&0x6) ) {
            ALOGD("Smart LensStatus------- : %02x  ->  %02x (%d)   Level:%d", mZoomStatusBak, mZoomStatus, mZoomStatus&0x4, mZoomCurrLevel);
            mZoomStatusBak = mZoomStatus;
            if ( mZoomStatus&0x6 )
                zoom_change = 1;
        }

#ifdef ZOOM_CTRL_THREAD
#else
        if (!mPictureStart && zoom_change && (mMsgEnabled & CAMERA_MSG_ZOOM_STEP_NOTIFY)) {
            zoom_change = 0;
            mLensStatus = mZoomStatus | (lens_checked << 3);
            lens_checked = 0;
            ALOGD("SmartThread: callback  zoom level : %d, status : %02x, checked : %x02x", mZoomCurrLevel, mLensStatus, lens_checked);
            mNotifyCb(CAMERA_MSG_ZOOM_STEP_NOTIFY, mLensStatus, mZoomCurrLevel, mCallbackCookie);
        }
#endif

#if 0
        if(mSelfShotFDReading) {
            nativeGetParameters(CAM_CID_SMART_SELF_SHOT_FD_INFO1, &smart_self_shot_fd_info1);
            nativeGetParameters(CAM_CID_SMART_SELF_SHOT_FD_INFO2, &smart_self_shot_fd_info2);
            //ALOGD("mSelfShotFDReading : %d, smart_self_shot_fd_info1:%x, smart_self_shot_fd_info2 : %x", mSelfShotFDReading, smart_self_shot_fd_info1, smart_self_shot_fd_info2);
            if(mMsgEnabled & CAMERA_MSG_PREVIEW_METADATA) {
                camera_memory_t *fdCallbackHeap = NULL;
                fdCallbackHeap = mGetMemoryCb(-1, sizeof(camera_frame_metadata_t) * NUM_OF_DETECTED_FACES, 1, 0);

                if(smart_self_shot_fd_info1 != 0 || smart_self_shot_fd_info2 != 0) {
	                mFrameMetadata.number_of_faces = 1;
	                mFaces[0].rect[0] = (smart_self_shot_fd_info1 >> 16) & 0xFFFF;								// left
	                mFaces[0].rect[1] = (smart_self_shot_fd_info1 >> 0) & 0xFFFF;								// top
	                mFaces[0].rect[2] = mFaces[0].rect[0] + ((smart_self_shot_fd_info2 >> 16) & 0xFFFF);	// right
	                mFaces[0].rect[3] = mFaces[0].rect[1] + ((smart_self_shot_fd_info2 >> 0) & 0xFFFF);	// bottom
	                //ALOGD("rect[0]:%d, rect[1]:%d, rect[2]:%d, rect[3]:%d", mFaces[0].rect[0], mFaces[0].rect[1], mFaces[0].rect[2], mFaces[0].rect[3]);
                } else {
	                mFrameMetadata.number_of_faces = 0;
	                memset(mFaces, 0, sizeof(camera_face_t) * NUM_OF_DETECTED_FACES);
                }
                mFrameMetadata.faces = mFaces;
                mDataCb(CAMERA_MSG_PREVIEW_METADATA, fdCallbackHeap, 0, &mFrameMetadata, mCallbackCookie);
                fdCallbackHeap->release(fdCallbackHeap);
            }
        }
#else
        if(mSelfShotFDReading) {
            nativeGetParameters(CAM_CID_SMART_SELF_SHOT_FD_INFO1, &smart_self_shot_fd_info1);
            nativeGetParameters(CAM_CID_SMART_SELF_SHOT_FD_INFO2, &smart_self_shot_fd_info2);
            //ALOGD("mSelfShotFDReading : %d, smart_self_shot_fd_info1:%x, smart_self_shot_fd_info2 : %x", mSelfShotFDReading, smart_self_shot_fd_info1, smart_self_shot_fd_info2);
            if(mMsgEnabled & CAMERA_MSG_PREVIEW_METADATA) {
                camera_memory_t *fdCallbackHeap = NULL;
                fdCallbackHeap = mGetMemoryCb(-1, sizeof(camera_frame_metadata_t) * NUM_OF_DETECTED_FACES, 1, 0);

                if(smart_self_shot_fd_info1 != 0 || smart_self_shot_fd_info2 != 0) {
	                mFrameMetadata.number_of_faces = 1;
	                fd_info_left = (smart_self_shot_fd_info1 >> 16) & 0xFFFF;								// left
	                fd_info_top = (smart_self_shot_fd_info1 >> 0) & 0xFFFF;								// top
	                fd_info_right = fd_info_left + ((smart_self_shot_fd_info2 >> 16) & 0xFFFF);	// right
	                fd_info_bottom = fd_info_top + ((smart_self_shot_fd_info2 >> 0) & 0xFFFF);	// bottom
	                mFaces[0].rect[0] = ((int)((2000 * fd_info_left) / 1280)) - 1000;
	                mFaces[0].rect[1] = ((int)((2000 * fd_info_top) / 720)) - 1000;
	                mFaces[0].rect[2] = ((int)((2000 * fd_info_right) / 1280)) - 1000;
	                mFaces[0].rect[3] = ((int)((2000 * fd_info_bottom) / 720)) - 1000;
                } else {
	                mFrameMetadata.number_of_faces = 0;
	                memset(mFaces, 0, sizeof(camera_face_t) * NUM_OF_DETECTED_FACES);
                }
                //ALOGD("rect[0]:%d, rect[1]:%d, rect[2]:%d, rect[3]:%d", mFaces[0].rect[0], mFaces[0].rect[1], mFaces[0].rect[2], mFaces[0].rect[3]);
                mFrameMetadata.faces = mFaces;
                mDataCb(CAMERA_MSG_PREVIEW_METADATA, fdCallbackHeap, 0, &mFrameMetadata, mCallbackCookie);
                fdCallbackHeap->release(fdCallbackHeap);
            }
        }

#endif
    }

    if (!mPictureRunning && !mPictureStart && time_setting) {
		int set_real_time;
        time_t rawtime;
        struct tm *timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        set_real_time = ((timeinfo->tm_hour) << 8) | (timeinfo->tm_min);
        ALOGD("setting real time : %x", set_real_time);
        nativeSetParameters(CAM_CID_TIME_INFO, set_real_time);
        time_setting = 0;
        mLock.lock();
        if (mFirstStart == 1)
            mFirstStart = 2;
        mLock.unlock();
    }
#if 0
    if (mFirmwareResult) {
        ALOGD("Smart Firmware Update : %s", (mFirmwareResult == 1 ? "Success" : "Fail"));
        mNotifyCb(FIRMWARE_MSG_NOTIFY, mFirmwareResult, 0, mCallbackCookie);
        mFirmwareResult = 0;
    }
#endif
    usleep(50000);
//    ALOGV("smartThread X");

    return true;
}
#endif  //#ifdef SMART_THREAD

status_t ISecCameraHardware::autoFocus()
{
    ALOGV("autoFocus EX");
    /* signal autoFocusThread to run once */

    /* for NSM Mode*/
    if (mPASMMode == MODE_SMART_SUGGEST) {
        nativeSetParameters(CAM_CID_NSM_FD_WRITE, 0xFFFFFF);
        mParameters.set(SecCameraParameters::KEY_NSM_FD_INFO, 0xFFFFFF);
        ALOGD("@@@ autoFocus : NSM [FD INFO reset: 0x%08x]",
            mParameters.getInt(SecCameraParameters::KEY_NSM_FD_INFO));
    }
    /* end NSM Mode */

    mAutoFocusCondition.signal();
    return NO_ERROR;
}

bool ISecCameraHardware::autoFocusThread()
{
    /* block until we're told to start.  we don't want to use
     * a restartable thread and requestExitAndWait() in cancelAutoFocus()
     * because it would cause deadlock between our callbacks and the
     * caller of cancelAutoFocus() which both want to grab the same lock
     * in CameraServices layer.
     */
    mAutoFocusLock.lock();
    mAutoFocusCondition.wait(mAutoFocusLock);
    mAutoFocusLock.unlock();

    /* check early exit request */
    if (mAutoFocusExit)
        return false;

    ALOGV("autoFocusThread E");
    LOG_PERFORMANCE_START(1);

    mAutoFocusRunning = true;

    if (!IsAutoFocusSupported()) {
        ALOGV("autofocus not supported");
        mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
        goto out;
    }

    if (!autoFocusCheckAndWaitPreview()) {
        ALOGI("autoFocusThread: preview not started");
        mNotifyCb(CAMERA_MSG_FOCUS, false, 0, mCallbackCookie);
        goto out;
    }

    /* Start AF operations */
    if (!nativeSetAutoFocus()) {
        ALOGE("autoFocusThread X: error, nativeSetAutofocus");
        goto out;
    }

    if (!mAutoFocusRunning) {
        ALOGV("autoFocusThread X: AF is canceled");
        nativeSetParameters(CAM_CID_FOCUS_MODE, mFocusMode | V4L2_FOCUS_MODE_DEFAULT);
        mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
        goto out;
    }

    if (mMsgEnabled & CAMERA_MSG_FOCUS) {

		 int read_status = nativeGetPreAutoFocus();
		 ALOGD("read_status : %x", read_status);
		 int get_val = read_status & 0x1FF;

		 if (mSamsungApp) {
            if (get_val) {
                ALOGD("autoFocusThread X: PRE AF success %d", get_val);
                mNotifyCb(CAMERA_MSG_FOCUS, PRE_AF_SUCCESS, get_val, mCallbackCookie);
            } else {
                ALOGW("autoFocusThread X: PRE AF fail");
                mNotifyCb(CAMERA_MSG_FOCUS, PRE_AF_FAIL, 0, mCallbackCookie);
            }
		 }

		 if (read_status != AF_STATUS_FOCUSING) {
            read_status = nativeGetAutoFocus();
            if (read_status == 0) {
                ALOGE("autoFocusThread X: nativeGetAutofocus: error %d", read_status);
            }
        } else {
            ALOGE("AF Timeout!! AF is still in progress(%d)", read_status);
        }

		 if (get_val) {
            ALOGD("autoFocusThread X: AF success %d", get_val);
            if (mSamsungApp) {
                mNotifyCb(CAMERA_MSG_FOCUS, true, get_val, mCallbackCookie);
            } else {
                mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
            }

            /* for NSM Mode */
            mLock.lock();
            mbGetFDinfo = true;
            mLock.unlock();
            ALOGE("@@@ autoFocusThread  success : mbGetFDinfo = %d", mbGetFDinfo);
            /* end NSM Mode */

        } else {
            ALOGW("autoFocusThread X: AF fail");
            mNotifyCb(CAMERA_MSG_FOCUS, false, 0, mCallbackCookie);
        }
#if 0
		 if (!mRecordingRunning) {
            /* get Manual Focus index value */
            read_status = nativeGetManualFocus();
            int default_index = (read_status & 0xFF0000) >> 16;
            int near_index = (read_status & 0x00FFFF);

            ALOGD("autoFocusThread X: MF - read_status 0x%x default 0x%x, near limit 0x%x", read_status, default_index, near_index);
            //mNotifyCb(CAMERA_MSG_MANUAL_FOCUS_NOTIFY, default_index, near_index, mCallbackCookie);
        }
#endif
    }

out:
    mAutoFocusRunning = false;

    LOG_PERFORMANCE_END(1, "total");
    return true;
}

status_t ISecCameraHardware::cancelAutoFocus()
{
    ALOGV("cancelAutoFocus: autoFocusThread is %s",
        mAutoFocusRunning ? "running" : "not running");

    if (!IsAutoFocusSupported())
        return NO_ERROR;
    status_t err = NO_ERROR;
#if 0
    if (mAutoFocusRunning) {
        ALOGE("%s [%d]", __func__, mAutoFocusRunning);
        int i, waitUs = 3000, tryCount = (500 * 1000) / waitUs;

        err = nativeCancelAutoFocus();

        for (i = 1; i <= tryCount; i++) {
            if (mAutoFocusRunning)
                usleep(waitUs);
            else
                break;

            if (!(i % 40))
                ALOGD("AF, waiting to be cancelled\n");
        }

        if (CC_UNLIKELY(i > tryCount))
            ALOGI("cancelAutoFocus: cancel timeout");
    } else {
        ALOGV("%s [%d]", __func__, mAutoFocusRunning);
        err = nativeSetParameters(CAM_CID_FOCUS_MODE, mFocusMode | V4L2_FOCUS_MODE_DEFAULT);
    }
#else
    err = nativeCancelAutoFocus();
	if(!mAutoFocusRunning){
		 ALOGV("%s [%d]", __func__, mAutoFocusRunning);
        err = nativeSetParameters(CAM_CID_FOCUS_MODE, mFocusMode | V4L2_FOCUS_MODE_DEFAULT);
	}
	mAutoFocusRunning = false;
#endif

    if (CC_UNLIKELY(err != NO_ERROR)) {
        ALOGE("cancelAutoFocus: error, nativeCancelAutofocus");
        return UNKNOWN_ERROR;
    }

    mAutoFocusRunning = false;
    return NO_ERROR;
}

#if FRONT_ZSL
bool ISecCameraHardware::zslpictureThread()
{
    mZSLindex = nativeGetFullPreview();

    if (CC_UNLIKELY(mZSLindex < 0)) {
        ALOGE("zslpictureThread: error, nativeGetFullPreview");
        return true;
    }

    nativeReleaseFullPreviewFrame(mZSLindex);

    return true;
}
#endif

/* --1 */
status_t ISecCameraHardware::takePicture()
{
    ALOGD("takePicture E");

    int facedetVal = 0;
#if VENDOR_FEATURE
#if 1 /* WORKAROUND */
	mFLiteCaptureSize = mPictureSize;
    if (mPictureSize.width < 640 || mPictureSize.width < 480) {
        mFLiteCaptureSize.width = 640;
        mFLiteCaptureSize.height = 480;
    }

    mRawSize = mPictureSize;

    facedetVal = getFaceDetection();
    ALOGD("takePicture facedetVal:%d", facedetVal);
#endif
#endif

    if (mPreviewRunning == false) {
        ALOGW("WARN(%s): preview is not initialized", __func__);
        int retry = 10;
        while (retry > 0 || mPreviewRunning == false) {
            usleep(5000);
            retry--;
        }
    }

	mPictureStart = true;

	if (mCaptureMode != RUNNING_MODE_SINGLE || facedetVal == V4L2_FACE_DETECTION_BLINK) {
		if (mPreviewRunning) {
			ALOGW("takePicture: warning, preview is running");
			stopPreview();
		}
		nativeSetFastCapture(false);
	}
	mPictureStart = false;

    if (mDualCapture) {
        mPictureSize.width = REC_CAPTURE_WIDTH;
        mPictureSize.height = REC_CAPTURE_HEIGHT;
    }

    Mutex::Autolock lock(mLock);
    if (mPictureRunning) {
        ALOGE("takePicture: error, picture already running");
        return INVALID_OPERATION;
    }

#ifdef BURST_SHOT_SUPPORT
	if (mCaptureMode == RUNNING_MODE_BURST) {
		ALOGD("mTakeBurst is true: BurstPictureThread run.");
		if (mBurstPictureThread->run("burstPictureThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
			ALOGE("BurstPictureThread: error, Not starting take BurstPicture");
			return UNKNOWN_ERROR;
		}
	} else if (mCaptureMode == RUNNING_MODE_HDR || mCaptureMode == RUNNING_MODE_LOWLIGHT) {
		ALOGD("mTakeHDR is true: HDRPictureThread run.");
		if (mHDRPictureThread->run("HDRPictureThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
			ALOGE("HDRPictureThread: error, Not starting take HDR Picture");
			return UNKNOWN_ERROR;
		}
	} else if (mCaptureMode == RUNNING_MODE_AE_BRACKET) {
		ALOGD("mTakeAEB is true: AEBPictureThread run.");
		if (mAEBPictureThread->run("AEBPictureThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
			ALOGE("AEBPictureThread: error, Not starting take AEB Picture");
			return UNKNOWN_ERROR;
		}
	//} else if (mCaptureMode == RUNNING_MODE_BLINK) {
	} else if (facedetVal == V4L2_FACE_DETECTION_BLINK) {
		ALOGD("mTakeBlink is true: BlinkPictureThread run.");
		if (mBlinkPictureThread->run("BlinkPictureThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
			ALOGE("BlinkPictureThread: error, Not starting take Blink Picture");
			return UNKNOWN_ERROR;
		}
	} else if (mDualCapture) {
		ALOGD("mDualCapture is true: RecordingPictureThread run.");
		if (mRecordingPictureThread->run("RecordingPictureThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
			ALOGE("RecordingPictureThread: error, Not starting take Recording Picture");
			return UNKNOWN_ERROR;
		}
	} else {
		if (mPictureThread->run("pictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
			ALOGE("takePicture: error, Not starting take picture");
			return UNKNOWN_ERROR;
		}
	}
#else
	if (mPictureThread->run("pictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
		ALOGE("takePicture: error, Not starting take picture");
		return UNKNOWN_ERROR;
	}
#endif

    ALOGD("takePicture X");
    return NO_ERROR;
}

void ISecCameraHardware::Fimc_stream_true_part2(void)
{
	nativeSetParameters(CAM_CID_STREAM_PART2, 0);
}

/* --2 */
bool ISecCameraHardware::pictureThread()
#if VENDOR_FEATURE
{
	int err;
	ALOGD("pictureThread E");

	err = NO_ERROR;

	if ((mCaptureMode == RUNNING_MODE_SINGLE) && (mFactoryTestNum == 0)) {
		nativeSetFastCapture(true);

		/* Start fast capture */
		mCaptureStarted = true;
		err = nativeSetParameters(CAM_CID_SET_FAST_CAPTURE, 0);
		mCaptureStarted = false;

		if (mCancelCapture && mSamsungApp) {
			ALOGD("pictureThread mCancelCapture %d", mCancelCapture);
			mCancelCapture = false;
			return false;
		}

		if (err != NO_ERROR)
			ALOGE("%s: Fast capture command is failed.", __func__);
		else
			ALOGD("%s: Mode change command is issued for fast capture.", __func__);
	}

	if (mCaptureMode == RUNNING_MODE_SINGLE) {
		if (mPreviewRunning && !mFullPreviewRunning) {
			ALOGW("takePicture: warning, preview is running");
			stopPreview();
		}
		nativeSetFastCapture(false);
	}
	mPictureRunning = true;

	if (mFactoryTestNum != 0) {
		if ((mFactoryTestNum == 131) ||      // WIDE Resolution
				(mFactoryTestNum == 132) ||  // TELE Resolution
				(mFactoryTestNum == 133) ||  // Tilt
				(mFactoryTestNum == 136) ||  // dust
				(mFactoryTestNum == 137) ||  // lens shading
				(mFactoryTestNum == 138) ||  // noise
				(mFactoryTestNum == 165)) {  // RED image test
			ALOGD("Don't set fast_capture mFactoryTestNum = %d", mFactoryTestNum);
			nativeSetParameters(CAM_CID_MAIN_FORMAT, 0);
		} else {
			mCaptureStarted = true;
			err = nativeSetParameters(CAM_CID_SET_FAST_CAPTURE , 0);
			mCaptureStarted = false;
			if (err != NO_ERROR)
				ALOGE("%s: Fast capture command is failed.", __func__);
			else
				ALOGD("%s: Mode change command is issued for fast capture", __func__);
		}
	}

	if ( mCaptureMode == RUNNING_MODE_RAW ) {
         return pictureThread_RAW();
	}

	mPictureLock.lock();

    LOG_PERFORMANCE_START(1);
	ALOGD("pictureThread: single picture thread. call nativeStartPostview");
	if (!nativeStartPostview()) {
		ALOGE("pictureThread: error, nativeStartPostview");
		mPictureLock.unlock();
		goto out;
	} else {
		ALOGD("pictureThread: nativeStartPostview");
	}

	if (!nativeGetPostview(1)) {
		ALOGE("pictureThread: error, nativeGetPostview");
		mPictureLock.unlock();
		mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
		goto out;
	} else {
		ALOGD("pictureThread: nativeGetPostview");
	}

    if ((mFactoryTestNum == 131) ||  // WIDE Resolution
        (mFactoryTestNum == 132) ||  // TELE Resolution
        (mFactoryTestNum == 133) ||  // Tilt
        (mFactoryTestNum == 136) ||  // dust
        (mFactoryTestNum == 137) ||  // lens shading
        (mFactoryTestNum == 138) ||  // noise
        (mFactoryTestNum == 165)) // RED image test
    {
        if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
            ALOGD("Postview mPostviewFrameSize = %d", mPostviewFrameSize);
            mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
        }
        return Factory_pictureThread_Resolution();
    }
    nativeStopSnapshot();

	/*
	   JPEG capture
	*/
    LOG_PERFORMANCE_START(2);
    if (!nativeStartSnapshot()) {
        ALOGE("pictureThread: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }
    LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

#if 0
    if ((mMsgEnabled & CAMERA_MSG_SHUTTER) && (mSceneMode != SCENE_MODE_NIGHTSHOT)) {
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
        mPictureLock.lock();
    }
#endif

    LOG_PERFORMANCE_START(3);
	/* --4 */
    int postviewOffset;

	if (!nativeGetSnapshot(1, &postviewOffset)) {
		ALOGE("pictureThread: error, nativeGetSnapshot");
		mPictureLock.unlock();
		mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
		goto out;
	}
	LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

	mPictureLock.unlock();

	/* Display postview */
	LOG_PERFORMANCE_START(4);

	/* callbacks for capture */
	if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
		ALOGD("Postview mPostviewFrameSize = %d", mPostviewFrameSize);
		mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
	}
	if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL)) {
		ALOGD("JPEG COMPLRESSED");
		mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
	}
	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
		ALOGD("RAW mRawFrameSize = %d", mRawFrameSize);
		mDataCb(CAMERA_MSG_RAW_IMAGE, mPostviewHeap, 0, NULL, mCallbackCookie);
	}
	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
		ALOGD("RAW image notify");
		mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
	}

    LOG_PERFORMANCE_END(4, "callback functions");

#if DUMP_FILE
    dumpToFile(mJpegHeap->base(), mPictureFrameSize, "/data/capture01.jpg");
#endif

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();
    LOG_PERFORMANCE_END(1, "total");

    ALOGD("pictureThread X");
    return false;
}
#else
{
    if (mMovieMode) {
        doRecordingCapture();
    } else {
        if (mPreviewRunning && !mFullPreviewRunning) {
            ALOGW("takePicture: warning, preview is running");
            stopPreview();
        }

        doCameraCapture();
    }

    return false;
}
#endif

status_t ISecCameraHardware::doCameraCapture()
{
    ALOGD("doCameraCapture E");

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);

    LOG_PERFORMANCE_START(2);
    if (!nativeStartSnapshot()) {
        ALOGE("doCameraCapture: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }
    LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

    if ((mMsgEnabled & CAMERA_MSG_SHUTTER) && (mSceneMode != SCENE_MODE_NIGHTSHOT)) {
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
        mPictureLock.lock();
    }

    LOG_PERFORMANCE_START(3);
    int postviewOffset;
    if (!nativeGetSnapshot(1, &postviewOffset)) {
        ALOGE("doCameraCapture: error, nativeGetSnapshot");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }
    LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

    mPictureLock.unlock();

    if ((mMsgEnabled & CAMERA_MSG_SHUTTER) && (mSceneMode == SCENE_MODE_NIGHTSHOT))
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);

    /* Display postview */
    LOG_PERFORMANCE_START(4);

    /* callbacks for capture */
    if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) && (mRawHeap != NULL))
        mDataCb(CAMERA_MSG_RAW_IMAGE, mRawHeap, 0, NULL, mCallbackCookie);

    if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY)
        mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);

    if ((mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) && (mRawHeap != NULL))
        mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mRawHeap, 0, NULL, mCallbackCookie);

    if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL))
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);

    LOG_PERFORMANCE_END(4, "callback functions");

#if DUMP_FILE
    dumpToFile(mJpegHeap->base(), mPictureFrameSize, "/data/capture01.jpg");
#endif

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("doCameraCapture X");
    return NO_ERROR;
}

status_t ISecCameraHardware::doRecordingCapture()
{
    ALOGD("doRecordingCapture: E");

    if (!mRecordingRunning) {
        ALOGI("doRecordingCapture: nothing to do");
        return NO_ERROR;
    }

    mPictureLock.lock();

    bool ret = false;
    int index = mPostRecordIndex;
    if (index < 0) {
        ALOGW("WARN(%s):(%d)mPostRecordIndex(%d) invalid", __func__, __LINE__, mPostRecordIndex);
        index = 0;
    }

    ExynosBuffer rawFrame422Buf;

#if 0
    rawFrame422Buf.size.extS[0] = mFliteNode.buffer[index].size.extS[0];

    if (allocMem(mIonCameraClient, &rawFrame422Buf, 1 << 1) == false) {
        ALOGE("ERR(%s):(%d)allocMem(rawFrame422Buf) fail", __func__, __LINE__);
        mPictureLock.unlock();
        goto out;
    }

    ALOGD("DEBUG(%s): mFLiteSize(%d x %d), mFliteNode.buffer[index].size.extS[0](%d)", __FUNCTION__, mFLiteSize.width, mFLiteSize.height, mFliteNode.buffer[index].size.extS[0]);

    memcpy(rawFrame422Buf.virt.extP[0], mFliteNode.buffer[index].virt.extP[0], mFliteNode.buffer[index].size.extS[0]);

    ret = nativeGetRecordingJpeg(&rawFrame422Buf, mFLiteSize.width, mFLiteSize.height);
    if (CC_UNLIKELY(!ret)) {
        ALOGE("doRecordingCapture: error, nativeGetRecordingJpeg");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }
#else
    int dstW = mPreviewSize.width;
    int dstH = mPreviewSize.height;

    rawFrame422Buf.size.extS[0] = dstW * dstH * 2;

    if (allocMem(mIonCameraClient, &rawFrame422Buf, 1 << 1) == false) {
        ALOGE("ERR(%s):(%d)allocMem(rawFrame422Buf) fail", __FUNCTION__, __LINE__);
        mPictureLock.unlock();
        goto out;
    }

    /* csc start flite(s) -> rawFrame422Buf */
    if (nativeCSCRecordingCapture(&(mFliteNode.buffer[index]), &rawFrame422Buf) != NO_ERROR)
        ALOGE("ERR(%s):(%d)nativeCSCRecordingCapture() fail", __FUNCTION__, __LINE__);

    ret = nativeGetRecordingJpeg(&rawFrame422Buf, dstW, dstH);
    if (CC_UNLIKELY(!ret)) {
        ALOGE("doRecordingCapture: error, nativeGetRecordingJpeg");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }
#endif

    mPictureLock.unlock();

#if DUMP_FILE
    dumpToFile(mJpegHeap->base(), mPictureFrameSize, "/data/capture01.jpg");
#endif

    if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);

out:
    freeMem(&rawFrame422Buf);

    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    ALOGD("doRecordingCapture: X");
    return NO_ERROR;
}

bool ISecCameraHardware::BlinkPictureThread()
{
    int i, ncnt;
    int count = 1;
    int shutter_count = 3;
    int postviewOffset;
    int err;

    ALOGD("BlinkPictureThread E");

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);

    nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_START_CAPTURE);

#if 0
    for (i = shutter_count ; i ; i--) {
        ncnt = shutter_count- i + 1;
        ALOGD("BlinkPictureThread: Shutter_sound %d", ncnt);
        nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_FRAME_SYNC);
        if (mMsgEnabled & CAMERA_MSG_SHUTTER) {
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
            mPictureLock.lock();
        }
    }
#endif

    for (i = count ; i ; i--) {
        ncnt = count-i+1;
        ALOGD("BlinkPictureThread: StartPostview %d frame E", ncnt);
        if (!nativeStartPostview()) {
            ALOGE("BlinkPictureThread: error, nativeStartPostview");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("BlinkPictureThread: StartPostview %d frame X", ncnt);
        }

        if (!nativeGetPostview(ncnt)) {
            ALOGE("BlinkPictureThread: error, nativeGetPostview");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGE("BlinkPictureThread: nativeGetPostview");
        }

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            ALOGD("RAW mRawFrameSize = %d", mRawFrameSize);
            mDataCb(CAMERA_MSG_RAW_IMAGE, mPostviewHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
            ALOGD("JPEG mRawFrameSize = %d", mPostviewFrameSize);
            mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
        }
		nativeStopSnapshot();

        LOG_PERFORMANCE_START(2);
        ALOGD("BlinkPictureThread: StartSnapshot %d frame E", ncnt);
        if (!nativeStartSnapshot()) {
            ALOGE("BlinkPictureThread: error, nativeStartSnapshot");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("BlinkPictureThread: StartSnapshot %d frame X", ncnt);
        }
        LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

        LOG_PERFORMANCE_START(3);
        if (!nativeGetSnapshot(ncnt, &postviewOffset)) {
            ALOGE("BlinkPictureThread: error, nativeGetSnapshot");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGD("BlinkPictureThread: nativeGetSnapshot %d frame ", ncnt);
        }
        LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

        mPictureLock.unlock();

        LOG_PERFORMANCE_START(4);

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            mDataCb(CAMERA_MSG_RAW_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        }
        if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL)) {
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
        }

        LOG_PERFORMANCE_END(4, "callback functions");
		nativeStopSnapshot();
    }

out:
    //nativeSetParameters(CAM_CID_BURSTSHOT_PROC, V4L2_INT_STATE_BURST_STOP);
	nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("BlinkPictureThread X");
    return false;
}

#ifdef BURST_SHOT_SUPPORT
bool ISecCameraHardware::burstPictureThread_postview(int ncnt)
{
	ALOGD("BURSTSHOT: burstPictureThread_postview E. call nativeStartPostview");
    if (!nativeStartPostview()) {
        ALOGE("BURSTSHOT burstPictureThread_postview: error, nativeStartPostview");
        mPictureLock.unlock();
        return false;
    }

    ALOGD("BURSTSHOT-----004 : nativeGetPostview start");
    if (!nativeGetPostview(ncnt)) {
        ALOGE("BURSTSHOT Burst: error, nativeGetPostview");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        return false;
    }

    if (mFlagANWindowRegister) {
        bool ret = nativeFlushSurface(mPreviewWindowSize.width, mPreviewWindowSize.height,
				mPostviewFrameSize, 0, CAMERA_HEAP_POSTVIEW);
        if (CC_UNLIKELY(!ret)) {
            ALOGE("%s::flushSurface() fail", __func__);
            /* We wonder if it's possible to validate our surface, using the below checking code.
            * Fix it if you have a better idea. */
        }
    }

    if ((mBurstShot.getAttrib() & BURST_ATTRIB_NOTUSING_POSTVIEW) == 0) {
        if ((mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) &&
            (mPASMMode!= MODE_BEST_SHOT)){
            mPictureLock.unlock();
            ALOGD("BURSTSHOT  JPEG mRawFrameSize = %d", mRawFrameSize);
            mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
            mPictureLock.lock();
        }
    }

    return true;
}

bool ISecCameraHardware::burstPictureThread()
{
    int postviewOffset;
    int ncnt = 0;
	int max;
    status_t rc = NO_ERROR;
    bool bCaptureModeSet = false;

    ALOGD("BURSTSHOT burstPictureThread E mCaptureMode=%d", mCaptureMode);

    mLock.lock();
    mPictureRunning = true;
    mLock.unlock();

    if ( mBurstShot.isInit() == false ) {
        ALOGD("BURSTSHOT burstPictureThread not init");
        goto burst_out;
    }

    mPictureLock.lock();

    int w, h;
    mParameters.getPictureSize(&w, &h);
    mPictureSize.width = w;
    mPictureSize.height = h;

	max = mBurstShot.getCountMax();

	nativeSetParameters(CAM_CID_CAPTURE_CNT, max);

	ALOGD("BURSTSHOT-----start burstshot proc = %d x %d", w, h);
	nativeSetParameters(CAM_CID_BURSTSHOT_PROC, V4L2_INT_STATE_BURST_START);

	mFlagANWindowRegister = true;

	if (mBurstSoundCount == 0) {
		/* Thread is waiting only in case of sound interrupt is not issued. */
		mBurstShotLock.lock();
		ALOGD("BURSTSHOT----- burstPictureThread waiting for first sound interrupt...");
		mBurstShotCondition.wait(mBurstShotLock);
		mBurstShotLock.unlock();
	} else if ( mBurstSoundCount > 0) {
		/* If sound interrupt is issued before waiting, skip thread waiting. */
		ALOGD("BURSTSHOT----- sound interrupt is issued before waiting. No signal waiting.");
	}

	if (mBurstShotExit)
		return false;

	for (int i = 0; i < max; i++) {
        ncnt = i;
        ALOGD("BURSTSHOT-----001  BurstPictureThread H:%d T:%d", mBurstShot.getIndexHead(),  mBurstShot.getIndexTail());

		if (mBurstStopReq == CAMERA_BURST_STOP_REQ) {
			ALOGD("BURSTSHOT: send stop command. mBurstSoundMaxCount = %d", mBurstSoundMaxCount);
			nativeSetParameters(CAM_CID_BURSTSHOT_PROC, V4L2_INT_STATE_BURST_STOP_REQ);
		}

        if ((i == max - 1) && (mBurstStopReq == CAMERA_BURST_STOP_NONE)) {
            ALOGD("BURSTSHOT-----001-1 stop_req : mBurstStopReq: %d, max: %d, i: %d", mBurstStopReq, max, i);
            mBurstStopReq = CAMERA_BURST_STOP_REQ;
            nativeSetParameters(CAM_CID_BURSTSHOT_PROC, V4L2_INT_STATE_BURST_STOP_REQ);
        }

		ALOGD("BURSTSHOT: BurstPictureThread ncnt = %d, mBurstSoundMaxCount = %d", ncnt, mBurstSoundMaxCount);
		if (ncnt < mBurstSoundMaxCount) {
			if (mPASMMode != MODE_MAGIC) {
				ALOGD("BURSTSHOT-----002 Postview() : mBurstStopReq :%d", mBurstStopReq);
				if ( burstPictureThread_postview(ncnt) == false ) {
					mPictureLock.unlock();
					goto burst_out;
				}
			}

			nativeStopSnapshot();
			ALOGD("BURSTSHOT-----003 : nativeStartSnapshot(): %d", mBurstSoundCount );
			if (!nativeStartSnapshot()) {
				ALOGE("BURSTSHOT Burst pictureThread: error, nativeStartSnapshot");
				mPictureLock.unlock();
				goto burst_out;
			}

			ALOGD("BURSTSHOT-----004 : nativeGetSnapshot()");
			if (!nativeGetSnapshot(ncnt, &postviewOffset)) {
				ALOGE("BURSTSHOT Burst pictureThread: error, nativeGetSnapshot");
				mPictureLock.unlock();
				mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
				goto burst_out;
			}

			ALOGD("BURSTSHOT-----005 : mBurstShot.push() size=%d", mPictureFrameSize);
			if ( mBurstShot.push(ncnt) == false ) {
				ALOGE("BURSTSHOT mBurstShot slot FULL");
				mPictureLock.unlock();
				mBurstShot.DumpState();
				mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
				goto burst_out;
			}

			if (mFlagANWindowRegister && mPASMMode == MODE_MAGIC) {
				bool ret = nativeFlushSurfaceYUV420(mPreviewWindowSize.width, mPreviewWindowSize.height,
						mPostviewFrameSize, 0, CAMERA_HEAP_POSTVIEW);
				if (CC_UNLIKELY(!ret)) {
					ALOGE("%s::flushSurface() fail", __func__);
					/* We wonder if it's possible to validate our surface, using the below checking code.
					 * Fix it if you have a better idea. */
				}
				/* PostView Callback */
				if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME){
					mPictureLock.unlock();
					mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
					mPictureLock.lock();
				}
			}
			nativeStopSnapshot();

			ALOGD("BURSTSHOT-----006 ------------------ %d/%d, STATE=%d", ncnt, mBurstSoundMaxCount, mBurstStopReq );
			ALOGD("BURSTSHOT start burstWriteThread. mBurstWriteRunning = %d", mBurstWriteRunning);
			if ( mBurstWriteRunning == false ) {
				if (mBurstWriteThread->run("burstWriteThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
					mPictureLock.unlock();
					usleep(10*1000);
					mPictureLock.lock();
					if (mBurstWriteThread->run("burstWriteThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
						mPictureLock.unlock();
						ALOGE("burstWriteThread: Not starting burstWriteThread");
						goto burst_out;
					}
				}
			}
		} else {
			mBurstStopReq = CAMERA_BURST_STOP_END;
		}

		if (mBurstStopReq == CAMERA_BURST_STOP_END) {
			ALOGE("BURSTSHOT: Stop burst shot. dump and break burst sequence.");
			mBurstShot.DumpState();
			break;
		}

        int lenzoff_timer = 0;
        while (true) {
            if (mBurstPictureThread->exitRequested()) {
                mPictureLock.unlock();
                mNotifyCb(CAMERA_MSG_ERROR, 0, 0, mCallbackCookie);
                ALOGE("BURSTSHOT Burst exitRequested().");
                goto burst_out;
            }

            bool enable = mBurstShot.isLimit();
            if (enable)
                break;

            ALOGE("BURSTSHOT ION memory Full");
            mBurstShot.DumpState();
            usleep(100*1000);
        }
    }
    mPictureLock.unlock();

burst_out:
    ALOGE("BURSTSHOT==========V4L2_INT_STATE_BURST_STOP proc() : %d, STATE=%d",mBurstSoundMaxCount, mBurstStopReq );
    nativeSetParameters(CAM_CID_BURSTSHOT_PROC, V4L2_INT_STATE_BURST_STOP);

    mLock.lock();
    nativeStopSnapshot();
    mFlagANWindowRegister = false;
    mPictureRunning = false;
#ifdef USE_CONTEXTUAL_FILE_NAME
	mBurstShot.freeContextualFileName();
	mBurstShot.setContextualState(false);
#endif
    mLock.unlock();

    usleep(50*1000);

#if VENDOR_FEATURE
	if (mMsgEnabled & CAMERA_MSG_SHOT_END) {
		ALOGE("BURSTSHOT==========END, Notify(0x40000), mBurstSoundCount = %d", mBurstSoundCount);
		mNotifyCb(CAMERA_MSG_SHOT_END, mBurstSoundCount, mBurstSoundCount, mCallbackCookie);
	}
#endif
    return false;
}

bool ISecCameraHardware::burstWriteThread()
{
    const int PATH_SIZE = 256;
    camera_memory_t *stringHeap = NULL;
	int stringHeapFd = -1;
    camera_memory_t *dataHeap = NULL;
	int dataHeapFd = -1;

	burst_item   *pitem, item;

    if ( mBurstShot.isInit() == false ) {
        ALOGD("BURSTSHOT burstPictureThread not init");
        goto burst_write_out;
    }

    ALOGD("BURSTSHOT: burstWriteThread E---------------");
    mBurstWriteRunning = true;

    if (mCaptureMode != RUNNING_MODE_BURST) {
        ALOGD("Not BurstMode");
        goto burst_write_out;
    }

    while( !mBurstShot.isEmpty() ) {

        if (mBurstWriteThread->exitRequested() ) {
            mNotifyCb(CAMERA_MSG_ERROR, 0, 0, mCallbackCookie);
            ALOGE("BURSTSHOT Burst mDeleteBurst is set.");
            goto burst_write_out;
        }

        for(int i=0; i<10, mBurstShot.isEmpty();i++) {
            if ( mPictureRunning == false )
                goto burst_write_out;
            usleep(100*1000);
        }

        //save images
        if ( mBurstShot.pop(&item) == false ) {
            mBurstShot.DumpState();
            goto burst_write_out;
        }

        ALOGD("BURSTSHOT MEM: pop start -- item->virt = %p getTailItem[%d] size(%d)", item.virt, item.ix, item.size);

		if (mEnableStrCb) {
			stringHeap = mGetMemoryCb(-1, PATH_SIZE, 1, &stringHeapFd);
			if (!stringHeap || stringHeap->data == MAP_FAILED) {
				ALOGE("ERR(%s): heap creation fail string", __func__);
				mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
				goto burst_write_out;
			}

			mBurstShot.getFileName((char*)stringHeap->data, item.frame_num, item.group);
			if (!nativeSaveJpegPicture((char*)stringHeap->data, &item)) {
				ALOGE("BURSTSHOT Burst thread : error, nativeSaveJpegPicture");
				mBurstShot.free(&item);
				goto burst_write_out;
			}
		} else {
			dataHeap = mGetMemoryCb(-1, item.size, 1, &dataHeapFd);
			if (!dataHeap || dataHeap->data == MAP_FAILED) {
				ALOGE("ERR(%s): dataHeap creation fail", __func__);
				mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
				goto burst_write_out;
			}
			memcpy((uint8_t *)dataHeap->data, (uint8_t *)item.virt, item.size);
		}

#ifdef  BURST_SHOT_SUPPORT_TEST
        usleep(1000*1000);
#endif

		mBurstShot.free(&item);

#ifdef  BURST_SHOT_SUPPORT_TEST
        mBurstShot.DumpState();
#endif

		if (mEnableStrCb) {
			if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
				ALOGD("BURSTSHOT : CAMERA_MSG_COMPRESSED_IMAGE start - string callback");
				mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, stringHeap, 0, NULL, mCallbackCookie);
			}
			stringHeap->release(stringHeap);
			stringHeap = NULL;
			stringHeapFd = -1;
		} else {
			if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
				ALOGD("BURSTSHOT : CAMERA_MSG_COMPRESSED_IMAGE start - string callback");
				mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, dataHeap, 0, NULL, mCallbackCookie);
			}
			dataHeap->release(dataHeap);
			dataHeap = NULL;
			dataHeapFd = -1;
		}
        ALOGD("BURSTSHOT : CAMERA_MSG_COMPRESSED_IMAGE end");
    }

burst_write_out:
	ALOGD("BURSTSHOT: mBurstWriteRunning set to false");
    mBurstWriteRunning = false;
    mPictureLock.lock();
    if (mPictureRunning == false){
        ALOGD("BURSTSHOT ***** mPictureRunning=%d", mPictureRunning);
        mBurstShot.release();
        mBurstShot.DumpState();
    }
    mPictureLock.unlock();

    ALOGE("BURSTSHOT BURST SHOT: burstWriteThread X---------------");

    return false;
}
#endif

bool ISecCameraHardware::HDRPictureThread()
{
    int i, ncnt;
	int count = 3;
    int postviewOffset;
    int err;

    ALOGD("HDRPictureThread E");

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);

	nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_START_CAPTURE);

#if 0
    for ( i = count ; i ; i--) {
        ncnt = count-i+1;
        ALOGD("HDRPictureThread: AEB %d frame", ncnt);
        nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_FRAME_SYNC);
        if (mMsgEnabled & CAMERA_MSG_SHUTTER) {
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
            mPictureLock.lock();
        }
    }
#endif

    for ( i = count ; i ; i--) {

        if (i != count) {
            mPictureLock.lock();
        }

        if (!nativeStartYUVSnapshot()) {
	        ALOGE("HDRPictureThread: error, nativeStartYUVSnapshot");
	        mPictureLock.unlock();
	        goto out;
	    }
	    ALOGE("nativeGetYUVSnapshot: count[%d], i[%d]",count,i);

	    if (!nativeGetYUVSnapshot(count-i+1, &postviewOffset)) {
	        ALOGE("HDRPictureThread: error, nativeGetYUVSnapshot");
	        mPictureLock.unlock();
	        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
	        goto out;
	    } else {
	        ALOGE("HDRPictureThread: success, nativeGetYUVSnapshot");
	    }

		mPictureLock.unlock();

        if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
            ALOGD("YUV mHDRTotalFrameSize(mHDRFrameSize) = %d, %d frame", mHDRFrameSize, i);
            mPictureLock.unlock();
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mHDRHeap, 0, NULL, mCallbackCookie);
        }
        nativeStopSnapshot();
    }

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("HDRPictureThread X");
    return false;

}

bool ISecCameraHardware::AEBPictureThread()
{
    int i, ncnt;
	int count = 3;
    int postviewOffset;
    int err;

    ALOGD("AEBPictureThread E");

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);

	nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_START_CAPTURE);

    for ( i = count ; i ; i--) {
        ncnt = count-i+1;
        ALOGD("AEBPictureThread: AEB %d frame", ncnt);
        nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_FRAME_SYNC);
#if 0
        if (mMsgEnabled & CAMERA_MSG_SHUTTER) {
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
            mPictureLock.lock();
        }
#endif
    }

    for (i = count ; i ; i--) {
        ncnt = count-i+1;
        ALOGD("AEBPictureThread: StartPostview %d frame E", ncnt);
        if (!nativeStartPostview()) {
            ALOGE("AEBPictureThread: error, nativeStartPostview");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("AEBPictureThread: StartPostview %d frame X", ncnt);
        }

        if (!nativeGetPostview(ncnt)) {
            ALOGE("AEBPictureThread: error, nativeGetPostview");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGE("AEBPictureThread: nativeGetPostview");
        }

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            ALOGD("RAW mRawFrameSize = %d", mRawFrameSize);
            mDataCb(CAMERA_MSG_RAW_IMAGE, mPostviewHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
            ALOGD("JPEG mRawFrameSize = %d", mPostviewFrameSize);
            mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
        }
		nativeStopSnapshot();

        LOG_PERFORMANCE_START(2);
        ALOGD("AEBPictureThread: StartSnapshot %d frame E", ncnt);
        if (!nativeStartSnapshot()) {
            ALOGE("AEBPictureThread: error, nativeStartSnapshot");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("AEBPictureThread: StartSnapshot %d frame X", ncnt);
        }
        LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

        LOG_PERFORMANCE_START(3);
        if (!nativeGetSnapshot(ncnt, &postviewOffset)) {
            ALOGE("AEBPictureThread: error, nativeGetSnapshot");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGD("AEBPictureThread: nativeGetSnapshot %d frame ", ncnt);
        }
        LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

        mPictureLock.unlock();

        LOG_PERFORMANCE_START(4);

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            mDataCb(CAMERA_MSG_RAW_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
        }
        if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL)) {
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        }

        LOG_PERFORMANCE_END(4, "callback functions");
		nativeStopSnapshot();
    }

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("AEBPictureThread X");
    return false;

}

bool ISecCameraHardware::RecordingPictureThread()
{
    int i, ncnt;
	int count = mMultiFullCaptureNum;
    int postviewOffset;
    int err;

    ALOGD("RecordingPictureThread : count (%d)E", count);

	if (mCaptureMode == RUNNING_MODE_SINGLE) {
		if (mPreviewRunning && !mFullPreviewRunning) {
			ALOGW("RecordingPictureThread: warning, preview is running");
			stopPreview();
		}
	}

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);
	ALOGD("RecordingPictureThread : postview width(%d), height(%d)", 
		mOrgPreviewSize.width, mOrgPreviewSize.height);
    nativeSetParameters(CAM_CID_SET_POSTVIEW_SIZE,
        (int)(mOrgPreviewSize.width<<16|mOrgPreviewSize.height));
	nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_START_CAPTURE);

    for (i = count ; i ; i--) {
        ncnt = count-i+1;
        ALOGD("RecordingPictureThread: StartPostview %d frame E", ncnt);
        if (!nativeStartPostview()) {
            ALOGE("RecordingPictureThread: error, nativeStartPostview");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("RecordingPictureThread: StartPostview %d frame X", ncnt);
        }

        if (!nativeGetPostview(ncnt)) {
            ALOGE("RecordingPictureThread: error, nativeGetPostview");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGE("RecordingPictureThread: nativeGetPostview");
        }

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            ALOGD("RAW mRawFrameSize = %d", mRawFrameSize);
            mDataCb(CAMERA_MSG_RAW_IMAGE, mPostviewHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
            ALOGD("JPEG mRawFrameSize = %d", mPostviewFrameSize);
            mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
        }

        LOG_PERFORMANCE_START(2);
        ALOGD("RecordingPictureThread: StartSnapshot %d frame E", ncnt);
        if (!nativeStartSnapshot()) {
            ALOGE("RecordingPictureThread: error, nativeStartSnapshot");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("RecordingPictureThread: StartSnapshot %d frame X", ncnt);
        }
        LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

        LOG_PERFORMANCE_START(3);
        if (!nativeGetSnapshot(ncnt, &postviewOffset)) {
            ALOGE("RecordingPictureThread: error, nativeGetSnapshot");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGD("RecordingPictureThread: nativeGetSnapshot %d frame ", ncnt);
        }
        LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

        mPictureLock.unlock();

        LOG_PERFORMANCE_START(4);

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            mDataCb(CAMERA_MSG_RAW_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
        }
        if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL)) {
            ALOGD("RecordingPictureThread: CAMERA_MSG_COMPRESSED_IMAGE (%d) ", ncnt);
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
			
            // This delay is added for waiting until saving jpeg file is completed.
            // if it take short time to save jpeg file, this delay can be deleted.  
            usleep(300*1000);
        }

        LOG_PERFORMANCE_END(4, "callback functions");
    }

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    if (mDualCapture) {
        int cap_sizes_width;
        int cap_sizes_height;
			
        mDualCapture = false;
        mParameters.getPictureSize(&cap_sizes_width, &cap_sizes_height);
        mPictureSize.width = cap_sizes_width;
        mPictureSize.height = cap_sizes_height;
    }	
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("RecordingPictureThread X");
    return false;

}

bool ISecCameraHardware::dumpPictureThread()
{
	int err;
	mPictureRunning = true;

	if (mPreviewRunning && !mFullPreviewRunning) {
		ALOGW("takePicture: warning, preview is running");
		stopPreview();
	}

	/* Start fast capture */
	mCaptureStarted = true;
	err = nativeSetParameters(CAM_CID_SET_FAST_CAPTURE, 0);
	mCaptureStarted = false;
	if (mCancelCapture && mSamsungApp) {
		ALOGD("pictureThread mCancelCapture %d", mCancelCapture);
		mCancelCapture = false;
		return false;
	}
	if (err != NO_ERROR) {
		ALOGE("%s: Fast capture command is failed.", __func__);
	} else {
		ALOGD("%s: Mode change command is issued for fast capture.", __func__);
	}

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);
	LOG_PERFORMANCE_START(2);
    if (!nativeStartSnapshot()) {
        ALOGE("pictureThread: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }
	LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

    LOG_PERFORMANCE_START(3);
	/* --4 */
    int postviewOffset;

	if (!nativeGetSnapshot(1, &postviewOffset)) {
		ALOGE("pictureThread: error, nativeGetSnapshot");
		mPictureLock.unlock();
		mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
		goto out;
	}
	LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

	mPictureLock.unlock();

	/* Display postview */
	LOG_PERFORMANCE_START(4);

	/* callbacks for capture */
	if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
		ALOGD("Postview mPostviewFrameSize = %d", mPostviewFrameSize);
		mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
	}
	if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL)) {
		ALOGD("JPEG COMPLRESSED");
		mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
	}
	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
		ALOGD("RAW mRawFrameSize = %d", mRawFrameSize);
		mDataCb(CAMERA_MSG_RAW_IMAGE, mPostviewHeap, 0, NULL, mCallbackCookie);
	}
	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
		ALOGD("RAW image notify");
		mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
	}

    LOG_PERFORMANCE_END(4, "callback functions");

#if DUMP_FILE
    dumpToFile(mJpegHeap->base(), mPictureFrameSize, "/data/capture01.jpg");
#endif

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();
    LOG_PERFORMANCE_END(1, "total");

    ALOGD("pictureThread X");
    return false;
}

status_t ISecCameraHardware::pictureThread_RAW()
{
    int postviewOffset;
	int jpegSize;
	uint8_t *RAWsrc;
    ALOGE("pictureThread_RAW E");

    mPictureLock.lock();

	nativeSetParameters(CAM_CID_MAIN_FORMAT, 5);

    if (!nativeStartSnapshot()) {
        ALOGE("pictureThread: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }

#if 0
    if ((mMsgEnabled & CAMERA_MSG_SHUTTER) && (mSceneMode != SCENE_MODE_NIGHTSHOT)) {
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
        mPictureLock.lock();
    }
#endif

    if (!nativeGetSnapshot(0, &postviewOffset)) {	//if (!nativeGetSnapshot(1, &postviewOffset)) {
        ALOGE("pictureThread: error, nativeGetSnapshot");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }

    RAWsrc = (uint8_t *)mPictureBufDummy[0].virt.extP[0];

    if(mJpegHeap != NULL)
        memcpy((uint8_t *)mJpegHeap->data, RAWsrc, mPictureFrameSize );

    if ((mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) && (mJpegHeap != NULL)) {	//CAMERA_MSG_COMPRESSED_IMAGE
        ALOGD("RAW COMPLRESSED");
        mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mJpegHeap, 0, NULL, mCallbackCookie);	//CAMERA_MSG_COMPRESSED_IMAGE
    }

    nativeSetParameters(CAM_CID_MAIN_FORMAT, 1);

    if (!nativeStartSnapshot()) {
        ALOGE("pictureThread: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }

    if (!nativeGetSnapshot(1, &postviewOffset)) {
        ALOGE("pictureThread_RAW : error, nativeGetMultiSnapshot");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }

    if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
		ALOGD("JPEG COMPRESSED");
        mPictureLock.unlock();
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        mPictureLock.lock();
    }

	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
		ALOGD("RAW image notify");
		mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
	}

    mPictureLock.unlock();

    out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    ALOGD("pictureThread_RAW X");
    return false;
}

status_t ISecCameraHardware::Factory_pictureThread_Resolution()
{
    int postviewOffset;
    ALOGE("Factory_pictureThread_Resolution E");

    if (!nativeStartYUVSnapshot()) {
        ALOGE("pictureThread: error, nativeStartYUVSnapshot");
        mPictureLock.unlock();
        goto out;
    }

#if 0
    if (mMsgEnabled & CAMERA_MSG_SHUTTER) {
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
        mPictureLock.lock();
    }
#endif

    if (!nativeGetOneYUVSnapshot()) {
        ALOGE("pictureThread: error, nativeGetOneYUVSnapshot");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    } else {
        ALOGE("pictureThread: success, nativeGetOneYUVSnapshot");
    }

    mParameters.set("factory-resol-exif-fno", mFactoryExifFnum);
    ALOGD("pictureThread: mFactoryExifFnum [%d]", mFactoryExifFnum);
    mParameters.set("factory-resol-exif-iso", mFactoryExifISO);
    ALOGD("pictureThread: mFactoryExifISO [%d]", mFactoryExifISO);
    mParameters.set("factory-resol-exif-speed", mFactoryExifShutterSpeed);
    ALOGD("pictureThread: mFactoryExifShutterSpeed [%d]", mFactoryExifShutterSpeed);

    nativeDumpYUV();

#if 1
    if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
        ALOGD("YUV mRawFrameSize = %d", mRawFrameSize);
        mPictureLock.unlock();
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mYUVHeap, 0, NULL, mCallbackCookie);
        goto out;
    }
#endif

    mPictureLock.unlock();

    out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    ALOGD("Factory_pictureThread_Resolution X");
    return false;
}

status_t ISecCameraHardware::cancelPicture()
{
    mPictureThread->requestExitAndWait();

    ALOGD("cancelPicture EX");
    return NO_ERROR;
}

status_t ISecCameraHardware::sendCommand(int32_t command, int32_t arg1, int32_t arg2)
{
    ALOGV("sendCommand E: command %d, arg1 %d, arg2 %d", command, arg1, arg2);
    int max;

    switch(command) {
    case CAMERA_CMD_DISABLE_POSTVIEW:
        mDisablePostview = arg1;
        break;

    case CAMERA_CMD_SET_SAMSUNG_APP:
        mSamsungApp = true;
        break;

    case CAMERA_CMD_START_FACE_DETECTION:
        if (arg1 == CAMERA_FACE_DETECTION_HW) {
            ALOGI("Not supported Face Detection HW");
            mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, "0");
            //return BAD_VALUE; /* return BAD_VALUE if not supported. */	/* delete for Self-shot mode */
        } else if (arg1 == CAMERA_FACE_DETECTION_SW) {
            if (arg2 > 0) {
                ALOGI("Support Face Detection SW");
                char Result[12];
                CLEAR(Result);
                snprintf(Result, sizeof(Result), "%d", arg2);
                mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, Result);
            } else {
                ALOGI("Not supported Face Detection SW");
                mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, "0");
                return BAD_VALUE;
            }
        } else {
            ALOGI("Not supported Face Detection");
        }
        break;

    case CAMERA_CMD_STOP_FACE_DETECTION:
        if (arg1 == CAMERA_FACE_DETECTION_SW) {
            mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, "0");
        } else if (arg1 == CAMERA_FACE_DETECTION_HW) {
            /* TODO: If implement the CAMERA_FACE_DETECTION_HW, remove the "return UNKNOWN_ERROR" */
            ALOGI("Not supported Face Detection HW");
            mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, "0");
            //return UNKNOWN_ERROR;	/* delete for Self-shot mode */
        }
        break;

    case CAMERA_CMD_SET_TOUCH_AF_POSITION:
#if 0
        if ((mSensorSize.width == 0 && mSensorSize.height == 0) ||
            ((mSensorSize.width*10/mSensorSize.height) != (mPreviewSize.width*10/mPreviewSize.height))) {
            int SensorSize = 0;
            nativeGetParameters(CAM_CID_SENSOR_OUTPUT_SIZE, &SensorSize);
            mSensorSize.height = SensorSize&0xFFFF;
            mSensorSize.width = (SensorSize>>16)&0xFFFF;
        }
        ALOGD("CAMERA_CMD_SET_TOUCH_AF_POSITION  ~~~~~~~~~ ");
        /* X */
        if (mPreviewSize.width != mSensorSize.width) {
            int diff = 0, x_position = 0;
            diff = mSensorSize.width*1000/mPreviewSize.width;
            x_position = (arg1*diff)/1000;
            nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSX, x_position);
            ALOGD("TouchAF X position : %d -> %d", arg1, x_position);
        } else {
            nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSX, arg1);
        }
        /* Y */
        if (mPreviewSize.width != mSensorSize.width) {
            int diff = 0, y_position = 0;
            diff = mSensorSize.height*1000/mPreviewSize.height;
            y_position = (arg2*diff)/1000;
            nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSY, y_position);
            ALOGD("TouchAF y position : %d -> %d", arg2, y_position);
        } else {
            nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSY, arg2);
        }
#else
        ALOGD("CAMERA_CMD_SET_TOUCH_AF_POSITION  X:%d, Y:%d", arg1, arg2);
        nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSX, arg1);
        nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSY, arg2);

#endif
        break;

    case HAL_OBJECT_TRACKING_STARTSTOP:
        if (mCameraId == CAMERA_FACING_BACK) {
            nativeSetParameters(CAM_CID_OBJ_TRACKING, arg1);
        }
        break;

    case CAMERA_CMD_START_STOP_TOUCH_AF:
        if (!mPreviewRunning && arg1) {
            ALOGW("Preview is not started before Touch AF");
            return NO_INIT;
        }
        ALOGD("CAMERA_CMD_START_STOP_TOUCH_AF  ~~~~~~~~~ arg1 == %d", arg1);
        nativeSetParameters(CAM_CID_SET_TOUCH_AF, arg1);
        break;

    case CAMERA_CMD_SET_FLIP:
        nativeSetParameters(CAM_CID_HFLIP, arg1, 0);
        nativeSetParameters(CAM_CID_HFLIP, arg1, 1);
        mMirror = arg1;
        break;

#ifdef BURST_SHOT_SUPPORT
	case CAMERA_CMD_RUN_BURST_TAKE:

        // mBurstShot.init(); : burst shot continue ...
        mBurstShot.setAttrib(arg1);
		if (arg1 & BURST_ATTRIB_STRING_CB_MODE)
			mEnableStrCb = true;
		else if (arg1 & BURST_ATTRIB_IMAGE_CB_MODE)
			mEnableStrCb = false;
		else
			mEnableStrCb = true;

        if ( mPictureRunning == false ) {
            if ( arg2 > 0 )
                mBurstShot.setCountMax(arg2);
            char *path = getBurstSavePath();
            if ( path != NULL )
                mBurstShot.setPath(path);

			mBurstSoundMaxCount = mBurstShot.getCountMax();

            mBurstSoundCount = 0;
            mBurstStopReq = CAMERA_BURST_STOP_NONE;
        }
        ALOGD("BURSTSHOT: CAMERA_CMD_RUN_BURST_TAKE--1, arg1 = %d, arg2 = %d, mBurstSoundMaxCount = %d",
				arg1, arg2, mBurstSoundMaxCount);
		break;
    case CAMERA_CMD_STOP_BURST_TAKE: //1572
        ALOGD("BURSTSHOT: CAMERA_CMD_STOP_BURST_TAKE--- %d, %d, %d/%d",
				mPictureRunning, mBurstWriteRunning, mBurstSoundCount, mBurstSoundMaxCount);

        if (mBurstStopReq == CAMERA_BURST_STOP_NONE) {
			mBurstSoundMaxCount = mBurstSoundCount;

            mBurstStopReq = CAMERA_BURST_STOP_REQ;
        }

		if (mPictureRunning == false && mBurstWriteRunning == false) {
            mBurstShot.release();
        }
        break;
#endif

    case CAMERA_CMD_SMART_AUTO_S1_RELEASE:
        if (mCameraId == CAMERA_FACING_BACK) {
            ALOGD("CAMERA_CMD_SMART_AUTO_S1_RELEASE  ~~~~~~~~~ ");
            nativeSetParameters(CAM_CID_S1_PUSH, 0);
        }
        break;

    case HAL_START_CONTINUOUS_AF:
        ALOGD("HAL_START_CONTINUOUS_AF  ~~~~~~~~~ ");
        nativeSetParameters(CAM_CID_CONTINUOUS_AF, 1);
        break;

    case HAL_STOP_CONTINUOUS_AF:
        ALOGD("HAL_STOP_CONTINUOUS_AF  ~~~~~~~~~ ");
        nativeSetParameters(CAM_CID_CONTINUOUS_AF, 0);
        break;

    case CAMERA_CMD_SETZOOM:
        if (mCameraId == CAMERA_FACING_BACK) {
            max = mParameters.getInt(CameraParameters::KEY_MAX_ZOOM);
            if ( arg1 < 0 || arg1 > max ) {  // ZOOM 0~15
                ALOGE("CAMERA_CMD_SETZOOM failed not support level=%d ", arg1);
                break;
            }

            ALOGD("CAMERA_CMD_SETZOOM  req=%d, currentlevel=%d/%d, zoomstate=%d", arg1, mZoomCurrLevel, max, mZoomStatus);
            if (arg1 != mZoomCurrLevel && mZoomStatus == 0 ) {
                int err = nativeSetParameters(CAM_CID_ZOOM, arg1);
                if (err < 0) {
                    ALOGE("%s: CAMERA_CMD_SETZOOM failed", __func__);
                    return err;
                }
            }
        }
	break;

	case CAMERA_CMD_GET_WB_CUSTOM_VALUE:
	{
		int x, y;
		char str[32];

		x = nativegetWBcustomX();
		y = nativegetWBcustomY();

		mWBcustomValue.x = x;
		mWBcustomValue.y = y;

		sprintf(str, "%d,%d", x, y);
		ALOGD("CAMERA_CMD_GET_WB_CUSTOM_VALUE %s", str);
		mParameters.set("wb-custom", str);
	}
	break;

	case HAL_AF_LAMP_CONTROL:
		nativeSetParameters(CAM_CID_TIMER_LED, (arg1 ? mTimerSet : V4L2_TIMER_LED_OFF));
	break;
	

#ifndef FCJUNG
    case CAMERA_CMD_ISP_DEBUG: //9000
        ALOGD("ISPD CAMERA_CMD_ISP_DEBUG==============");
        ISP_DBG_input();
        break;

    case CAMERA_CMD_ISP_LOG: //9001
        ALOGD("ISPD CAMERA_CMD_ISP_LOG==============");
        ISP_DBG_logwrite();
        break;

    case CAMERA_CMD_FACTORY_SYS_MODE://9005
        switch (arg2) {
        case FACTORY_SYSMODE_MONITOR:
            ALOGD("CAMERA_CMD_FACTORY_SYS_MODE==============MONITOR");
            nativeSetParameters(CAM_CID_FACTORY_CAM_SYS_MODE, arg2);
            startPreview();
            break;
        case FACTORY_SYSMODE_CAPTURE:
        case FACTORY_SYSMODE_PARAM:
            ALOGD("CAMERA_CMD_FACTORY_SYS_MODE==============%s",
                arg2 == FACTORY_SYSMODE_CAPTURE ? "FACTORY_SYSMODE_CAPTURE" : "FACTORY_SYSMODE_PARAM");
            mFactorySysMode = 1;    /* don't destroy surface */
            stopPreview();
            mFactorySysMode = 0;
            nativeSetParameters(CAM_CID_FACTORY_CAM_SYS_MODE, arg2);
            break;
        }
		break;

    case CAMERA_CMD_FACTORY_SEND_VALUE:
        nativeSetParameters(CAM_CID_FACTORY_SEND_SETTING, arg1);
        nativeSetParameters(CAM_CID_FACTORY_SEND_VALUE, arg2);
        break;

    case CAMERA_CMD_FACTORY_SEND_WORD_VALUE:
        nativeSetParameters(CAM_CID_FACTORY_SEND_SETTING, arg1);
        nativeSetParameters(CAM_CID_FACTORY_SEND_WORD_VALUE, arg2);
        break;

    case CAMERA_CMD_FACTORY_SEND_LONG_VALUE:
        nativeSetParameters(CAM_CID_FACTORY_SEND_SETTING, arg1);
        nativeSetParameters(CAM_CID_FACTORY_SEND_LONG_VALUE, arg2);
        break;
#endif /* FCJUNG */

    case CAMERA_CMD_SET_AFAE_DIVISION_AE_POSITION:
    {
#if 0
        int diff = 0, x_position = 0, y_position = 0;
        int ae_window_x_position = 0, ae_window_y_position = 0;
        ALOGD("CAMERA_CMD_SET_AFAE_DIVISION_AE_POSITION  X(arg1):%d, Y(arg2):%d", arg1, arg2);

	 /* 1. correcting the coordinate */
	 /* 1-1. X-coordinate */
	 if (mPreviewSize.width != mSensorSize.width) {
            diff = mSensorSize.width*1000/mPreviewSize.width;
            x_position = (arg1*diff)/1000;
        } else {
            x_position = arg1;
        }

	 /* 1-1. Y-coordinate */
	 if (mPreviewSize.width != mSensorSize.width) {
            diff = mSensorSize.height*1000/mPreviewSize.height;
            y_position = (arg2*diff)/1000;
        } else {
            y_position = arg2;
        }

	 /* 2. calculating the AE window size */
	 ae_window_x_position = x_position/(mPreviewSize.width/NUM_OF_AE_WINDOW_ROW);
	 ae_window_y_position = y_position/(mPreviewSize.height/NUM_OF_AE_WINDOW_COLUMN);
#else
        int x_position = 0, y_position = 0;
        int ae_window_x_position = 0, ae_window_y_position = 0;
        x_position = arg1;
        y_position = arg2;
	 /* 2. calculating the AE window size */
	 ae_window_x_position = x_position/(1280/NUM_OF_AE_WINDOW_ROW);
	 ae_window_y_position = y_position/(720/NUM_OF_AE_WINDOW_COLUMN);
#endif
        if(ae_window_x_position > NUM_OF_AE_WINDOW_ROW) {
            ae_window_x_position = NUM_OF_AE_WINDOW_ROW - 1;
        } else if(ae_window_y_position > NUM_OF_AE_WINDOW_COLUMN) {
            ae_window_y_position = NUM_OF_AE_WINDOW_COLUMN - 1;
        }

	 ALOGD("CAMERA_CMD_SET_AFAE_DIVISION_AE_POSITION  x_position:%d, y_position:%d", x_position, y_position);
	 ALOGD("CAMERA_CMD_SET_AFAE_DIVISION_AE_POSITION  ae_window_x_position:%d, ae_window_y_position:%d", ae_window_x_position, ae_window_y_position);
        nativeSetParameters(CAM_CID_CAMERA_AE_POSITION_X, ae_window_x_position);
        nativeSetParameters(CAM_CID_CAMERA_AE_POSITION_Y, ae_window_y_position);
        break;
    }

    case CAMERA_CMD_SET_AFAE_DIVISION_WINDOW_SIZE:
        ALOGD("CAMERA_CMD_SET_AFAE_DIVISION_WINDOW_SIZE  X(arg1):%d, Y(arg2):%d", arg1, arg2);
        nativeSetParameters(CAM_CID_CAMERA_AE_WINDOW_SIZE_WIDTH, SIZE_OF_AE_WINDOW_ROW);
        nativeSetParameters(CAM_CID_CAMERA_AE_WINDOW_SIZE_HEIGHT, SIZE_OF_AE_WINDOW_COLUMN);
        break;

    case CAMERA_CMD_SET_AFAE_DIVISION_PREVIEW_TOUCH_CTRL:
        ALOGD("CAMERA_CMD_SET_AFAE_DIVISION_PREVIEW_TOUCH_CTRL  X(arg1):%d, Y(arg2):%d", arg1, arg2);
        nativeSetParameters(CAM_CID_CAMERA_AE_PREVIEW_TOUCH_CTRL, arg1);
	break;

    case RECORDING_TAKE_PICTURE:
        if (mDualCapture) {
            mMultiFullCaptureNum++;
            ALOGD("Recording Capture cnt [%d]", mMultiFullCaptureNum);
        } else {
            mMultiFullCaptureNum = 1;
        }
        mDualCapture = true;
        if (mMultiFullCaptureNum > 6) {
            ALOGD("RECORDING_TAKE_PICTURE: error %d", mMultiFullCaptureNum);
            mMultiFullCaptureNum = 6;
            return NO_ERROR;
        }
        if (!nativeStartDualCapture(mMultiFullCaptureNum)) {
            ALOGD("nativeStartDualCapture: error");
            mNotifyCb(CAMERA_MSG_ERROR, -9, 0, mCallbackCookie);
            return NO_ERROR;
        }
        break;	

    default:
        ALOGE("no matching case");
        break;
    }

    return NO_ERROR;
}

void ISecCameraHardware::release()
{
	if (mPreviewThread != NULL) {
		mPreviewThread->requestExitAndWait();
		mPreviewThread.clear();
	}

	if (mPreviewZoomThread != NULL) {
		mPreviewZoomThread->requestExitAndWait();
		mPreviewZoomThread.clear();
	}

#if FRONT_ZSL
	if (mZSLPictureThread != NULL) {
		mZSLPictureThread->requestExitAndWait();
		mZSLPictureThread.clear();
	}
#endif

#if IS_FW_DEBUG_THREAD
	if (mCameraId == CAMERA_FACING_FRONT || mCameraId == FD_SERVICE_CAMERA_ID) {
		mStopDebugging = true;
		mDebugCondition.signal();
		if (mDebugThread != NULL) {
			mDebugThread->requestExitAndWait();
			mDebugThread.clear();
		}
	}
#endif

	if (mRecordingThread != NULL) {
		mRecordingThread->requestExitAndWait();
		mRecordingThread.clear();
	}

	if (mPostRecordThread != NULL) {
		mPostRecordThread->requestExit();
		mPostRecordExit = true;
		mPostRecordCondition.signal();
		mPostRecordThread->requestExitAndWait();
		mPostRecordThread.clear();
	}

	if (mAutoFocusThread != NULL) {
		/* this thread is normally already in it's threadLoop but blocked
		 * on the condition variable.  signal it so it wakes up and can exit.
		 */
		mAutoFocusThread->requestExit();
		mAutoFocusExit = true;
		mAutoFocusCondition.signal();
		mAutoFocusThread->requestExitAndWait();
		mAutoFocusThread.clear();
	}

#ifdef SMART_THREAD
    if (mSmartThread != NULL) {
        mSmartThread->requestExitAndWait();
        mSmartThread.clear();
    }
#endif

#ifdef ZOOM_CTRL_THREAD
    if (mZoomCtrlThread != NULL) {
        mZoomCtrlThread->requestExitAndWait();
        mZoomCtrlThread.clear();
    }
#endif

    if (mPictureThread != NULL) {
        mPictureThread->requestExitAndWait();
        mPictureThread.clear();
    }

	if (mPreviewHeap) {
		mPreviewHeap->release(mPreviewHeap);
		mPreviewHeap = 0;
		mPreviewHeapFd = -1;
	}

	if (mRecordingHeap) {
		mRecordingHeap->release(mRecordingHeap);
		mRecordingHeap = 0;
	}

	if (mRawHeap != NULL) {
		mRawHeap->release(mRawHeap);
		mRawHeap = 0;
	}

	if (mJpegHeap) {
		mJpegHeap->release(mJpegHeap);
		mJpegHeap = 0;
	}

	if (mHDRHeap) {
		mHDRHeap->release(mHDRHeap);
		mHDRHeap = 0;
	}

	if (mPostviewHeap) {
		mPostviewHeap->release(mPostviewHeap);
		mPostviewHeap = 0;
	}

#ifdef BURST_SHOT_SUPPORT
    if (mBurstPictureThread != NULL) {
		mBurstShotExit = true;
		mBurstShotCondition.signal();
        mBurstPictureThread->requestExitAndWait();
        mBurstPictureThread.clear();
    }
    if (mBurstWriteThread != NULL) {
        mBurstWriteThread->requestExitAndWait();
        mBurstWriteThread.clear();
    }
#endif

    if (mHDRPictureThread != NULL) {
        mHDRPictureThread->requestExitAndWait();
        mHDRPictureThread.clear();
    }
    if (mAEBPictureThread != NULL) {
        mAEBPictureThread->requestExitAndWait();
        mAEBPictureThread.clear();
    }
    if (mBlinkPictureThread != NULL) {
        mBlinkPictureThread->requestExitAndWait();
        mBlinkPictureThread.clear();
    }
    if (mRecordingPictureThread != NULL) {
        mRecordingPictureThread->requestExitAndWait();
        mRecordingPictureThread.clear();
    }
	if (mDumpPictureThread != NULL) {
        mDumpPictureThread->requestExitAndWait();
        mDumpPictureThread.clear();
    }

	if (mShutterThread != NULL) {
        mShutterThread->requestExitAndWait();
        mShutterThread.clear();
    }

	nativeDestroySurface();
}

status_t ISecCameraHardware::dump(int fd) const
{
    return NO_ERROR;
}

int ISecCameraHardware::getCameraId() const
{
    return mCameraId;
}

status_t ISecCameraHardware::setParameters(const CameraParameters &params)
{
    LOG_PERFORMANCE_START(1);

    if (mPictureRunning) {
        ALOGW("setParameters: warning, capture is not complete. please wait...");
        Mutex::Autolock l(&mPictureLock);
    }

    Mutex::Autolock l(&mLock);

    ALOGV("DEBUG(%s): [Before Param] %s", __FUNCTION__, params.flatten().c_str());

    status_t rc, final_rc = NO_ERROR;

    if ((rc = setRecordingMode(params)))
        final_rc = rc;
    if ((rc = setMovieMode(params)))
        final_rc = rc;
    if ((rc = setFirmwareMode(params)))
        final_rc = rc;

    if ((rc = setDtpMode(params)))
        final_rc = rc;
    if ((rc = setVtMode(params)))
        final_rc = rc;
    if ((rc = setVideoSize(params)))
        final_rc = rc;
    if ((rc = setPreviewSize(params)))
        final_rc = rc;
    if ((rc = setPreviewFormat(params)))
        final_rc = rc;
    if ((rc = setPictureSize(params)))
        final_rc = rc;
    if ((rc = setPictureFormat(params)))
        final_rc = rc;
    if ((rc = setThumbnailSize(params)))
        final_rc = rc;
    if ((rc = setJpegThumbnailQuality(params)))
        final_rc = rc;
    if ((rc = setJpegQuality(params)))
        final_rc = rc;
    if ((rc = setFrameRate(params)))
        final_rc = rc;
    if ((rc = setRotation(params)))
        final_rc = rc;
    if ((rc = setFocusMode(params)))
        final_rc = rc;

	/* UI settings */
	if (mCameraId == CAMERA_FACING_BACK) {
		/* Set anti-banding both rear and front camera if needed. */
#ifdef USE_CSC_FEATURE
		if ((rc = setAntiBanding()))			final_rc = rc;
#else
		if ((rc = setAntiBanding(params)))		final_rc = rc;
#endif
		if ((rc = setSceneMode(params)))		final_rc = rc;
		if ((rc = setFocusAreas(params)))		final_rc = rc;
		if ((rc = setFlash(params)))			final_rc = rc;
		if ((rc = setMetering(params)))			final_rc = rc;
		if ((rc = setMeteringAreas(params)))        final_rc = rc;
		if ((rc = setAutoContrast(params)))		final_rc = rc;
		/* Set for Adjust Image */
                if ((rc = setSharpness(params)))        final_rc = rc;
                if ((rc = setContrast(params)))         final_rc = rc;
                if ((rc = setSaturation(params)))       final_rc = rc;
                if ((rc = setColorAdjust(params)))    final_rc = rc;
		if ((rc = setAntiShake(params)))		final_rc = rc;
		if ((rc = setFaceBeauty(params)))		final_rc = rc;
		/* setCapturemode: Do not set before setPASMmode() */
		if ((rc = setZoomActionMethod(params)))     final_rc = rc;
		if ((rc = setZoom(params)))             final_rc = rc;
		if ((rc = setDzoom(params)))            final_rc = rc;
		if ((rc = setZoomSpeed(params)))        final_rc = rc;
	} else {
		if ((rc = setBlur(params)))				final_rc = rc;
	}
	if ((rc = setZoom(params)))				final_rc = rc;
	if ((rc = setWhiteBalance(params)))		final_rc = rc;
	if ((rc = setEffect(params)))			final_rc = rc;
	if ((rc = setBrightness(params)))		final_rc = rc;
	if ((rc = setAELock(params)))			final_rc = rc;
	if ((rc = setAWBLock(params)))			final_rc = rc;
	if ((rc = setGps(params)))			final_rc = rc;

#if VENDOR_FEATURE
	if ((rc = setWeather(params)))			final_rc = rc;
	if ((rc = setCityId(params)))			final_rc = rc;

    /* for NSM Mode */
    if ((rc = setNsmSystem(params))) final_rc = rc;
    if ((rc = setNsmState(params))) final_rc = rc;

    if ((rc = setNsmRGB(params))) final_rc = rc;
    if ((rc = setNsmContSharp(params))) final_rc = rc;

    if ((rc = setNsmHueAllRedOrange(params))) final_rc = rc;
    if ((rc = setNsmHueYellowGreenCyan(params))) final_rc = rc;
    if ((rc = setNsmHueBlueVioletPurple(params))) final_rc = rc;
    if ((rc = setNsmSatAllRedOrange(params))) final_rc = rc;
    if ((rc = setNsmSatYellowGreenCyan(params))) final_rc = rc;
    if ((rc = setNsmSatBlueVioletPurple(params))) final_rc = rc;

    if ((rc = setNsmR(params))) final_rc = rc;
    if ((rc = setNsmG(params))) final_rc =rc;
    if ((rc = setNsmB(params))) final_rc =rc;
    if ((rc = setNsmGlobalContrast(params))) final_rc =rc;
    if ((rc = setNsmGlobalSharpness(params))) final_rc =rc;

    if ((rc = setNsmHueAll(params))) final_rc =rc;
    if ((rc = setNsmHueRed(params))) final_rc =rc;
    if ((rc = setNsmHueOrange(params))) final_rc =rc;
    if ((rc = setNsmHueYellow(params))) final_rc =rc;
    if ((rc = setNsmHueGreen(params))) final_rc =rc;
    if ((rc = setNsmHueCyan(params))) final_rc =rc;
    if ((rc = setNsmHueBlue(params))) final_rc =rc;
    if ((rc = setNsmHueViolet(params))) final_rc =rc;
    if ((rc = setNsmHuePurple(params))) final_rc =rc;

    if ((rc = setNsmSatAll(params))) final_rc =rc;
    if ((rc = setNsmSatRed(params))) final_rc =rc;
    if ((rc = setNsmSatOrange(params))) final_rc =rc;
    if ((rc = setNsmSatYellow(params))) final_rc =rc;
    if ((rc = setNsmSatGreen(params))) final_rc =rc;
    if ((rc = setNsmSatCyan(params))) final_rc =rc;
    if ((rc = setNsmSatBlue(params))) final_rc =rc;
    if ((rc = setNsmSatViolet(params))) final_rc =rc;
    if ((rc = setNsmSatPurple(params))) final_rc =rc;
    /* end NSM Mode */
#endif /* VENDOR_FEATURE */

#if defined(KOR_CAMERA)
    if ((rc = setSelfTestMode(params)))
        final_rc = rc;
#endif

#if VENDOR_FEATURE
#ifndef FCJUNG
    if (mCameraId == CAMERA_FACING_BACK) {
        if (mFactoryTestNum != 0)
        {
            if ((rc = setLDC(params))) final_rc = rc;
            if ((rc = setLSC(params))) final_rc = rc;
            if ((rc = setApertureCmd(params))) final_rc = rc;
            if ((rc = setApertureStep(params))) final_rc = rc;
            if ((rc = setApertureStepPreview(params))) final_rc = rc;
            if ((rc = setApertureStepCapture(params))) final_rc = rc;
            if ((rc = setFactoryPunt(params))) final_rc = rc;
            if ((rc = setFactoryPuntShortScanData(params))) final_rc = rc;
            if ((rc = setFactoryPuntLongScanData(params))) final_rc = rc;
            if ((rc = setFactoryPuntRangeData(params))) final_rc = rc;
            if ((rc = setFactoryPuntInterpolation(params))) final_rc = rc;
            if ((rc = setFactoryOIS(params))) final_rc = rc;
            if ((rc = setFactoryOISDecenter(params))) final_rc = rc;
            if ((rc = setFactoryOISMoveShift(params))) final_rc = rc;
            if ((rc = setFactoryOISCenteringRangeData(params))) final_rc = rc;
            if ((rc = setFactoryZoom(params))) final_rc = rc;
            if ((rc = setFactoryZoomStep(params))) final_rc = rc;
            if ((rc = setFactoryZoomRangeCheckData(params))) final_rc = rc;
            if ((rc = setFactoryZoomSlopeCheckData(params))) final_rc = rc;
            if ((rc = setFactoryFailStop(params))) final_rc = rc;
            if ((rc = setFactoryNoDeFocus(params))) final_rc = rc;
            if ((rc = setFactoryInterPolation(params))) final_rc = rc;
            if ((rc = setFactoryCommon(params))) final_rc = rc;
            if ((rc = setFactoryVib(params))) final_rc = rc;
            if ((rc = setFactoryVibRangeData(params))) final_rc = rc;
            if ((rc = setFactoryGyro(params))) final_rc = rc;
            if ((rc = setFactoryGyroRangeData(params))) final_rc = rc;
            if ((rc = setFactoryBacklashMaxThreshold(params))) final_rc = rc;
            if ((rc = setFactoryBacklashCount(params))) final_rc = rc;
            if ((rc = setFactoryBacklash(params))) final_rc = rc;
            if ((rc = setFactoryAF(params))) final_rc = rc;
            if ((rc = setFactoryAFStepSet(params))) final_rc = rc;
            if ((rc = setFactoryAFPosition(params))) final_rc = rc;
            if ((rc = setFactoryAFZone(params))) final_rc = rc;
            if ((rc = setFactoryAFLens(params))) final_rc = rc;
            if ((rc = setFactoryDeFocus(params))) final_rc = rc;
            if ((rc = setFactoryDeFocusWide(params))) final_rc = rc;
            if ((rc = setFactoryDeFocusTele(params))) final_rc = rc;
            if ((rc = setFactoryResol(params))) final_rc = rc;
#if 1
            if ((rc = setFactoryLVTarget(params))) final_rc = rc;
            if ((rc = setFactoryAdjIRIS(params))) final_rc = rc;
            if ((rc = setFactoryLiveViewGain(params))) final_rc = rc;
            if ((rc = setFactoryLiveViewGainOffsetSign(params))) final_rc = rc;
            if ((rc = setFactoryLiveViewGainOffsetVal(params))) final_rc = rc;
            if ((rc = setFactoryShClose(params))) final_rc = rc;
            if ((rc = setFactoryShCloseIRISNum(params))) final_rc = rc;
            if ((rc = setFactoryShCloseSetIRIS(params))) final_rc = rc;
            if ((rc = setFactoryShCloseIso(params))) final_rc = rc;
            if ((rc = setFactoryShCloseRange(params))) final_rc = rc;
            if ((rc = setFactoryCaptureGain(params))) final_rc = rc;
            if ((rc = setFactoryCaptureGainOffsetSign(params))) final_rc = rc;
            if ((rc = setFactoryCaptureGainOffsetVal(params))) final_rc = rc;
            if ((rc = setFactoryLscFshdTable(params))) final_rc = rc;
            if ((rc = setFactoryLscFshdReference(params))) final_rc = rc;
            if ((rc = setFactoryIRISRange(params))) final_rc = rc;
            if ((rc = setFactoryGainLiveViewRange(params))) final_rc = rc;
            if ((rc = setFactoryShCloseSpeedTime(params))) final_rc = rc;
            if ((rc = setFactoryCameraMode(params))) final_rc = rc;
            if ((rc = setFactoryFlash(params))) final_rc = rc;
            if ((rc = setFactoryFlashChgChkTime(params))) final_rc = rc;
            if ((rc = setFactoryAFScanLimit(params))) final_rc = rc;
            if ((rc = setFactoryAFScanRange(params))) final_rc = rc;
            if ((rc = setFactoryFlashRange(params))) final_rc = rc;
            if ((rc = setFactoryWB(params))) final_rc = rc;
            if ((rc = setFactoryWBRange(params))) final_rc = rc;
            if ((rc = setFactoryFlicker(params))) final_rc = rc;
            if ((rc = setFactoryAETarget(params))) final_rc = rc;
            if ((rc = setFactoryAFLEDtime(params))) final_rc = rc;
            if ((rc = setFactoryCamSysMode(params))) final_rc = rc;
            if ((rc = setFactoryCaptureGainRange(params))) final_rc = rc;
            if ((rc = setFactoryCaptureCtrl(params))) final_rc = rc;
            if ((rc = setFactoryWBvalue(params))) final_rc = rc;
            if ((rc = setFactoryAFLEDRangeData(params))) final_rc = rc;
            if ((rc = setFactoryAFDiffCheck(params))) final_rc = rc;
            if ((rc = setFactoryDefectPixel(params))) final_rc = rc;
            if ((rc = setFactoryDFPXNLVCAP(params))) final_rc = rc;
            if ((rc = setFactoryDFPXNLVDR1HD(params))) final_rc = rc;
            if ((rc = setFactoryDFPXNLVDR0(params))) final_rc = rc;
            if ((rc = setFactoryDFPXNLVDR1(params))) final_rc = rc;
            if ((rc = setFactoryDFPXNLVDR2(params))) final_rc = rc;
            if ((rc = setFactoryDFPXNLVDRHS(params))) final_rc = rc;
            if ((rc = setFactoryAFLEDlevel(params))) final_rc = rc;
            if ((rc = setFactoryIspFwVerCheck(params))) final_rc = rc;
            if ((rc = setFactoryOisVerCheck(params))) final_rc = rc;
            if ((rc = setFactoryTiltScanLimitData(params))) final_rc = rc;
            if ((rc = setFactoryTiltAFRangeData(params))) final_rc = rc;
            if ((rc = setFactoryTiltAFScanLimitData(params))) final_rc = rc;
            if ((rc = setFactoryTiltDiffRangeData(params))) final_rc = rc;
            if ((rc = setFactoryIRCheckRGainData(params))) final_rc = rc;
            if ((rc = setFactoryIRCheckBGainData(params))) final_rc = rc;
//            if ((rc = setFactoryIRCheckGAvgData(params))) final_rc = rc;	//?
            if ((rc = setFactoryTiltField(params))) final_rc = rc;
            if ((rc = setFactoryFlashManCharge(params))) final_rc = rc;
            if ((rc = setFactoryFlashManEn(params))) final_rc = rc;
            if ((rc = setFactoryTilt(params))) final_rc = rc;
            if ((rc = setFactoryIRCheck(params))) final_rc = rc;
            if ((rc = setFactoryFlashCharge(params))) final_rc = rc;
            if ((rc = setFactoryMemCopy(params))) final_rc = rc;
            if ((rc = setFactoryMemMode(params))) final_rc = rc;
/* ccjung */
            if ((rc = setFactoryEEPWrite(params))) final_rc = rc;
            if ((rc = setFactoryLensCap(params))) final_rc = rc;
            if ((rc = setFactoryCheckSum(params))) final_rc = rc;

            if ((rc = setFactoryLogWriteAll(params))) final_rc = rc;
            if ((rc = setFactoryDataErase(params))) final_rc = rc;
            if ((rc = setFactoryNoLensOff(params))) final_rc = rc;
            if ((rc = setFactoryFocusCloseOpenCheck(params))) final_rc = rc;
            if ((rc = setFactoryResolutionLog(params))) final_rc = rc;
            if ((rc = setFactoryShadingLog(params))) final_rc = rc;
//            if ((rc = setFactoryWriteShadingData(params))) final_rc = rc;	//?
//            if ((rc = setFactoryClipValue(params))) final_rc = rc;
            if ((rc = setFactoryTiltCoordinate(params))) final_rc = rc;
//            if ((rc = setFactoryAFLED(params))) final_rc = rc;
            if ((rc = setFactoryDefectNoiseLevel(params))) final_rc = rc;
            if ((rc = setFactoryComapareMemory(params))) final_rc = rc;
#endif
        }
    }
#endif /* FCJUNG */
#endif /* VENDOR_FEATURE */

    LOG_PERFORMANCE_END(1, "total");

    ALOGV("DEBUG(%s): [After Param] %s", __FUNCTION__, params.flatten().c_str());

    ALOGD("setParameters X: %s", final_rc == NO_ERROR ? "success" : "failed");
    return final_rc;
}

#if VENDOR_FEATURE
status_t ISecCameraHardware::setPASMmode(const CameraParameters& params)
{
    const char *str = params.get(CameraParameters::KEY_MODE);
    const char *prevStr = mParameters.get(CameraParameters::KEY_MODE);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    ALOGE("setPASMmode: %s, %s", str, prevStr);

    int val;
    val = SecCameraParameters::lookupAttr(CameraModes, ARRAY_SIZE(CameraModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setPASMmode: warning, not supported value(%s)", str);
        return BAD_VALUE;
    }

    mPASMMode = val;
    mParameters.set(CameraParameters::KEY_MODE, str);

    if (mCameraId == CAMERA_FACING_BACK)
        return nativeSetParameters(CAM_CID_PASM_MODE, val);
    else
        return NO_ERROR;
}
#endif /* VENDOR_FEATURE */

void ISecCameraHardware::setDropFrame(int count)
{
    /* should be locked */
    if (mDropFrameCount < count)
        mDropFrameCount = count;
}

status_t ISecCameraHardware::setAELock(const CameraParameters &params)
{
    const char *str_support = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
    if (str_support == NULL || strcmp(str_support, "true"))
        return NO_ERROR;

    const char *str = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    const char *prevStr = mParameters.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    ALOGV("setAELock: %s", str);
    if (!(!strcmp(str, "true") || !strcmp(str, "false")))
        return BAD_VALUE;

    mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, str);
#if 0
    int val;
    if (!strcmp(str, "true"))
        val = AE_LOCK;
    else
        val = AE_UNLOCK;
#endif

    return NO_ERROR;
    /*  TODO : sctrl not defined */
    /* return nativeSetParameters(CAM_CID_AE_LOCK_UNLOCK, val); */
}

status_t ISecCameraHardware::setAWBLock(const CameraParameters &params)
{
    const char *str_support = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
    if (str_support == NULL || strcmp(str_support, "true"))
        return NO_ERROR;

    const char *str = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
    const char *prevStr = mParameters.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    ALOGV("setAWBLock: %s", str);
    if (!(!strcmp(str, "true") || !strcmp(str, "false")))
        return BAD_VALUE;

    mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, str);
#if 0
    int val;
    if (!strcmp(str, "true"))
        val = AWB_LOCK;
    else
        val = AWB_UNLOCK;
#endif

    return NO_ERROR;
    /*  TODO : sctrl not defined */
    /*  return nativeSetParameters(CAM_CID_AWB_LOCK_UNLOCK, val); */
}

#ifdef PX_COMMON_CAMERA
/*
 * called when starting preview
 */
 inline void ISecCameraHardware::setDropUnstableInitFrames()
{
    int32_t frameCount = 3;

    if (mCameraId == CAMERA_FACING_BACK) {
        if (mbFirst_preview_started == false) {
            /* When camera_start_preview is called for the first time after camera application starts. */
            if (mSceneMode == SCENE_MODE_NIGHTSHOT || mSceneMode == SCENE_MODE_FIREWORKS)
                frameCount = 3;
            else
                frameCount = mSamsungApp ? INITIAL_REAR_SKIP_FRAME : 6;

            mbFirst_preview_started = true;
        } else {
            /* When startPreview is called after camera application got started. */
           frameCount = (mSamsungApp && mMovieMode) ? (INITIAL_REAR_SKIP_FRAME + 4) : 2;
        }
    } else {
        if (mbFirst_preview_started == false) {
            /* When camera_start_preview is called for the first time after camera application starts. */
            frameCount = INITIAL_FRONT_SKIP_FRAME;
            mbFirst_preview_started = true;
        } else {
            /* When startPreview is called after camera application got started. */
            frameCount = 2;
        }
    } /* (mCameraId == CAMERA_FACING_BACK) */

    setDropFrame(frameCount);
}
#endif

status_t ISecCameraHardware::setFirmwareMode(const CameraParameters &params)
{
    const char *str = params.get(SecCameraParameters::KEY_FIRMWARE_MODE);
	int err;
    if (str == NULL)
        return NO_ERROR;

    int val = SecCameraParameters::lookupAttr(firmwareModes, ARRAY_SIZE(firmwareModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGE("setFirmwareMode: error, invalid value %s", str);
        return BAD_VALUE;
    }

    ALOGV("setFirmwareMode: %s", str);
    mFirmwareMode = (cam_fw_mode)val;
    mParameters.set(SecCameraParameters::KEY_FIRMWARE_MODE, str);

    err = nativeSetParameters(CAM_CID_FW_MODE, val);
	 if (mFirmwareMode == CAM_FW_MODE_UPDATE) {
        if (err >= 0)
            mFirmwareResult = 1;    /* success */
        else if (err == -2)
            mFirmwareResult = 3;    /* fail : no file */
        else
            mFirmwareResult = 2;    /* fail */
    }

	 if (mFirmwareResult) {
        ALOGD("Smart Firmware Update : %s", (mFirmwareResult == 1 ? "Success" : "Fail"));
#if VENDOR_FEATURE
        mNotifyCb(CAMERA_MSG_FIRMWARE_NOTIFY, mFirmwareResult, 0, mCallbackCookie);
#endif
        mFirmwareResult = 0;
    }

	return NO_ERROR;
}

status_t ISecCameraHardware::setDtpMode(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_DTP_MODE);

#ifdef DEBUG_PREVIEW_NO_FRAME /* 130221.DSLIM Delete me in a week*/
    if ((val >= 0) && !mFactoryMode) {
        mFactoryMode = true;
        ALOGD("setDtpMode: Factory mode (DTP %d)", val);
    } else if ((val < 0) && mFactoryMode) {
        mFactoryMode = false;
        ALOGD("setDtpMode: not Factory mode");
    }
#endif

    if (val == -1 || mDtpMode == (bool)val)
        return NO_ERROR;

    ALOGD("setDtpMode: %d", val);
    mDtpMode = val ? true : false;
    mParameters.set(SecCameraParameters::KEY_DTP_MODE, val);
    if (mDtpMode > 0)
        setDropFrame(15);

    return nativeSetParameters(CAM_CID_DTP_MODE, val);
}

status_t ISecCameraHardware::setVtMode(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_VT_MODE);
    if (val == -1 || mVtMode == (cam_vt_mode)val)
        return NO_ERROR;

    ALOGV("setVtmode: %d", val);
    mVtMode = (cam_vt_mode)val;
    mParameters.set(SecCameraParameters::KEY_VT_MODE, val);

    return nativeSetParameters(CAM_CID_VT_MODE, val);
}

status_t ISecCameraHardware::setMovieMode(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_MOVIE_MODE);
#if 0
    const char* prev_val = mParameters.get(SecCameraParameters::KEY_MOVIE_MODE);
    int prev_intVal = mParameters.getInt(SecCameraParameters::KEY_MOVIE_MODE);
#endif

    if (val == -1 || mMovieMode == (bool)val)
        return NO_ERROR;

    ALOGV("setMovieMode: %d", val);
    mMovieMode = val ? true : false;
    mParameters.set(SecCameraParameters::KEY_MOVIE_MODE, val);

    /* Activate the below codes if effect setting has no problem in recording mode */
    if (((mCameraId == CAMERA_FACING_FRONT) || (mCameraId == FD_SERVICE_CAMERA_ID))
      && nativeIsInternalISP()) {
        if (mMovieMode)
            return nativeSetParameters(CAM_CID_IS_S_FORMAT_SCENARIO, IS_MODE_PREVIEW_VIDEO);
        else
            return nativeSetParameters(CAM_CID_IS_S_FORMAT_SCENARIO, IS_MODE_PREVIEW_STILL);
    }

    return nativeSetParameters(CAM_CID_MOVIE_MODE, val);
}

status_t ISecCameraHardware::setRecordingMode(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_RECORDING_HINT);
    const char *prevStr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    mParameters.set(CameraParameters::KEY_RECORDING_HINT, str);

    String8 recordHint(str);
    ALOGV("setRecordingMode: %s", recordHint.c_str());

    if (recordHint == "true") {
        mFps = mMaxFrameRate / 1000;
        ALOGD("DEBUG(%s): fps(%d) %s ", __FUNCTION__, mFps, recordHint.c_str());

        mMovieMode = true;
    } else {
        mMovieMode = false;
    }

    return NO_ERROR;
#ifdef NOTDEFINED
    if (((mCameraId == CAMERA_FACING_FRONT) || (mCameraId == FD_SERVICE_CAMERA_ID))
      && nativeIsInternalISP()) {
        if (mMovieMode)
            return nativeSetParameters(CAM_CID_IS_S_FORMAT_SCENARIO, IS_MODE_PREVIEW_VIDEO);
        else
            return nativeSetParameters(CAM_CID_IS_S_FORMAT_SCENARIO, IS_MODE_PREVIEW_STILL);
    }

    return nativeSetParameters(CAM_CID_MOVIE_MODE, mMovieMode);
#endif
}

status_t ISecCameraHardware::setPreviewSize(const CameraParameters &params)
{
    int width, height;
    params.getPreviewSize(&width, &height);

    if (width <= 0 || height <= 0)
        return BAD_VALUE;

    int count;
    image_rect_type panoramaSize;
    const image_rect_type *sizes, *defaultSizes = NULL, *size = NULL;

    if (mCameraId == CAMERA_FACING_BACK) {
        count = ARRAY_SIZE(backPreviewSizes);
        sizes = backPreviewSizes;
    } else {
        count = ARRAY_SIZE(frontPreviewSizes);
        sizes = frontPreviewSizes;
    }

retry:
    if (mPASMMode == MODE_PANORAMA && width == 1920) {
        panoramaSize.width = 1920;
        panoramaSize.height = 1080;
        size = &panoramaSize;
    } else {
        for (int i = 0; i < count; i++) {
            if (((uint32_t)width == sizes[i].width) && ((uint32_t)height == sizes[i].height)) {
                size = &sizes[i];
                break;
            }
        }
    }

    if (CC_UNLIKELY(!size)) {
        if (!defaultSizes) {
            defaultSizes = sizes;
            if (mCameraId == CAMERA_FACING_BACK) {
                count = ARRAY_SIZE(hiddenBackPreviewSizes);
                sizes = hiddenBackPreviewSizes;
            } else {
                count = ARRAY_SIZE(hiddenFrontPreviewSizes);
                sizes = hiddenFrontPreviewSizes;
            }
            goto retry;
        } else
            sizes = defaultSizes;

        ALOGW("setPreviewSize: warning, not supported size(%dx%d)", width, height);
        size = &sizes[0];
    }

    mPreviewSize = *size;
    mParameters.setPreviewSize((int)size->width, (int)size->height);

    /* FLite size depends on preview size */
#if VENDOR_FEATURE
   if((mMovieMode == true) && (mVideoSize.height == 1080)){
	mFLiteSize.height = 1080;
	mFLiteSize.width = 1920;
   } else if((mPreviewSize.width == 352) && (mPreviewSize.height == 288)){
	mFLiteSize.height = 576;
	mFLiteSize.width = 704;
   } else if((mPreviewSize.width == 640) && (mPreviewSize.height == 480)){
	mFLiteSize.height = 720;
	mFLiteSize.width = 960;
   }else{
	mFLiteSize = *size;
  }
#else
    mFLiteSize = *size;
#endif

    /* backup orginal preview size due to ALIGN */
    mOrgPreviewSize = mPreviewSize;
    mPreviewSize.width = ALIGN(mPreviewSize.width, 128);
    mPreviewSize.height = ALIGN(mPreviewSize.height, 2);

#if 0
#if VENDOR_FEATURE
#else
    if (mPreviewSize.width < 640 || mPreviewSize.width < 480) {
        mFLiteSize.width = 640;
        mFLiteSize.height = 480;
    }

    /* HACK */
    if ((mPreviewSize.width * 9) == (mPreviewSize.height * 16)) {
        mFLiteSize.width = 1024;
        mFLiteSize.height = 576;
    }
#endif
#endif

    ALOGD("DEBUG(%s)(%d): preview size %dx%d/%dx%d/%dx%d", __FUNCTION__, __LINE__,
                width, height, mOrgPreviewSize.width, mOrgPreviewSize.height,
                mPreviewSize.width, mPreviewSize.height);

    return NO_ERROR;
}

status_t ISecCameraHardware::setPreviewFormat(const CameraParameters &params)
{
    const char *str = params.getPreviewFormat();
    const char *prevStr = mParameters.getPreviewFormat();
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(previewPixelFormats, ARRAY_SIZE(previewPixelFormats), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setPreviewFormat: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(previewPixelFormats[0].desc);
        goto retry;
    }

    ALOGD("setPreviewFormat: %s", str);
    mPreviewFormat = (cam_pixel_format)val;
    ALOGV("setPreviewFormat: mPreviewFormat = %s",
        mPreviewFormat == CAM_PIXEL_FORMAT_YVU420P ? "YV12" :
        mPreviewFormat == CAM_PIXEL_FORMAT_YUV420SP ? "NV21" :
        "Others");
    mParameters.setPreviewFormat(str);
    return NO_ERROR;
}

status_t ISecCameraHardware::setVideoSize(const CameraParameters &params)
{

    int width = 0, height = 0;
    params.getVideoSize(&width, &height);

    if ((mVideoSize.width == (uint32_t)width) && (mVideoSize.height == (uint32_t)height))
        return NO_ERROR;

    int count;
    const image_rect_type *sizes, *defaultSizes = NULL, *size = NULL;

    if (mCameraId == CAMERA_FACING_BACK) {
        count = ARRAY_SIZE(backRecordingSizes);
        sizes = backRecordingSizes;
    } else {
        count = ARRAY_SIZE(frontRecordingSizes);
        sizes = frontRecordingSizes;
    }

retry:
    for (int i = 0; i < count; i++) {
        if (((uint32_t)width == sizes[i].width) && ((uint32_t)height == sizes[i].height)) {
            size = &sizes[i];
            break;
        }
    }

    if (CC_UNLIKELY(!size)) {
        if (!defaultSizes) {
            defaultSizes = sizes;
            if (mCameraId == CAMERA_FACING_BACK) {
                count = ARRAY_SIZE(hiddenBackRecordingSizes);
                sizes = hiddenBackRecordingSizes;
            } else {
                count = ARRAY_SIZE(hiddenFrontRecordingSizes);
                sizes = hiddenFrontRecordingSizes;
            }
            goto retry;
        } else {
            sizes = defaultSizes;
        }

        ALOGW("setVideoSize: warning, not supported size(%dx%d)", width, height);
        size = &sizes[0];
    }

    ALOGD("setVideoSize: recording %dx%d", size->width, size->height);
    mVideoSize = *size;
    mParameters.setVideoSize((int)size->width, (int)size->height);

    /* const char *str = mParameters.get(CameraParameters::KEY_VIDEO_SIZE); */
    return NO_ERROR;
}

status_t ISecCameraHardware::setPictureSize(const CameraParameters &params)
{
    int width, height;
	int err;
    params.getPictureSize(&width, &height);

	/* RAW capture temp  width = 4608; , height = 3456; */

    ALOGD("DEBUG(%s): previous %dx%d after %dx%d", __FUNCTION__,
            mPictureSize.width, mPictureSize.height, width, height);

    if ((mPictureSize.width == (uint32_t)width) && (mPictureSize.height == (uint32_t)height)) {
        /* Continuous duplicate size setting makes ISP problem
           for all factory test (including 103 Centering, 123 Defect) */
        if (0 != mFactoryTestNum)
            return NO_ERROR;
    }

#if 0
    if ((mPictureSize.width == (uint32_t)width) && (mPictureSize.height == (uint32_t)height)) {
        mFLiteCaptureSize = mPictureSize;
        if (mPictureSize.width < 640 || mPictureSize.width < 480) {
            mFLiteCaptureSize.width = 640;
            mFLiteCaptureSize.height = 480;
        }
        return NO_ERROR;
    }
#endif

    int count;
    const image_rect_type *sizes, *defaultSizes = NULL, *size = NULL;

    if (mCameraId == CAMERA_FACING_BACK) {
        count = ARRAY_SIZE(backPictureSizes);
        sizes = backPictureSizes;
    } else {
        count = ARRAY_SIZE(frontPictureSizes);
        sizes = frontPictureSizes;
    }

retry:
    for (int i = 0; i < count; i++) {
        if (((uint32_t)width == sizes[i].width) && ((uint32_t)height == sizes[i].height)) {
            size = &sizes[i];
            break;
        }
    }

    if (CC_UNLIKELY(!size)) {
        if (!defaultSizes) {
            defaultSizes = sizes;
            if (mCameraId == CAMERA_FACING_BACK) {
                count = ARRAY_SIZE(hiddenBackPictureSizes);
                sizes = hiddenBackPictureSizes;
            } else {
                count = ARRAY_SIZE(hiddenFrontPictureSizes);
                sizes = hiddenFrontPictureSizes;
            }
            goto retry;
        } else
            sizes = defaultSizes;

        ALOGW("setPictureSize: warning, not supported size(%dx%d)", width, height);
        size = &sizes[0];
    }

    ALOGD("setPictureSize: %dx%d", size->width, size->height);
    mPictureSize = *size;
    mParameters.setPictureSize((int)size->width, (int)size->height);

    mFLiteCaptureSize = mPictureSize;
    if (mPictureSize.width < 640 || mPictureSize.width < 480) {
        mFLiteCaptureSize.width = 640;
        mFLiteCaptureSize.height = 480;
    }

    mRawSize = mPictureSize;

	uint32_t sizeData = (mFLiteCaptureSize.width << 16) | (mFLiteCaptureSize.height & 0xFFFF);
    err = nativeSetParameters(CAM_CID_SET_SNAPSHOT_SIZE, sizeData);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("setPictureSize: error");
        return UNKNOWN_ERROR;
    }

	/* Postview size need to be set before capture start */
	if (FRM_RATIO(mFLiteCaptureSize) == CAM_FRMRATIO_HD) {
		mPostviewSize.width = 1280;
		mPostviewSize.height = 720;
	} else if (FRM_RATIO(mFLiteCaptureSize) == CAM_FRMRATIO_D1) {
		mPostviewSize.width = 1056;
		mPostviewSize.height = 704;
	} else if (FRM_RATIO(mFLiteCaptureSize) == CAM_FRMRATIO_SQUARE) {
		mPostviewSize.width = 704;
		mPostviewSize.height = 704;
	} else {
		mPostviewSize.width = 960;
		mPostviewSize.height = 720;
	}
	nativeSetParameters(CAM_CID_SET_POSTVIEW_SIZE,
			(int)(mPostviewSize.width<<16|mPostviewSize.height));

    return NO_ERROR;
}

status_t ISecCameraHardware::setPictureFormat(const CameraParameters &params)
{
    const char *str = params.getPictureFormat();
    const char *prevStr = mParameters.getPictureFormat();
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(picturePixelFormats, ARRAY_SIZE(picturePixelFormats), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setPictureFormat: warning, not supported value(%s)", str);
#if 0
        str = reinterpret_cast<const char*>(picturePixelFormats[0].desc);
        goto retry;
#else
        return BAD_VALUE;
#endif	/* FOR HAL TEST */
    }

    ALOGV("setPictureFormat: %s", str);
    mPictureFormat = (cam_pixel_format)val;
    mParameters.setPictureFormat(str);
    return NO_ERROR;
}

status_t ISecCameraHardware::setThumbnailSize(const CameraParameters &params)
{
    int width, height;
    width = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    height = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);

    if (mThumbnailSize.width == (uint32_t)width && mThumbnailSize.height == (uint32_t)height)
        return NO_ERROR;

    int count;
    const image_rect_type *sizes, *size = NULL;

    if (mCameraId == CAMERA_FACING_BACK) {
        count = ARRAY_SIZE(backThumbSizes);
        sizes = backThumbSizes;
    } else {
        count = ARRAY_SIZE(frontThumbSizes);
        sizes = frontThumbSizes;
    }

    for (int i = 0; i < count; i++) {
        if ((uint32_t)width == sizes[i].width && (uint32_t)height == sizes[i].height) {
            size = &sizes[i];
            break;
        }
    }

    if (!size) {
        ALOGW("setThumbnailSize: warning, not supported size(%dx%d)", width, height);
        size = &sizes[0];
    }

    ALOGV("setThumbnailSize: %dx%d", size->width, size->height);
    mThumbnailSize = *size;
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, (int)size->width);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, (int)size->height);

    return NO_ERROR;
}

status_t ISecCameraHardware::setJpegThumbnailQuality(const CameraParameters &params)
{
    int val = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    int prevVal = mParameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < 1 || val > 100)) {
        ALOGE("setJpegThumbnailQuality: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGV("setJpegThumbnailQuality: %d", val);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, val);

    return 0;
}

status_t ISecCameraHardware::setJpegQuality(const CameraParameters &params)
{
    int val = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    int prevVal = mParameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < 1 || val > 100)) {
        ALOGE("setJpegQuality: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGV("setJpegQuality: %d", val);
    mJpegQuality = val;
    mParameters.set(CameraParameters::KEY_JPEG_QUALITY, val);

    return nativeSetParameters(CAM_CID_JPEG_QUALITY, val);
}

status_t ISecCameraHardware::setFrameRate(const CameraParameters &params)
{
    int min, max;
    params.getPreviewFpsRange(&min, &max);
    int frameRate = params.getPreviewFrameRate();
    int prevFrameRate = mParameters.getPreviewFrameRate();
    if ((frameRate != -1) && (frameRate != prevFrameRate))
        mParameters.setPreviewFrameRate(frameRate);

    if (CC_UNLIKELY(min < 0 || max < 0 || max < min)) {
        ALOGE("setFrameRate: error, invalid range(%d, %d)", min, max);
        return BAD_VALUE;
    }

    /* 0 means auto frame rate */
    int val = (min == max) ? min : 0;
    mMaxFrameRate = max;

    if (mMovieMode)
        mFps = mMaxFrameRate / 1000;
    else
        mFps = val / 1000;

    ALOGV("setFrameRate: %d,%d,%d", min, max, mFps);

    if (mFrameRate == val)
        return NO_ERROR;

    mFrameRate = val;

    const char *str = params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE);
    if (CC_LIKELY(str)) {
        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, str);
    } else {
        ALOGE("setFrameRate: corrupted data (params)");
        char buffer[32];
        CLEAR(buffer);
        snprintf(buffer, sizeof(buffer), "%d,%d", min, max);
        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, buffer);
    }

    mParameters.setPreviewFrameRate(val/1000);

    if (mFrameRate == 15000)
        val = 30000;    /* use skip frame */

    return nativeSetParameters(CAM_CID_FRAME_RATE, val/1000);
}

status_t ISecCameraHardware::setRotation(const CameraParameters &params)
{
    int val = params.getInt(CameraParameters::KEY_ROTATION);
    int prevVal = mParameters.getInt(CameraParameters::KEY_ROTATION);
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val != 0 && val != 90 && val != 180 && val != 270)) {
        ALOGE("setRotation: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGV("setRotation: %d", val);
    mParameters.set(CameraParameters::KEY_ROTATION, val);

    if (mVtMode)
        return nativeSetParameters(CAM_CID_ROTATION, val);

    return NO_ERROR;
}

status_t ISecCameraHardware::setPreviewFrameRate(const CameraParameters &params)
{
    int val = params.getPreviewFrameRate();
    int prevVal = mParameters.getPreviewFrameRate();
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < 0 || val > (mMaxFrameRate / 1000) )) {
        ALOGE("setPreviewFrameRate: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGV("setPreviewFrameRate: %d", val);
    mFrameRate = val * 1000;
    mParameters.setPreviewFrameRate(val);

    return NO_ERROR;
}

status_t ISecCameraHardware::setSceneMode(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_SCENE_MODE);
    const char *prevStr = mParameters.get(CameraParameters::KEY_SCENE_MODE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;

retry:
    val = SecCameraParameters::lookupAttr(sceneModes, ARRAY_SIZE(sceneModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setSceneMode: warning, not supported value(%s)", str);
#if 0
        str = reinterpret_cast<const char*>(sceneModes[0].desc);
        goto retry;
#else
        return BAD_VALUE;
#endif	/* FOR HAL TEST */
    }

    ALOGV("setSceneMode: %s", str);
    mSceneMode = (cam_scene_mode)val;
    mParameters.set(CameraParameters::KEY_SCENE_MODE, str);

    return nativeSetParameters(CAM_CID_SCENE_MODE, val);
}

#if VENDOR_FEATURE
#ifdef BURST_SHOT_SUPPORT
status_t ISecCameraHardware::setBurstSavePath(const CameraParameters& params)
{
    const char *str  = params.get(CameraParameters::KEY_CAPTURE_BURST_FILEPATH);
    const char *prevStr = mParameters.get(CameraParameters::KEY_CAPTURE_BURST_FILEPATH);
    ALOGV("setBurstSavePath ~~~~~~~~~~~~~~ str : %s, prevStr : %s",str, prevStr);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    ALOGD("setBurstSavePath: %s", str);
    mParameters.set(CameraParameters::KEY_CAPTURE_BURST_FILEPATH, str);

    return NO_ERROR;
}

char* ISecCameraHardware::getBurstSavePath(void)
{
    const char *str  = mParameters.get(CameraParameters::KEY_CAPTURE_BURST_FILEPATH);
    ALOGV("getBurstSavePath ~~~~~~~~~~~~~~ str : %s",str);

    return (char*)str;
}
#endif

status_t ISecCameraHardware::setCapturemode(const CameraParameters &params)
{
    const char *str  = params.get(CameraParameters::KEY_CAPTURE_MODE);
    const char *prevStr = mParameters.get(CameraParameters::KEY_CAPTURE_MODE);
    const int faceDetection = getFaceDetection();

#if 0
    /* Capture mode should be set after error case to prevent ISP malfunction. */
    if (str == NULL /* || (prevStr && !strcmp(str, prevStr)) */)
#else
    /* 14.02.11. heechul.,
       Continuous duplicate capture mode setting makes halt when auto focus.
       FD blink is a kind of capture mode. So although capture mode is same,
       if FD mode changes from Blink or to Blink, capture mode must change.
       */
    if (str == NULL || (prevStr && !strcmp(str, prevStr))) {
        if((faceDetection == mFaceDetectionStatus) ||
           ((faceDetection != V4L2_FACE_DETECTION_BLINK) &&
            (mFaceDetectionStatus != V4L2_FACE_DETECTION_BLINK)))
            return NO_ERROR;
    }
#endif

    if(str != NULL) {
        ALOGD("setCapturemode: str: %s, prevStr: %s, FDmode: %d",str, prevStr, faceDetection);

        mCaptureMode = SecCameraParameters::lookupAttr(captureMode, ARRAY_SIZE(captureMode), str);
        mParameters.set(CameraParameters::KEY_CAPTURE_MODE, str);
    }
    mFaceDetectionStatus = faceDetection;

    if (mCameraId == CAMERA_FACING_BACK)
        if(faceDetection == V4L2_FACE_DETECTION_BLINK) {
            ALOGD("blink");
            return nativeSetParameters(CAM_CID_SET_CAPTURE_MODE, RUNNING_MODE_BLINK);
        } else {
            ALOGD("no blink");
            return nativeSetParameters(CAM_CID_SET_CAPTURE_MODE, mCaptureMode);
        }
    else
        return NO_ERROR;
}

status_t ISecCameraHardware::setAFLED(const CameraParameters& params)
{
	//CameraParameters::KEY_AF_LAMP = "af-lamp"
    const char *str  = params.get("af-lamp");
    const char *prevStr = mParameters.get("af-lamp");
    //ALOGV("setAFLED ~~~~~~~~~~~~~~ str : %s, prevStr : %s",str, prevStr);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(AFLEDValues, ARRAY_SIZE(AFLEDValues), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setAFLED: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGD("setAFLED: %s", str);
    mParameters.set("af-lamp", str);

    nativeSetParameters(CAM_CID_AF_LED, val);

    return NO_ERROR;
}

status_t ISecCameraHardware::setTimerMode(const CameraParameters& params)
{
    const char *str  = params.get("timer");
    const char *prevStr = mParameters.get("timer");
    //ALOGV("setTimerMode ~~~~~~~~~~~~~~ str : %s, prevStr : %s",str, prevStr);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(timerLEDValues, ARRAY_SIZE(timerLEDValues), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setTimerMode: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGD("setTimerMode: %s", str);
    mParameters.set("timer", str);
    mTimerSet = val;

    nativeSetParameters(CAM_CID_TIMER_MODE, val);

    return NO_ERROR;
}

status_t ISecCameraHardware::setFocusAreaModes(const CameraParameters& params)
{
    const char *str  = params.get(CameraParameters::KEY_FOCUS_AREA_MODE);
    const char *prevStr = mParameters.get(CameraParameters::KEY_FOCUS_AREA_MODE);
    //ALOGV("setFocusAreaModes ~~~~~~~~~~~~~~ str : %s, prevStr : %s",str, prevStr);

    if (str == NULL /* || (prevStr && !strcmp(str, prevStr)) */)
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(focusareaModes, ARRAY_SIZE(focusareaModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setFocusAreaModes: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGD("setFocusAreaModes: %s", str);

    mFocusArea = (cam_focus_area)val;

    mParameters.set(CameraParameters::KEY_FOCUS_AREA_MODE, str);

    return nativeSetParameters(CAM_CID_FOCUS_AREA_MODE, val);
}

status_t ISecCameraHardware::setFaceDetection(const CameraParameters& params)
{
    const char *str  = params.get(CameraParameters::KEY_FACEDETECT);
    const char *prevStr = mParameters.get(CameraParameters::KEY_FACEDETECT);
    //ALOGV("setFaceDetection ~~~~~~~~~~~~~~ str : %s, prevStr : %s",str, prevStr);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(facedetectionModes, ARRAY_SIZE(facedetectionModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setFaceDetection: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGD("setFaceDetection: %s", str);
    mParameters.set(CameraParameters::KEY_FACEDETECT, str);

#if 1
    if (mCameraId == CAMERA_FACING_BACK) {
        if (val == V4L2_FACE_DETECTION_NORMAL) {
            ALOGD("setFaceDetection: start getting the FD Info");
            nativeSetParameters(CAM_CID_SET_FD_FOCUS_SELECT, 0x2);

            mSelfShotFDReading = 1;
        } else {
            ALOGD("setFaceDetection: top getting the FD Info");
            nativeSetParameters(CAM_CID_SET_FD_FOCUS_SELECT, 0x0);

            roi_x_pos = 0;
            roi_y_pos = 0;
            mSelfShotFDReading = 0;
        }
    }
#endif

    return nativeSetParameters(CAM_CID_FACE_DETECTION, val);
}


int ISecCameraHardware::getFaceDetection(void)
{
    const char *str  = mParameters.get(CameraParameters::KEY_FACEDETECT);
    //ALOGV("getFaceDetection ~~~~~~~~~~~~~~ str : %s",str);

    int val;
retry:
    val = SecCameraParameters::lookupAttr(facedetectionModes, ARRAY_SIZE(facedetectionModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("getFaceDetection: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGV("getFaceDetection value: %d", val);

    return val;
}


status_t ISecCameraHardware::setROIBox(const CameraParameters& params)
{
    const char *str_x  = params.get(CameraParameters::KEY_CURRENT_ROI_LEFT);
    const char *str_y  = params.get(CameraParameters::KEY_CURRENT_ROI_TOP);

    const char *str_width  = params.get(CameraParameters::KEY_CURRENT_ROI_WIDTH);
    const char *str_height  = params.get(CameraParameters::KEY_CURRENT_ROI_HEIGHT);


    if(str_x == NULL && str_y == NULL && str_width == NULL && str_height == NULL)
        return NO_ERROR;

retry:
#if 0
    val = SecCameraParameters::lookupAttr(facedetectionModes, ARRAY_SIZE(facedetectionModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setROIBox: warning, not supported value(%s)", str);
        return NO_ERROR;
    }
#endif

    ALOGD("setROIBox: x[%s], y[%s]", str_x, str_y);
    ALOGD("setROIBox: width[%s], height[%s]", str_width, str_height);


    if(str_x != NULL && str_y != NULL) {
        roi_x_pos = (int)atoi(str_x);
        roi_y_pos = (int)atoi(str_y);
        ALOGD("setROIBox: roi_x_pos[%d], roi_y_pos[%d]", roi_x_pos, roi_y_pos);

        uint32_t roi_pos_data = (roi_x_pos  << 16) | (roi_y_pos & 0x0FFFF);

        nativeSetParameters(CAM_CID_SET_ROI_BOX, roi_pos_data);
    }

    if(str_width != NULL && str_height != NULL) {
        roi_width = (int)atoi(str_width);
        roi_height = (int)atoi(str_height);
        ALOGD("setROIBox: roi_width[%d], height[%d]", roi_width, roi_height);

        uint32_t roi_wid_hei_data = (roi_width  << 16) | (roi_height & 0x0FFFF);

        nativeSetParameters(CAM_CID_SET_ROI_BOX_WIDTH_HEIGHT, roi_wid_hei_data);
    }

    return NO_ERROR;
}
#endif /* VENDOR_FEATURE */

/* -------------------Focus Area STARTS here---------------------------- */
status_t ISecCameraHardware::findCenter(struct FocusArea *focusArea,
        struct FocusPoint *center)
{
    /* range check */
    if ((focusArea->top > focusArea->bottom) || (focusArea->right < focusArea->left)) {
        ALOGE("findCenter: Invalid value range");
        return -EINVAL;
    }

    center->x = (focusArea->left + focusArea->right) / 2;
    center->y = (focusArea->top + focusArea->bottom) / 2;

    /* ALOGV("%s: center point (%d, %d)", __func__, center->x, center->y); */
    return NO_ERROR;
}

status_t ISecCameraHardware::normalizeArea(struct FocusPoint *center)
{
    struct FocusPoint tmpPoint;
    size_t hRange, vRange;
    double hScale, vScale;

    tmpPoint.x = center->x;
    tmpPoint.y = center->y;

    /* ALOGD("%s: before x = %d, y = %d", __func__, tmpPoint.x, tmpPoint.y); */

    hRange = FOCUS_AREA_RIGHT - FOCUS_AREA_LEFT;
    vRange = FOCUS_AREA_BOTTOM - FOCUS_AREA_TOP;
    hScale = (double)mPreviewSize.height / (double) hRange;
    vScale = (double)mPreviewSize.width / (double) vRange;

    /* Nomalization */
    /* ALOGV("normalizeArea: mPreviewSize.width = %d, mPreviewSize.height = %d",
            mPreviewSize.width, mPreviewSize.height);
     */

    tmpPoint.x = (center->x + vRange / 2) * vScale;
    tmpPoint.y = (center->y + hRange / 2) * hScale;

    center->x = tmpPoint.x;
    center->y = tmpPoint.y;

    if (center->x == 0 && center->y == 0) {
        ALOGE("normalizeArea: Invalid focus center point");
        return -EINVAL;
    }

    return NO_ERROR;
}

status_t ISecCameraHardware::checkArea(ssize_t top,
        ssize_t left,
        ssize_t bottom,
        ssize_t right,
        ssize_t weight)
{
    /* Handles the invalid regin corner case. */
    if ((0 == top) && (0 == left) && (0 == bottom) && (0 == right) && (0 == weight)) {
        ALOGE("checkArea: error, All values are zero");
        return NO_ERROR;
    }

    if ((FOCUS_AREA_WEIGHT_MIN > weight) || (FOCUS_AREA_WEIGHT_MAX < weight)) {
        ALOGE("checkArea: error, Camera area weight is invalid %d", weight);
        return -EINVAL;
    }

    if ((FOCUS_AREA_TOP > top) || (FOCUS_AREA_BOTTOM < top)) {
        ALOGE("checkArea: error, Camera area top coordinate is invalid %d", top );
        return -EINVAL;
    }

    if ((FOCUS_AREA_TOP > bottom) || (FOCUS_AREA_BOTTOM < bottom)) {
        ALOGE("checkArea: error, Camera area bottom coordinate is invalid %d", bottom );
        return -EINVAL;
    }

    if ((FOCUS_AREA_LEFT > left) || (FOCUS_AREA_RIGHT < left)) {
        ALOGE("checkArea: error, Camera area left coordinate is invalid %d", left );
        return -EINVAL;
    }

    if ((FOCUS_AREA_LEFT > right) || (FOCUS_AREA_RIGHT < right)) {
        ALOGE("checkArea: error, Camera area right coordinate is invalid %d", right );
        return -EINVAL;
    }

    if (left >= right) {
        ALOGE("checkArea: error, Camera area left larger than right");
        return -EINVAL;
    }

    if (top >= bottom) {
        ALOGE("checkArea: error, Camera area top larger than bottom");
        return -EINVAL;
    }

    return NO_ERROR;
}

/* TODO : muliple focus area is not supported yet */
status_t ISecCameraHardware::parseAreas(const char *area,
        size_t areaLength,
        struct FocusArea *focusArea,
        int *num_areas)
{
    status_t ret = NO_ERROR;
    char *ctx;
    char *pArea = NULL;
    char *pEnd = NULL;
    const char *startToken = "(";
    const char endToken = ')';
    const char sep = ',';
    ssize_t left, top, bottom, right, weight;
    char *tmpBuffer = NULL;

    if (( NULL == area ) || ( 0 >= areaLength)) {
        ALOGE("parseAreas: error, area is NULL or areaLength is less than 0");
        return -EINVAL;
    }

    tmpBuffer = (char *)malloc(areaLength);
    if (NULL == tmpBuffer) {
        ALOGE("parseAreas: error, tmpBuffer is NULL");
        return -ENOMEM;
    }

    memcpy(tmpBuffer, area, areaLength);

    pArea = strtok_r(tmpBuffer, startToken, &ctx);

    do {
		char *pStart = NULL;
        pStart = pArea;
        if (NULL == pStart) {
            ALOGE("parseAreas: error, Parsing of the left area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            left = static_cast<ssize_t>(strtol(pStart, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the top area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            top = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the right area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            right = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the bottom area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            bottom = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the weight area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            weight = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (endToken != *pEnd) {
            ALOGE("parseAreas: error, malformed area!");
            ret = -EINVAL;
            break;
        }

        ret = checkArea(top, left, bottom, right, weight);
        if (NO_ERROR != ret)
            break;

        /*
        ALOGV("parseAreas: Area parsed [%dx%d, %dx%d] %d",
                ( int ) left,
                ( int ) top,
                ( int ) right,
                ( int ) bottom,
                ( int ) weight);
         */

        pArea = strtok_r(NULL, startToken, &ctx);

        focusArea->left = (int)left;
        focusArea->top = (int)top;
        focusArea->right = (int)right;
        focusArea->bottom = (int)bottom;
        focusArea->weight = (int)weight;
        (*num_areas)++;
    } while ( NULL != pArea );

    if (NULL != tmpBuffer)
        free(tmpBuffer);

    return ret;
}

/* TODO : muliple focus area is not supported yet */
status_t ISecCameraHardware::setFocusAreas(const CameraParameters &params)
{
    if (!IsAutoFocusSupported())
        return NO_ERROR;

    const char *str = params.get(CameraParameters::KEY_FOCUS_AREAS);
    const char *prevStr = mParameters.get(CameraParameters::KEY_FOCUS_AREAS);
    if ((str == NULL) || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    struct FocusArea focusArea;
    struct FocusPoint center;
    int err, num_areas = 0;
    const char *maxFocusAreasStr = params.get(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS);
    if (!maxFocusAreasStr) {
        ALOGE("setFocusAreas: error, KEY_MAX_NUM_FOCUS_AREAS is NULL");
        return NO_ERROR;
    }

    int maxFocusAreas = atoi(maxFocusAreasStr);
    if (!maxFocusAreas) {
        ALOGD("setFocusAreas: FocusAreas is not supported");
        return NO_ERROR;
    }

    /* Focus area parse here */
    err = parseAreas(str, (strlen(str) + 1), &focusArea, &num_areas);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("setFocusAreas: error, parseAreas %s", str);
        return BAD_VALUE;
    }
    if (CC_UNLIKELY(num_areas > maxFocusAreas)) {
        ALOGE("setFocusAreas: error, the number of areas is more than max");
        return BAD_VALUE;
    }

    /* find center point */
    err = findCenter(&focusArea, &center);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("setFocusAreas: error, findCenter");
        return BAD_VALUE;
    }

    /* Normalization */
    err = normalizeArea(&center);
    if (err < 0) {
        ALOGE("setFocusAreas: error, normalizeArea");
        return BAD_VALUE;
    }

    ALOGD("setFocusAreas: FocusAreas(%s) to (%d, %d)", str, center.x, center.y);

    mParameters.set(CameraParameters::KEY_FOCUS_AREAS, str);

//#ifdef ENABLE_TOUCH_AF
    if (CC_UNLIKELY(mFocusArea != V4L2_FOCUS_AREA_TRACKING)) {
        err = nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSX, center.x);
        if (CC_UNLIKELY(err < 0)) {
            ALOGE("setFocusAreas: error, SET_TOUCH_AF_POSX");
            return UNKNOWN_ERROR;
        }

        err = nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSY, center.y);
        if (CC_UNLIKELY(err < 0)) {
            ALOGE("setFocusAreas: error, SET_TOUCH_AF_POSX");
            return UNKNOWN_ERROR;
        }

        return nativeSetParameters(CAM_CID_SET_TOUCH_AF, 1);
    } else{
        return nativeSetParameters(CAM_CID_SET_TOUCH_AF, 0);
    }
//#endif

    return NO_ERROR;
}

/* -------------------Focus Area ENDS here---------------------------- */
#if VENDOR_FEATURE
status_t ISecCameraHardware::setWeather(const CameraParameters& params)
{
	int val = params.getInt(CameraParameters::KEY_WEATHER);
	int prevVal = mParameters.getInt(CameraParameters::KEY_WEATHER);
	if (prevVal == val)
		return NO_ERROR;

	if (CC_UNLIKELY(val < 0 || val > 5)) {
		ALOGE("setWeather: error, invalid value(%d)", val);
		return BAD_VALUE;
	}

	ALOGD("setWeather: %d", val);

	mWeather = val;
	mParameters.set(CameraParameters::KEY_WEATHER, val);

	return NO_ERROR;
}

status_t ISecCameraHardware::setCityId(const CameraParameters& params)
{
	long long int val = params.getInt64(CameraParameters::KEY_CITYID);

	if (val <= 0) {
		ALOGD("setCityid : null value");
		mCityId = 0;
		mParameters.set(CameraParameters::KEY_CITYID, 0);
	} else {
		mCityId = val;
		const char *v = params.get(CameraParameters::KEY_CITYID);
		mParameters.set(CameraParameters::KEY_CITYID, v);
		ALOGD("setCityid: %s", v);
	}

	return NO_ERROR;
}

status_t ISecCameraHardware::setBracketAEBvalue(const CameraParameters& params)
{
    int val = params.getInt(CameraParameters::KEY_BRACKET_AEB);
    int prevVal = mParameters.getInt(CameraParameters::KEY_BRACKET_AEB);

    /* AEB Value : 1~6 */
    if (val > 6 || val < 0 || prevVal == val)
        return NO_ERROR;

    ALOGD("setBracketAEBvalue: %d", val);
    mParameters.set(CameraParameters::KEY_BRACKET_AEB, val);

    return nativeSetParameters(CAM_CID_BRACKET_AEB, val);
}

status_t ISecCameraHardware::setIso(const CameraParameters &params)
{
#if 0
    const char *str = params.get(SecCameraParameters::KEY_ISO);
    const char *prevStr = mParameters.get(SecCameraParameters::KEY_ISO);
    if (str == NULL || (prevStr && !strcmp(str, prevStr))) {
        ALOGD("setIso: 1 str:%s, prevStr:%s", str, prevStr);
        return NO_ERROR;
    }
    if (prevStr == NULL && !strcmp(str, isos[0].desc)) {  /* default */
        ALOGD("setIso: 2 str:%s, prevStr:%s", str, prevStr);
        return NO_ERROR;
    }

    int val;
retry:
    val = SecCameraParameters::lookupAttr(isos, ARRAY_SIZE(isos), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setIso: warning, not supported value(%s)", str);
        /* str = reinterpret_cast<const char*>(isos[0].desc);
        goto retry;
         */
        return BAD_VALUE;
    }

    ALOGD("setIso: %s", str);
    mParameters.set(SecCameraParameters::KEY_ISO, str);

    return nativeSetParameters(CAM_CID_ISO, val);
#else
    const char *str = params.get(CameraParameters::KEY_ISO);
    const char *prevStr = mParameters.get(CameraParameters::KEY_ISO);
    if (str == NULL || (prevStr && !strcmp(str, prevStr))) {
        return NO_ERROR;
    }
    if (prevStr == NULL && !strcmp(str, isos[0].desc)) {  /* default */
        return NO_ERROR;
    }

    int val;
retry:
    val = SecCameraParameters::lookupAttr(isos, ARRAY_SIZE(isos), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setIso: warning, not supported value(%s)", str);
        /* str = reinterpret_cast<const char*>(isos[0].desc);
        goto retry;
         */
        return BAD_VALUE;
    }

    ALOGD("setIso: %s", str);
    mParameters.set(CameraParameters::KEY_ISO, str);

    return nativeSetParameters(CAM_CID_ISO, val);

#endif
}
#endif /* VENDOR_FEATURE */

status_t ISecCameraHardware::setBrightness(const CameraParameters &params)
{
    int val;
    if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
        val = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    } else {
        switch (mSceneMode) {
        case SCENE_MODE_BEACH_SNOW:
            val = 2;
            break;

        default:
            val = 0;
            break;
        }
    }

    int prevVal = mParameters.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    int max = mParameters.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
    int min = mParameters.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < min || val > max)) {
        ALOGE("setBrightness: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGV("setBrightness: %d", val);
    mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, val);

    if(mPASMMode == MODE_SNOW) {
        ALOGD("Current is snow mode. AP don't set EV value when snow-shot.");
        return NO_ERROR;
    } else {
        return nativeSetParameters(CAM_CID_BRIGHTNESS, val);
    }
}

status_t ISecCameraHardware::setWhiteBalance(const CameraParameters &params)
{
    const char *str;
    if (mSamsungApp) {
        if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
            str = params.get(CameraParameters::KEY_WHITE_BALANCE);
        } else {
            switch (mSceneMode) {
            case SCENE_MODE_SUNSET:
            case SCENE_MODE_CANDLE_LIGHT:
                str = CameraParameters::WHITE_BALANCE_DAYLIGHT;
                break;

            case SCENE_MODE_DUSK_DAWN:
                str = CameraParameters::WHITE_BALANCE_FLUORESCENT;
                break;

            default:
                str = CameraParameters::WHITE_BALANCE_AUTO;
                break;
            }
        }
    } else
        str = params.get(CameraParameters::KEY_WHITE_BALANCE);

    const char *prevStr = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(whiteBalances, ARRAY_SIZE(whiteBalances), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setWhiteBalance: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(whiteBalances[0].desc);
        goto retry;
    }

    ALOGV("setWhiteBalance: %s", str);
    mParameters.set(CameraParameters::KEY_WHITE_BALANCE, str);

    return nativeSetParameters(CAM_CID_WHITE_BALANCE, val);
}

status_t ISecCameraHardware::setFlash(const CameraParameters &params)
{
    if (!IsFlashSupported())
        return NO_ERROR;

    const char *str;
    if (mSamsungApp) {
        if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
            str = params.get(CameraParameters::KEY_FLASH_MODE);
        } else {
            switch (mSceneMode) {
            case SCENE_MODE_PORTRAIT:
            case SCENE_MODE_PARTY_INDOOR:
            case SCENE_MODE_BACK_LIGHT:
            case SCENE_MODE_TEXT:
                str = params.get(CameraParameters::KEY_FLASH_MODE);
                break;

            default:
                str = CameraParameters::FLASH_MODE_OFF;
                break;
            }
        }
    } else {
        str = params.get(CameraParameters::KEY_FLASH_MODE);
    }

    const char *prevStr = mParameters.get(CameraParameters::KEY_FLASH_MODE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(flashModes, ARRAY_SIZE(flashModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setFlash: warning, not supported value(%s)", str);
        return BAD_VALUE; /* return BAD_VALUE if invalid parameter */
    }

    ALOGV("setFlash: %s", str);
    mFlashMode = (cam_flash_mode)val;
    mParameters.set(CameraParameters::KEY_FLASH_MODE, str);

    return nativeSetParameters(CAM_CID_FLASH, val);
}

status_t ISecCameraHardware::setMetering(const CameraParameters &params)
{
    const char *str;
    if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
        str = params.get(SecCameraParameters::KEY_METERING);
    } else {
        switch (mSceneMode) {
        case SCENE_MODE_LANDSCAPE:
            str = SecCameraParameters::METERING_MATRIX;
            break;

        case SCENE_MODE_BACK_LIGHT:
            if (mFlashMode == V4L2_FLASH_MODE_OFF)
                str = SecCameraParameters::METERING_SPOT;
            else
                str = SecCameraParameters::METERING_CENTER;
            break;

        default:
            str = SecCameraParameters::METERING_CENTER;
            break;
        }
    }

    const char *prevStr = mParameters.get(SecCameraParameters::KEY_METERING);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;
    if (prevStr == NULL && !strcmp(str, meterings[0].desc)) /* default */
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(meterings, ARRAY_SIZE(meterings), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setMetering: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(meterings[0].desc);
        goto retry;
    }

    ALOGD("setMetering: %s", str);
    mParameters.set(SecCameraParameters::KEY_METERING, str);

    return nativeSetParameters(CAM_CID_METERING, val);
}

status_t ISecCameraHardware::setMeteringAreas(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_METERING_AREAS);
    const char *prevStr = mParameters.get(CameraParameters::KEY_METERING_AREAS);
     if ((str == NULL) || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    /* Metering area use same strcut with Focus area */
    struct FocusArea meteringArea;
    int err, num_areas = 0;
    const char *maxMeteringAreasStr = params.get(CameraParameters::KEY_MAX_NUM_METERING_AREAS);
    if (!maxMeteringAreasStr) {
        ALOGE("setMeteringAreas: error, KEY_MAX_NUM_METERING_AREAS is NULL");
        return NO_ERROR;
    }

    int maxMeteringAreas = atoi(maxMeteringAreasStr);
    if (!maxMeteringAreas) {
        ALOGD("setMeteringAreas: FocusAreas is not supported");
        return NO_ERROR;
    }

    /* Metering area parse and check(max value) here */
    err = parseAreas(str, (strlen(str) + 1), &meteringArea, &num_areas);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("setMeteringAreas: error, parseAreas %s", str);
        return BAD_VALUE;
    }
    if (CC_UNLIKELY(num_areas > maxMeteringAreas)) {
        ALOGE("setMeteringAreas: error, the number of areas is more than max");
        return BAD_VALUE;
    }

    ALOGD("setMeteringAreas = %s\n", str);
    mParameters.set(CameraParameters::KEY_METERING_AREAS, str);

    return NO_ERROR;
}

status_t ISecCameraHardware::setFocusMode(const CameraParameters &params)
{
	status_t Ret = NO_ERROR;

    const char *str = params.get(CameraParameters::KEY_FOCUS_MODE);
    const char *prevStr = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int count, val;
    const cam_strmap_t *focusModes;

    if (mCameraId == CAMERA_FACING_BACK) {
        count = ARRAY_SIZE(backFocusModes);
        focusModes = backFocusModes;
    } else {
        count = ARRAY_SIZE(frontFocusModes);
        focusModes = frontFocusModes;
    }

retry:
    val = SecCameraParameters::lookupAttr(focusModes, count, str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setFocusMode: warning, not supported value(%s)", str);
        return BAD_VALUE; /* return BAD_VALUE if invalid parameter */
    }

    ALOGD("setFocusMode: %s", str);
    mFocusMode = (cam_focus_mode)val;
    mParameters.set(CameraParameters::KEY_FOCUS_MODE, str);
    if (val == V4L2_FOCUS_MODE_MACRO) {
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES,
                B_KEY_MACRO_FOCUS_DISTANCES_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES,
                B_KEY_NORMAL_FOCUS_DISTANCES_VALUE);
    }


    if (mCameraId == CAMERA_FACING_BACK) {
        if (val == V4L2_FOCUS_MODE_MACRO) {
            mFocusRange = V4L2_FOCUS_RANGE_MACRO;
            nativeSetParameters(CAM_CID_FOCUS_RANGE, mFocusRange);
        } else if (val == V4L2_FOCUS_MODE_AUTO) {
            if (mMovieMode) {
                mFocusRange = V4L2_FOCUS_RANGE_AUTO_MACRO;
                nativeSetParameters(CAM_CID_FOCUS_RANGE, mFocusRange);
            } else {
                mFocusRange = V4L2_FOCUS_RANGE_AUTO;
                nativeSetParameters(CAM_CID_FOCUS_RANGE, mFocusRange);
            }
        }
    }

#if 0
    if (mCameraId == CAMERA_FACING_BACK) {
        if (val == V4L2_FOCUS_MODE_SELFSHOT) {
            ALOGD("setFocusMode: FOCUS_MODE_SELFSHOT");
            nativeSetParameters(CAM_CID_SET_FD_FOCUS_SELECT, 0x2);

            setFaceDetection(V4L2_FACE_DETECTION_NORMAL);
            mSelfShotFDReading = 1;
        } else {
            ALOGD("setFocusMode: not FOCUS_MODE_SELFSHOT");
            nativeSetParameters(CAM_CID_SET_FD_FOCUS_SELECT, 0x0);

            //setFaceDetection(V4L2_FACE_DETECTION_OFF);
            roi_x_pos = 0;
            roi_y_pos = 0;
            mSelfShotFDReading = 0;
        }
    }
#endif
    return nativeSetParameters(CAM_CID_FOCUS_MODE, val);
}

status_t ISecCameraHardware::setEffect(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_EFFECT);
    const char *prevStr = mParameters.get(CameraParameters::KEY_EFFECT);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(effects, ARRAY_SIZE(effects), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setEffect: warning, not supported value(%s)", str);
#if 0
        str = reinterpret_cast<const char*>(effects[0].desc);
        goto retry;
#else
        return BAD_VALUE;
#endif	/* FOR HAL TEST */
    }

    ALOGV("setEffect: %s", str);
    mParameters.set(CameraParameters::KEY_EFFECT, str);

    return nativeSetParameters(CAM_CID_EFFECT, val);
}

status_t ISecCameraHardware::setZoom(const CameraParameters &params)
{
    if (!mZoomSupport)
        return NO_ERROR;

    int val = params.getInt(CameraParameters::KEY_ZOOM);
    int prevVal = mParameters.getInt(CameraParameters::KEY_ZOOM);
    int err;

    if (val == -1 || prevVal == val)
        return NO_ERROR;

    int max = params.getInt(CameraParameters::KEY_MAX_ZOOM);
    if (CC_UNLIKELY(val < 0 || val > max)) {
        ALOGE("setZoom: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

#if VENDOR_FEATURE
    if (prevVal != val ) {
	    ALOGD("setZoom succ: %d->%d, curr:%d/%d", prevVal, val, mZoomCurrLevel, max);
	    mZoomSetVal = val;
     	    mParameters.set(CameraParameters::KEY_ZOOM, val);

	    if (val != mZoomCurrLevel && !mZoomStatus ) {
		    err = nativeSetParameters(CAM_CID_ZOOM, val);
		    if (err < 0) {
			    ALOGE("%s: setZoom failed", __func__);
			    return err;
		    }
	    }
    }

    return NO_ERROR;
#else
    mParameters.set(CameraParameters::KEY_ZOOM, val);

    if (mEnableDZoom)
        /* Set AP zoom ratio */
        return nativeSetZoomRatio(val);
    else
        /* Set ISP/sensor zoom ratio */
        return nativeSetParameters(CAM_CID_ZOOM, val);
#endif
}

status_t ISecCameraHardware::setDzoom(const CameraParameters& params)
{
    int val = params.getInt("dzoom");
    int prevVal = mParameters.getInt("dzoom");
    int err;

    if (prevVal == val)
        return NO_ERROR;
    if (val < V4L2_ZOOM_LEVEL_0 || val >= 12) {
        ALOGE("invalid value for DZOOM val = %d", val);
        return BAD_VALUE;
    }

    ALOGD("setDZoom LEVEL %d->%d", prevVal, val);
    mParameters.set("dzoom", val);

    err = nativeSetParameters(CAM_CID_DZOOM, val);
    if (err < 0) {
        ALOGE("%s: setDZoom failed", __func__);
        return err;
    }

    return NO_ERROR;
}

status_t ISecCameraHardware::setZoomActionMethod(const CameraParameters& params)
{
    /* 0 : key, 1: zoom-ring */
    int err = 0;
    int actionMethod = params.getInt("zoom-ring" /*CameraParameters::KEY_ZOOM_RING*/);
    int prevActionMethod= mParameters.getInt("zoom-ring" /*CameraParameters::KEY_ZOOM_RING*/);

    if ((actionMethod == prevActionMethod) ||
        (actionMethod < 0))
        return NO_ERROR;

    mParameters.set("zoom-ring" /*CameraParameters::KEY_ZOOM_RING*/, actionMethod);
    mZoomActionMethod = actionMethod;

    err = nativeSetParameters(CAM_CID_CAMERA_ZOOM_ACTION_METHOD, actionMethod);
    if (err < 0) {
        ALOGE("%s : setZoomActionMethod failed", __func__);
        return err;
    }
    return NO_ERROR;
}

#if VENDOR_FEATURE
status_t ISecCameraHardware::setOpticalZoomCtrl(const CameraParameters& params)
{
    const char *str = params.get(CameraParameters::KEY_ZOOM_ACTION);
    const char *prevStr = mParameters.get(CameraParameters::KEY_ZOOM_ACTION);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;

    val = SecCameraParameters::lookupAttr(opticalzoomValues, ARRAY_SIZE(opticalzoomValues), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setOpticalZoomCtrl: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGD("setOpticalZoomCtrl: %s", str);

    mParameters.set(CameraParameters::KEY_ZOOM_ACTION, str);

    if (val == V4L2_OPTICAL_ZOOM_STOP) {
        // Initialize Zoom Speed Value
        mParameters.set("zoom-speed", -1);
    }
#ifdef ZOOM_CTRL_THREAD
    zoom_cmd = val;
    return NO_ERROR;
#else
    return nativeSetParameters(CAM_CID_OPTICAL_ZOOM_CTRL, val);
#endif
}
#endif /* VENDOR_FEATURE */

status_t ISecCameraHardware::setZoomSpeed(const CameraParameters& params)
{
    int speed = params.getInt("zoom-speed");
    int prevSpeed = mParameters.getInt("zoom-speed");

    if ( speed == -1 || speed == prevSpeed )
        return NO_ERROR;

	mParameters.set("zoom-speed", speed);

    return nativeSetParameters(CAM_CID_CAMERA_ZOOM_SPEED, speed);
}

#if VENDOR_FEATURE
status_t ISecCameraHardware::setSmartZoommode(const CameraParameters& params)
{
    const char *str = params.get("smartzoom"/*CameraParameters::KEY_SMART_ZOOM*/);
    const char *prevStr = mParameters.get("smartzoom"/*CameraParameters::KEY_SMART_ZOOM*/);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
    val = SecCameraParameters::lookupAttr(SmartZoomModes, ARRAY_SIZE(SmartZoomModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setSmartZoommode: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGD("setSmartZoommode: %s", str);
    mParameters.set("smartzoom"/*CameraParameters::KEY_SMART_ZOOM*/, str);

    return nativeSetParameters(CAM_CID_SMART_ZOOM, val);
}
#endif /* VENDOR_FEATURE */

status_t ISecCameraHardware::setAutoContrast(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_AUTO_CONTRAST);
    int prevVal = mParameters.getInt(SecCameraParameters::KEY_AUTO_CONTRAST);
    if (val == -1 || prevVal == val)
        return NO_ERROR;
    if (prevVal == -1 && val == 0)  /* default */
        return NO_ERROR;

    ALOGV("setAutoContrast: %d", val);
    mParameters.set(SecCameraParameters::KEY_AUTO_CONTRAST, val);

    return nativeSetParameters(CAM_CID_AUTO_CONTRAST, val);
}


status_t ISecCameraHardware::setColorAdjust(const CameraParameters& params)
{
    int val = params.getInt(SecCameraParameters::KEY_COLOR);
    int prevVal = mParameters.getInt(SecCameraParameters::KEY_COLOR);

    /* Value : 0~22 */
    if (val > 22 || val < 0 || prevVal == val)
        return NO_ERROR;

    ALOGE("setColorAdjust: %d", val);

    mParameters.set(SecCameraParameters::KEY_COLOR, val);

    return nativeSetParameters(CAM_CID_COLOR_ADJUST, val);
}

status_t ISecCameraHardware::setSharpness(const CameraParameters& params)
{
    int val = params.getInt("sharpness");
    int prevVal = mParameters.getInt("sharpness");
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < -2 || val > 2)) {
        ALOGE("setSharpness: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGD("setSharpness: %d", val);
    mParameters.set("sharpness", val);
	
    if (mSceneMode == SCENE_MODE_NONE)
        return nativeSetParameters(CAM_CID_SHARPNESS, val + 2);

    return NO_ERROR;
}

status_t ISecCameraHardware::setContrast(const CameraParameters& params)
{
    int val = params.getInt("contrast");
    int prevVal = mParameters.getInt("contrast");
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < -2 || val > 2)) {
        ALOGE("setContrast: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGD("setContrast: %d", val);
    mParameters.set("contrast", val);

    if (mSceneMode == SCENE_MODE_NONE)
        return nativeSetParameters(CAM_CID_CONTRAST, val + 2);

    return NO_ERROR;
}

status_t ISecCameraHardware::setSaturation(const CameraParameters& params)
{
    int val = params.getInt("saturation");
    int prevVal = mParameters.getInt("saturation");
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < -2 || val > 2)) {
        ALOGE("setSaturation: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGD("setSaturation: %d", val);
    mParameters.set("saturation", val);

    if (mSceneMode == SCENE_MODE_NONE)
        return nativeSetParameters(CAM_CID_SATURATION, val + 2);

    return NO_ERROR;
}

#if VENDOR_FEATURE
status_t ISecCameraHardware::setImageStabilizermode(const CameraParameters& params)
{
    const char *str  = params.get(CameraParameters::KEY_IMAGE_STABILIZER);
    const char *prevStr = mParameters.get(CameraParameters::KEY_IMAGE_STABILIZER);
    //ALOGV("setImageStabilizermode ~~~~~~~~~~~~~~ str : %s, prevStr : %s",str, prevStr);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
    int err;
retry:
    val = SecCameraParameters::lookupAttr(imagestabilizerModes, ARRAY_SIZE(imagestabilizerModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setImageStabilizermode: warning, not supported value(%s)", str);
        return NO_ERROR;
    }

    ALOGD("setImageStabilizermode: %s", str);
    mParameters.set(CameraParameters::KEY_IMAGE_STABILIZER, str);

    err = nativeSetParameters(CAM_CID_IMAGE_STABILIZER, val);
    if (err < 0 ) {
        ALOGE("setImageStabilizermode is failed\n");
    }

    return NO_ERROR;
}
#endif /* VENDOR_FEATURE */

status_t ISecCameraHardware::setAntiShake(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_ANTI_SHAKE);
    int prevVal = mParameters.getInt(SecCameraParameters::KEY_ANTI_SHAKE);
    if (val == -1 || prevVal == val)
        return NO_ERROR;
    if (prevVal == -1 && val == 0)  /* default */
        return NO_ERROR;

    ALOGV("setAntiShake: %d", val);
    mParameters.set(SecCameraParameters::KEY_ANTI_SHAKE, val);

    return nativeSetParameters(CAM_CID_ANTISHAKE, val);
}

status_t ISecCameraHardware::setFaceBeauty(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_FACE_BEAUTY);
    int prevVal = mParameters.getInt(SecCameraParameters::KEY_FACE_BEAUTY);
    if (val == -1 || prevVal == val)
        return NO_ERROR;
    if (prevVal == -1 && val == 0)  /* default */
        return NO_ERROR;

    ALOGV("setFaceBeauty: %d", val);
    mParameters.set(SecCameraParameters::KEY_FACE_BEAUTY, val);

    return nativeSetParameters(CAM_CID_FACE_BEAUTY, val);
}

status_t ISecCameraHardware::setBlur(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_BLUR);
    int prevVal = mParameters.getInt(SecCameraParameters::KEY_BLUR);
    if (val == -1 || prevVal == val)
        return NO_ERROR;
    if (prevVal == -1 && val == 0)  /* default */
        return NO_ERROR;

    ALOGV("setBlur: %d", val);
    mParameters.set(SecCameraParameters::KEY_BLUR, val);
    if (val > 0)
        setDropFrame(2);

    return nativeSetParameters(CAM_CID_BLUR, val);
}

int ISecCameraHardware::checkFnumber(int f_val, int zoomLevel)
{
    int err = NO_ERROR;

    if (f_val == 0) {
        ALOGD("checkFnumber: f number is set to default value. f_val = %d",
                f_val);
        return err;
    }

    switch (zoomLevel) {
    case 0:
        if (f_val != 31 && f_val != 90)
        err = BAD_VALUE;
        break;
    case 1:
        if (f_val != 34 && f_val != 95)
        err = BAD_VALUE;
        break;
    case 2:
        if (f_val != 35 && f_val != 100)
        err = BAD_VALUE;
        break;
    case 3:
        if (f_val != 37 && f_val != 104)
        err = BAD_VALUE;
        break;
    case 4:
        if (f_val != 38 && f_val != 109)
        err = BAD_VALUE;
        break;
    case 5:
        if (f_val != 40 && f_val != 113)
        err = BAD_VALUE;
        break;
    case 6:
        if (f_val != 41 && f_val != 116)
        err = BAD_VALUE;
        break;
    case 7:
        if (f_val != 42 && f_val != 119)
        err = BAD_VALUE;
        break;
    case 8:
        if (f_val != 43 && f_val != 122)
        err = BAD_VALUE;
        break;
    case 9:
        if (f_val != 44 && f_val != 125)
        err = BAD_VALUE;
        break;
    case 10:
        if (f_val != 45 && f_val != 127)
        err = BAD_VALUE;
        break;
    case 11:
        if (f_val != 46 && f_val != 129)
        err = BAD_VALUE;
        break;
    case 12:
        if (f_val != 46 && f_val != 131)
        err = BAD_VALUE;
        break;
    case 13:
        if (f_val != 47 && f_val != 134)
        err = BAD_VALUE;
        break;
    case 14:
        if (f_val != 48 && f_val != 136)
        err = BAD_VALUE;
        break;
    case 15:
        if (f_val != 49 && f_val != 139)
        err = BAD_VALUE;
        break;
    case 16:
        if (f_val != 50 && f_val != 142)
        err = BAD_VALUE;
        break;
    case 17:
        if (f_val != 51 && f_val != 145)
        err = BAD_VALUE;
        break;
    case 18:
        if (f_val != 52 && f_val != 148)
        err = BAD_VALUE;
        break;
    case 19:
        if (f_val != 54 && f_val != 152)
        err = BAD_VALUE;
        break;
    case 20:
        if (f_val != 55 && f_val != 156)
        err = BAD_VALUE;
        break;
    case 21:
        if (f_val != 56 && f_val != 159)
        err = BAD_VALUE;
        break;
    case 22:
        if (f_val != 58 && f_val != 163)
        err = BAD_VALUE;
        break;
    case 23:
        if (f_val != 59 && f_val != 167)
        err = BAD_VALUE;
        break;
    case 24:
        if (f_val != 60 && f_val != 170)
        err = BAD_VALUE;
        break;
    case 25:
        if (f_val != 61 && f_val != 173)
        err = BAD_VALUE;
        break;
    case 26:
        if (f_val != 62 && f_val != 176)
        err = BAD_VALUE;
        break;
    case 27:
        if (f_val != 63 && f_val != 179)
        err = BAD_VALUE;
        break;
    case 28:
        if (f_val != 63 && f_val != 182)
        err = BAD_VALUE;
        break;
    case 29:
        if (f_val != 63 && f_val != 184)
        err = BAD_VALUE;
        break;
    default:
        err = NO_ERROR;
        break;
    }
    return err;
}

static void getAntiBandingFromLatinMCC(char *temp_str, int tempStrLength)
{
    char value[10];
    char country_value[10];

    if (temp_str == NULL) {
        ALOGE("ERR(%s) (%d): temp_str is null", __FUNCTION__, __LINE__);
        return;
    }

    memset(value, 0x00, sizeof(value));
    memset(country_value, 0x00, sizeof(country_value));
    if (!property_get("gsm.operator.numeric", value,"")) {
        strncpy(temp_str, CameraParameters::ANTIBANDING_60HZ, tempStrLength);
        return;
    }
    memcpy(country_value, value, 3);

    /** MCC Info. Jamaica : 338 / Argentina : 722 / Chile : 730 / Paraguay : 744 / Uruguay : 748  **/
    if (strstr(country_value,"338") || strstr(country_value,"722") ||
        strstr(country_value,"730") || strstr(country_value,"744") ||
        strstr(country_value,"748"))
        strncpy(temp_str, CameraParameters::ANTIBANDING_50HZ, tempStrLength);
    else
        strncpy(temp_str, CameraParameters::ANTIBANDING_60HZ, tempStrLength);
}

static int IsLatinOpenCSC()
{
    char sales_code[5] = {0};
    property_get("ro.csc.sales_code", sales_code, "");
    if (strstr(sales_code,"TFG") || strstr(sales_code,"TPA") || strstr(sales_code,"TTT") || strstr(sales_code,"JDI") || strstr(sales_code,"PCI") )
        return 1;
    else
        return 0;
}

void ISecCameraHardware::chooseAntiBandingFrequency()
{
#if NOTDEFINED
    status_t ret = NO_ERROR;
    int LatinOpenCSClength = 5;
    char *LatinOpenCSCstr = NULL;
    char *CSCstr = NULL;
    const char *defaultStr = "50hz";

    if (IsLatinOpenCSC()) {
        LatinOpenCSCstr = (char *)malloc(LatinOpenCSClength);
        if (LatinOpenCSCstr == NULL) {
            ALOGE("LatinOpenCSCstr is NULL");
            CSCstr = (char *)defaultStr;
            memset(mAntiBanding, 0, sizeof(mAntiBanding));
            strcpy(mAntiBanding, CSCstr);
            return;
        }
        memset(LatinOpenCSCstr, 0, LatinOpenCSClength);

        getAntiBandingFromLatinMCC(LatinOpenCSCstr, LatinOpenCSClength);
        CSCstr = LatinOpenCSCstr;
    } else {
        CSCstr = (char *)SecNativeFeature::getInstance()->getString("CscFeature_Camera_CameraFlicker");
    }

    if (CSCstr == NULL || strlen(CSCstr) == 0) {
        CSCstr = (char *)defaultStr;
    }
    memset(mAntiBanding, 0, sizeof(mAntiBanding));
    strcpy(mAntiBanding, CSCstr);
    ALOGV("mAntiBanding = %s",mAntiBanding);

    if (LatinOpenCSCstr != NULL)
       free(LatinOpenCSCstr);
#endif
}

status_t ISecCameraHardware::setAntiBanding()
{
    status_t ret = NO_ERROR;
    const char *prevStr = mParameters.get(CameraParameters::KEY_ANTIBANDING);

    if (prevStr && !strcmp(mAntiBanding, prevStr))
        return NO_ERROR;

retry:
    int val = SecCameraParameters::lookupAttr(antibandings, ARRAY_SIZE(antibandings), mAntiBanding);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGE("setAntiBanding: error, not supported value(%s)", mAntiBanding);
        return BAD_VALUE;
    }
    ALOGD("setAntiBanding: %s", mAntiBanding);
    mParameters.set(CameraParameters::KEY_ANTIBANDING, mAntiBanding);
    return nativeSetParameters(CAM_CID_ANTIBANDING, val);
}

status_t ISecCameraHardware::setAntiBanding(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_ANTIBANDING);
    const char *prevStr = mParameters.get(CameraParameters::KEY_ANTIBANDING);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;

retry:
    val = SecCameraParameters::lookupAttr(antibandings, ARRAY_SIZE(antibandings), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setAntiBanding: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(antibandings[0].desc);
        goto retry;
    }

    ALOGV("setAntiBanding: %s, val: %d", str, val);
    mParameters.set(CameraParameters::KEY_ANTIBANDING, str);

    return nativeSetParameters(CAM_CID_ANTIBANDING, val);
}

status_t ISecCameraHardware::setGps(const CameraParameters &params)
{
    const char *latitude = params.get(CameraParameters::KEY_GPS_LATITUDE);
    const char *logitude = params.get(CameraParameters::KEY_GPS_LONGITUDE);
    const char *altitude = params.get(CameraParameters::KEY_GPS_ALTITUDE);
    if (latitude && logitude && altitude) {
        ALOGV("setParameters: GPS latitude %f, logitude %f, altitude %f",
             atof(latitude), atof(logitude), atof(altitude));
        mParameters.set(CameraParameters::KEY_GPS_LATITUDE, latitude);
        mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, logitude);
        mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, altitude);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
        mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
        mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
    }

    const char *timestamp = params.get(CameraParameters::KEY_GPS_TIMESTAMP);
    if (timestamp) {
        ALOGV("setParameters: GPS timestamp %s", timestamp);
        mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, timestamp);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
    }

    const char *progressingMethod = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    if (progressingMethod) {
        ALOGV("setParameters: GPS timestamp %s", timestamp);
        mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, progressingMethod);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    }

    return NO_ERROR;
}

/*------------------------------------------------------------------*/
/*   ISP Debug Code                                                 */
/*------------------------------------------------------------------*/
#define ISP_DBG_STATE	  "/data/media/0/ISPD/ISP_S.DAT"
#define ISP_DBG_CMD		  "/data/media/0/ISPD/ISP_C.DAT"
#define ISP_DBG_RESULT    "/data/media/0/ISPD/ISP_R.DAT"
#define ISP_DBG_LOGFILTER "/data/media/0/ISPD/ISP_TUNING.DAT"
#define ISP_DBG_CONFIG    "/data/media/0/ISPD/ISP_CONFIG.DAT"

#define ISP_DBG_MAX_BUFFER  0xFF

#define ISP_DBG_STATE_ING   "ING"
#define ISP_DBG_STATE_END   "END"
#define ISP_DBG_STATE_ER1   "E01" // cmd not found
#define ISP_DBG_STATE_ER2   "E02" // param
#define ISP_DBG_STATE_ER3   "E03" // opcode not found
#define ISP_DBG_STATE_ER4   "E04" // data parsing error
#define ISP_DBG_STATE_ER5   "E05" //
#define ISP_DBG_STATE_ER6   "E06" // malloc fail

enum {
    ISP_DBG_READ=0,
    ISP_DBG_WRITE,
    ISP_DBG_READ_MEM,
    ISP_DBG_WRITE_MEM,
    ISP_DBG_READ_FILE,
    ISP_DBG_WRITE_FILE,
    ISP_DBG_LOGV,
};

bool ISecCameraHardware::ISP_DBG_ouput_state(char* state)
{
    FILE *fp = NULL;
    fp = fopen(ISP_DBG_STATE, "wt");
    if ( fp == NULL ){
        ALOGE("ISP_DBG_ouput_state fp = null!!!");
        return true;
    }

    fflush(stdout);
    fwrite(state, sizeof(char), 3, fp);
    fflush(fp);

    if (fp)
        fclose(fp);

    return true;
}

bool ISecCameraHardware::ISP_DBG_input(void)
{
    FILE *fp = NULL;
    int fileSize = 0;
    char* buffer = NULL;

    fp = fopen(ISP_DBG_CMD, "rb");
    if ( fp == NULL ) {
        ALOGD("ISP_DBG_input() file open NG: %s", ISP_DBG_CMD);
        ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER1);
        return false;
    }

    ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ING);

    //fflush(stdin);

    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);

    ALOGD("ISPD ISP_DBG_input()----filesize=%d", fileSize);

    buffer = (char *) malloc(fileSize+1);
    if ( buffer == NULL ) {
        if (fp)
            fclose(fp);
        ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER6);

        return false;
    }

    fread(buffer, sizeof(char), fileSize , fp);
    buffer[fileSize]= '\0';

    ISP_DBG_proc(buffer, fileSize);

    fflush(fp);

    if (buffer)
    {
        free(buffer);
		buffer = NULL;
    }

    if (fp)
    {
        fclose(fp);
		fp = NULL;
    }

    return true;
}

int ISecCameraHardware::CharToByte(char *buf, int size)
{
    int i, p, out=0;

    for ( i=0, p=0, out=0; i<size; i++) {
        char c = buf[i] ;

        if ( 'a' <= c && c <= 'f' )
            c = c - 'a' + 10;
        else if ( 'A' <= c && c <= 'F' )
            c = c - 'A' + 10;
        else if ( '0' <= c && c <= '9' )
            c = c - '0';
        else
            continue;

        if ( p%2==0 ) {
            buf[out] = c<<4;
        }
        else {
            buf[out] += c;
            //ALOGD("ISPD CharToByte() buf:%2s, buf[out]:%x", buf, buf[out]);
            out++;
        }
        p++;
    }

    return out;
}

int ISecCameraHardware::intpow(int n,int m)
{
    int out=1;
    for (int i=0;i<m;i++)
        out *= n;

    return out;
}

int ISecCameraHardware::CharToNumber(char *buf, int size)
{
    int i, out=0;

    for ( i=0; i<size; i++) {
        char c = buf[i] ;

        if ( '0' <= c && c <= '9' )
            c = c - '0';
        else
            continue;

        out += ( c * intpow(10, size-i-1));
    }
    return out;
}

int ISecCameraHardware::find_seperator(char *buf,  int size)
{
    int i;

    for (i=0; i<size;i++) {
        if ( buf[i]==' ' || buf[i]==',' ) {
            buf[i] = '\0';
            break;
        }
    }
    return ( i<size ) ?  i+1  : size;
}

void ISecCameraHardware::upper(char *buf, int size)
{
    for (int i=0;i<size;i++)
        if ( 'a' <= buf[i] && buf[i] <= 'z' )
            buf[i] -= 0x20;
}

bool ISecCameraHardware::ISP_DBG_proc(char *buffer, int size)
{
    /* support type
    1) w	    Category    Byte Number : w 0 1 5
    2) r	    Category    Byte Number : r 0 1 100
    3) wm  Address1 Data~ : wm 8045000 00998877626
    4) rm   Address 1   Size : rm 90897877 100
    5) wf    filename   Address   Data~ :
    6) rf     filename  Address   Size :
    7) ALOGV filename   : logv test.dat
    */

    const int max = 16; /*data max length */
    int s=0, e=0;
    int len=0;
    char* workBUF=NULL;

    while (buffer[s]==' ' || buffer[s]=='\r' || buffer[s]=='\n' || buffer[s]=='\t' ) s++;

    e=size-1;
    while ((buffer[e]==' ' || buffer[e]=='\r' || buffer[e]=='\n' || buffer[e]=='\t' ) && e >=0 ) e--;
    size=e;

    char *cmd = &buffer[s];
    len = find_seperator(cmd, e-s+1);
    upper(cmd, len);
    s += len;

    ALOGD("ISPD ISP_DBG_proc()-1----cmd=%s  s(%d), e(%d) len(%d),", cmd, s, e, len);

    /* Read / Write */
    if ( strncmp("R", cmd, max )==0 || strncmp("W", cmd, max )==0 ) {
        char *category= &buffer[s];
        len = find_seperator(category, e-s+1);
        s += len;
        len = CharToByte(category, len);

        char *subcategory= &buffer[s];
        len = find_seperator(subcategory, e-s+1);
        s += len;
        len = CharToByte(subcategory, len);

        if ( strncmp("R", cmd, max )==0 ) {

            char *length = &buffer[s];
            len = find_seperator(length, e-s+1);

            if ( len < 1 ) {
                ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER5);
                return false;
            }

            int read_length = CharToNumber(length, len);
            ALOGD("ISPD ISP_DBG_proc()-2----read_length=%d, len=%d", read_length, len);
            if (read_length > ISP_DBG_MAX_BUFFER ) {
                ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER4);
                return false;
            }

            int cmd_size = 2;
            workBUF = (char *)malloc(read_length+cmd_size);
            if ( workBUF == NULL ) {
                ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER6);
                return false;
            }

            workBUF[0]=*category;
            workBUF[1]=*subcategory;

            ALOGD("ISPD CAM_CID_ISP_DEBUG_READ  cate:%x, sub:%x  length:%d===", *category, *subcategory, read_length);
            nativeGetExtParameters(CAM_CID_ISP_DEBUG_READ, workBUF, read_length+cmd_size);

            ISP_DBG_ouput(workBUF, read_length);

            ISP_DBG_ouput_state((char*)ISP_DBG_STATE_END);

            if (workBUF)
                free(workBUF);

        }
        else { /*strncmp("W", cmd, max )==0 */

            char *data = &buffer[s];
            len = CharToByte(data, e-s+1);

            if ( len < 1 ) {
                ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER5);
                return false;
            }

            int cmd_size = 2;
            workBUF = (char *)malloc( len+cmd_size);
            if ( workBUF == NULL ) {
                ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER6);
                return false;
            }
            workBUF[0]=*category;
            workBUF[1]=*subcategory;
            memcpy(&workBUF[2], data, len);

            ALOGD("ISPD CAM_CID_ISP_DEBUG_WRITE   cate:%x, sub:%x  data:%x===", workBUF[0], workBUF[1], workBUF[2]);
            nativeSetExtParameters(CAM_CID_ISP_DEBUG_WRITE, workBUF, len+cmd_size);

            ISP_DBG_ouput_state((char*)ISP_DBG_STATE_END);

            if (workBUF)
                free(workBUF);
        }
    }
    else if ( strncmp("RM", cmd, max )==0 || strncmp("WM", cmd, max )==0 ) {

        char *address = &buffer[s];
        len = find_seperator(address, e-s+1);
        s += len;
        len = CharToByte(address, len);

        if ( strncmp("RM", cmd, max )==0 ) {

            char *length = &buffer[s];
            len = find_seperator(length, e-s+1);

            if ( len < 1 ) {
                ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER5);
                return false;
            }

            int read_length = CharToNumber(length, len);
            ALOGD("ISPD ISP_DBG_proc()-2----read_length=%d, len=%d e=%d, s=%d", read_length, len, e, s);
            //			if (read_length > ISP_DBG_MAX_BUFFER ) {
            //				ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER4);
            //				return false;
            //			}

            int cmd_size = 4;
            workBUF = (char *)malloc(read_length+cmd_size);
            if ( workBUF == NULL ) {
                ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER6);
                return false;
            }
            memcpy(&workBUF[0], address, cmd_size);

            ALOGD("ISPD CAM_CID_ISP_DEBUG_READ  address:%x, [%02x,%02x,%02x,%02x]length:%d===", *(int*)address, workBUF[0], workBUF[1],workBUF[2], workBUF[3], *(int*)&workBUF[4]);
            nativeGetExtParameters(CAM_CID_ISP_DEBUG_READ_MEM, workBUF, read_length+cmd_size);
            ALOGD("ISPD CAM_CID_ISP_DEBUG_READ  [%02x,%02x,%02x,%02x]", workBUF[0], workBUF[1],workBUF[2], workBUF[3]);

            ISP_DBG_ouput(workBUF, read_length);

            ISP_DBG_ouput_state((char*)ISP_DBG_STATE_END);

            if (workBUF)
                free(workBUF);

        } else {  /* strncmp("wm", cmd, max )==0 */

        char *data = &buffer[s];
        len = CharToByte(data, e-s+1);

        if ( len < 1 ) {
            ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER5);
            return false;
        }

        int cmd_size = 4;
        workBUF = (char *)malloc(len+cmd_size);
        if ( workBUF == NULL ) {
            ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER6);
            return false;
        }
        memcpy(&workBUF[0], address, cmd_size);
        memcpy(&workBUF[cmd_size], data, len);

        ALOGD("ISPD CAM_CID_ISP_DEBUG_READ  address:%x, %x %x===", *(int*)address, workBUF[4], workBUF[5]);
        nativeSetExtParameters(CAM_CID_ISP_DEBUG_WRITE_MEM, workBUF, len+cmd_size);

        ISP_DBG_ouput_state((char*)ISP_DBG_STATE_END);

        if (workBUF)
            free(workBUF);

        }
    }
    else if ( strncmp("RF", cmd, max )==0 ) {
    }
    else if ( strncmp("WF", cmd, max )==0 ) {
    }
    else if ( strncmp("LOGV", cmd, max )==0 ) {
        len=0;
        char *filename = &buffer[s];
        len = find_seperator(filename, e-s+1);

        nativeSetExtParameters(CAM_CID_ISP_DEBUG_LOGV, filename, len);

        ISP_DBG_ouput_state((char*)ISP_DBG_STATE_END);
    }
    else {
        ISP_DBG_ouput_state((char*)ISP_DBG_STATE_ER3);
        return false;
    }
    return true;
}

bool ISecCameraHardware::ISP_DBG_ouput(char *buffer, int size)
{
    FILE *fp = NULL;
    fp = fopen(ISP_DBG_RESULT, "wb");

    fflush(stdout);
    fwrite(buffer, sizeof(char), size, fp);
    fflush(fp);

    if (fp)
        fclose(fp);

    ISP_DBG_ouput_state((char*)ISP_DBG_STATE_END);

    return true;
}

bool ISecCameraHardware::ISP_DBG_logwrite(void)
{
    FILE *fp = NULL;
    int readSize = 0;
    char buffer[256];
    int    nLogNo = 1;

    // Log Filter ------------------------------------------------------------------------------------------------
    readSize = 8;  // 8byte : filter flag

    fp = fopen(ISP_DBG_LOGFILTER, "rb");
    if ( fp != NULL ) {
        //fflush(stdin);
        fseek(fp, 0, SEEK_END);
        rewind(fp);

        fread(buffer, sizeof(char), readSize , fp);
        buffer[readSize]='\0';

        fflush(fp);
        if (fp)
            fclose(fp);

        int cmd_size = 2;
        int  len = CharToByte(buffer, 8);

        char workBUF[16];
        workBUF[0]=0x0d; // category
        workBUF[1]=0x10; // sub
        memcpy(&workBUF[2], buffer, 4); // data
        workBUF[6]=0;

        ALOGD("ISPD ISP_DBG_logwrite()----%2x%2x%2x%2x", workBUF[2],workBUF[3],workBUF[4],workBUF[5] );
        nativeSetExtParameters(CAM_CID_ISP_DEBUG_WRITE, workBUF, len+cmd_size);
    }

    //- Log Name------------------------------------------------------------------------------------------------
    readSize = 6;  // 6byte :log No

    fp = fopen(ISP_DBG_CONFIG, "rb");

    if ( fp != NULL ) {
        //fflush(stdin);
        fseek(fp, 0, SEEK_END);
        rewind(fp);

        fread(buffer, sizeof(char), readSize , fp);
        buffer[readSize]='\0';

        nLogNo = atoi( buffer );
        nLogNo ++;

        fflush(fp);
        if (fp)
            fclose(fp);
    }

    fp = fopen(ISP_DBG_CONFIG, "wb");
    fflush(stdout);

    sprintf(buffer, "%06d%c", nLogNo, 0);    // 6 = readSize
    fwrite(buffer, sizeof(char), readSize , fp);

    fflush(fp);
    if (fp)
        fclose(fp);

    //- Log Writer------------------------------------------------------------------------------------------------

    sprintf(buffer, "_ISP_%06d.LOG%c", nLogNo, NULL);
    ALOGD("ISPD ISP_DBG_logwrite()----filename=%s", buffer);

    nativeSetExtParameters(CAM_CID_ISP_DEBUG_LOGV, buffer, strlen(buffer));

    return true;
}

#if defined(KOR_CAMERA)
status_t ISecCameraHardware::setSelfTestMode(const CameraParameters &params)
{
    int val = params.getInt("selftestmode");
    if (val == -1)
        return NO_ERROR;

    ALOGV("selftestmode: %d", val);
    mSamsungApp = val ? true : false;

    return NO_ERROR;
}
#endif

bool ISecCameraHardware::allocMemSinglePlane(ion_client ionClient, ExynosBuffer *buf, int index, bool flagCache)
{
    if (ionClient == 0) {
        ALOGE("ERR(%s): ionClient is zero (%d)", __func__, ionClient);
        return false;
    }

    if (buf->size.extS[index] != 0) {
        int flagIon = (flagCache == true) ? ION_FLAG_CACHED : 0;

        /* HACK: For non-cacheable */
        buf->fd.extFd[index] = ion_alloc(ionClient, buf->size.extS[index], 0, ION_HEAP_SYSTEM_MASK, 0);
        if (buf->fd.extFd[index] <= 0) {
            ALOGE("ERR(%s): ion_alloc(%d, %d) failed", __func__, index, buf->size.extS[index]);
            buf->fd.extFd[index] = -1;
            freeMemSinglePlane(buf, index);
            return false;
        }

        buf->virt.extP[index] = (char *)ion_map(buf->fd.extFd[index], buf->size.extS[index], 0);
        if ((buf->virt.extP[index] == (char *)MAP_FAILED) || (buf->virt.extP[index] == NULL)) {
            ALOGE("ERR(%s): ion_map(%d) failed", __func__, buf->size.extS[index]);
            buf->virt.extP[index] = NULL;
            freeMemSinglePlane(buf, index);
            return false;
        }
    }

    return true;
}

void ISecCameraHardware::freeMemSinglePlane(ExynosBuffer *buf, int index)
{
    if (0 < buf->fd.extFd[index]) {
        if (buf->virt.extP[index] != NULL) {
			int ret = 0;
            ret = ion_unmap(buf->virt.extP[index], buf->size.extS[index]);
            if (ret < 0)
                ALOGE("ERR(%s):ion_unmap(%p, %d) fail", __FUNCTION__, buf->virt.extP[index], buf->size.extS[index]);
        }
        ion_free(buf->fd.extFd[index]);
    }

    buf->fd.extFd[index] = -1;
    buf->virt.extP[index] = NULL;
    buf->size.extS[index] = 0;
}

bool ISecCameraHardware::allocMem(ion_client ionClient, ExynosBuffer *buf, int cacheIndex)
{
    for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++) {
        bool flagCache = ((1 << i) & cacheIndex) ? true : false;
        if (allocMemSinglePlane(ionClient, buf, i, flagCache) == false) {
            freeMem(buf);
            ALOGE("ERR(%s): allocMemSinglePlane(%d) fail", __func__, i);
            return false;
        }
    }

    return true;
}

void ISecCameraHardware::freeMem(ExynosBuffer *buf)
{
    for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++)
        freeMemSinglePlane(buf, i);
}

void ISecCameraHardware::mInitRecSrcQ(void)
{
    Mutex::Autolock lock(mRecordSrcLock);
    mRecordSrcIndex = -1;

    mRecordSrcQ.clear();
}

int ISecCameraHardware::getRecSrcBufSlotIndex(void)
{
    Mutex::Autolock lock(mRecordSrcLock);
    mRecordSrcIndex++;
    mRecordSrcIndex = mRecordSrcIndex % FLITE_BUF_CNT;
    return mRecordSrcIndex;
}

void ISecCameraHardware::mPushRecSrcQ(rec_src_buf_t *buf)
{
    Mutex::Autolock lock(mRecordSrcLock);
    mRecordSrcQ.push_back(buf);
}

bool ISecCameraHardware::mPopRecSrcQ(rec_src_buf_t *buf)
{
    List<rec_src_buf_t *>::iterator r;

    Mutex::Autolock lock(mRecordSrcLock);

    if (mRecordSrcQ.size() == 0)
        return false;

    r = mRecordSrcQ.begin()++;

    buf->buf = (*r)->buf;
    buf->timestamp = (*r)->timestamp;
    mRecordSrcQ.erase(r);

    return true;
}

int ISecCameraHardware::mSizeOfRecSrcQ(void)
{
    Mutex::Autolock lock(mRecordSrcLock);

    return mRecordSrcQ.size();
}

#if 0
bool ISecCameraHardware::setRecDstBufStatus(int index, enum REC_BUF_STATUS status)
{
    Mutex::Autolock lock(mRecordDstLock);

    if (index < 0 || index >= REC_BUF_CNT) {
        ALOGE("ERR(%s): index(%d) out of range, status(%d)", __func__, index, status);
        return false;
    }

    mRecordDstStatus[index] = status;
    return true;
}
#endif

int ISecCameraHardware::getRecDstBufIndex(void)
{
    Mutex::Autolock lock(mRecordDstLock);

    for (int i = 0; i < REC_BUF_CNT; i++) {
        mRecordDstIndex++;
        mRecordDstIndex = mRecordDstIndex % REC_BUF_CNT;

        if (mRecordFrameAvailable[mRecordDstIndex] == true) {
            mRecordFrameAvailableCnt--;
            mRecordFrameAvailable[mRecordDstIndex] = false;
            return mRecordDstIndex;
        }
    }

    return -1;
}

void ISecCameraHardware::setAvailDstBufIndex(int index)
{
    Mutex::Autolock lock(mRecordDstLock);
    mRecordFrameAvailableCnt++;
    mRecordFrameAvailable[index] = true;
    return;
}

void ISecCameraHardware::mInitRecDstBuf(void)
{
    Mutex::Autolock lock(mRecordDstLock);

    ExynosBuffer nullBuf;

    mRecordDstIndex = -1;
    mRecordFrameAvailableCnt = REC_BUF_CNT;

    for (int i = 0; i < REC_BUF_CNT; i++) {
#ifdef BOARD_USE_MHB_ION
        for (int j = 0; j < REC_PLANE_CNT; j++) {
            if (mRecordDstHeap[i][j] != NULL)
                mRecordDstHeap[i][j]->release(mRecordDstHeap[i][j]);
            mRecordDstHeap[i][j] = NULL;
            mRecordDstHeapFd[i][j] = -1;
        }
#else
        if (0 < mRecordingDstBuf[i].fd.extFd)
            freeMem(&mRecordingDstBuf[i]);
#endif
        mRecordingDstBuf[i] = nullBuf;
        mRecordFrameAvailable[i] = true;
    }
}

int ISecCameraHardware::getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf, bool flagAndroidColorFormat)
{
    int FrameSize = 0;
    ExynosBuffer alignedBuf;

    /* ALOGV("[%s] (%d) colorFormat %d", __func__, __LINE__, colorFormat); */
    switch (colorFormat) {
    /* 1p */
    case V4L2_PIX_FMT_RGB565 :
    case V4L2_PIX_FMT_YUYV :
    case V4L2_PIX_FMT_UYVY :
    case V4L2_PIX_FMT_VYUY :
    case V4L2_PIX_FMT_YVYU :
        alignedBuf.size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(colorFormat), w, h);
	if(h==1080){
		alignedBuf.size.extS[0] += 1024*28;
	}
        /* ALOGV("V4L2_PIX_FMT_YUYV buf->size.extS[0] %d", alignedBuf->size.extS[0]); */
        alignedBuf.size.extS[1] = SPARE_SIZE;
        alignedBuf.size.extS[2] = 0;
        break;
    /* 2p */
    case V4L2_PIX_FMT_NV12 :
    case V4L2_PIX_FMT_NV12T :
    case V4L2_PIX_FMT_NV21 :
    case V4L2_PIX_FMT_NV12M :
    case V4L2_PIX_FMT_NV21M :
        if (flagAndroidColorFormat == true) {
            alignedBuf.size.extS[0] = w * h;
            alignedBuf.size.extS[1] = w * h / 2;
            alignedBuf.size.extS[2] = SPARE_SIZE;
        } else {
            alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16);
            alignedBuf.size.extS[1] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16) / 2;
            alignedBuf.size.extS[2] = SPARE_SIZE;
        }
        /* ALOGV("V4L2_PIX_FMT_NV21 buf->size.extS[0] %d buf->size.extS[1] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1]); */
        break;
    case V4L2_PIX_FMT_NV12MT_16X16 :
        if (flagAndroidColorFormat == true) {
            alignedBuf.size.extS[0] = w * h;
            alignedBuf.size.extS[1] = w * h / 2;
            alignedBuf.size.extS[2] = SPARE_SIZE;
        } else {
            alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16);
            alignedBuf.size.extS[1] = ALIGN(alignedBuf.size.extS[0] / 2, 256);
            alignedBuf.size.extS[2] = SPARE_SIZE;
        }
        /* ALOGV("V4L2_PIX_FMT_NV12M buf->size.extS[0] %d buf->size.extS[1] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1]); */
        break;
    case V4L2_PIX_FMT_NV16 :
    case V4L2_PIX_FMT_NV61 :
        alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16);
        alignedBuf.size.extS[1] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16);
        alignedBuf.size.extS[2] = SPARE_SIZE;
        /* ALOGV("V4L2_PIX_FMT_NV16 buf->size.extS[0] %d buf->size.extS[1] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1]); */
        break;
    /* 3p */
    case V4L2_PIX_FMT_YUV420 :
    case V4L2_PIX_FMT_YVU420 :
        /* http://developer.android.com/reference/android/graphics/ImageFormat.html#YV12 */
        if (flagAndroidColorFormat == true) {
            alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * h;
            alignedBuf.size.extS[1] = ALIGN_UP(w / 2, 16) * h / 2;
            alignedBuf.size.extS[2] = ALIGN_UP(w / 2, 16) * h / 2;
            alignedBuf.size.extS[3] = SPARE_SIZE;
        } else {
            alignedBuf.size.extS[0] = (w * h);
            alignedBuf.size.extS[1] = (w * h) >> 2;
            alignedBuf.size.extS[2] = (w * h) >> 2;
            alignedBuf.size.extS[3] = SPARE_SIZE;
        }
        /* ALOGV("V4L2_PIX_FMT_YUV420 Buf.size.extS[0] %d Buf.size.extS[1] %d Buf.size.extS[2] %d",
            alignedBuf.size.extS[0], alignedBuf.size.extS[1], alignedBuf.size.extS[2]); */
        break;
    case V4L2_PIX_FMT_YUV420M :
    case V4L2_PIX_FMT_YVU420M :
        if (flagAndroidColorFormat == true) {
            alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * h;
            alignedBuf.size.extS[1] = ALIGN_UP(w / 2, 16) * h / 2;
            alignedBuf.size.extS[2] = ALIGN_UP(w / 2, 16) * h / 2;
            alignedBuf.size.extS[3] = SPARE_SIZE;
        } else {
            alignedBuf.size.extS[0] = ALIGN_UP(w,   32) * ALIGN_UP(h,  16);
            alignedBuf.size.extS[1] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
            alignedBuf.size.extS[2] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
            alignedBuf.size.extS[3] = SPARE_SIZE;
        }
        /* ALOGV("V4L2_PIX_FMT_YUV420M buf->size.extS[0] %d buf->size.extS[1] %d buf->size.extS[2] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1], alignedBuf->size.extS[2]); */
        break;
    case V4L2_PIX_FMT_YUV422P :
        alignedBuf.size.extS[0] = ALIGN_UP(w,   16) * ALIGN_UP(h,  16);
        alignedBuf.size.extS[1] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
        alignedBuf.size.extS[2] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
        alignedBuf.size.extS[3] = SPARE_SIZE;
        /* ALOGV("V4L2_PIX_FMT_YUV422P Buf.size.extS[0] %d Buf.size.extS[1] %d Buf.size.extS[2] %d",
            alignedBuf.size.extS[0], alignedBuf.size.extS[1], alignedBuf.size.extS[2]); */
        break;
	case V4L2_PIX_FMT_JPEG:
		alignedBuf.size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_YUYV), w, h);
		alignedBuf.size.extS[1] = SPARE_SIZE;
		alignedBuf.size.extS[2] = 0;
		ALOGD("V4L2_PIX_FMT_JPEG buf->size.extS[0] = %d", alignedBuf.size.extS[0]);
		break;
    default:
        ALOGE("ERR(%s):unmatched colorFormat(%d)", __func__, colorFormat);
        return 0;
        break;
    }

    for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++)
        FrameSize += alignedBuf.size.extS[i];

    if (buf != NULL) {
        for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++) {
            buf->size.extS[i] = alignedBuf.size.extS[i];

            /* if buf has vadr, calculate another vadr per plane */
            if (buf->virt.extP[0] != NULL && i > 0) {
                if (buf->size.extS[i] != 0)
                    buf->virt.extP[i] = buf->virt.extP[i - 1] + buf->size.extS[i - 1];
                else
                    buf->virt.extP[i] = NULL;
            }
        }
    }
    return (FrameSize - SPARE_SIZE);
}

}; /* namespace android */
#endif /* ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP */
