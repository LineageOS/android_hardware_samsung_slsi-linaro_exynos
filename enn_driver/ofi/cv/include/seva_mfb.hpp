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

#ifndef SEVA_MFB_HPP
#define SEVA_MFB_HPP

#include <memory>
#include <seva_solution.hpp>

#include "mfb_param.hpp"

/**
 * @file seva_mfb.hpp
 * @brief Seva Solution MFBlender class definition.
 */

namespace seva {
namespace sol {

class MFBlender {
  public:
    /**
     * @brief Constructor.
     * @param [in] width Width of the image to process.
     * @param [in] height Height of the image to process.
     * @param [in] framecnt Number of frames to blend.
     *
     * Initialize MFBlender class.
     * Width and height of in/out frame need to be passed through constructor.
     * Also need number of frames to blend.
     */
    MFBlender(size_t width, size_t height, size_t framecnt);

    /**
     * @brief Destructor.
     */
    ~MFBlender();

    /**
     * @brief Set MFB parameter.
     * @param [in] param MFB parameter.
     *
     * Set MFB parameter.
     * Needs mfb_para_t structure. It will duplicate parameter to keep as it is.
     */
    RESULT SetParameter(mfb_para_t &param);

    /**
     * @brief Create and set Buffer for input frame.
     * @param [in] index Index of input frames want to create.
     *
     * Create a Buffer for input frame on index.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as input frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     */
    Buffer CreateInputFrameBuffer(size_t index);

    /**
     * @brief Create and set Buffer for input frame Y channel.
     * @param [in] index Index of input frames want to create.
     *
     * Create a Buffer for input frame Y channel on index.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as input frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     */
    Buffer CreateInputFrameBufferY(size_t index);

    /**
     * @brief Create and set Buffer for input frame UV channel.
     * @param [in] index Index of input frames want to create.
     *
     * Create a Buffer for input frame UV channel on index.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as input frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     */
    Buffer CreateInputFrameBufferUV(size_t index);

    /**
     * @brief Create and set Buffer for output frame.
     *
     * Create a Buffer for output frame.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as output frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     */
    Buffer CreateOutputFrameBuffer();

    /**
     * @brief Create and set Buffer for output frame Y channel.
     *
     * Create a Buffer for output frame Y channel.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as output frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     */
    Buffer CreateOutputFrameBufferY();

    /**
     * @brief Create and set Buffer for output frame UV channel.
     *
     * Create a Buffer for output frame UV channel.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as output frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     */
    Buffer CreateOutputFrameBufferUV();

    /**
     * @brief Create and set Buffer for output motion map.
     *
     * Create a Buffer for motion map.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as motion map on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     */
    Buffer CreateOutputMotionMapBuffer();

    /**
     * @brief Set index of reference frame.
     * @param [in] index index of reference frame.
     *
     * Set index of reference frame
     * Rest of frames are merged to the reference frame.
     */
    RESULT SetReferenceFrameIndex(size_t index);

    /**
     * @brief Set index of non-reference frame.
     * @param [in] index index of non-reference frame.
     *
     * Set index of non-reference frame
     * Non reference frame index to calculate motion percentage.
     */
    RESULT SetNonReferenceFrameIndex(size_t index);

    /**
     * @brief Set Buffer for input frame.
     * @param [in] index Index of input frames want to set.
     * @param [in] input Buffer for input frame.
     *
     * Set Buffer for input frame on index.
     * This function can be use to set input buffer with Buffer created from other class or instance.
     */
    RESULT SetInputFrameBuffer(size_t index, Buffer &input);

    /**
     * @brief Set Buffer for input frame Y channel.
     * @param [in] index Index of input frames want to set.
     * @param [in] input Buffer for input frame.
     *
     * Set Buffer for input frame on index.
     * This function can be use to set input buffer with Buffer created from other class or instance.
     */
    RESULT SetInputFrameBufferY(size_t index, Buffer &input);

    /**
     * @brief Set Buffer for input frame UV channel.
     * @param [in] index Index of input frames want to set.
     * @param [in] input Buffer for input frame.
     *
     * Set Buffer for input frame on index.
     * This function can be use to set input buffer with Buffer created from other class or instance.
     */
    RESULT SetInputFrameBufferUV(size_t index, Buffer &input);

    /**
     * @brief Set Buffer for output frame.
     * @param [in] output Buffer for output frame.
     *
     * Set Buffer for output frame.
     * This function can be use to set output buffer with Buffer created from other class or instance.
     */
    RESULT SetOutputFrameBuffer(Buffer &output);

    /**
     * @brief Set Buffer for output frame Y channel.
     * @param [in] output Buffer for output frame Y channel.
     *
     * Set Buffer for output frame Y channel.
     * This function can be use to set output buffer with Buffer created from other class or instance.
     */
    RESULT SetOutputFrameBufferY(Buffer &output);

    /**
     * @brief Set Buffer for output frame UV channel.
     * @param [in] output Buffer for output frame UV channel.
     *
     * Set Buffer for output frame UV channel.
     * This function can be use to set output buffer with Buffer created from other class or instance.
     */
    RESULT SetOutputFrameBufferUV(Buffer &output);

    /**
     * @brief Set Buffer for output motion map.
     * @param [in] motmap Buffer for output motion map.
     *
     * Set Buffer for output motion map.
     * This function can be use to set output motion map with Buffer created from other class or instance.
     */
    RESULT SetOutputMotionMapBuffer(Buffer &motmap);

    /**
     * @brief Check buffer and parameters are ready to execute MFBlender.
     *
     * Check buffer and parameters are ready to execute MFBlender.
     * When all the input/output buffers and paramers are ready, it returns true.
     */
    bool IsReadyToExecute();

    /**
     * @brief Execute MFB process.
     *
     * Execute MFB process.
     */
    RESULT Execute();

  private:
    struct _seva_v;
    std::unique_ptr<_seva_v> pv;

    MFBlender(MFBlender &&);
    MFBlender &operator=(MFBlender &&);
    MFBlender(MFBlender &) = delete;  // no copy allowed.
    MFBlender operator=(MFBlender &) = delete;
};

}  // namespace sol
}  // namespace seva

#endif  // SEVA_MFB_HPP
