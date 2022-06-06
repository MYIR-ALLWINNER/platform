/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : deinterlace_new.c
 * Description : hardware deinterlacing
 * History :
 *
 */

#include <memoryAdapter.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS // Otherwise not available in C++.
#include <inttypes.h>
#include <deinterlace_new.h>
#include <ditInterface.h>
#include <deinterlace3.h>
#include <stdlib.h>
#include <unistd.h>
#include <cdx_log.h>

#define DI_DEVICE_NAME "/dev/deinterlace"

//-----------------------------------------------------------------------------

static int setBufAddr(struct di_fb* fb, DIFrame* frame)
{
    //note: need to support more foramt
    fb->format = frame->mPixelFormat;
    fb->size.width = frame->mWidth;
    fb->size.height = frame->mHeight;
    fb->buf.ystride = ALIGN(frame->mAlignSize,frame->mWidth);

    if(frame->mBufFd < 0)
    {
        fb->dma_buf_fd = -1;
        fb->buf.y_addr = frame->mAddrPhy;
        fb->buf.cb_addr = frame->mAddrPhy + ALIGN(frame->mAlignSize,frame->mWidth) *
            ALIGN(frame->mAlignSize,frame->mHeight);
    }
    else
    {
        fb->dma_buf_fd = frame->mBufFd;
        fb->buf.y_addr = 0;
        fb->buf.cb_addr = ALIGN(frame->mAlignSize,frame->mWidth) *
            ALIGN(frame->mAlignSize,frame->mHeight);
    }

    switch(fb->format)
    {
        case DRM_FORMAT_YUV420:
        case DRM_FORMAT_YVU420:
            fb->buf.cstride = frame->mWidth/2;
            fb->buf.cr_addr = fb->buf.cb_addr + (ALIGN(frame->mAlignSize,frame->mWidth) *
                ALIGN(frame->mAlignSize,frame->mHeight))/4;
            break;
        case DRM_FORMAT_YUV422:
        case DRM_FORMAT_YVU422:
            fb->buf.cstride = frame->mWidth;
            fb->buf.cr_addr = fb->buf.cb_addr + (ALIGN(frame->mAlignSize,frame->mWidth) *
                ALIGN(frame->mAlignSize,frame->mHeight))/2;
            break;
        case DRM_FORMAT_NV12:
        case DRM_FORMAT_NV21:
            fb->buf.cstride = frame->mWidth;
            fb->buf.cr_addr = 0;
            break;
        case DRM_FORMAT_NV16:
        case DRM_FORMAT_NV61:
            fb->buf.cstride = frame->mWidth*2;
            fb->buf.cr_addr = 0;
            break;
        default:
            logw("note: format not support");
    }

    logv("Format:%d width:%u height:%u dma_buf_fd:%d "
        "y_addr:%llx cb_addr:%llx cr_addr:%llx ystride:%u cstride:%u\n",
        fb->format, fb->size.width, fb->size.height, fb->dma_buf_fd,
        fb->buf.y_addr, fb->buf.cb_addr, fb->buf.cr_addr,
        fb->buf.ystride, fb->buf.cstride);
    return 0;
}

