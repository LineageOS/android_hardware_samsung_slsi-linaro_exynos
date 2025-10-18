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
#define LOG_TAG "ExynosGDC"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "exynos_format.h"
#include <linux/videodev2.h>
#include <videodev2_exynos_gdc.h>
#include "exynos_v4l2.h"
#include "ExynosGDC.h"
#include <sys/system_properties.h>

ExynosGDC::ExynosGDC()
{
    GDC_LOGV("");

#ifdef GDC_USE_OLD_FUNCTION_ALIGNMENT
    m_videoNum = GDC_VIDEO_NUM;
#endif
    m_videoFd = -1;

    m_srcBufList.clear();
    m_dstBufList.clear();

    m_debugLevel = (int)LIBGDC_DEBUG_LEVEL_OFF;

    /* Check per-frame log property */
    char prop[100];
    gdc_property_get("persist.vendor.gdc.debug", prop, 0);
    m_debugLevel = (atoi(prop) < LIBGDC_DEBUG_LEVEL_LAST) ? atoi(prop) : LIBGDC_DEBUG_LEVEL_LAST;
    if (m_debugLevel > (int)LIBGDC_DEBUG_LEVEL_OFF) {
        GDC_LOGI("Debug log printing is enabled (%d)", m_debugLevel);
    }
}

ExynosGDC::~ExynosGDC()
{
    GDC_LOGV("");

    m_videoNum = -1;
    m_videoFd = -1;
}

#ifdef GDC_USE_OLD_FUNCTION_ALIGNMENT
status_t ExynosGDC::open(void)
#else
status_t ExynosGDC::open(int videoNum)
#endif
{
    char videoFileName[30];

#ifdef GDC_USE_OLD_FUNCTION_ALIGNMENT
    if (m_videoNum < 0) {
#else
    m_videoNum = videoNum;
    if (m_videoNum > GDC_VIDEO_NUM + 1) {    
#endif
        GDC_LOGE("Invalid videoNum(%d)", m_videoNum);
        return BAD_VALUE;
    }

    memset(&videoFileName, 0x00, sizeof(videoFileName));
    snprintf(videoFileName, sizeof(videoFileName), "%s%d", GDC_VIDEO_FILE_PREFIX, m_videoNum);

    m_videoFd = exynos_v4l2_open(videoFileName, O_RDWR, 0);
    if (m_videoFd < 0) {
        GDC_LOGE("Failed to exynos_v4l2_open. ret(%d)", m_videoFd);
        return INVALID_OPERATION;
    }

    GDC_LOGI("fd(%d) is opened.", m_videoFd);

    return NO_ERROR;
}

status_t ExynosGDC::release(void)
{
    status_t ret = NO_ERROR;

    if (m_videoFd < 0) {
        GDC_LOGW("FD(%d) already closed", m_videoFd);
        return NO_ERROR;
    }

    ret = exynos_v4l2_close(m_videoFd);
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to exynos_v4l2_close. fd(%d) ret(%d)", m_videoFd, ret);
        return ret;
    }

    GDC_LOGI("fd(%d) is closed.", m_videoFd);

    return NO_ERROR;
}

status_t ExynosGDC::setSrcImageSize(uint32_t width, uint32_t height)
{
    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_START) {
        GDC_LOGI("Src: size(%dx%d)", width, height);
    }

    m_srcBufFormat.width = width;
    m_srcBufFormat.height = height;

    return NO_ERROR;
}

status_t ExynosGDC::setDstImageSize(uint32_t width, uint32_t height)
{
    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_START) {
        GDC_LOGI("Dst: size(%dx%d)", width, height);
    }

    m_dstBufFormat.width = width;
    m_dstBufFormat.height = height;

    return NO_ERROR;
}

status_t ExynosGDC::setSrcColorFormat(uint32_t format, uint32_t planeCount)
{
    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_START) {
        GDC_LOGI("Src: format(%x) planeCount(%d)", format, planeCount);
    }

    m_srcBufFormat.format = format;
    m_srcBufFormat.planeCount = (uint8_t) planeCount;
    m_srcBufFormat.bufCount = GDC_V4L2_MAX_BUF_COUNT; //To support the buffer index in [0, 31]

    return NO_ERROR;
}

status_t ExynosGDC::setDstColorFormat(uint32_t format, uint32_t planeCount)
{
    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_START) {
        GDC_LOGI("Dst: format(%x) planeCount(%d)", format, planeCount);
    }

    m_dstBufFormat.format = format;
    m_dstBufFormat.planeCount = (uint8_t) planeCount;
    m_dstBufFormat.bufCount = GDC_V4L2_MAX_BUF_COUNT; //To support the buffer index in [0, 31]

    GDC_LOGV("GDC_V4L2_MAX_BUF_COUNT(%d)", GDC_V4L2_MAX_BUF_COUNT);

    return NO_ERROR;
}

