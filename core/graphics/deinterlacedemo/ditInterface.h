/*
 * =====================================================================================
 *
 *       Filename:  ditInterface.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年03月05日 19时39分05秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:
 *
 * =====================================================================================
 */
#ifndef DEINTERLACE_H
#define DEINTERLACE_H

#ifdef __cplusplus
extern "C" {
#endif

#define DI_TYPE_DI200 1
#define DI_TYPE_DI300 2

#define fourcc_code(a, b, c, d) ((unsigned int)(a) | ((unsigned int)(b) << 8) | \
                                ((unsigned int)(c) << 16) | ((unsigned int)(d) << 24))
/* packed YCbCr */
#define DRM_FORMAT_YUYV     fourcc_code('Y', 'U', 'Y', 'V') /* [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian */
#define DRM_FORMAT_YVYU     fourcc_code('Y', 'V', 'Y', 'U') /* [31:0] Cb0:Y1:Cr0:Y0 8:8:8:8 little endian */
#define DRM_FORMAT_UYVY     fourcc_code('U', 'Y', 'V', 'Y') /* [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian */
#define DRM_FORMAT_VYUY     fourcc_code('V', 'Y', 'U', 'Y') /* [31:0] Y1:Cb0:Y0:Cr0 8:8:8:8 little endian */

#define DRM_FORMAT_AYUV     fourcc_code('A', 'Y', 'U', 'V') /* [31:0] A:Y:Cb:Cr 8:8:8:8 little endian */

/*
 * 2 plane YCbCr
 * index 0 = Y plane, [7:0] Y
 * index 1 = Cr:Cb plane, [15:0] Cr:Cb little endian
 * or
 * index 1 = Cb:Cr plane, [15:0] Cb:Cr little endian
 */
#define DRM_FORMAT_NV12     fourcc_code('N', 'V', '1', '2') /* 2x2 subsampled Cr:Cb plane */
#define DRM_FORMAT_NV21     fourcc_code('N', 'V', '2', '1') /* 2x2 subsampled Cb:Cr plane */
#define DRM_FORMAT_NV16     fourcc_code('N', 'V', '1', '6') /* 2x1 subsampled Cr:Cb plane */
#define DRM_FORMAT_NV61     fourcc_code('N', 'V', '6', '1') /* 2x1 subsampled Cb:Cr plane */
#define DRM_FORMAT_NV24     fourcc_code('N', 'V', '2', '4') /* non-subsampled Cr:Cb plane */
#define DRM_FORMAT_NV42     fourcc_code('N', 'V', '4', '2') /* non-subsampled Cb:Cr plane */

/*
 * 3 plane YCbCr
 * index 0: Y plane, [7:0] Y
 * index 1: Cb plane, [7:0] Cb
 * index 2: Cr plane, [7:0] Cr
 * or
 * index 1: Cr plane, [7:0] Cr
 * index 2: Cb plane, [7:0] Cb
 */
#define DRM_FORMAT_YUV410   fourcc_code('Y', 'U', 'V', '9') /* 4x4 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU410   fourcc_code('Y', 'V', 'U', '9') /* 4x4 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV411   fourcc_code('Y', 'U', '1', '1') /* 4x1 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU411   fourcc_code('Y', 'V', '1', '1') /* 4x1 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV420   fourcc_code('Y', 'U', '1', '2') /* 2x2 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU420   fourcc_code('Y', 'V', '1', '2') /* 2x2 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV422   fourcc_code('Y', 'U', '1', '6') /* 2x1 subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU422   fourcc_code('Y', 'V', '1', '6') /* 2x1 subsampled Cr (1) and Cb (2) planes */
#define DRM_FORMAT_YUV444   fourcc_code('Y', 'U', '2', '4') /* non-subsampled Cb (1) and Cr (2) planes */
#define DRM_FORMAT_YVU444   fourcc_code('Y', 'V', '2', '4') /* non-subsampled Cr (1) and Cb (2) planes */

typedef struct DIFrameT DIFrame;

struct DIFrameT
{
    unsigned int mAddrPhy;
    unsigned char* mAddrVir;
    int mWidth;
    int mHeight;
    int mAlignSize;
    int mBufFd;
    int mPixelFormat;
    int mTopFeild;
    //DIFrame next;
};

typedef struct InputFrameT
{
    DIFrame pInPicture0;
    DIFrame pInPicture1;
    DIFrame pInPicture2;
} InputFrame;

typedef struct OutputFrameT
{
    DIFrame pOutPicture0;
    DIFrame pOutPicture1;
    DIFrame pOutPicture2;
} OutputFrame;

typedef struct RectCropT
{
    int left;
    int top;
    int right;
    int bottom;
} RectCrop;

typedef enum DIModeT
{
    DI_MODE_INVALID = 0,
    DI_MODE_60HZ,
    DI_MODE_30HZ,
    DI_MODE_BOB,
    DI_MODE_WEAVE,
    DI_MODE_TNR, /* only tnr */
} DIMode;

typedef struct DIParamT
{
    int filmDetect;
    int tnrOpen; //3D tnr
    int contrastOpen;
    RectCrop contrast;
    DIMode mode;
    int cropOpen;
    RectCrop crop;
    int quueOpen;

} DIParam;

typedef struct DeInterlaceInfoT
{
    int nFormat;
    int nDiHw;
    int nMaxTrackNum;
    int nValidPic;
} DeInterlaceInfo;

typedef struct DeinterlaceS  Deinterlace;

typedef struct DeinterlaceOps
{
    int (*reset)(Deinterlace* di);

    int (*getInfo)(Deinterlace* di, DeInterlaceInfo* pInfo);

    int (*setParamter)(Deinterlace* di, DIParam pParam);

    int (*process)(Deinterlace* di, InputFrame* input, OutputFrame* output);

} DeinterlaceOps;

struct DeinterlaceS
{
    DeinterlaceOps *ops;
};

//no1. create
Deinterlace* CdxDiCreate(int type);
//no2. destory
void CdxDiDestory(Deinterlace* pSelf, int type);

int CdxDiSetParameter(Deinterlace* di, DIParam pParam);

int CdxDiReset(Deinterlace* di);

int CdxDiGetInfo(Deinterlace* di, DeInterlaceInfo* pInfo);

int CdxDiProcess(Deinterlace* di, InputFrame* input, OutputFrame* output);


#ifdef __cplusplus
}
#endif

#endif