static int configBufAndStart(DeInterlaceCtx* context, InputFrame* input,OutputFrame* output)
{

    DiParaT3 di_para;
    memset(&di_para,0x0,sizeof(DiParaT3));

    switch(context->mode)
    {
        case DI300_MODE_BOB:
        case DI300_MODE_WEAVE:
            setBufAddr(&di_para.in_fb0,&input->pInPicture0);
            setBufAddr(&di_para.out_dit_fb0,&output->pOutPicture0);
            break;
        case DI300_MODE_30HZ:
            setBufAddr(&di_para.in_fb0,&input->pInPicture0);
            setBufAddr(&di_para.in_fb1,&input->pInPicture1);
            setBufAddr(&di_para.out_dit_fb0,&output->pOutPicture0);
            if(context->tnrOpen)
                setBufAddr(&di_para.out_tnr_fb0,&output->pOutPicture1);
            break;
        case DI300_MODE_60HZ:
            setBufAddr(&di_para.in_fb0,&input->pInPicture0);
            setBufAddr(&di_para.in_fb1,&input->pInPicture1);
            setBufAddr(&di_para.in_fb2,&input->pInPicture2);
            setBufAddr(&di_para.out_dit_fb0,&output->pOutPicture0);
            setBufAddr(&di_para.out_dit_fb1,&output->pOutPicture1);
            if(context->tnrOpen)
                setBufAddr(&di_para.out_tnr_fb0,&output->pOutPicture2);
            break;
        case DI300_MODE_TNR:
            setBufAddr(&di_para.in_fb0, &input->pInPicture0);
            setBufAddr(&di_para.in_fb1, &input->pInPicture1);
            setBufAddr(&di_para.out_tnr_fb0,&output->pOutPicture0);
            break;
        default:
            loge("di mode not support!");
    }
    di_para.is_interlace = 1;
    di_para.is_pulldown = 0;
    di_para.top_field_first = 1;
    di_para.base_field = 1;

    if (ioctl(context->fd, DI_IOC_PROCESS_FB, (unsigned long)&di_para) < 0)
    {
        loge("di process failed.%s",strerror(errno));
        return -1;
    }
    return 0;
}

static int checkFormat(DeInterlaceCtx* context, InputFrame* input, OutputFrame* output)
{
    int result = 0;
    switch(context->mode)
    {
        case DI_MODE_30HZ:
        case DI_MODE_60HZ:
        case DI_MODE_TNR:
            if(context->tnrOpen)
            {
                switch(input->pInPicture0.mPixelFormat)
                {
                    case DRM_FORMAT_NV12:
                    case DRM_FORMAT_NV21:
                    case DRM_FORMAT_YUV420:
                    case DRM_FORMAT_YVU420:
                        if(output->pOutPicture0.mPixelFormat != DRM_FORMAT_YUV420 &&
                            output->pOutPicture0.mPixelFormat != DRM_FORMAT_YVU420)
                        {
                            result = -1;
                        }
                        break;
                    case DRM_FORMAT_NV16:
                    case DRM_FORMAT_NV61:
                    case DRM_FORMAT_YUV422:
                    case DRM_FORMAT_YVU422:
                        if(output->pOutPicture0.mPixelFormat != DRM_FORMAT_YUV422 &&
                            output->pOutPicture0.mPixelFormat != DRM_FORMAT_YVU422)
                        {
                            result = -1;
                        }
                        break;
                    default:
                        loge("input format not support");
                        result = -1;
                }
            }
            break;
        case DI_MODE_BOB:
        case DI300_MODE_WEAVE:
            if(input->pInPicture0.mPixelFormat != input->pInPicture0.mPixelFormat)
                result =-1;
            break;
        default:
            logw("di mode invalid");

    }

    return result;
}