status_t ExynosGDC::setSrcBuffer(struct ExynosGDCBuf srcBuf)
{
    status_t ret = NO_ERROR;

    GDC_pixel_size pixelSize = GDC_PIXEL_SIZE_8BIT;
    GDC_pixel_comp_info compression = GDC_NO_COMP;
    uint32_t v4l2format = 0;

    GDC_LOGV("index(%d) planeCount(%d)", srcBuf.index, srcBuf.planeCount);

    ret = m_getFormatInfo(m_srcBufFormat.format, v4l2format, pixelSize, compression);
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to m_getFormatInfo. ret(%d) hal_format(%d) v4l2format(%d) pixelSize(%d) compression(%d)", ret, m_srcBufFormat.format, v4l2format, pixelSize, compression);
    }

    if (srcBuf.planeCount > get_yuv_planes(v4l2format)) {
        GDC_LOGV("Support meta plane to control per-frame size/format");

        void *metaAddr;
        int metaFd = srcBuf.planes[srcBuf.planeCount - 1].fd;
        uint32_t metaSize = srcBuf.planes[srcBuf.planeCount - 1].length;


        if ( metaFd> -1) {
            metaAddr = mmap(NULL, metaSize, (PROT_READ|PROT_WRITE), MAP_SHARED, metaFd, 0);
            if (metaAddr == MAP_FAILED || metaAddr == NULL) {
                GDC_LOGE("Failed to mmap Meta FD. fd(%d) size(%llu) addr(%p)", metaFd, (unsigned long long)metaSize, metaAddr);
                ret = INVALID_OPERATION;
            }

            memset(metaAddr, 0, metaSize);
            struct gdc_metadata* gdcMetadata = (struct gdc_metadata*)metaAddr;
            gdcMetadata->full_width = m_srcBufFormat.width;
            gdcMetadata->full_height = m_srcBufFormat.height;
            gdcMetadata->pixel_format = v4l2format;

            if (munmap(metaAddr, metaSize) < 0) {
                GDC_LOGE("munmap failed");
            }
        } else {
            GDC_LOGE("Invalid fd(%d)", srcBuf.planes[srcBuf.planeCount - 1].fd);
        }

    } else {
        //GDC_LOGW("Do NOT support meta plane to control per-frame size/format (%d/%d)", srcBuf.planeCount, get_yuv_planes(m_srcBufFormat.format));
    }

    m_srcBuf = srcBuf;

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_SET_BUFFERS) {
        GDC_LOGI("srcBuf[%d]: planeCount(%d) fmt(%d, %d, %d) fd(%d,%d,%d) length(%d,%d,%d) bytesperline(%d,%d,%d)",
                m_srcBuf.index, m_srcBuf.planeCount, m_srcBufFormat.format, m_srcBufFormat.planeCount, m_srcBufFormat.bufCount,
                m_srcBuf.planes[0].fd, m_srcBuf.planes[1].fd, m_srcBuf.planes[2].fd,
                m_srcBuf.planes[0].length, m_srcBuf.planes[1].length, m_srcBuf.planes[2].length,
                m_srcBuf.bytesperline[0], m_srcBuf.bytesperline[1], m_srcBuf.bytesperline[2]);
    }

    return NO_ERROR;
}

status_t ExynosGDC::setDstBuffer(struct ExynosGDCBuf dstBuf)
{
    status_t ret = NO_ERROR;

    GDC_pixel_size pixelSize = GDC_PIXEL_SIZE_8BIT;
    GDC_pixel_comp_info compression = GDC_NO_COMP;
    uint32_t v4l2format = 0;

    GDC_LOGV("index(%d) planeCount(%d) HWConnection(%d)", dstBuf.index, dstBuf.planeCount, dstBuf.HWConnection);

    ret = m_getFormatInfo(m_dstBufFormat.format, v4l2format, pixelSize, compression);
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to m_getFormatInfo. ret(%d) hal_format(%d) v4l2format(%d) pixelSize(%d) compression(%d)", ret, m_dstBufFormat.format, v4l2format, pixelSize, compression);
    }

    if (dstBuf.planeCount > get_yuv_planes(v4l2format)) {
        GDC_LOGV("Support meta plane to control per-frame size/format");

        void *metaAddr;
        int metaFd = dstBuf.planes[dstBuf.planeCount - 1].fd;
        uint32_t metaSize = dstBuf.planes[dstBuf.planeCount - 1].length;

        if ( metaFd> -1) {
            metaAddr = mmap(NULL, metaSize, (PROT_READ|PROT_WRITE), MAP_SHARED, metaFd, 0);
            if (metaAddr == MAP_FAILED || metaAddr == NULL) {
                GDC_LOGE("Failed to mmap Meta FD. fd(%d) size(%llu) addr(%p)", metaFd, (unsigned long long)metaSize, metaAddr);
                ret = INVALID_OPERATION;
            }

            memset(metaAddr, 0, metaSize);
            struct gdc_metadata* gdcMetadata = (struct gdc_metadata*)metaAddr;
            gdcMetadata->full_width = m_dstBufFormat.width;
            gdcMetadata->full_height = m_dstBufFormat.height;
            gdcMetadata->pixel_format = v4l2format;

            if (munmap(metaAddr, metaSize) < 0) {
                GDC_LOGE("munmap failed");
            }
        } else {
            GDC_LOGE("Invalid fd(%d)", dstBuf.planes[dstBuf.planeCount - 1].fd);
        }

    } else {
        //GDC_LOGW("Do NOT support meta plane to control per-frame size/format (%d/%d)", dstBuf.planeCount, get_yuv_planes(m_dstBufFormat.format));
    }

    m_dstBuf = dstBuf;

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_SET_BUFFERS) {
        GDC_LOGI("dstBuf[%d]: planeCount(%d) fmt(%d, %d, %d) fd(%d,%d,%d) length(%d,%d,%d) bytesperline(%d,%d,%d) HWConnection(%d)",
                m_dstBuf.index, m_dstBuf.planeCount, m_dstBufFormat.format, m_dstBufFormat.planeCount, m_dstBufFormat.bufCount,
                m_dstBuf.planes[0].fd, m_dstBuf.planes[1].fd, m_dstBuf.planes[2].fd,
                m_dstBuf.planes[0].length, m_dstBuf.planes[1].length, m_dstBuf.planes[2].length,
                m_dstBuf.bytesperline[0], m_dstBuf.bytesperline[1], m_dstBuf.bytesperline[2], m_dstBuf.HWConnection);
    }

    return NO_ERROR;
}

