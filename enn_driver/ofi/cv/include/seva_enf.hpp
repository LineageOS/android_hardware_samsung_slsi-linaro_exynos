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

#ifndef SEVA_ENF_HPP
#define SEVA_ENF_HPP

#include <memory>
#include <seva_solution.hpp>

#include "enf_param.hpp"

/**
 * @file seva_enf.hpp
 * @brief Seva Solution ENFilter class definition.
 */

namespace seva {
namespace sol {

/**
 * @brief represents the processing mode of ENF.
 */
enum ENFMODE {
    /*! \brief Process PRE-ENF. YUV.*/
    PRE_ENF = 0,
    /*! \brief Process PRE-ENF only for Y channel.*/
    PRE_ENF_Y_ONLY = 10,  // not a mistake. it is really ten.
    /*! \brief Process POST-ENF. YUV. Need motion map.*/
    POST_ENF = 2,
    /*! \brief Process POST-ENF only for Y channel.*/
    POST_ENF_Y_ONLY = 12,  // not a mistake. it is really twelve.
};

/**
 * @brief represents an ENFFilter object.
 *
 */

class ENFilter {
  public:
    /**
     * @brief Constructor.
     * @param [in] width Width of the image to process.
     * @param [in] height Height of the image to process.
     * @param [in] mode ENF processing mode.
     *
     * Initialize ENFilter class.
     * Width and height of in/out frame need to be passed through constructor.
     * Also need processing mode.
     */
    ENFilter(size_t width, size_t height, ENFMODE mode);

    /**
     * @brief Destructor.
     */
    ~ENFilter();

    /**
     * @brief Set ENF parameter for Y(luma) channel.
     * @param [in] param ENF parameter for Y channel.
     *
     * Set ENF parameter for Y(luma) channel.
     * Needs enf_para_t structure. It will duplicate passed parameter to keep as it is.
     */
    RESULT SetParameterY(enf_para_t &param);

    /**
     * @brief Set ENF parameter for UV(chroma) channel.
     * @param [in] param ENF parameter for UV channel.
     *
     * Set ENF parameter for UV(chroma) channel.
     * Needs enf_para_t structure. It will duplicate passed parameter to keep as it is.
     */
    RESULT SetParameterUV(enf_para_t &param);

    /**
     * @brief Create and set Buffer for input frame Y channel.
     *
     * Create a Buffer for input frame Y channel.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as input frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     * This calls SetInputFrameBufferY() function internally.
     */

    Buffer CreateInputFrameBufferY();

    /**
     * @brief Create and set Buffer for input frame UV channel.
     *
     * Create a Buffer for input frame UV channel.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as input frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     * This calls SetInputFrameBufferUV() function internally.
     */
    Buffer CreateInputFrameBufferUV();

    /**
     * @brief Create and set Buffer for input motion map.
     *
     * Create a Buffer for motion map.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as motion map on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     * This calls SetInputMotionMapBuffer() function internally.
     */
    Buffer CreateInputMotionMapBuffer();

    /**
     * @brief Create and set Buffer for output frame Y channel.
     *
     * Create a Buffer for output frame Y channel.
     * Proper size of memory will be allocated for this buffer.
     * Memory size will be decided according to width, height and mode which was provided on constructor.
     * It will create a Buffer and set it as output frame on this instance.
     * Buffer instance and allocated memory will be preserved while returned instance preserved.
     * This calls SetOutputFrameBufferY() function internally.
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
     * This calls SetOutputFrameBufferUV() function internally.
     */
    Buffer CreateOutputFrameBufferUV();

    /**
     * @brief Set Buffer for input frame Y channel.
     * @param [in] input Buffer for input frame Y channel.
     *
     * Set Buffer for input frame.
     * This function can be use to set input frame Y buffer with Buffer created from other class or instance.
     */
    RESULT SetInputFrameBufferY(Buffer &input);

    /**
     * @brief Set Buffer for input frame UV channel.
     * @param [in] input Buffer for input frame UV channel.
     *
     * Set Buffer for input frame.
     * This function can be use to set input frame UV buffer with Buffer created from other class or instance.
     */
    RESULT SetInputFrameBufferUV(Buffer &input);

    /**
     * @brief Set Buffer for motion map.
     * @param [in] motmap Buffer for motion map.
     *
     * Set Buffer for motion map.
     * This function can be use to set input motion map buffer with Buffer created from other class or instance.
     */
    RESULT SetInputMotionMapBuffer(Buffer &motmap);

    /**
     * @brief Set Buffer for output frame Y channel.
     * @param [in] output Buffer for output frame Y channel.
     *
     * Set Buffer for output frame.
     * This function can be use to set output frame Y buffer with Buffer created from other class or instance.
     */
    RESULT SetOutputFrameBufferY(Buffer &output);

    /**
     * @brief Set Buffer for output frame UV channel.
     * @param [in] output Buffer for output frame UV channel.
     *
     * Set Buffer for output frame.
     * This function can be use to set output frame UV buffer with Buffer created from other class or instance.
     */
    RESULT SetOutputFrameBufferUV(Buffer &output);

    /**
     * @brief Check buffer and parameters are ready to execute ENFilter.
     *
     * Check buffer and parameters are ready to execute ENFilter.
     * When all the input/output buffers and paramers are ready, it returns true.
     */
    bool IsReadyToExecute();

    /**
     * @brief Execute ENF process.
     *
     * Execute ENF process.
     */
    RESULT Execute();

  private:
    struct _seva_v;
    std::unique_ptr<_seva_v> pv;

    ENFilter(ENFilter &&);
    ENFilter &operator=(ENFilter &&);
    ENFilter(ENFilter &) = delete;  // no copy allowed.
    ENFilter operator=(ENFilter &) = delete;
};

}  // namespace sol
}  // namespace seva

#endif  // SEVA_ENF_HPP
