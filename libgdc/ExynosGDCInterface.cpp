/*
 * Copyright@ Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosGDCInterface"

#include "ExynosGDCInterface.h"

typedef enum ExynosGDCState {
    E_GDC_STATE_NONE = 0,
    E_GDC_STATE_CREATE,
    E_GDC_STATE_INIT,
    E_GDC_STATE_START,
    E_GDC_STATE_STOP,
    E_GDC_STATE_MAX
}E_GDC_STATE;

ExynosGDCInterface::ExynosGDCInterface()
{
    GDC_LOGV("");

    m_state = E_GDC_STATE_NONE;

    m_debugProfile = 0;
    m_procFrames = 0;
    m_startTime = m_endTime = m_totalProcTime = 0.0f;
}

ExynosGDCInterface::~ExynosGDCInterface()
{
    GDC_LOGV("");
}

status_t ExynosGDCInterface::create()
{
#ifndef USE_LEGACY_FUNCTION_ALIGNMENT
    return create(GDC_VIDEO_NUM);
}

status_t ExynosGDCInterface::create(int videoNum)
{
#else
    int videoNum = GDC_VIDEO_NUM;
#endif
    GDC_LOGI("%s: Inst: 0x%p", __FUNCTION__, this);

    m_state = E_GDC_STATE_CREATE;

    /* Profiling feature */
    char prop[100];
    m_gdc.gdc_property_get("persist.vendor.gdc.profile", prop, 0);
    m_debugProfile = atoi(prop);
    if (m_debugProfile > 0) {
        GDC_LOGI("Debug profiling is enabled (%d)", m_debugProfile);
    }

    return m_gdc.open(videoNum);
}

status_t ExynosGDCInterface::init(void)
{
    GDC_LOGI("");

    status_t ret = NO_ERROR;

    ret = m_gdc.setFormat();

    m_state = E_GDC_STATE_INIT;

    return ret;
}

status_t ExynosGDCInterface::destroy(void)
{
    GDC_LOGI("");

    if (stop() != NO_ERROR) {
        GDC_LOGW("stop is failed");
    }

    if (m_debugProfile > 0) {
        if (m_procFrames > 0 && m_totalProcTime > 0.0f) {
            GDC_LOGD("Profile: %.4f ms for %lu frames", (double)(m_totalProcTime / m_procFrames), m_procFrames);
        }
    }

    return m_gdc.release();
}

status_t ExynosGDCInterface::start()
{
    status_t ret = NO_ERROR;

    if (m_state == E_GDC_STATE_START) {
        //NOP
    } else if (m_state == E_GDC_STATE_INIT
            || m_state == E_GDC_STATE_STOP
            || m_state == E_GDC_STATE_CREATE) {
        GDC_LOGV("");
        ret = m_gdc.start();
        if (ret != NO_ERROR) {
            GDC_LOGE("Failed to start. ret(%d)", ret);
        }
    } else {
        GDC_LOGE("Can't start since current state is %d", m_state);
        ret = INVALID_OPERATION;
    }

    if (ret == NO_ERROR) {
        m_state = E_GDC_STATE_START;
    }

    return ret;
}

status_t ExynosGDCInterface::stop()
{
    GDC_LOGV("");

    status_t ret = NO_ERROR;

    if (m_state == E_GDC_STATE_STOP) {
        //NOP
    } else if (m_state == E_GDC_STATE_START) {
        ret = m_gdc.stop();
        if (ret != NO_ERROR) {
            GDC_LOGE("Failed to stop. ret(%d)", ret);
        }
    } else {
        GDC_LOGE("Can't stop since current state is %d", m_state);
        ret = INVALID_OPERATION;
    }

    if (ret == NO_ERROR) {
        m_state = E_GDC_STATE_STOP;
    }

    return ret;
}

status_t ExynosGDCInterface::setSrcImageSize(uint32_t width, uint32_t height)
{
    GDC_LOGV("size(%dx%d)", width, height);

    return m_gdc.setSrcImageSize(width, height);
}

status_t ExynosGDCInterface::setDstImageSize(uint32_t width, uint32_t height)
{
    GDC_LOGV("size(%dx%d)", width, height);

    return m_gdc.setDstImageSize(width, height);
}

status_t ExynosGDCInterface::setSrcColorFormat(uint32_t format, uint32_t planeCount)
{
    GDC_LOGV("format(%x) planecount(%d)", format, planeCount);

    return m_gdc.setSrcColorFormat(format, planeCount);
}