status_t ExynosGDC::setGridTable(struct ExynosGDCGridTable *gridTable)
{
    if (!gridTable) {
        GDC_LOGE("GridTable is NULL.");
        return BAD_VALUE;
    }

    GDC_LOGV("size(%dx%d)", gridTable->width, gridTable->height);

    m_gridTable = *gridTable;

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_GRID_VALUES) {
        /* Sampling from 33x33 to 5x5 */
        GDC_LOGI("Sampled gridX:");
        GDC_LOGI("[ 0][ 0]: %10d, [ 0][ 8]: %10d, [ 0][16]: %10d, [ 0][24]: %10d, [ 0][32]: %10d", m_gridTable.gridX[0][0], m_gridTable.gridX[0][8], m_gridTable.gridX[0][16], m_gridTable.gridX[0][24], m_gridTable.gridX[0][32]);
        GDC_LOGI("[ 8][ 0]: %10d, [ 8][ 8]: %10d, [ 8][16]: %10d, [ 8][24]: %10d, [ 8][32]: %10d", m_gridTable.gridX[8][0], m_gridTable.gridX[8][8], m_gridTable.gridX[8][16], m_gridTable.gridX[8][24], m_gridTable.gridX[8][32]);
        GDC_LOGI("[16][ 0]: %10d, [16][ 8]: %10d, [16][16]: %10d, [16][24]: %10d, [16][32]: %10d", m_gridTable.gridX[16][0], m_gridTable.gridX[16][8], m_gridTable.gridX[16][16], m_gridTable.gridX[16][24], m_gridTable.gridX[16][32]);
        GDC_LOGI("[24][ 0]: %10d, [24][ 8]: %10d, [24][16]: %10d, [24][24]: %10d, [24][32]: %10d", m_gridTable.gridX[24][0], m_gridTable.gridX[24][8], m_gridTable.gridX[24][16], m_gridTable.gridX[24][24], m_gridTable.gridX[24][32]);
        GDC_LOGI("[32][ 0]: %10d, [32][ 8]: %10d, [32][16]: %10d, [32][24]: %10d, [32][32]: %10d", m_gridTable.gridX[32][0], m_gridTable.gridX[32][8], m_gridTable.gridX[32][16], m_gridTable.gridX[32][24], m_gridTable.gridX[32][32]);

        GDC_LOGI("Sampled gridY:");
        GDC_LOGI("[ 0][ 0]: %10d, [ 0][ 8]: %10d, [ 0][16]: %10d, [ 0][24]: %10d, [ 0][32]: %10d", m_gridTable.gridY[0][0], m_gridTable.gridY[0][8], m_gridTable.gridY[0][16], m_gridTable.gridY[0][24], m_gridTable.gridY[0][32]);
        GDC_LOGI("[ 8][ 0]: %10d, [ 8][ 8]: %10d, [ 8][16]: %10d, [ 8][24]: %10d, [ 8][32]: %10d", m_gridTable.gridY[8][0], m_gridTable.gridY[8][8], m_gridTable.gridY[8][16], m_gridTable.gridY[8][24], m_gridTable.gridY[8][32]);
        GDC_LOGI("[16][ 0]: %10d, [16][ 8]: %10d, [16][16]: %10d, [16][24]: %10d, [16][32]: %10d", m_gridTable.gridY[16][0], m_gridTable.gridY[16][8], m_gridTable.gridY[16][16], m_gridTable.gridY[16][24], m_gridTable.gridY[16][32]);
        GDC_LOGI("[24][ 0]: %10d, [24][ 8]: %10d, [24][16]: %10d, [24][24]: %10d, [24][32]: %10d", m_gridTable.gridY[24][0], m_gridTable.gridY[24][8], m_gridTable.gridY[24][16], m_gridTable.gridY[24][24], m_gridTable.gridY[24][32]);
        GDC_LOGI("[32][ 0]: %10d, [32][ 8]: %10d, [32][16]: %10d, [32][24]: %10d, [32][32]: %10d", m_gridTable.gridY[32][0], m_gridTable.gridY[32][8], m_gridTable.gridY[32][16], m_gridTable.gridY[32][24], m_gridTable.gridY[32][32]);
    }

    return NO_ERROR;
}

status_t ExynosGDC::setInputSizeParam(struct ExynosGDCSizeParam inputSize)
{
    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_SET_SZPARAM) {
        GDC_LOGI("Input crop: (%d,%d %dx%d %dx%d)",
                inputSize.cropX, inputSize.cropY, inputSize.cropW, inputSize.cropH, inputSize.fullW, inputSize.fullH);
    }

    m_inputSizeParam = inputSize;

    return NO_ERROR;
}

status_t ExynosGDC::setOutputSizeParam(struct ExynosGDCSizeParam outputSize)
{
    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_SET_SZPARAM) {
        GDC_LOGI("Output crop: (%d,%d %dx%d %dx%d)",
                outputSize.cropX, outputSize.cropY, outputSize.cropW, outputSize.cropH, outputSize.fullW, outputSize.fullH);
    }

    m_outputSizeParam = outputSize;

    return NO_ERROR;

}

