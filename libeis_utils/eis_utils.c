/*
 * Copyright (C) 2011 The Android Open Source Project
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

/*!
 * \file      eis_utils.c
 * \brief     source file for libeis_utils
 * \author    Sungju Huh (sungju.huh@samsung.com)
 * \date      2021/02/08
 *
 * <b>Revision History: </b>
 * - 2021/02/08: Sungju Huh (sungju.huh@samsung.com) \n
 *   Initial version
 *
 */

#include <stdio.h>

#include "hardware/exynos/eis_utils.h"

/* vendor static info : width, height, eis width, eis height */
static int EIS_SIZE_LIST[][4] = {
    { 7680, 4320, 7680, 4320},    /* 8K 16:9 NO MARGIN */
    { 7680, 3296, 7680, 3296},    /* 8K 21:9 NO MARGIN */
#ifdef S5E9935
    { 3840, 2160, 4032, 2268},    /* UHD 16:9 SENSOR SIZE */
    { 3840, 1644, 4032, 1728},    /* UHD 21:9 SENSOR SIZE */
#else
    { 3840, 2160, 4608, 2592},    /* UHD 16:9 20% */
    { 3840, 1644, 4608, 1976},    /* UHD 21:9 20% */
#endif
};

int get_margin_size(int width, int height, int* eis_width, int* eis_height) {
    int retval = -1;
    int availableEISSizeListMax = sizeof(EIS_SIZE_LIST) / (sizeof(int) * 4);

    for (int i = 0; i < availableEISSizeListMax; i++) {
        if (width == EIS_SIZE_LIST[i][0] && height == EIS_SIZE_LIST[i][1]) {
            *eis_width  = EIS_SIZE_LIST[i][2];
            *eis_height = EIS_SIZE_LIST[i][3];
            retval = 0;
            break;
        }
    }
    return retval;
}

