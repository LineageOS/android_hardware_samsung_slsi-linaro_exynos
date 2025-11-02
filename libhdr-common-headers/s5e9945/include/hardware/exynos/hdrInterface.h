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

#define HDR_IF_VER      (1.1)

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
    HDR_BPC_FP16,
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

struct HdrLayerInfo {
    int dataspace;
    void *static_metadata;
    int static_len;
    void *dynamic_metadata;
    int dynamic_len;
    bool premult_alpha;
    enum HdrBpc bpc;
    enum RenderSource source;
    float *tf_matrix;
    bool bypass;
};

enum DebugMode {
    OFF = 0,
    BYPASS_OFF,
    BYPASS_ON_ALL,
    BYPASS_ON_COND,
};

class hdrInterface {
public:
    virtual ~hdrInterface() {}
    // init phase
    static hdrInterface *createInstance(void);

    virtual int sethdr10pMetaInterface(class hdr10pMetaInterface __attribute__((unused)) *hdr10pMetaIf) { return 0; }
    virtual int getHdrCoefSize(enum HdrHwId __attribute__((unused)) hw_id) { return 0; }

    // per frame setting phase (set when there is any change)
    virtual int setTargetInfo(struct HdrTargetInfo __attribute__((unused)) *tInfo) { return 0; }
    virtual void setHDRlayer(bool __attribute__((unused)) hasHdr) {}
    virtual void setRenderIntent(int __attribute__((unused)) rendIntent) {}
    virtual void setSensorInfo(float __attribute__((unused)) max, float __attribute__((unused)) val) {}

    // per frame & per layer in between phase
    virtual int initHdrCoefBuildup(enum HdrHwId __attribute__((unused)) hw_id) { return 0; }

    // per layer setting phase
    virtual bool needHdrProcessing(struct HdrLayerInfo __attribute__((unused)) *lInfo) { return false; }
    virtual int setLayerInfo(int __attribute__((unused)) layer_index, struct HdrLayerInfo __attribute__((unused)) *lInfo) { return 0; }

    // get coef phase
    virtual int getHdrCoefData(enum HdrHwId __attribute__((unused)) hw_id, int __attribute__((unused)) layer_index,
                             struct hdrCoefParcel __attribute__((unused)) *parcel) { return 0; }
    virtual int getHdrCoefData(enum HdrHwId __attribute__((unused)) hw_id, struct hdrCoefParcel __attribute__((unused)) *parcel) { return 0; }

    // debug phase
    virtual void setLogLevel(int __attribute__((unused)) log_level) {}
    virtual void setDebugMode(enum DebugMode __attribute__((unused)) debug_mode) {}
};

#endif /* __HDR_INTERFACE_H__ */