status_t ExynosGDC::setFormat()
{
    status_t ret = NO_ERROR;

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_SET_FORMAT) {
        GDC_LOGI("SRC size(%dx%d) planeCount(%d) format(%x) DST size(%dx%d) planeCount(%d) format(%x)",
                m_srcBufFormat.width, m_srcBufFormat.height,
                m_srcBufFormat.planeCount, m_srcBufFormat.format,
                m_dstBufFormat.width, m_dstBufFormat.height,
                m_dstBufFormat.planeCount, m_dstBufFormat.format);
    }

    ret = m_setFormat(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to setFormat for Input. ret(%d)", ret);
        return INVALID_OPERATION;
    }

    ret = m_setFormat(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (ret != NO_ERROR) {
        GDC_LOGE("Failed to setFormat for Output. ret(%d)", ret);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosGDC::start()
{
    status_t ret = NO_ERROR;

    GDC_LOGV("");

    m_srcBufList.clear();
    m_dstBufList.clear();

    /* Output stream on */
    ret = exynos_v4l2_streamon(m_videoFd, (enum v4l2_buf_type) GDC_OUTPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d][OUT]Failed to exynos_v4l2_streamon. ret(%d)", m_videoFd, ret);
        goto err_exit;
    }

    /* Input stream on */
    ret = exynos_v4l2_streamon(m_videoFd, (enum v4l2_buf_type) GDC_INPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d][IN]Failed to exynos_v4l2_streamon. ret(%d)", m_videoFd, ret);
        goto err_exit;
    }

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_START) {
        GDC_LOGI("inputcrop(%d,%d %dx%d), outputcrop(%d,%d %dx%d), gridsize(%dx%d), HWConnection(%d), grid_mode(%d), bypass_mode(%d), downscaleMode(%d)",
                m_inputSizeParam.cropX, m_inputSizeParam.cropY, m_inputSizeParam.cropW, m_inputSizeParam.cropH,
                m_outputSizeParam.cropX, m_outputSizeParam.cropY, m_outputSizeParam.cropW, m_outputSizeParam.cropH,
                m_gridTable.width, m_gridTable.height, m_dstBuf.HWConnection, m_srcBuf.gridMode, m_srcBuf.bypassMode, m_srcBuf.downscaleMode);

        GDC_LOGI("srcBuf: fmt(%d, %d, %d) fd(%d,%d,%d) length(%d,%d,%d) bytesperline(%d,%d,%d), dstBuf: fmt(%d, %d, %d) fd(%d,%d,%d) length(%d,%d,%d) bytesperline(%d,%d,%d)",
                m_srcBufFormat.format, m_srcBufFormat.planeCount, m_srcBufFormat.bufCount,
                m_srcBuf.planes[0].fd, m_srcBuf.planes[1].fd, m_srcBuf.planes[2].fd,
                m_srcBuf.planes[0].length, m_srcBuf.planes[1].length, m_srcBuf.planes[2].length,
                m_srcBuf.bytesperline[0], m_srcBuf.bytesperline[1], m_srcBuf.bytesperline[2],
                m_dstBufFormat.format, m_dstBufFormat.planeCount, m_dstBufFormat.bufCount,
                m_dstBuf.planes[0].fd, m_dstBuf.planes[1].fd, m_dstBuf.planes[2].fd,
                m_dstBuf.planes[0].length, m_dstBuf.planes[1].length, m_dstBuf.planes[2].length,
                m_dstBuf.bytesperline[0], m_dstBuf.bytesperline[1], m_dstBuf.bytesperline[2]);
    }

    return NO_ERROR;

err_exit:
    ret = stop();
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d]Failed to stop. ret(%d)", m_videoFd, ret);
    }
    return INVALID_OPERATION;
}