status_t ExynosGDCInterface::setDstColorFormat(uint32_t format, uint32_t planeCount)
{
    GDC_LOGV("format(%x) planecount(%d)", format, planeCount);

    return m_gdc.setDstColorFormat(format, planeCount);
}

status_t ExynosGDCInterface::setGridTable(int32_t *gridX, int32_t *gridY, uint32_t width, uint32_t height)
{
    struct ExynosGDCGridTable gridTable;

    GDC_LOGV("gridX(%p), gridY(%p), length(%dx%d)", gridX, gridY, width, height);

    for (uint32_t indexY = 0; indexY < height; indexY++) {
        for (uint32_t indexX = 0; indexX < width; indexX++) {
            gridTable.gridX[indexY][indexX] = gridX[(indexY * width) + indexX];
            gridTable.gridY[indexY][indexX] = gridY[(indexY * width) + indexX];
        }
    }

    gridTable.width = width;
    gridTable.height = height;

    return m_gdc.setGridTable(&gridTable);
}

status_t ExynosGDCInterface::setInputBuffer(struct ExynosGDCBuf srcBuf)
{
    GDC_LOGV("index(%d) planeCount(%d)", srcBuf.index, srcBuf.planeCount);

    return m_gdc.setSrcBuffer(srcBuf);
}

status_t ExynosGDCInterface::setOutputBuffer(struct ExynosGDCBuf dstBuf)
{
    GDC_LOGV("index(%d) planeCount(%d)", dstBuf.index, dstBuf.planeCount);

    return m_gdc.setDstBuffer(dstBuf);
}

status_t ExynosGDCInterface::setInputSize(struct ExynosGDCSizeParam sizeParam)
{
    GDC_LOGV("sizeParam(%d,%d %dx%d, %dx%d)",
            sizeParam.cropX, sizeParam.cropY,
            sizeParam.cropW, sizeParam.cropH,
            sizeParam.fullW, sizeParam.fullH);

    return m_gdc.setInputSizeParam(sizeParam);;
}

status_t ExynosGDCInterface::setOutputSize(struct ExynosGDCSizeParam sizeParam)
{
    GDC_LOGV("sizeParam(%d,%d %dx%d, %dx%d)",
            sizeParam.cropX, sizeParam.cropY,
            sizeParam.cropW, sizeParam.cropH,
            sizeParam.fullW, sizeParam.fullH);

    return m_gdc.setOutputSizeParam(sizeParam);;
}

status_t ExynosGDCInterface::runGDC()
{
    status_t ret = NO_ERROR;

    GDC_LOGV("");

    if (m_debugProfile) {
        m_startTime = now_ms();
    }

    ret = m_gdc.setFormat();
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to setFormat. ret(%d)", ret);
        return ret;
    }

    if (start() != NO_ERROR) {
        GDC_LOGE("Failed to start. ret(%d)", ret);
        return ret;
    }

    ret = m_gdc.setControl();
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to setControl. ret(%d)", ret);
        return ret;
    }

    ret = m_gdc.run();
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to run. ret(%d)", ret);
        /* Keep going */
    }

    if (stop() != NO_ERROR) {
        GDC_LOGE("Failed to stop. ret(%d)", ret);
        return ret;
    }

    if (m_debugProfile) {
        m_endTime = now_ms();
        m_procFrames++;
        m_totalProcTime += (m_endTime - m_startTime);
    }

    return NO_ERROR;
}

status_t ExynosGDCInterface::dump()
{
    GDC_LOGD("");

    return m_gdc.dump();
}

status_t ExynosGDCInterface::pollLast()
{
    status_t ret = NO_ERROR;

    GDC_LOGV("");

    if (m_debugProfile) {
        m_startTime = now_ms();
    }

    if (start() != NO_ERROR) {
        GDC_LOGE("Failed to start. ret(%d)", ret);
        return ret;
    }

    ret = m_gdc.setControl();
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to setControl. ret(%d)", ret);
        return ret;
    }

    ret = m_gdc.pollLast();
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to run. ret(%d)", ret);
        /* Keep going */
    }

    return ret;
}

status_t ExynosGDCInterface::pollFirst(uint32_t & bufIndex)
{
    status_t ret = NO_ERROR;

    ret = m_gdc.pollFirst(bufIndex);
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to run. ret(%d)", ret);
        /* Keep going */
    }

    if (m_debugProfile) {
        m_endTime = now_ms();
        m_procFrames++;
        m_totalProcTime += (m_endTime - m_startTime);
    }

    GDC_LOGV("bufIndex(%d)", bufIndex);

    return NO_ERROR;
}

double ExynosGDCInterface::now_ms(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec + (t.tv_usec / 1000000.0)) * 1000.0;
}

