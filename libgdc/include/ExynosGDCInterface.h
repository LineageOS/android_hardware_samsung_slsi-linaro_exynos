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

#ifndef EXYNOS_GDC_INTERFACE_H
#define EXYNOS_GDC_INTERFACE_H

#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <sys/time.h>

#include "ExynosGDC.h"

using namespace android;

class ExynosGDCInterface
{
    public:
        ExynosGDCInterface();
        virtual ~ExynosGDCInterface();

        virtual status_t create();
#ifndef USE_LEGACY_FUNCTION_ALIGNMENT
        virtual status_t create(int videoNum);
#endif
        virtual status_t init();
        virtual status_t destroy();

        virtual status_t start();
        virtual status_t stop();

    public:
        //Stream Configurations
        virtual status_t setSrcImageSize(uint32_t width, uint32_t height);
        virtual status_t setDstImageSize(uint32_t width, uint32_t height);
        virtual status_t setSrcColorFormat(uint32_t format, uint32_t planeCount);
        virtual status_t setDstColorFormat(uint32_t format, uint32_t planeCount);

        //Frame Configurations
        virtual status_t setGridTable(int32_t *gridX, int32_t *gridY, uint32_t width, uint32_t height);
        virtual status_t setInputBuffer(struct ExynosGDCBuf srcBuf);
        virtual status_t setOutputBuffer(struct ExynosGDCBuf dstBuf);
        virtual status_t setInputSize(struct ExynosGDCSizeParam sizeParam);
        virtual status_t setOutputSize(struct ExynosGDCSizeParam sizeParam);

        //Frame Processing
        virtual status_t runGDC();

        status_t pollFirst(uint32_t & bufIndex);
        status_t pollLast();

        //Debugging
        virtual status_t dump();

    private:
        ExynosGDC m_gdc;

        int       m_state;

        /* Profile Feature */
        int             m_debugProfile;
        unsigned long   m_procFrames;
        double          m_startTime;
        double          m_endTime;
        double          m_totalProcTime;
        double          now_ms(void);
};

#endif //EXYNOS_GDC_INTERAFCE_H