status_t ExynosGDC::updateGridForDownscale(uint32_t srcWidth, uint32_t srcHeight, uint32_t dstWidth, uint32_t dstHeight, bool gridMode)
{
    int wi = srcWidth;
    int hi = srcHeight;
    int wo = dstWidth;
    int ho = dstHeight;

    double ra_deg = 0; //No rotation
    double ra = ra_deg * PI / 180;

    double scaleInvX = (double)srcWidth / (double)dstWidth;
    double scaleInvY = (double)srcHeight / (double)dstHeight;
    int grid_mode = (int)gridMode;

    int nx = m_gridTable.width;
    int ny = m_gridTable.height;

    int wx, wy;
    if (grid_mode == 1)
    {
        wx = wo;
        wy = ho;
    }
    else
    {
        wx = wi;
        wy = hi;
    }

    for (int iy = 0; iy < ny; iy++)
    {
        for (int ix = 0; ix < nx; ix++)
        {
            double gx = -0.5 + ix * (0.5 + 0.5) / (nx - 1);
            double gy = -0.5 + iy * (0.5 + 0.5) / (ny - 1);

            double dx_tmp = cos(ra) * gx * scaleInvX - sin(ra) * gy * scaleInvY;
            double dy_tmp = sin(ra) * gx * scaleInvX + cos(ra) * gy * scaleInvY;

            dx_tmp = ((dx_tmp - gx) * 8192 * 512) * wx / wi;
            dy_tmp = ((dy_tmp - gy) * 6144 * 512) * wy / hi;

            m_gridTable.gridX[iy][ix] = round(dx_tmp);
            m_gridTable.gridY[iy][ix] = round(dy_tmp);
        }
    }

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_GRID_DOWNSCALE) {
        /* Sampling from 33x33 to 5x5 */
        GDC_LOGI("Downscaled mode enabled");
        GDC_LOGI("Recalculated gridX(%.5f):", scaleInvX);
        GDC_LOGI("[ 0][ 0]: %10d, [ 0][ 8]: %10d, [ 0][16]: %10d, [ 0][24]: %10d, [ 0][32]: %10d", m_gridTable.gridX[0][0], m_gridTable.gridX[0][8], m_gridTable.gridX[0][16], m_gridTable.gridX[0][24], m_gridTable.gridX[0][32]);
        GDC_LOGI("[ 8][ 0]: %10d, [ 8][ 8]: %10d, [ 8][16]: %10d, [ 8][24]: %10d, [ 8][32]: %10d", m_gridTable.gridX[8][0], m_gridTable.gridX[8][8], m_gridTable.gridX[8][16], m_gridTable.gridX[8][24], m_gridTable.gridX[8][32]);
        GDC_LOGI("[16][ 0]: %10d, [16][ 8]: %10d, [16][16]: %10d, [16][24]: %10d, [16][32]: %10d", m_gridTable.gridX[16][0], m_gridTable.gridX[16][8], m_gridTable.gridX[16][16], m_gridTable.gridX[16][24], m_gridTable.gridX[16][32]);
        GDC_LOGI("[24][ 0]: %10d, [24][ 8]: %10d, [24][16]: %10d, [24][24]: %10d, [24][32]: %10d", m_gridTable.gridX[24][0], m_gridTable.gridX[24][8], m_gridTable.gridX[24][16], m_gridTable.gridX[24][24], m_gridTable.gridX[24][32]);
        GDC_LOGI("[32][ 0]: %10d, [32][ 8]: %10d, [32][16]: %10d, [32][24]: %10d, [32][32]: %10d", m_gridTable.gridX[32][0], m_gridTable.gridX[32][8], m_gridTable.gridX[32][16], m_gridTable.gridX[32][24], m_gridTable.gridX[32][32]);

        GDC_LOGI("Recalculated gridY(%.5f):", scaleInvY);
        GDC_LOGI("[ 0][ 0]: %10d, [ 0][ 8]: %10d, [ 0][16]: %10d, [ 0][24]: %10d, [ 0][32]: %10d", m_gridTable.gridY[0][0], m_gridTable.gridY[0][8], m_gridTable.gridY[0][16], m_gridTable.gridY[0][24], m_gridTable.gridY[0][32]);
        GDC_LOGI("[ 8][ 0]: %10d, [ 8][ 8]: %10d, [ 8][16]: %10d, [ 8][24]: %10d, [ 8][32]: %10d", m_gridTable.gridY[8][0], m_gridTable.gridY[8][8], m_gridTable.gridY[8][16], m_gridTable.gridY[8][24], m_gridTable.gridY[8][32]);
        GDC_LOGI("[16][ 0]: %10d, [16][ 8]: %10d, [16][16]: %10d, [16][24]: %10d, [16][32]: %10d", m_gridTable.gridY[16][0], m_gridTable.gridY[16][8], m_gridTable.gridY[16][16], m_gridTable.gridY[16][24], m_gridTable.gridY[16][32]);
        GDC_LOGI("[24][ 0]: %10d, [24][ 8]: %10d, [24][16]: %10d, [24][24]: %10d, [24][32]: %10d", m_gridTable.gridY[24][0], m_gridTable.gridY[24][8], m_gridTable.gridY[24][16], m_gridTable.gridY[24][24], m_gridTable.gridY[24][32]);
        GDC_LOGI("[32][ 0]: %10d, [32][ 8]: %10d, [32][16]: %10d, [32][24]: %10d, [32][32]: %10d", m_gridTable.gridY[32][0], m_gridTable.gridY[32][8], m_gridTable.gridY[32][16], m_gridTable.gridY[32][24], m_gridTable.gridY[32][32]);
    }

    return NO_ERROR;
}

status_t ExynosGDC::setControl()
{
    status_t ret = NO_ERROR;
    struct v4l2_ext_controls extCtrls;
    struct v4l2_ext_control extCtrl;
    struct gdc_crop_param gdcCropParam;

    memset(&extCtrl, 0x00, sizeof(extCtrl));
    memset(&extCtrls, 0x00, sizeof(extCtrls));
    memset(&gdcCropParam, 0x00, sizeof(gdcCropParam));

    extCtrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    extCtrls.count = 1;
    extCtrls.controls = &extCtrl;
    extCtrl.id = V4L2_CID_CAMERAPP_GDC_GRID_CONTROL;
    extCtrl.ptr = &gdcCropParam;
    gdcCropParam.use_calculated_grid = true;

    switch(m_dstBuf.HWConnection) {
    case EXYNOS_GDC_MFC_CONNECTTION_M2M:
        gdcCropParam.votf_en = 0;
        break;
    case EXYNOS_GDC_MFC_CONNECTTION_OTF:
        gdcCropParam.votf_en = 0;
        gdcCropParam.out_mode = GDC_OUT_OTF;
        break;
    case EXYNOS_GDC_MFC_CONNECTTION_VIRTUAL_OTF:
        gdcCropParam.votf_en = 1;
        gdcCropParam.out_mode = GDC_OUT_VOTF;
        break;
    case EXYNOS_GDC_MFC_CONNECTTION_NONE:
    default:
        gdcCropParam.votf_en = 0;
        break;
    }

    /* Input crop */
    gdcCropParam.crop_start_x = m_inputSizeParam.cropX;
    gdcCropParam.crop_start_y = m_inputSizeParam.cropY;
    gdcCropParam.crop_width = m_inputSizeParam.cropW;
    gdcCropParam.crop_height = m_inputSizeParam.cropH;

    gdcCropParam.buf_Index = m_srcBuf.index;
    gdcCropParam.is_grid_mode = m_srcBuf.gridMode;
    gdcCropParam.is_bypass_mode = m_srcBuf.bypassMode;

    for (int i = 0; i < GDC_MAX_PLANES; i++) {
        gdcCropParam.src_bytesperline[i] = m_srcBuf.bytesperline[i];
        gdcCropParam.dst_bytesperline[i] = m_dstBuf.bytesperline[i];
    }

    if (m_srcBuf.downscaleMode) {
        /* Update grid for downscale */
        updateGridForDownscale(m_srcBufFormat.width, m_srcBufFormat.height, m_dstBufFormat.width, m_dstBufFormat.height, m_srcBuf.gridMode);
    }

    /* Grid table */
    for (uint32_t indexY = 0; indexY < m_gridTable.height; indexY++) {
        for (uint32_t indexX = 0; indexX < m_gridTable.width; indexX++) {
            gdcCropParam.calculated_grid_x[indexY][indexX] = m_gridTable.gridX[indexY][indexX];
            gdcCropParam.calculated_grid_y[indexY][indexX] = m_gridTable.gridY[indexY][indexX];
        }
    }

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_SET_CONTROL) {
        GDC_LOGI("bufIndex(%d) InputCrop(%d,%d %dx%d) GridSize(%dx%d), votf_en(%d), grid_mode(%d), bypass_mode(%d), downscaleMode(%d), out_mode(%d)",
                gdcCropParam.buf_Index,
                gdcCropParam.crop_start_x, gdcCropParam.crop_start_y,
                gdcCropParam.crop_width, gdcCropParam.crop_height,
                m_gridTable.width, m_gridTable.height,
                gdcCropParam.votf_en, gdcCropParam.is_grid_mode, gdcCropParam.is_bypass_mode, m_srcBuf.downscaleMode,
                gdcCropParam.out_mode);
    }

    ret = exynos_v4l2_s_ext_ctrl(m_videoFd, &extCtrls);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d]Failed to exynos_v4l2_s_ext_ctrl. ret(%d)", m_videoFd, ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosGDC::run()
{
    status_t totalRet = NO_ERROR;
    status_t ret = NO_ERROR;

    GDC_LOGV("");

    /* Put output buffer */
    ret = m_qBuf(m_dstBuf, (uint32_t) GDC_OUTPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[OUT]Failed to qBuf. ret(%d)", ret);
        /* Keep going */
    }
    totalRet |= ret;

    /* Put input buffer */
    ret = m_qBuf(m_srcBuf, (uint32_t) GDC_INPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[IN]Failed to qBuf. ret(%d)", ret);
        /* Keep going */
    }
    totalRet |= ret;

    /* Get input buffer */
    ret = m_dqBuf(m_srcBuf, (uint32_t) GDC_INPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[IN]Failed to dqBuf. ret(%d)", ret);
        /* Keep going*/
    }
    totalRet |= ret;

    /* Get output buffer */
    ret = m_dqBuf(m_dstBuf, (uint32_t) GDC_OUTPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[OUT]Failed to dqBuf. ret(%d)", ret);
        /* Keep going */
    }
    totalRet |= ret;

    return totalRet;
}

