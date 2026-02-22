/*
 * Copyright (C) 2019 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    PriorityQueueWithLock.h
 * @brief   This is PriorityQueueWithLock class file.
 * @details This header defines PriorityQueueWithLock class.
 *          This class is implementing priority queue with locking.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#ifndef DRIVER_PRIORITYQUEUEWITHLOCK_H_
#define DRIVER_PRIORITYQUEUEWITHLOCK_H_

#include <queue>    // priority_queue
#include <cstdint>  // int32_t
#include <vector>

namespace android {
namespace nn {
namespace eden_driver {

template <typename T, class cmp>
class PriorityQueueWithLock {
 public:
    /**
     * @brief Return element of reference on top of queue.
     * @details This function returns an element on top of queue.
     * @param void
     * @returns reference of data on top
     */
    const T top(void) {
        std::lock_guard<std::mutex> lock(mutex_pqueue);

        return pqueue_.top();
    }

    /**
     * @brief Empty check
     * @details This function checks whether queue is empty or not.
     * @param void
     * @returns true(empty), false(not empty)
     */
    bool empty(void) {
        std::lock_guard<std::mutex> lock(mutex_pqueue);

        return pqueue_.empty();
    }

    /**
     * @brief Number of element on queue
     * @details This function returns # of element on queue.
     * @param void
     * @returns number of elements on queue
     */
    uint32_t size(void) {
        std::lock_guard<std::mutex> lock(mutex_pqueue);

        return pqueue_.size();
    }

    /**
     * @brief Push an element on queue
     * @details This function pushes an element on queue.
     * @param value to be pushed on queue
     * @returns void
     */
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_pqueue);

        pqueue_.push(value);
        return;
    }

    /**
     * @brief Remove an element on queue
     * @details This function removes an element on queue.
     * @param void
     * @returns void
     */
    void pop(void) {
        std::lock_guard<std::mutex> lock(mutex_pqueue);

        pqueue_.pop();
        return;
    }

 private:
    std::mutex mutex_pqueue;
    std::priority_queue<T, std::vector<T>, cmp> pqueue_;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_PRIORITYQUEUEWITHLOCK_H_

