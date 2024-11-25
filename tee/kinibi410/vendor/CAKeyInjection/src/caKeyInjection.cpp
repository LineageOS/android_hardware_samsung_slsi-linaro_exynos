/*
 * Copyright (c) 2013-2018 TRUSTONIC LIMITED
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the TRUSTONIC LIMITED nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "Key_injection_protocol.h"
#include "taKeyInjectionTool_uuid.h"

#undef LOG_TAG
#define LOG_TAG "CAKeyInjectionTool:lib"
#include "log.h"

#include "tee_client_api.h"
#include "mcSo.h"

TEEC_UUID uuid = taKeyInjectionTool_UUID;
static TEEC_Context  context;
static TEEC_Session  session;

// -------------------------------------------------------------
TEEC_Result caOpen()
{
    TEEC_Result     nError;

    LOG_D("[CAKeyInjectionTool] %s", __func__);

    /* Create Device context  */
    nError = TEEC_InitializeContext(NULL, &context);
    if (nError != TEEC_SUCCESS) {
        LOG_E("[CAKeyInjectionTool] %s: TEEC_InitializeContext failed (%08x), %d", __func__, nError, __LINE__);
        if (nError == TEEC_ERROR_COMMUNICATION) {
            LOG_E("[CAKeyInjectionTool] %s: The client could not communicate with the service, %d", __func__, __LINE__);
        }
        return nError;
    }

    LOG_D("[CAKeyInjectionTool] %s", __func__);

    memset(&session, 0, sizeof(TEEC_Session));

    nError = TEEC_OpenSession(&context,
                              &session,            /* OUT session */
                              &uuid,               /* destination UUID */
                              TEEC_LOGIN_PUBLIC,   /* connectionMethod */
                              NULL,                /* connectionData */
                              NULL,                /* IN OUT operation */
                              NULL                 /* OUT returnOrigin, optional */
                             );
    if (nError != TEEC_SUCCESS) {
        LOG_E("[CAKeyInjectionTool] %s: TEEC_OpenSession failed (%08x), %d", __func__, nError, __LINE__);
        return nError;
    }

    return TEEC_SUCCESS;
}

// -------------------------------------------------------------

#define MY_LOG_ERRNO(fmt, ...) \
    LOG_E("%s: %s " fmt, __func__, strerror(errno), ##__VA_ARGS__)

static bool read_key(const std::string& path, std::string* buffer)
{

    int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOG_W("Cannot open key file %s (%s)", path.c_str(), strerror(errno));
        return false;
    }

    struct stat stat;
    if (::fstat(fd, &stat) < 0) {
        MY_LOG_ERRNO("getting size for Key file %s", path.c_str());
        ::close(fd);
        return false;
    }

    void* addr = ::mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        MY_LOG_ERRNO("mapping file to memory");
        ::close(fd);
        return false;
    }

    /* Give service type to driver so it knows how to allocate and copy */
    buffer->resize(stat.st_size);
    LOG_I("buffer size: %zu", buffer->length());
    ::memcpy(&(*buffer)[0], addr, buffer->length());
    ::munmap(addr, stat.st_size);
    ::close(fd);
    return true;
}
// ------------------------------------------------------------------
static bool caWriteSoToFile(const char* filename, const uint8_t *data, size_t datalen) {
    std::ofstream fs(filename, std::ios::binary);

    if (!fs.is_open()) {
        LOG_E("Could not open key file.");
        return false;
    }
    fs.write((const char*)data, datalen);
    fs.flush();
    if (fs.fail()) {
        LOG_E("Could not write to key file.");
        return false;
    }
    fs.close();
    return true;
}

/**
 * Securely overrides sensitive data with zeros.
 * Functions is guaranteed not to be optimized away.
 *
 * @param   s   address to memory that should be cleaned
 * @param   n   amount of bytes to override
 */
static void secure_memclean(void *s, size_t n) {
    volatile uint8_t *p = (uint8_t*)s;
    while (n--) {
        *p++ = 0;
    }
}

TEEC_Result caInjectkey(std::string key_path, uint32_t keyId, std::string key_so_path)
{
    TEEC_Result    nError;
    TEEC_Operation sOperation;

    LOG_D("[CAKeyInjectionTool] %s", __func__);

    std::string key;
    if (!read_key(key_path, &key)) {
        LOG_E("Could not read key file.");
        fprintf(stderr, "Could not read key file.\n");
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    if ((uint32_t)key.length() > 32 * 1024) {
        fprintf(stderr, "Key too big\n");
        return TEEC_ERROR_BAD_PARAMETERS;
    }

    uint32_t decryption_key_size = (uint32_t)(sizeof(decryption_key_t) + key.length());
    decryption_key_t* decryption_key = (decryption_key_t*)::malloc(decryption_key_size);
    ::memset(decryption_key, 0, decryption_key_size);

    decryption_key->keyId = keyId;
    decryption_key->keySize = (uint32_t)key.length();
    ::memcpy(decryption_key->key, &key[0], key.length());
    secure_memclean(&key[0], key.length());

    uint32_t so_blob_size = (uint32_t)MC_SO_SIZE_F22(0, decryption_key_size);
    uint8_t* so_blob = (uint8_t*)::malloc(so_blob_size);
    ::memset(so_blob, 0, so_blob_size);

    memset(&sOperation, 0, sizeof(TEEC_Operation));

    sOperation.params[0].tmpref.buffer = static_cast<void*>(decryption_key);
    sOperation.params[0].tmpref.size = decryption_key_size;
    sOperation.params[1].tmpref.buffer = so_blob;
    sOperation.params[1].tmpref.size = so_blob_size;

    sOperation.paramTypes = TEEC_PARAM_TYPES(
                                TEEC_MEMREF_TEMP_INPUT,
                                TEEC_MEMREF_TEMP_OUTPUT,
                                TEEC_NONE,
                                TEEC_NONE);

    nError = TEEC_InvokeCommand(&session,
                                CMD_KIT_WRAP_KEY,
                                &sOperation,       /* IN OUT operation */
                                NULL               /* OUT returnOrigin, optional */
                               );

    if (nError != TEEC_SUCCESS) {
        LOG_E("[CAKeyInjectionTool] %s: TEEC_InvokeCommand failed (%08x), %d", __func__, nError, __LINE__);
    } else {
        LOG_D_BUF("key blob", so_blob, sOperation.params[1].tmpref.size);
        // write file into file system
        std::stringstream stream;
        stream << std::setfill ('0') << std::setw((int)sizeof(keyId) * 2) << std::hex << keyId;
        std::string result( stream.str() );

        std::string so_file_name = key_so_path + "SYSENC_" + result + ".so";
        if (!caWriteSoToFile(so_file_name.c_str(), so_blob, sOperation.params[1].tmpref.size)) {
            LOG_E("%s: write to file failed, %d", __func__, __LINE__);
            nError = TEEC_ERROR_GENERIC;
        }
    }

    free(decryption_key);
    free(so_blob);
    return nError;
}

// -------------------------------------------------------------
void caClose()
{
    LOG_D("[CAKeyInjectionTool] %s", __func__);
    /* Close the session */
    TEEC_CloseSession(&session);

    /* Finalize */
    TEEC_FinalizeContext(&context);
}

