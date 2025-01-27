/*
 * Copyright (c) 2013-2020 TRUSTONIC LIMITED
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
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "tlcTeeKeymaster_if.h"
#include "km_util.h"
#include "km_shared_util.h"
#include "cust_tee_keymaster_impl.h"
#include "cust_tee_keymaster_utils.h"

/* ExySp */
#if KEYMASTER_WANTED_VERSION == 4
#define DEFAULT_OBJECT_FILE_OLD "/mnt/vendor/persist/keybox_sample.so"
#else
#define DEFAULT_OBJECT_FILE_OLD "/mnt/vendor/persist/security/attest_keybox.so"
#endif
#define DEFAULT_OBJECT_FILE_NEW "/mnt/vendor/efs/keybox_sample.so"
#define DEFAULT_ATTESTATION_IDS_FILE_OLD "/mnt/vendor/persist/attestation_ids.so"
#define DEFAULT_ATTESTATION_IDS_FILE_NEW "/mnt/vendor/efs/attestation_ids.so"

int HAL_Configure(TEE_SessionHandle sessionHandle)
{
    struct TEE_Session *session = (struct TEE_Session *)sessionHandle;
    keymaster_error_t ret = KM_ERROR_OK;
    mcSessionHandle_t *session_handle = &session->sessionHandle;
    tciMessage_ptr tci = session->pTci;
    mcBulkMap_t srcMapInfo = {0, 0};
    uint8_t *so_p = NULL;
    uint32_t so_len = 0;
    int rc = 0;

    LOG_D("HAL_CONFIGURE");

    /* Get version information from Android system properties */
    bootimage_info_t bootinfo;
    rc = get_version_info(&bootinfo);
    if (rc) {
        LOG_E("Failed to read version info.");
        return rc;
    }
    LOG_I("Read os_version = %u, os_patchlevel = %u, vendor_patchlevel = %u, boot_patchlevel = %u", bootinfo.os_version, bootinfo.os_patchlevel, bootinfo.vendor_patchlevel, bootinfo.boot_patchlevel);

    /* Configure the TA. */
    keymaster_key_param_t param[3] = {
        {KM_TAG_OS_VERSION, {bootinfo.os_version}},
        {KM_TAG_OS_PATCHLEVEL, {bootinfo.os_patchlevel}},
        {KM_TAG_VENDOR_PATCHLEVEL, {bootinfo.vendor_patchlevel}}
    };
    keymaster_key_param_set_t params = {param, 3};

    /* TODO Reference implementation rely on attestation data is stored in
       FileSystem. If the attestation data is stored in RPMB, this part
       should be removed, and add the RPMB read/write interface in
       get_attestation_data() and set_attestation_data() in hal_attestation.c */

    so_len = getFileContent(DEFAULT_OBJECT_FILE_OLD, &so_p);
    if (so_len <= 0) {
	LOG_D("%s(): could not load %s now", __func__, DEFAULT_OBJECT_FILE_OLD);

	so_len = getFileContent(DEFAULT_OBJECT_FILE_NEW, &so_p);
    }

    if (so_len > 0) {
        LOG_D("so_p(%p) size %d", so_p, so_len);
        //return KM_ERROR_INVALID_ARGUMENT;
        CHECK_RESULT_OK(map_buffer(session_handle,
                so_p , so_len, &srcMapInfo));

        tci->command.header.commandId = CMD_ID_TEE_LOAD_ATTESTATION_DATA;
        tci->load_attestation_data.data.data = (uint32_t)srcMapInfo.sVirtualAddr;
        tci->load_attestation_data.data.data_length = (uint32_t)srcMapInfo.sVirtualLen;

        CHECK_RESULT_OK(transact(session_handle, tci));
    }
    else {
        LOG_D("%s(): could not load %s now", __func__, DEFAULT_OBJECT_FILE_NEW);
    }
    unmap_buffer(session_handle, so_p, &srcMapInfo);
    memset(&srcMapInfo, 0, sizeof(srcMapInfo));
    free(so_p);
    so_p = NULL;

#if KEYMASTER_WANTED_VERSION == 4
    /* ExySp */
    /* Load the secure object with the attestation ids if any. */
    so_len = getFileContent(DEFAULT_ATTESTATION_IDS_FILE_OLD, &so_p);
    if (so_len <= 0) {
        LOG_D("%s(): could not load %s now", __func__, DEFAULT_ATTESTATION_IDS_FILE_OLD);

        so_len = getFileContent(DEFAULT_ATTESTATION_IDS_FILE_NEW, &so_p);
    }
    if (so_len > 0) {
        LOG_D("so_p(%p) size %d", so_p, so_len);
        CHECK_RESULT_OK(map_buffer(session_handle,
                so_p , so_len, &srcMapInfo));

        tci->command.header.commandId = CMD_ID_TEE_LOAD_ATTESTATION_IDS;
        tci->load_attestation_ids.data.data = (uint32_t)srcMapInfo.sVirtualAddr;
        tci->load_attestation_ids.data.data_length = (uint32_t)srcMapInfo.sVirtualLen;

        CHECK_RESULT_OK(transact(session_handle, tci));
    }
    else {
        LOG_D("%s(): could not load %s now", __func__, DEFAULT_ATTESTATION_IDS_FILE_NEW);
    }
#endif

    ret = TEE_Configure(sessionHandle, &params);
    if (ret) {
        LOG_E("TEE_Configure() failed.");
    }

end:
    unmap_buffer(session_handle, so_p, &srcMapInfo);
    free(so_p);
    if (ret != KM_ERROR_OK)
        rc = -EBUSY;
    return rc;
}

keymaster_error_t HAL_DestroyAttestationIds(TEE_SessionHandle sessionHandle)
{
    /* Try to remove the file if any */
    int ret = remove(DEFAULT_ATTESTATION_IDS_FILE_OLD);
    if (ret != 0) {
	ret = remove(DEFAULT_ATTESTATION_IDS_FILE_NEW);

	if (ret != 0) {
            LOG_E("Error: Cannot remove attestation ids");
	}
        /* Just discard any error and continue */
    }
    return TEE_DestroyAttestationIds(sessionHandle);
}
