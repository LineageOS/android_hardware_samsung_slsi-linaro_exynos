/*
 * Copyright (C) 2019 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#ifndef DEBUG_UTIL_HPP
#define DEBUG_UTIL_HPP

#include <string>
#include <IRDocument.h>

void _saveString(std::string path, std::string content);
void _saveBuffer(std::string path, const char *buffer, unsigned int size);

void _saveCvtOut(std::string xml, BlobPair bins);
void _saveGcOut(const char *buffer, unsigned int size);

bool _isDbgFileOut();

#endif  // DEBUG_UTIL_HPP