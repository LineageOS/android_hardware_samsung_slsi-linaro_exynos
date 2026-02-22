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

#ifndef MFB_PARAM_HPP
#define MFB_PARAM_HPP

#include <stdint.h>

namespace seva {
namespace sol {

#define MAX_MFB_FRAME_CNT (10)
#define MIN_MFB_FRAME_CNT (3)

typedef struct __attribute__((packed)) {
    /**
     * @brief frame priority index
     *
     *  frame priority index. The longer frame difference has the the smaller
     *  weight, because it gets more likely motion error as time from the
     *  reference frame gets longer.
     */
    int32_t w_mult[MAX_MFB_FRAME_CNT];

    /**
     * @brief uv shift parameter to be used in weight map.
     *
     *  uv shift parameter to be used in weight map.
     */
    int32_t uv_mult;

    /**
     * @brief reference frame weighting
     *
     *  reference frame weighting. All weight values of non-reference frame are
     *  smaller than mRefWeight, so as to assign the largest merging at the
     *  reference frame.
     */
    int32_t ref_weight;

    /**
     * @brief peak threshold.
     *
     *  peak threshold to be used for calculation motion percentage.
     */
    int32_t peak_threshold;

    /**
     * @brief luma gain to adjust weight map.
     *
     *  luma gain to adjust weight map. Depending on brightness level, it
     *  controls more or less merging weight.
     */
    int32_t luma_gain;

    /**
     * @brief chroma gain to adjust weight map.
     *
     *  chroma gain to adjust weight map. Depending on brightness level,
     *  it controls more or less merging weight.
     */
    int32_t chroma_gain;

    /**
     * @brief luma offset to adjust weight map.
     *
     *  luma offset to adjust weight map. Depending on brightness level,
     *  it controls more or less merging weight.
     */
    int32_t luma_offset;

    /**
     * @brief green color adjust.
     *
     *  parameter adjusting the weight map to emphasize green contents
     *  such as grasses or trees.
     */
    int32_t green;

    /**
     * @brief skin color adjust.
     *
     *  parameter adjusting the weight map to emphasize human skin contents.
     */
    int32_t skin;

    /**
     * @brief weight map brightness adjust
     *
     *  parameter adjusting the weight map brightness.
     */
    int lshift;

    /**
     * @brief weight map brightness adjust
     *
     *  parameter adjusting the weight map brightness.
     */
    int rshift;

} mfb_para_t;

}  // namespace sol
}  // namespace seva

#endif  // MFB_PARAM_HPP