static int DI300Process(Deinterlace* pSelf, InputFrame* input, OutputFrame* output)
{
    DeInterlaceCtx* context = (DeInterlaceCtx*)pSelf;
    struct di_size size_in;
    struct di_rect rect;
    struct di_dit_mode dit_mode;
    struct di_fmd_enable fmd_en;
    struct di_tnr_mode tnr_mode;
    struct di_demo_crop_arg demo_arg;

    if(checkFormat(context, input, output) < 0)
    {
        loge("format not support");
        return -1;
    }
    if(context->fd < 0)
    {
        loge("device no open!");
        return -1;
    }
    if (context->resetFlag)
    {
        if (ioctl(context->fd, DI_IOC_RESET, 0) > 0)
        {
            loge("DI_IOC_RESET failed\n");
            return -1;
        }
        context->resetFlag = 0;
    }
    size_in.width = input->pInPicture0.mWidth;
    size_in.height = input->pInPicture0.mHeight;
    if (ioctl(context->fd, DI_IOC_SET_VIDEO_SIZE, &size_in) > 0)
    {
        loge("DI_IOC_SET_VIDEO_SIZE failed\n");
        return -1;
    }

    if(context->cropOpen)
    {
        rect.left = context->crop.left;
        rect.top = context->crop.top;
        rect.right = context->crop.right;
        rect.bottom = context->crop.bottom;
    }
    else
    {
        rect.left = 0;
        rect.top = 0;
        rect.right = size_in.width;
        rect.bottom = size_in.height;

    }
    if (ioctl(context->fd, DI_IOC_SET_VIDEO_CROP, &rect) > 0)
    {
        loge("DI_IOC_SET_VIDEO_CROP failed\n");
        return -1;
    }

    switch(context->mode)
    {
         case DI_MODE_30HZ:
            dit_mode.intp_mode = DI_DIT_INTP_MODE_MOTION;
            dit_mode.out_frame_mode = DI_DIT_OUT_1FRAME;
            break;
         case DI_MODE_60HZ:
            dit_mode.intp_mode = DI_DIT_INTP_MODE_MOTION;
            dit_mode.out_frame_mode = DI_DIT_OUT_2FRAME;
            break;
         case DI_MODE_BOB:
            dit_mode.intp_mode = DI_DIT_INTP_MODE_BOB;
            dit_mode.out_frame_mode = DI_DIT_OUT_1FRAME;
            break;
         case DI_MODE_WEAVE:
            dit_mode.intp_mode = DI_DIT_INTP_MODE_WEAVE;
            dit_mode.out_frame_mode = DI_DIT_OUT_1FRAME;
            break;
         case DI_MODE_TNR:
            dit_mode.intp_mode = DI_DIT_INTP_MODE_INVALID;
            dit_mode.out_frame_mode = DI_DIT_OUT_0FRAME;
            break;
         default:
            dit_mode.intp_mode = DI_DIT_INTP_MODE_INVALID;
            dit_mode.out_frame_mode = DI_DIT_OUT_0FRAME;
    }

    if (ioctl(context->fd, DI_IOC_SET_DIT_MODE, &dit_mode) > 0)
    {
        loge("DI_IOC_SET_DIT_MODE failed,error:%s\n",strerror(errno));
        return -1;
    }

    if(context->filmDetect)
    {
        fmd_en.en = 1;
        if (ioctl(context->fd, DI_IOC_SET_FMD_ENABLE, &fmd_en) > 0)
        {
            loge("DI_IOC_SET_FMD_ENABLE failed\n");
            return -1;
        }
    }

    if(context->tnrOpen)
    {
        tnr_mode.mode = DI_TNR_MODE_FIX;
        tnr_mode.level = DI_TNR_LEVEL_MIDDLE;
        if (ioctl(context->fd, DI_IOC_SET_TNR_MODE, &tnr_mode) > 0)
        {
            loge("DI_IOC_SET_TNR_MODE failed\n");
            return -1;
        }
    }

    if(context->contrastOpen)
    {
        if(context->tnrOpen)
        {
            demo_arg.tnr_demo.left = context->contrast.left;
            demo_arg.tnr_demo.top = context->contrast.top;
            demo_arg.tnr_demo.right = context->contrast.right;
            demo_arg.tnr_demo.bottom = context->contrast.bottom;
        }
        demo_arg.dit_demo.left = context->contrast.left;
        demo_arg.dit_demo.top = context->contrast.top;
        demo_arg.dit_demo.right = context->contrast.right;
        demo_arg.dit_demo.bottom = context->contrast.bottom;
        if (ioctl(context->fd, DI_IOC_SET_DEMO_CROP, &demo_arg) < 0)
        {
            loge("DI_IOC_SET_DEMO_CROP failed.");
            return -1;
        }
    }

    if (ioctl(context->fd, DI_IOC_CHECK_PARA, 0) > 0)
    {
        loge("DI_IOC_CHECK_PARA\n");
        return -1;
    }
    return configBufAndStart(context,input,output);

}

static int DI300GetInfo(Deinterlace* pSelf, DeInterlaceInfo *pInfo)
{
    DeInterlaceCtx* context = (DeInterlaceCtx*)pSelf;

    if(context == NULL || pInfo == NULL)
    {
        loge("paramter error!");
        return -1;
    }

    pInfo->nFormat = PIXEL_FORMAT_YV12; //di300 60Hz mode needed

    pInfo->nMaxTrackNum = MAX_TRACK_NUM;
    pInfo->nDiHw = 1;
    return 0;
}

