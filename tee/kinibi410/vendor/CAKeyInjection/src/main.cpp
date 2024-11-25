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

#include "caKeyInjection.h"

#include "buildTag.h"

#undef LOG_TAG
#define LOG_TAG "CAKeyInjectionTool"
#include "log.h"

static void returnExitCode(int exitCode);

int main(int argc, char *args[])
{
    TEEC_Result nResult;
    std::string key_path;
    std::string key_so_path;
    uint32_t    key_id = 0;
    extern char* optarg;
    int opt;

    LOG_I("Copyright (c) Trustonic Limited 2013-2018");
    LOG_I(MOBICORE_COMPONENT_BUILD_TAG);

    while ((opt = getopt(argc, args, "hk:i:o:")) != -1) {
        switch (opt) {
            case 'k':
                key_path = optarg;
                break;
            case 'i':
                key_id = atoi(optarg);
                if (key_id != 0) {
                    printf(" Currently the only supported Key Id is 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'o':
                key_so_path = optarg;
                break;
            case 'h':
                printf("\nUsage: %s [-h] [-ki]\n", args[0]);
                printf("-h : this help\n");
                printf("-i : the Key Id\n");
                printf("-k : the path to the encryption key file\n");
                printf("-o : the path to the output key SO file\n");
                returnExitCode(0);
                break;
            default:
                fprintf(stderr, "\nUsage: %s [-ki]\n", args[0]);
                exit(EXIT_FAILURE);
        }
    }

    nResult = caOpen();
    if (nResult != TEEC_SUCCESS) {
        LOG_E("Could not open session with Trusted Application.");
        fprintf(stderr, "Could not open session with Trusted Application.\n");
        returnExitCode(2);
    }

    nResult = caInjectkey(key_path, key_id, key_so_path);
    if (nResult != TEEC_SUCCESS) {
        LOG_E("Could not send command to Trusted Application.");
        fprintf(stderr, "Could not send command to Trusted Application.\n");
        caClose();
        returnExitCode(2);
    }

    caClose();
    returnExitCode(0);
    return 0;
}

static void returnExitCode(int exitCode)
{
    if (0 != exitCode) {
        LOG_E("Failure");
    } else {
        LOG_I("Success");
    }
    fprintf(stderr, "CA exit code: %08x\n", exitCode);
    exit(exitCode);
}