status_t ExynosGDC::stop()
{
    status_t totalRet = NO_ERROR;
    status_t ret = NO_ERROR;

    GDC_LOGV("");

    /* Input stream off */
    ret = exynos_v4l2_streamoff(m_videoFd, (enum v4l2_buf_type) GDC_INPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d][IN]Failed to exynos_v4l2_streamoff. ret(%d)", m_videoFd, ret);
        /* continue */
    }
    totalRet |= ret;

    /* Output stream off */
    ret = exynos_v4l2_streamoff(m_videoFd, (enum v4l2_buf_type) GDC_OUTPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d][OUT]Failed to exynos_v4l2_streamoff. ret(%d)", m_videoFd, ret);
        /* continue */
    }
    totalRet |= ret;

    return totalRet;
}

status_t ExynosGDC::dump()
{
    GDC_LOGD("");

    return NO_ERROR;
}

status_t ExynosGDC::pollLast()
{
    status_t totalRet = NO_ERROR;
    status_t ret = NO_ERROR;

    GDC_LOGV("buf_index(%d)", m_srcBuf.index);

    /* Put output buffer */
    ret = m_qBuf(m_dstBuf, (uint32_t) GDC_OUTPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[OUT]Failed to qBuf. ret(%d)", ret);
        /* Keep going */
    }
    totalRet |= ret;

    /* Put input buffer */
    ret = m_qBuf(m_srcBuf, (uint32_t) GDC_INPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[IN]Failed to qBuf. ret(%d)", ret);
        /* Keep going */
    }
    totalRet |= ret;

    //push_back buf to list
    m_srcBufList.push_back(m_srcBuf);
    m_dstBufList.push_back(m_dstBuf);

    return totalRet;
}

status_t ExynosGDC::pollFirst(uint32_t & bufIndex)
{
    status_t totalRet = NO_ERROR;
    status_t ret = NO_ERROR;

    struct ExynosGDCBuf srcBuf;
    struct ExynosGDCBuf dstBuf;

    //pop_front buf from list
    if (m_srcBufList.size() <= 0) {
        GDC_LOGE("[IN]Empty m_srcBufList. size(%d)", (int)m_srcBufList.size());
    }
    srcBuf = m_srcBufList.front();
    m_srcBufList.pop_front();

    if (m_dstBufList.size() <= 0) {
        GDC_LOGE("[OUT]Empty m_dstBufList. size(%d)", (int)m_dstBufList.size());
    }
    dstBuf = m_dstBufList.front();
    m_dstBufList.pop_front();

    /* Get input buffer */
    ret = m_dqBuf(srcBuf, (uint32_t) GDC_INPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[IN]Failed to dqBuf. ret(%d)", ret);
        /* Keep going*/
    }
    totalRet |= ret;

    /* Get output buffer */
    ret = m_dqBuf(dstBuf, (uint32_t) GDC_OUTPUT_V4L2_BUF_TYPE);
    if (ret != NO_ERROR) {
        GDC_LOGE("[OUT]Failed to dqBuf. ret(%d)", ret);
        /* Keep going */
    }
    totalRet |= ret;

    bufIndex = srcBuf.index;

    return totalRet;

}

