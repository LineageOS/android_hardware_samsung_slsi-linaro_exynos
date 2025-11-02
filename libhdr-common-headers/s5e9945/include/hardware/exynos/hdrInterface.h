/*
 *  libhdr-common-header/hdrInterface.h
 *
 *   Copyright 2018 Samsung Electronics Co., Ltd.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef __HDR_INTERFACE_H__
#define __HDR_INTERFACE_H__

#include <hardware/exynos/hdr10pMetaInterface.h>
#include <system/graphics.h>

enum RenderSource {
    REND_ORI = 0,
    REND_GPU,
    REND_G2D,
    REND_MAX,
};

enum HdrHwId {
    HDR_HW_DPU = 0,
    HDR_HW_G2D,
    HDR_HW_MAX,
};

enum HdrCapa {
    HDR_CAPA_INNER = 0,
    HDR_CAPA_OUTER,
    HDR_CAPA_MAX,
};

enum HdrBpc {
    HDR_BPC_8 = 0,
    HDR_BPC_10,
    HDR_BPC_MAX,
};
enum HdrErrFlag {
    HDR_ERR_NO = 0,	/* OK - NO ERROR */
    HDR_ERR_INVAL,	/* INVALID VALUE */
    HDR_ERR_PTR,	/* POINTER ERROR */
    HDR_ERR_NOPERM,	/* NOT PERMITTED */
    HDR_ERR_MAX,
};

struct hdrCoefParcel {
    void *hdrCoef;
};

struct HdrTargetInfo {
    int dataspace;
    unsigned int min_luminance;
    unsigned int max_luminance;
    enum HdrBpc bpc;
    enum HdrCapa hdr_capa;
};

struct hdrCoef {
    unsigned int hdr_en;
    unsigned int oetf_en;
    unsigned int oetf_x[33];
    unsigned int oetf_y[33];
    unsigned int eotf_en;
    unsigned int eotf_x[129];
    unsigned int eotf_y[129];
    unsigned int gm_en;
    unsigned int gm_coef[9];
    unsigned int tm_en;
    unsigned int tm_coef[3];
    unsigned int tm_rngx[2];
    unsigned int tm_rngy[2];
    unsigned int tm_x[33];
    unsigned int tm_y[33];
};

class hdrInterface {
public:
    virtual ~hdrInterface() {}
    static hdrInterface *createInstance(const char __attribute__((unused)) *docname);
    virtual int setTargetInfo(struct HdrTargetInfo __attribute__((unused)) *tInfo) { return 0; }
    virtual int sethdr10pMetaInterface(class hdr10pMetaInterface __attribute__((unused)) *hdr10pMetaIf) { return 0; }
    virtual int initHdrCoefBuildup(enum HdrHwId __attribute__((unused)) hw_id) { return 0; }
    virtual int getHdrCoefSize(enum HdrHwId __attribute__((unused)) hw_id) { return 0; }
    virtual void setHDRlayer(bool __attribute__((unused)) hasHdr) {}
    virtual void setRenderIntent(int __attribute__((unused)) rendIntent) {}
    virtual void setSensorInfo(float __attribute__((unused)) max, float __attribute__((unused)) val) {}

    virtual int setLayerInfo( int __attribute__((unused)) layer_index, int __attribute__((unused)) dataspace,
                             void __attribute__((unused)) *static_metadata, int __attribute__((unused)) static_len,
                             void __attribute__((unused)) *dynamic_metadata, int __attribute__((unused)) dynamic_len,
                             bool __attribute__((unused)) premult_alpha, enum HdrBpc __attribute__((unused)) bpc,
                             enum RenderSource __attribute__((unused)) source, float __attribute__((unused)) *tf_matrix,
			     bool __attribute__((unused)) bypass) { return 0; }
    virtual int getHdrCoefData(enum HdrHwId __attribute__((unused)) hw_id, int __attribute__((unused)) layer_index,
                             struct hdrCoefParcel __attribute__((unused)) *parcel) { return 0; }
    virtual int getHdrCoefData(enum HdrHwId __attribute__((unused)) hw_id, struct hdrCoefParcel __attribute__((unused)) *parcel) { return 0; }
    virtual int getHdrCoef(android_dataspace_t ids[], int mastering_luminance[], int n_layer,
            android_dataspace_t ods, int peak_luminance, struct hdrCoef output[4], int res_map[4]);

    virtual void setLogLevel(int __attribute__((unused)) log_level) {}
};

#endif /* __HDR_INTERFACE_H__ */
