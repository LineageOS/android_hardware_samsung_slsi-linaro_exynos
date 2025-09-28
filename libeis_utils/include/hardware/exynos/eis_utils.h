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
 * \file      eis_utils.h
 * \brief     header file for eis_utils
 * \author    Sungju Huh (sungju.huh@samsung.com)
 * \date      2021/02/08
 *
 * <b>Revision History: </b>
 * - 2021/02/08 : Sungju Huh (sungju.huh@samsung.com) \n
 *   Initial version
 *
 */

/*!
 * \defgroup eis_utils
 * \brief API for eis_utils
 * \addtogroup Exynos
 */

#ifndef __EIS_UTILS_H__
#define __EIS_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* EIS_UTILS */

int get_margin_size(int width, int height, int* eis_width, int* eis_height);

#ifdef __cplusplus
}
#endif

#endif /* __EIS_UTILS_H__ */