int ExynosGDC::m_getFormatInfo(int hal_pixel_format, uint32_t &v4l2_format, GDC_pixel_size &pixelSize, GDC_pixel_comp_info &compression)
{
    status_t ret = NO_ERROR;

    pixelSize = GDC_PIXEL_SIZE_8BIT;
    compression = GDC_NO_COMP;

    v4l2_format = HAL_PIXEL_FORMAT_2_V4L2_PIX(hal_pixel_format);

    gdc_pixel_format pixel_format = static_cast<gdc_pixel_format>(hal_pixel_format);
    switch (pixel_format) {
    /* Lossless */
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
#if 1 /* Not support YCrCb 8 bit */
        v4l2_format = -1;
#endif
        /* Not support YCrCb 8 bit
        v4l2_format = V4L2_PIX_FMT_NV21M;
        pixelSize = GDC_PIXEL_SIZE_8BIT;
        compression = GDC_COMP;
        */
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
#if 1 /* Not support YCrCb 10 bit */
        v4l2_format = -1;
#endif
        /* Not support YCrCb 10 bit
        v4l2_format = V4L2_PIX_FMT_NV21M;
        pixelSize = GDC_PIXEL_SIZE_10BIT;
        compression = GDC_COMP;
        */
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
        v4l2_format = V4L2_PIX_FMT_NV12M;
        pixelSize = GDC_PIXEL_SIZE_8BIT;
        compression = GDC_COMP;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
        v4l2_format = V4L2_PIX_FMT_NV12M_P010;
        pixelSize = GDC_PIXEL_SIZE_10BIT;
        compression = GDC_COMP;
        break;
    /* Lossy */
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
        v4l2_format = V4L2_PIX_FMT_NV12M_SBWCL_8B;
        pixelSize = GDC_PIXEL_SIZE_8BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
        v4l2_format = V4L2_PIX_FMT_NV12M_SBWCL_10B;
        pixelSize = GDC_PIXEL_SIZE_10BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_32_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12M_SBWCL_32_8B;
        pixelSize = GDC_PIXEL_SIZE_8BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_64_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12M_SBWCL_64_8B;
        pixelSize = GDC_PIXEL_SIZE_8BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SPN_32_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12N_SBWCL_32_8B;
        pixelSize = GDC_PIXEL_SIZE_8BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SPN_64_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12N_SBWCL_64_8B;
        pixelSize = GDC_PIXEL_SIZE_8BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_32_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12M_SBWCL_32_10B;
        pixelSize = GDC_PIXEL_SIZE_10BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SP_M_10B_64_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12M_SBWCL_64_10B;
        pixelSize = GDC_PIXEL_SIZE_10BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_32_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12N_SBWCL_32_10B;
        pixelSize = GDC_PIXEL_SIZE_10BIT;
        compression = GDC_COMP_LOSS;
        break;
    case gdc_pixel_format::HAL_PIXEL_FORMAT_EXYNOS_420_SPN_10B_64_SBWC_L:
        v4l2_format = V4L2_PIX_FMT_NV12N_SBWCL_64_10B;
        pixelSize = GDC_PIXEL_SIZE_10BIT;
        compression = GDC_COMP_LOSS;
        break;
    }

    GDC_LOGV("compression(%d)", compression);

    if (v4l2_format == -1) {
        GDC_LOGE("[FD%d]Failed to getFormatInfo. ret(%d) hal_format(%d) v4l2format(%d) pixelSize(%d) compression(%d)", m_videoFd, ret, hal_pixel_format, v4l2_format, pixelSize, compression);
        ret = INVALID_OPERATION;
    }

    return ret;
}

