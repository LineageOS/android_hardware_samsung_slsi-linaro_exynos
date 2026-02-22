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

#ifndef ENF_PARAM_HPP
#define ENF_PARAM_HPP

#include <stdint.h>

namespace seva {
namespace sol {

#define ENF_NUM_SUB_BAND 7
#define ENF_NUM_TOTAL_GAIN 2
#define ENF_NUM_DITHER_GAIN 2
#define ENF_NUM_THRESHOLD 2
#define ENF_NUM_SLOPE 2
#define ENF_NUM_GAIN_LIMIT 2

typedef struct {
    uint16_t threshold[ENF_NUM_THRESHOLD];
    uint16_t slope_p[ENF_NUM_SLOPE];
    uint16_t slope_n[ENF_NUM_SLOPE];
} enf_sub_band_t;

typedef struct {
    uint16_t threshold[ENF_NUM_THRESHOLD];
    uint16_t slope[ENF_NUM_SLOPE];
} enf_coring_t;

typedef struct {
    uint16_t threshold[ENF_NUM_THRESHOLD];
    uint16_t slope[ENF_NUM_SLOPE];
    uint16_t gain_n[ENF_NUM_GAIN_LIMIT];
    uint16_t gain_p[ENF_NUM_GAIN_LIMIT];
} enf_halo_t;

typedef struct {
    uint16_t threshold;
    uint16_t slope;
    uint16_t gain;
} enf_bright_t;

typedef struct {
    uint16_t m[ENF_NUM_TOTAL_GAIN];
    uint16_t s[ENF_NUM_TOTAL_GAIN];
} enf_total_gain_t;

typedef struct {
    int16_t gain[ENF_NUM_DITHER_GAIN];
} enf_dither_t;

typedef struct __attribute__((packed)) {
    enf_sub_band_t sub_band[ENF_NUM_SUB_BAND];
    enf_coring_t coring;
    enf_halo_t halo;
    enf_bright_t bright;
    enf_total_gain_t total_gain;
    enf_dither_t dither;

    uint16_t reserved0;  // me
    uint16_t reserved1;  // de
    uint16_t reserved2;  // hh
    uint16_t reserved3;  // em
    uint16_t reserved4;  // ee
} enf_para_t;

}  // namespace sol
}  // namespace seva

#endif  // ENF_PARAM_HPP
