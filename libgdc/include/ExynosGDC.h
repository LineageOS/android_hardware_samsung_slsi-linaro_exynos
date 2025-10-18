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

#ifndef EXYNOS_GDC_H
#define EXYNOS_GDC_H

#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <linux/videodev2.h>

#include "ExynosGDCTypes.h"
#include "exynos_format.h"

#include <list>
#include <math.h>

#define GDC_VIDEO_FILE_PREFIX       "/dev/video"
#define GDC_VIDEO_NUM               155
#define GDC_INPUT_V4L2_BUF_TYPE     V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
#define GDC_OUTPUT_V4L2_BUF_TYPE    V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
#define GDC_V4L2_MEMORY_TYPE        V4L2_MEMORY_DMABUF
#define GDC_V4L2_MAX_BUF_COUNT      VIDEO_MAX_FRAME
#define GDC_BUF_MAX_PLANE           4 //Image planes(3) + Metadata plane(1)
#define PI                          3.141592653589793

typedef enum {
    LIBGDC_DEBUG_LEVEL_OFF = 0,         // not printing any logs
    LIBGDC_DEBUG_LEVEL_START,           // printing start info
    LIBGDC_DEBUG_LEVEL_SET_SZPARAM,     // printing size param info
    LIBGDC_DEBUG_LEVEL_SET_FORMAT,      // printing v4l2 format info
    LIBGDC_DEBUG_LEVEL_SET_CONTROL,     // printing gdcCropParam info
    LIBGDC_DEBUG_LEVEL_SET_BUFFERS,     // printing buffer info
    LIBGDC_DEBUG_LEVEL_GRID_VALUES,     // printing grid data
    LIBGDC_DEBUG_LEVEL_GRID_DOWNSCALE,  // printing downscaled grid data
    LIBGDC_DEBUG_LEVEL_LAST
} LIBGDC_DEBUG_LEVEL;

using namespace android;

class ExynosGDC
{
public:
    ExynosGDC();
    virtual ~ExynosGDC();

#ifdef GDC_USE_OLD_FUNCTION_ALIGNMENT
    virtual status_t open();
#else
    virtual status_t open(int videoNum);
#endif
    virtual status_t release();

public: //Data setting functions
    virtual status_t setSrcImageSize(uint32_t width, uint32_t height);
    virtual status_t setDstImageSize(uint32_t width, uint32_t height);
    virtual status_t setSrcColorFormat(uint32_t format, uint32_t planeCount);
    virtual status_t setDstColorFormat(uint32_t format, uint32_t planeCount);
    virtual status_t setSrcBuffer(struct ExynosGDCBuf srcBuf);
    virtual status_t setDstBuffer(struct ExynosGDCBuf dstBuf);
    virtual status_t setGridTable(struct ExynosGDCGridTable *gridTable);
    virtual status_t setInputSizeParam(struct ExynosGDCSizeParam inputSize);
    virtual status_t setOutputSizeParam(struct ExynosGDCSizeParam outputSize);
    virtual status_t updateGridForDownscale(uint32_t srcWidth, uint32_t srcHeight, uint32_t dstWidth, uint32_t dstHeight, bool gridMode);

public: //H/W related functions
    virtual status_t setFormat();
    virtual status_t start();
    virtual status_t setControl();
    virtual status_t run();
    virtual status_t stop();

    virtual status_t dump();

    virtual status_t pollLast();
    virtual status_t pollFirst(uint32_t & bufIndex);

    //Runtime debugging feature
    int gdc_property_retrieve(const char *key, char *value);
    int gdc_property_get_internal(const char *key, char *value, const char *default_value);
    int gdc_property_get(const char *key, char *value, int default_value);

private: //Internal functions
    status_t m_setFormat(uint32_t v4l2BufType);
    status_t m_qBuf(struct ExynosGDCBuf buf, uint32_t v4l2BufType);
    status_t m_dqBuf(struct ExynosGDCBuf buf, uint32_t v4l2BufType);
    int m_getFormatInfo(int hal_pixel_format, uint32_t &v4l2_format, GDC_pixel_size &pixelSize, GDC_pixel_comp_info &compression);

private: //Member variables
    struct ExynosGDCBufFormat {
        uint32_t width;
        uint32_t height;
        uint8_t planeCount;
        uint32_t format;
        uint32_t bufCount;

        ExynosGDCBufFormat() {
            width = 0;
            height = 0;
            planeCount = 0;
            format = 0;
            bufCount = 0;
        }

        ExynosGDCBufFormat& operator =(const ExynosGDCBufFormat &other) {
            this->width = other.width;
            this->height = other.height;
            this->planeCount = other.planeCount;
            this->format = other.format;
            this->bufCount = other.bufCount;

            return *this;
        }

        bool operator ==(const ExynosGDCBufFormat &other) {
            bool equal = true;

            equal &= (this->width == other.width);
            equal &= (this->height == other.height);
            equal &= (this->planeCount == other.planeCount);
            equal &= (this->format == other.format);
            equal &= (this->bufCount== other.bufCount);

            return equal;
        }

        bool operator !=(const ExynosGDCBufFormat &other) {
            return !(*this == other);
        }
    };

    int m_videoNum;
    int m_videoFd;
    struct ExynosGDCBufFormat m_srcBufFormat;
    struct ExynosGDCBufFormat m_dstBufFormat;
    struct ExynosGDCBuf m_srcBuf;
    struct ExynosGDCBuf m_dstBuf;
    struct ExynosGDCGridTable m_gridTable;
    struct ExynosGDCSizeParam m_inputSizeParam;
    struct ExynosGDCSizeParam m_outputSizeParam;

    std::list<ExynosGDCBuf> m_srcBufList;
    std::list<ExynosGDCBuf> m_dstBufList;
    int m_debugLevel;
};

#endif //EXYNOS_GDC_H
