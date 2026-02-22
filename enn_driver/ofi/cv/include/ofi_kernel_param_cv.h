#ifndef __OFI_KERNEL_PARAM_CV_H
#define __OFI_KERNEL_PARAM_CV_H

typedef struct __attribute__((packed)) {
    uint8_t coef[12];
    uint8_t sh;
}
gaussian3x3_para_t;

#define ENF_NUM_SUB_BAND        7
#define ENF_NUM_TOTAL_GAIN      2
#define ENF_NUM_DITHER_GAIN     2
#define ENF_NUM_THRESHOLD       2
#define ENF_NUM_SLOPE           2
#define ENF_NUM_GAIN_LIMIT      2

typedef struct {
    uint16_t threshold[ENF_NUM_THRESHOLD];
    uint16_t slope_p[ENF_NUM_SLOPE];
    uint16_t slope_n[ENF_NUM_SLOPE];
} enf_sub_band_t;

typedef struct {
    uint16_t threshold[ENF_NUM_THRESHOLD];
    uint16_t slope[ENF_NUM_SLOPE];
} enf_coring_t;

typedef struct {
    uint16_t threshold[ENF_NUM_THRESHOLD];
    uint16_t slope[ENF_NUM_SLOPE];
    uint16_t gain_n[ENF_NUM_GAIN_LIMIT];
    uint16_t gain_p[ENF_NUM_GAIN_LIMIT];
} enf_halo_t;

typedef struct {
    uint16_t threshold;
    uint16_t slope;
    uint16_t gain;
} enf_bright_t;

typedef struct {
    uint16_t m[ENF_NUM_TOTAL_GAIN];
    uint16_t s[ENF_NUM_TOTAL_GAIN];
} enf_total_gain_t;

typedef struct {
    int16_t gain[ENF_NUM_DITHER_GAIN];
} enf_dither_t;

typedef struct __attribute__((packed)) {
    enf_sub_band_t    sub_band[ENF_NUM_SUB_BAND];
    enf_coring_t          coring;
    enf_halo_t              halo;
    enf_bright_t          bright;
    enf_total_gain_t    total_gain;
    enf_dither_t          dither;

    uint16_t                  motion_enable;
    uint16_t                  dither_enable;
    uint16_t          hh_proc_on;
    uint16_t          enf_mode;
    uint16_t          edge_enh_en;
}
enf_para_t;

typedef struct __attribute__((packed)) {
    int image_height;
    int image_width;
    int refFrameIdx;
    int nonRefFrameIdx;
    int Wmult[10];
    int uvmult;
    int mRefWeight;
    int mPeakThreshold;
    int lumaGain;
    int chromaGain;
    int lumaOffset;
    int mNumFrames;
    int green;
    int skin;
    int lshift;
    int rshift;
}
BLEND_para_t;

// DMA param
typedef enum {
    //bpp        kernel restriction
    OFI_DATA_NONE = 0,
    OFI_DATA_U8,                 //  8
    OFI_DATA_U16,                //  8*2
    OFI_DATA_U32,                //  8*4
    OFI_DATA_S8,                 //  8
    OFI_DATA_S16,                //  8*2
    OFI_DATA_S32,                //  8*4

    OFI_DATA_RGBX888,            //  8*4
    OFI_DATA_RGB888,             //  8*3
    OFI_DATA_BGRX888,            //  8*4
    OFI_DATA_BGR888,             //  8*3
    OFI_DATA_LUV888,             //  8*3

    OFI_DATA_YUV422_1P,          //  8*2
    OFI_DATA_YUV422_2P,          //  8*2       invalid for kernel, use OFI_DATA_U8 instead
    OFI_DATA_YUV422_UV,          //  8*2

    OFI_DATA_YUV420_NV21,        //  12        invalid for kernel, use OFI_DATA_U8 instead
    OFI_DATA_YUV420_NV12,        //  12        invalid for kernel, use OFI_DATA_U8 instead
    OFI_DATA_YUV420_NV21_UV,     //  8*2
    OFI_DATA_YUV420_NV12_UV,     //  8*2

    OFI_DATA_F8,                 //  8
    OFI_DATA_F16,                //  8*2
    OFI_DATA_F32,                //  8*4

    OFI_DATA_MAX
} ofi_data_type_e;

typedef enum {
    DS_2X2_SEL,
    DS_2X1_SEL,
} ds_type_e;

typedef enum {
    DMA_ROT_NONE,
    DMA_ROT_CW_90,
    DMA_ROT_CW_180,
    DMA_ROT_CW_270,
    DMA_ROT_FLIP_V,
    DMA_ROT_FLIP_H,
    DMA_ROT_TRNSP_LT_RB, //from left top to right bottom
    DMA_ROT_TRNSP_RT_LB, //from right top to left bottom
} rot_type_e;

typedef enum {
    DMA_NORMAL,
    DMA_EXT_U8U16,
    DMA_EXT_U16U32,
    DMA_EXT_U8U32,
    DMA_EXT_S8S16,
    DMA_EXT_S16S32,
    DMA_EXT_S8S32,
    DMA_EXT_S8U16,
    DMA_EXT_S16U32,
    DMA_EXT_S8U32,
    DMA_DUP8,
    DMA_DUP16,
    DMA_DUP2D8,
    DMA_SAT_U16U8,
    DAM_SAT_U32U16,
    DMA_SAT_U32U8,
    DMA_SAT_U16S8,
    DMA_SAT_U32S16,
    DMA_SAT_U32S8,
    DMA_SAT_S16U8,
    DMA_SAT_S32U16,
    DMA_SAT_S32U8,
    DMA_SAT_S16S8,
    DMA_SAT_S32S16,
    DMA_SAT_S32S8,
    DMA_TRNC_16_8,
    DMA_TRNC_32_16,
    DMA_TRNC_32_8,
    DMA_AVG_16_8,
    DMA_AVG_32_16,
    DMA_AVG_32_8,
} conversion_type_e;

typedef struct __attribute__((packed)) {
    uint32_t data_type; //ofi_data_type_e
}
dm_para_hdr_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
}
interleave_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
}
deinterleave_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
    uint32_t type; //ds_type_e
    uint32_t sample_index_w;
    uint32_t sample_index_h;
}
down_scale_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
    uint32_t type; //rot_type_e
}
rotate_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
    uint32_t type; //conversion_type_e
}
convert_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
}
min_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
}
max_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
}
sum_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
}
square_sum_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
}
average_para_t;

typedef struct {
     unsigned int dims[3]; //channel/width/height
}buffer_info_t;

 typedef struct {
     unsigned int inbuffer_index;
     unsigned int outbuffer_index;
 } node_info_t;

typedef struct {
    unsigned int buffer_infos_offset;
    char data[0];
} node_info_list_t;


//TODO : not supported yet
/*
typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
    uint32_t target_data_type; //general_data_type_e
    uint32_t scale;
    uint32_t bias;
}
quantize_para_t;

typedef struct __attribute__((packed)) {
    dm_para_hdr_t hdr;
    uint32_t target_data_type; //general_data_type_e
    uint32_t scale;
    uint32_t bias;
}
dequantize_para_t;
*/

#endif
