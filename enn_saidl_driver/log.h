/*
 * Copyright (C) 2017 The Android Open Source Project
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

/** @file log.h
    @brief NPU log header
*/
#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ENN"
#define ENN_VERSION "v1.7.1-2"
#define VERSION_MAJOR 1
#define VERSION_MINOR 7
#define VERSION_BUILD 1
#define ONDEVICE_GG "v09.26.2022"

enum _eden_log_flags {
    ENN_LOG_FORCE = 0,
    ENN_LOG_NN = 1,
    ENN_LOG_RT = 2,
    ENN_LOG_UD = 3,
    ENN_LOG_LINK = 4,
    ENN_LOG_EMA = 5,
    ENN_LOG_GTEST = 6,
    ENN_LOG_HIDL = 7,
    ENN_LOG_RT_STUB = 8,
    ENN_LOG_DRIVER = 9,
    ENN_LOG_CL = 10,
    ENN_LOG_TOOL = 11,
    ENN_LOG_MAX_FLAG
};

#define ENN_FORCE       (1 << ENN_LOG_FORCE)
#define ENN_NN          (1 << ENN_LOG_NN)
#define ENN_RT          (1 << ENN_LOG_RT)
#define ENN_UD          (1 << ENN_LOG_UD)
#define ENN_LINK        (1 << ENN_LOG_LINK)
#define ENN_EMA         (1 << ENN_LOG_EMA)
#define ENN_GTEST       (1 << ENN_LOG_GTEST)
#define ENN_HIDL        (1 << ENN_LOG_HIDL)
#define ENN_RT_STUB     (1 << ENN_LOG_RT_STUB)
#define ENN_DRIVER       (1 << ENN_LOG_DRIVER)
#define ENN_CL          (1 << ENN_LOG_CL)
#define ENN_TOOL        (1 << ENN_LOG_TOOL)
#define ENN_MAX_FLAG    (1 << ENN_LOG_MAX_FLAG)

/*
 * comment out to mask
 */
static int glive_log = ENN_LOG_FORCE \
    | ENN_NN \
    | ENN_RT \
    | ENN_UD\
    | ENN_LINK \
    | ENN_EMA \
    | ENN_GTEST \
    | ENN_HIDL \
    | ENN_RT_STUB \
    | ENN_DRIVER \
    | ENN_CL \
    | ENN_TOOL;

#define IS_LOG_ON(FLAG)  ((glive_log & (FLAG)) != 0)

#define __SHARP_X(x) #x
#define __STR(x) __SHARP_X(x)

#if   defined(LINUX_LOG)
#include <stdio.h>
#define LL_DBG 'D'
#define LL_INF 'I'
#define LL_WRN 'W'
#define LL_ERR 'E'
#ifdef ENN_DEBUG
#define __LOGX(flag, log_level, fmt, ...)                                                                      \
    if (IS_LOG_ON(flag)) {                                                                                     \
        fprintf(stdout, "[Exynos][ENN][" ENN_VERSION "][" LOG_TAG "][%c] %s:" __STR(__LINE__) ": " fmt "\n", \
                log_level, __FUNCTION__, ##__VA_ARGS__);                                                       \
        fflush(stdout);                                                                                        \
    }
#else
#define __LOGX(flag, log_level, fmt, ...)                                                                      \
    if (IS_LOG_ON(flag)) {                                                                                     \
        fprintf(stdout, "[Exynos][ENN][" ENN_VERSION "][" LOG_TAG "][%c] %s:" __STR(__LINE__) ": " fmt "\n", \
                log_level, __FUNCTION__, ##__VA_ARGS__);                                                       \
    }
#endif
#elif defined(ANDROID_LOG)
#include <android/log.h>
#define LL_DBG ANDROID_LOG_DEBUG
#define LL_INF ANDROID_LOG_INFO
#define LL_WRN ANDROID_LOG_WARN
#define LL_ERR ANDROID_LOG_ERROR
#define __LOGX(flag, log_level, fmt, ...)                                                               \
    if (IS_LOG_ON(flag)) {                                                                              \
        __android_log_print(log_level, "ENN", "[Exynos][ENN][" ENN_VERSION "][" LOG_TAG              \
                                        "] %s:" __STR(__LINE__) ": " fmt, __FUNCTION__, ##__VA_ARGS__); \
    }
#else
#error "Either LINUX_LOG or ANDROID_LOG should be applied."
#endif

#define LOGD(flag, fmt, ...) __LOGX(flag, LL_DBG, fmt, ## __VA_ARGS__)
#define LOGI(flag, fmt, ...) __LOGX(flag, LL_INF, fmt, ## __VA_ARGS__)
#define LOGW(flag, fmt, ...) __LOGX(flag, LL_WRN, fmt, ## __VA_ARGS__)
#define LOGE(flag, fmt, ...) __LOGX(flag, LL_ERR, fmt, ## __VA_ARGS__)

/*
 * override LOGD only if ENN_DEBUG is not applied
 */
#ifndef ENN_DEBUG
#undef LOGD
#define LOGD(FLAG, ...)  { }
#endif  // !ENN_DEBUG

/*
 * override all if REMOVE_LOG is applied
 */
#ifdef REMOVE_LOG
#undef __LOGX
#define __LOGX(flag, ...)  { }
#endif  // REMOVE_LOG

#endif  // COMMON_LOG_H_
