/*
 * =====================================================================================
 *
 *       Filename:  deinterlace_new.h
 *
 *    Description:  :
 *
 *        Version:  1.0
 *        Created:  2020年02月28日 14时46分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:
 *
 * =====================================================================================
 */
#ifndef DEINTERLACE_NEW_H
#define DEINTERLACE_NEW_H

#include "ditInterface.h"
#define MAX_TRACK_NUM 4

#define MAX_PIC_NODE_NUM 16

#define ALIGN(x,y) (((y) + (x-1)) & ~(x-1))

typedef enum PixelFormatT
{
    PIXEL_FORMAT_DEFAULT            = 0,

    PIXEL_FORMAT_YUV_PLANER_420     = 1,
    PIXEL_FORMAT_YUV_PLANER_422     = 2,
    PIXEL_FORMAT_YUV_PLANER_444     = 3,

    PIXEL_FORMAT_YV12               = 4,
    PIXEL_FORMAT_NV21               = 5,
    PIXEL_FORMAT_NV12               = 6,
    PIXEL_FORMAT_YUV_MB32_420       = 7,
    PIXEL_FORMAT_YUV_MB32_422       = 8,
    PIXEL_FORMAT_YUV_MB32_444       = 9,

    PIXEL_FORMAT_RGBA               = 10,
    PIXEL_FORMAT_ARGB               = 11,
    PIXEL_FORMAT_ABGR               = 12,
    PIXEL_FORMAT_BGRA               = 13,

    PIXEL_FORMAT_YUYV               = 14,
    PIXEL_FORMAT_YVYU               = 15,
    PIXEL_FORMAT_UYVY               = 16,
    PIXEL_FORMAT_VYUY               = 17,

    PIXEL_FORMAT_PLANARUV_422       = 18,
    PIXEL_FORMAT_PLANARVU_422       = 19,
    PIXEL_FORMAT_PLANARUV_444       = 20,
    PIXEL_FORMAT_PLANARVU_444       = 21,
    PIXEL_FORMAT_P010_UV            = 22,
    PIXEL_FORMAT_P010_VU            = 23,

    PIXEL_FORMAT_MIN = PIXEL_FORMAT_DEFAULT,
    PIXEL_FORMAT_MAX = PIXEL_FORMAT_P010_VU,
} PixelFormat;

typedef struct DeInterlaceParamT
{
    int nTrackNum;
} DeInterlaceParam;

typedef struct DiPictureT
{

} DiPicture;

typedef struct PictureNodeT PictureNode;
struct PictureNodeT
{
    int bUseFlag;
    DiPicture  sPicture;
    PictureNode*    pNext;
};

typedef struct DeInterlaceCtx
{
    Deinterlace interfae;
    int fd;
    int picCount;
    //param
    int filmDetect;
    int tnrOpen; //3D tnr
    int contrastOpen;
    RectCrop contrast;
    DIMode mode;
    int cropOpen;
    RectCrop crop;
	int quueOpen;
	unsigned int nLastPts;
    struct ScMemOpsS* pMem;
	struct DIFrameT tempFrame[3];
    int resetFlag;
} DeInterlaceCtx;

Deinterlace* DI300Create( );
int DI300Destory(Deinterlace* pSelf);

#endif