status_t ExynosGDC::m_setFormat(uint32_t v4l2BufType)
{
    status_t ret = NO_ERROR;
    bool isInput = false;
    struct v4l2_format v4l2Format;
    struct v4l2_requestbuffers v4l2ReqBufs;
    struct ExynosGDCBufFormat *bufFormat = NULL;
    GDC_pixel_size pixelSize = GDC_PIXEL_SIZE_8BIT;
    GDC_pixel_comp_info compression = GDC_NO_COMP;
    uint32_t v4l2format = 0;

    switch ((enum v4l2_buf_type) v4l2BufType) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        isInput = true;
        break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
        isInput = false;
        break;
    default:
        GDC_LOGE("NOT supported v4l2BufType(%d)", v4l2BufType);
        return BAD_VALUE;
    }

    if (isInput) {
        bufFormat = &m_srcBufFormat;
    } else {
        bufFormat = &m_dstBufFormat;
    }

    ret = m_getFormatInfo(bufFormat->format, v4l2format, pixelSize, compression);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d]Failed to m_getFormatInfo. ret(%d) hal_format(%d) v4l2format(%d) pixelSize(%d) compression(%d)", m_videoFd, ret, bufFormat->format, v4l2format, pixelSize, compression);
    }

    v4l2Format.fmt.pix_mp.width = bufFormat->width;
    v4l2Format.fmt.pix_mp.height = bufFormat->height;
    v4l2Format.fmt.pix_mp.pixelformat = v4l2format;
    v4l2Format.fmt.pix_mp.num_planes = bufFormat->planeCount;
    if (compression >= GDC_COMP) {
        v4l2Format.fmt.pix_mp.flags = GET_PIXEL_FLAG(pixelSize, compression);
    }

    if (m_debugLevel >= (int)LIBGDC_DEBUG_LEVEL_SET_FORMAT) {
        GDC_LOGI("isInput(%d) WxH(%dx%d) fmt.pix_mp.flags(%x) pixelSize(%d) compression(%d) halformat(%x) v4l2format(%c%c%c%c)",
                    isInput, v4l2Format.fmt.pix_mp.width, v4l2Format.fmt.pix_mp.height, v4l2Format.fmt.pix_mp.flags,
                    pixelSize, compression, bufFormat->format,
                    (char)((v4l2format >> 0) & 0xFF),
                    (char)((v4l2format >> 8) & 0xFF),
                    (char)((v4l2format >> 16) & 0xFF),
                    (char)((v4l2format >> 24) & 0xFF));
    }

    v4l2Format.type = v4l2BufType;

    ret = exynos_v4l2_s_fmt(m_videoFd, &v4l2Format);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d][%s]Failed to exynos_v4l2_s_fmt. ret(%d)",
                m_videoFd, (isInput ? "IN" : "OUT"), ret);
        return ret;
    }

    v4l2ReqBufs.count = bufFormat->bufCount;
    v4l2ReqBufs.type = v4l2BufType;
    v4l2ReqBufs.memory = GDC_V4L2_MEMORY_TYPE;

    ret = exynos_v4l2_reqbufs(m_videoFd, &v4l2ReqBufs);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d][%s]Failed to exynos_v4l2_reqbufs. ret(%d)",
                m_videoFd, (isInput ? "IN" : "OUT"), ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosGDC::m_qBuf(struct ExynosGDCBuf buf, uint32_t v4l2BufType)
{
    status_t ret = NO_ERROR;
    bool isInput = false;
    struct v4l2_buffer v4l2Buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    memset(&v4l2Buf, 0x00, sizeof(struct v4l2_buffer));
    memset(&planes, 0x00, (sizeof(struct v4l2_plane) * VIDEO_MAX_PLANES));

    /* Buffer set-up */
    v4l2Buf.m.planes = planes;
    v4l2Buf.index = buf.index;
    v4l2Buf.length = buf.planeCount;
    v4l2Buf.memory = GDC_V4L2_MEMORY_TYPE;

    switch ((enum v4l2_buf_type) v4l2BufType) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        isInput = true;
        v4l2Buf.type = GDC_INPUT_V4L2_BUF_TYPE;
        v4l2Buf.flags |= ((V4L2_BUF_FLAG_NO_CACHE_CLEAN) | (V4L2_BUF_FLAG_NO_CACHE_INVALIDATE));
        break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
        isInput = false;
        v4l2Buf.type = GDC_OUTPUT_V4L2_BUF_TYPE;
        v4l2Buf.flags |= (V4L2_BUF_FLAG_NO_CACHE_CLEAN);
        break;
    default:
        GDC_LOGE("NOT supported v4l2BufType(%d)", v4l2BufType);
        return BAD_VALUE;
    }

    /* Plane set-up */
    for (uint32_t planeIndex = 0; planeIndex < v4l2Buf.length; planeIndex++) {
        v4l2Buf.m.planes[planeIndex].m.fd = buf.planes[planeIndex].fd;
        v4l2Buf.m.planes[planeIndex].length = buf.planes[planeIndex].length;
    }

    ret = exynos_v4l2_qbuf(m_videoFd, &v4l2Buf);
    if (ret != NO_ERROR) {
        GDC_LOGE("[FD%d][%s]Failed to exynos_v4l2_qbuf. ret(%d)",
                m_videoFd, (isInput ? "IN" : "OUT"), ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosGDC::m_dqBuf(struct ExynosGDCBuf buf, uint32_t v4l2BufType)
{
    status_t ret = NO_ERROR;
    bool isInput = false;
    struct v4l2_buffer v4l2Buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    memset(&v4l2Buf, 0x00, sizeof(struct v4l2_buffer));
    memset(&planes, 0x00, (sizeof(struct v4l2_plane) * VIDEO_MAX_PLANES));

    /* Buffer set-up */
    v4l2Buf.m.planes = planes;
    v4l2Buf.length = buf.planeCount;
    v4l2Buf.memory = GDC_V4L2_MEMORY_TYPE;

    switch ((enum v4l2_buf_type) v4l2BufType) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
        isInput = true;
        v4l2Buf.type = GDC_INPUT_V4L2_BUF_TYPE;
        break;
    case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
        isInput = false;
        v4l2Buf.type = GDC_OUTPUT_V4L2_BUF_TYPE;
        break;
    default:
        GDC_LOGE("NOT supported v4l2BufType(%d)", v4l2BufType);
        return BAD_VALUE;
    }

    ret = exynos_v4l2_dqbuf(m_videoFd, &v4l2Buf);
    if (ret != NO_ERROR) {
        if (ret != -EAGAIN) {
            GDC_LOGE("[FD%d][%s]Failed to exynos_v4l2_dqbuf. ret(%d)",
                    m_videoFd, (isInput ? "IN" : "OUT"), ret);
            return ret;
        }
    }

    if (v4l2Buf.flags & V4L2_BUF_FLAG_ERROR) {
        GDC_LOGE("[FD%d][%s]dqbuf error. (%d)",
                m_videoFd, (isInput ? "IN" : "OUT"), V4L2_BUF_FLAG_ERROR);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

/* For runtime debugging feature */
int ExynosGDC::gdc_property_retrieve(const char *key, char *value)
{
    return __system_property_get(key, value);
}

int ExynosGDC::gdc_property_get_internal(const char *key, char *value, const char *default_value)
{
    int len;

    len = gdc_property_retrieve(key, value);
    if (len > 0)
    {
        return len;
    }
    if (default_value)
    {
        len = (int)strlen(default_value);
        if (len >= PROP_VALUE_MAX)
        {
            len = PROP_VALUE_MAX - 1;
        }
        memcpy(value, default_value, (size_t)len);
        value[len] = '\0';
    }
    return len;
}

int ExynosGDC::gdc_property_get(const char *key, char *value, int default_value)
{
    char buffer[33];
    snprintf(buffer, 33, "%d", default_value);
    return gdc_property_get_internal(key, value, buffer);
}