static int DI300SetParam(Deinterlace* pSelf, DIParam pParam)
{
    DeInterlaceCtx* context = (DeInterlaceCtx*)pSelf;

    if(context == NULL)
    {
        loge("paramter error!");
        return -1;
    }

    context->filmDetect = pParam.filmDetect;
    context->contrastOpen = pParam.contrastOpen;
    context->mode = pParam.mode;
    context->tnrOpen = pParam.tnrOpen;
    context->quueOpen = pParam.quueOpen;
    context->cropOpen = pParam.cropOpen;

    switch(context->mode)
    {
        case DI_MODE_30HZ:
        case DI_MODE_60HZ:
            if(context->tnrOpen && context->cropOpen)
            {
                loge("30/60hz mode and tnr open we should not used crop");
                return -1;
            }
            break;
        case DI_MODE_TNR:
            if(context->tnrOpen == 0)
            {
                loge("mode tnr but tnr is close");
                return -1;
            }
            break;
        default:
            logw("no check param");
    }

    return 0;
}

static int releaseDeInterlace(DeInterlaceCtx* pSelf)
{

   if(pSelf->fd < 0)
   {
        loge("device fd is unvalidable");
        return 0;
   }
   close(pSelf->fd);
   pSelf->fd = -1;

   return 0;
}


static int getDIVersion(int fd)
{
    int ret;
    struct di_version v;

    ret = ioctl(fd, DI_IOC_GET_VERSION, &v);
    if (ret)
    {
        logw("DI_IOC_GET_VERSION failed\n");
        return ret;
    }

    logd("di version[%d.%d.%d], hw_ip[%d]\n",
        v.version_major,
        v.version_minor,
        v.version_patchlevel,
        v.ip_version);
    return ret;
}

static int setTimeout(int fd)
{
    int ret = 0;
    struct di_timeout_ns t;

    t.wait4start = 500000000ULL;
    t.wait4finish = 600000000ULL;
    ret = ioctl(fd, DI_IOC_SET_TIMEOUT, &t);
    if (ret)
    {
        logw("DI_IOC_SET_TIMEOUT failed\n");
        return ret;
    }

    return ret;
}

static int initDeInterlace(DeInterlaceCtx* pSelf)
{

    if (pSelf->fd != -1)
    {
        logw("already init...");
        return 0;
    }

    pSelf->fd = open(DI_DEVICE_NAME, O_RDWR);
    if (pSelf->fd == -1)
    {
        loge("open hw devices failure, errno(%s)", strerror(errno));
        return -1;
    }

    if (getDIVersion(pSelf->fd) < 0)
    {
        logw("getVersion failed!\n");
    }

    /*set deinterlace waiting start and finishing timeout*/
    if (setTimeout(pSelf->fd) < 0)
    {
        logw("setTimeout failed!\n");
    }
    pSelf->nLastPts = 0;

    pSelf->picCount = 0;
    logv("hw deinterlace init success...");
    return 0;
}

static int DI300Reset(Deinterlace* pSelf)
{
    DeInterlaceCtx* context = (DeInterlaceCtx*)pSelf;

    logd("%s", __FUNCTION__);

    if(releaseDeInterlace(context) < 0)
        return -1;

    return initDeInterlace(context);
}

static struct DeinterlaceOps mDiOps =
{
    .reset       = DI300Reset,
    .getInfo     = DI300GetInfo,
    .setParamter = DI300SetParam,
    .process     = DI300Process,

};

Deinterlace* DI300Create( )
{
    DeInterlaceCtx* context;

    context = (DeInterlaceCtx*)malloc(sizeof(DeInterlaceCtx));

    if(context == NULL)
    {
        logw("deinterlace create failed");
        return NULL;
    }
    memset(context, 0, sizeof(DeInterlaceCtx));
    // we must set the fd to -1; or it will close the file which fd is 0 when destroy
    context->fd = -1;

    context->interfae.ops = &mDiOps;
    if(initDeInterlace(context)<0)
    {
        return NULL;
    }
    context->resetFlag = 1;
    return &context->interfae;
}


int DI300Destory(Deinterlace* pSelf)
{
    DeInterlaceCtx* context;
    //void* retVal;
    context = (DeInterlaceCtx*)pSelf;

    if(releaseDeInterlace(context) < 0)
        return -1;

    //pthread_join(&context->mThreadId, &retVal);

    free(context);

    return 0;
}
