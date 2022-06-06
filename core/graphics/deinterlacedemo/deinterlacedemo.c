/*
 * File : Deinterlacedemo.c
 * Description : hardware deinterlacing
 * History :
 */
//=====================================================

#include <string.h>
#include "ditInterface.h"
#include <memoryAdapter.h>
#include <veAdapter.h>
#include <sc_interface.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

typedef size_t  size_addr;

const char* FpInPicPath0 = "/data/picture/picture_in0.yuv";
static FILE* FpInPic0 = NULL;
const char* FpInPicPath1 = "/data/picture/picture_in1.yuv";
static FILE* FpInPic1 = NULL;

const char* FpOutPicPath = "/data/picture/picture_out.yuv";
static FILE* FpOutPic = NULL;

typedef struct VideoInfo
{
    int nWidth;
    int nHeight;
    int nSize;
    int nInPixelFormat;
    int nOutPixelFormat;

    void* pInputVir0;
    unsigned int nInputPhy0;
    void* pInputVir1;
    unsigned int nInputPhy1;
    void* pOutputVir0;
    unsigned int nOutputPhy0;

    VeOpsS* pVeOpsS;
    void* pVeOpsSelf;
    InputFrameT nInput;
    OutputFrameT nOutput;
    DIParam nParam;

}VideoInfo;

int FileSize( const char* FpInPicPath)
{
    int size = 0;
    FILE *fp;

    fp = fopen(FpInPicPath, "r");
    if(!fp)
    {
        printf("open picture file failed!\n");
        return -1;
    }

    fseek(fp,0L,SEEK_END);
    size=ftell(fp);
    fclose(fp);

    return size;
}

int DiPrepare(VideoInfo*  pVideoInfo)
{
    int ret = 0;
    int nInPixFor;
    int nWidth;
    int nHeight;

    FpInPic0 = fopen(FpInPicPath0, "r");
    FpInPic1 = fopen(FpInPicPath1, "r");
    if (!FpInPic0 ||!FpInPic0)
    {
        printf("open picture file failed!\n");
        printf("the input data is NULL, please Enter data into the directory: /data/picture/\n");
        printf("And name the two files as: picture_in0.yuv and picture_in1.yuv \n");
        return -1;
    }

    printf("\nplease enter the Width( must be 16 aligned): \n");
    scanf("%d",&nWidth);
    printf("please enter the Height( must be 16 aligned): \n");
    scanf("%d",&nHeight);

    pVideoInfo->nWidth = nWidth;
    pVideoInfo->nHeight = nHeight;
    printf("the Resolution = %d x %d \n",pVideoInfo->nWidth, pVideoInfo->nHeight);
    pVideoInfo->nSize = pVideoInfo->nWidth * pVideoInfo->nHeight*3/2;

    printf("PixelFormat: \n");
    printf("0: DRM_FORMAT_YUV422\n");
    printf("1: DRM_FORMAT_YUV420\n");
    printf("2: DRM_FORMAT_YVU420\n");
    printf("3: DRM_FORMAT_NV12\n");
    printf("4: DRM_FORMAT_NV21\n");
    printf("please choose the InPixelFormat: \n");
    scanf("%d",&nInPixFor);

    switch(nInPixFor)
    {
        case 0:
            pVideoInfo->nInPixelFormat = DRM_FORMAT_YUV422;
            break;
        case 1:
            pVideoInfo->nInPixelFormat = DRM_FORMAT_YUV420;
            break;
        case 2:
            pVideoInfo->nInPixelFormat = DRM_FORMAT_YVU420;
            break;
        case 3:
            pVideoInfo->nInPixelFormat = DRM_FORMAT_NV12;
            break;
        case 4:
            pVideoInfo->nInPixelFormat = DRM_FORMAT_NV21;
            break;
        default:
            printf("wrong InPixelFormat!\n");
            ret = -1;
            break;
    }
    pVideoInfo->nOutPixelFormat = DRM_FORMAT_YUV420;

    return ret;
}

int DiIOParamConfig(VideoInfo* pVideoInfo)
{
    VeConfig nVeConfig;
    struct ScMemOpsS* pMemops = NULL;
    int nType = VE_OPS_TYPE_NORMAL;

    pMemops = MemAdapterGetOpsS();

    if(pMemops == NULL)
    {
        printf("memops is NULL\n");
        return -1;
    }
    CdcMemOpen(pMemops);

    pVideoInfo->pVeOpsS = GetVeOpsS(nType);
    if(pVideoInfo->pVeOpsS == NULL)
    {
        printf("get ve ops failed , type = %d\n",nType);
        return -1;
    }

    memset(&nVeConfig, 0, sizeof(VeConfig));
    nVeConfig.nDecoderFlag = 0;
    nVeConfig.nEncoderFlag = 1;
    nVeConfig.nEnableAfbcFlag = 0;
    nVeConfig.nFormat = 0;
    nVeConfig.nWidth = 0;
    nVeConfig.nResetVeMode = 0;

    pVideoInfo->pVeOpsSelf = CdcVeInit(pVideoInfo->pVeOpsS, &nVeConfig);
    if(pVideoInfo->pVeOpsSelf == NULL)
    {
        printf("init ve ops failed\n");
        return -1;
    }

    pVideoInfo->pInputVir0 = CdcMemPalloc(pMemops, pVideoInfo->nSize,(void *)pVideoInfo->pVeOpsS, (void *)pVideoInfo->pVeOpsSelf);
    pVideoInfo->nInputPhy0 = (size_addr)CdcMemGetPhysicAddress(pMemops, pVideoInfo->pInputVir0);
    pVideoInfo->pInputVir1 = CdcMemPalloc(pMemops, pVideoInfo->nSize,(void *)pVideoInfo->pVeOpsS, (void *)pVideoInfo->pVeOpsSelf);
    pVideoInfo->nInputPhy1 = (size_addr)CdcMemGetPhysicAddress(pMemops, pVideoInfo->pInputVir1);

    //input buffer get data
    fread(pVideoInfo->pInputVir0, 1, pVideoInfo->nSize, FpInPic0);
    fread(pVideoInfo->pInputVir1, 1, pVideoInfo->nSize, FpInPic1);

    pVideoInfo->nInput.pInPicture0.mBufFd = -1;
    pVideoInfo->nInput.pInPicture0.mAddrPhy = (unsigned int)pVideoInfo->nInputPhy0;
    pVideoInfo->nInput.pInPicture0.mAddrVir = (unsigned char*)pVideoInfo->pInputVir0;
    pVideoInfo->nInput.pInPicture0.mWidth = pVideoInfo->nWidth;
    pVideoInfo->nInput.pInPicture0.mHeight = pVideoInfo->nHeight;
    pVideoInfo->nInput.pInPicture0.mPixelFormat = pVideoInfo->nInPixelFormat;
    pVideoInfo->nInput.pInPicture0.mAlignSize = 16;

    pVideoInfo->nInput.pInPicture1.mBufFd = -1;
    pVideoInfo->nInput.pInPicture1.mAddrPhy = (unsigned int)pVideoInfo->nInputPhy1;
    pVideoInfo->nInput.pInPicture1.mAddrVir = (unsigned char*)pVideoInfo->pInputVir1;
    pVideoInfo->nInput.pInPicture1.mWidth = pVideoInfo->nWidth;
    pVideoInfo->nInput.pInPicture1.mHeight = pVideoInfo->nHeight;
    pVideoInfo->nInput.pInPicture1.mPixelFormat = pVideoInfo->nInPixelFormat;
    pVideoInfo->nInput.pInPicture1.mAlignSize = 16;

    //Configure the output buffer parameters
    pVideoInfo->pOutputVir0 = CdcMemPalloc(pMemops, pVideoInfo->nSize,(void *)pVideoInfo->pVeOpsS, (void *)pVideoInfo->pVeOpsSelf);
    pVideoInfo->nOutputPhy0 = (size_addr)CdcMemGetPhysicAddress(pMemops, pVideoInfo->pOutputVir0);

    pVideoInfo->nOutput.pOutPicture0.mBufFd = -1;
    pVideoInfo->nOutput.pOutPicture0.mAddrPhy = (unsigned int)pVideoInfo->nOutputPhy0;
    pVideoInfo->nOutput.pOutPicture0.mAddrVir = (unsigned char*)pVideoInfo->pOutputVir0;
    pVideoInfo->nOutput.pOutPicture0.mWidth = pVideoInfo->nWidth;
    pVideoInfo->nOutput.pOutPicture0.mHeight = pVideoInfo->nHeight;
    pVideoInfo->nOutput.pOutPicture0.mPixelFormat = pVideoInfo->nOutPixelFormat;
    pVideoInfo->nOutput.pOutPicture0.mAlignSize = 16;

    return 0;
}

int DiProcessMode0(Deinterlace* di, VideoInfo* pVideoInfo)
{
    int ret;

    ret = CdxDiSetParameter(di, pVideoInfo->nParam);//DiSetParameter function entry
    if (ret)
    {
        printf("DeInterlace Set Parameter fail, quit.\n");
        return -1;
    }

    ret = CdxDiProcess(di, &pVideoInfo->nInput, &pVideoInfo->nOutput);
    if (ret)
    {
        printf("DeInterlace Process fail, quit.\n");
        return -1;
    }
    else
    {
        printf("DeInterlace process is successful!\n");
        printf("We have done DeInterlace processing on the input yuv data.\n");
        printf("You can check the output data\n");
        printf("The path of the output data:%s \n\n",FpOutPicPath);
    }

    //Save output picture
    FpOutPic = fopen(FpOutPicPath, "wb");
    if(FpOutPic == NULL)
    {
        printf("open picture file failed!\n");
        return -1;
    }

    if(FpOutPic)
    {
        if(pVideoInfo->nSize > 0)
            fwrite(pVideoInfo->pOutputVir0, 1, pVideoInfo->nSize, FpOutPic);
    }
    return 0;
}

int DiProcessMode1(Deinterlace* di, VideoInfo* pVideoInfo)
{
    int ret;
    int i = 0;
    int nOffset = 0;
    int nTnrProTime;

    printf("TRN 3D Open \n");
    printf( "please enter the processing times of TNR:  \n");
    scanf("%d", &nTnrProTime);

    if (nTnrProTime * pVideoInfo->nSize > FileSize(FpInPicPath1) )
    {
        printf("nTnrProTime is too big , the max num is %d \n",FileSize(FpInPicPath1)/pVideoInfo->nSize );
        return -1;
    }

    while (i++ < nTnrProTime)
    {
        printf("*********** the TNR process times =%d \n", i);
        //the two inputs for the first frame need to be equal,
        //because the FpOutPic is empty now
        if (i == 1)
        {
            printf("the first frame of TNR \n");
            ret = CdxDiSetParameter(di, pVideoInfo->nParam);
            if (ret)
            {
                printf("DeInterlace Set Parameter fail, quit.\n");
                return -1;
            }

            ret = CdxDiProcess(di, &pVideoInfo->nInput, &pVideoInfo->nOutput);
            if (ret)
            {
                printf("DeInterlace Process fail, quit.\n");
                return -1;
            }
        }
        else
        {
            //The output from the previous frame is the input for this time
            fread(pVideoInfo->pInputVir0, 1, pVideoInfo->nSize, FpOutPic);
            pVideoInfo->nInput.pInPicture0.mBufFd = -1;
            pVideoInfo->nInput.pInPicture0.mAddrPhy = (unsigned int)pVideoInfo->nInputPhy0;
            pVideoInfo->nInput.pInPicture0.mAddrVir = (unsigned char*)pVideoInfo->pInputVir0;
            pVideoInfo->nInput.pInPicture0.mWidth = pVideoInfo->nWidth;
            pVideoInfo->nInput.pInPicture0.mHeight = pVideoInfo->nHeight;
            pVideoInfo->nInput.pInPicture0.mPixelFormat = pVideoInfo->nOutPixelFormat;
            pVideoInfo->nInput.pInPicture0.mAlignSize = 16;

            nOffset += pVideoInfo->nSize;
            fseek(FpInPic1, nOffset, 0);//Skip one frame that has been read

            fread(pVideoInfo->pInputVir1, 1, pVideoInfo->nSize, FpInPic1);//Read next frame
            ret = CdxDiSetParameter(di, pVideoInfo->nParam);//DiSetParameter function entry
            if (ret)
            {
                printf("DeInterlace Set Parameter fail, quit.\n");
                return -1;
            }

            ret = CdxDiProcess(di, &pVideoInfo->nInput, &pVideoInfo->nOutput);
            if (ret)
            {
                printf("DeInterlace Process fail, quit.\n");
                return -1;
            }
            else if(i == nTnrProTime)
            {
                printf("TNR process is successful!\n");
                printf("We have done TNR processing on the input yuv data.\n");
                printf("You can check the output data\n");
                printf("The path of the output data:%s \n\n",FpOutPicPath);
            }
        }

        //Save output picture
        FpOutPic = fopen(FpOutPicPath, "wb");
        if(FpOutPic == NULL)
        {
            printf("open picture file failed!\n");
            return -1;
        }

        if(FpOutPic)
        {
            if(pVideoInfo->nSize > 0)
                fwrite(pVideoInfo->pOutputVir0, 1, pVideoInfo->nSize, FpOutPic);
        }
    }
    return 0;
}

int main (int argc, char* argv[])
{
    int ret;
    int nMode;
    Deinterlace* di = NULL;
    int nDiType = DI_TYPE_DI300;
    struct ScMemOpsS* pMemops = NULL;
    VideoInfo  pVideoInfo;

    printf("====================================================================== \n");
    printf("welcome to use this deinterlace !\n");
    printf("now, deinterlacedemo  begin!\n");
    printf("====================================================================== \n");

    ret = DiPrepare(&pVideoInfo);
    if(ret)
    {
        printf("preparation fail\n");
        goto EXIT;
    }

    di = CdxDiCreate(nDiType);//create DI
    if (di == NULL)
    {
        printf("can not create DeInterlace, quit.\n");
        goto EXIT;
    }

    ret = DiIOParamConfig(&pVideoInfo);
    if (ret)
    {
        printf("input and output Configure the parameters fail! \n");
        goto EXIT;
    }

    pVideoInfo.nParam.contrastOpen = 0;
    pVideoInfo.nParam.cropOpen = 0;
    pVideoInfo.nParam.filmDetect = 0;

    printf("Attention please!\n");
    printf("0: 30HZ only: (pParam.tnrOpen = 0) && (pParam.mode = DI_MODE_30HZ)\n");
    printf("1: TNR only: (pParam.tnrOpen = 1) && (pParam.mode = DI_MODE_TNR)\n");
    printf("please choose the mode:  \n");
    scanf ("%d",&nMode);

    switch(nMode)
    {
        case 0:
            pVideoInfo.nParam.tnrOpen = 0;
            pVideoInfo.nParam.mode = DI_MODE_30HZ;
            break;
        case 1:
            pVideoInfo.nParam.tnrOpen = 1;
            pVideoInfo.nParam.mode = DI_MODE_TNR;
            break;
        default:
            printf("wrong mode!\n");
            break;
    }

    ///Deinterlace(only) process
    if (nMode == 0)
    {
        ret = DiProcessMode0(di, &pVideoInfo);
        if (ret)
        {
            printf("DiProcessMode0 fail, quit.\n");
            goto EXIT;
        }
    }
    else if (nMode == 1)///TNR(only) process
    {
        ret = DiProcessMode1(di, &pVideoInfo);
        if (ret)
        {
            printf("DiProcessMode0 fail, quit.\n");
            goto EXIT;
        }
    }
   else
   {
       printf ("wrong enter mode\n");
       goto EXIT;
   }

EXIT:
//free

    if (di)
        CdxDiDestory(di, nDiType);

    if(pVideoInfo.pVeOpsS)
    {
        if(pMemops)
        {
            if(pVideoInfo.pInputVir0)
                CdcMemPfree(pMemops, pVideoInfo.pInputVir0, (void *)pVideoInfo.pVeOpsS, (void *)pVideoInfo.pVeOpsSelf);
            if(pVideoInfo.pInputVir1)
                CdcMemPfree(pMemops, pVideoInfo.pInputVir1, (void *)pVideoInfo.pVeOpsS, (void *)pVideoInfo.pVeOpsSelf);
            if(pVideoInfo.pOutputVir0)
                CdcMemPfree(pMemops, pVideoInfo.pOutputVir0, (void *)pVideoInfo.pVeOpsS, (void *)pVideoInfo.pVeOpsSelf);
        }
        CdcVeRelease(pVideoInfo.pVeOpsS,pVideoInfo.pVeOpsSelf);
    }

    if(pMemops)
        CdcMemClose(pMemops);

    if(FpInPic0)
    {
        fclose(FpInPic0);
        FpInPic0 = NULL;
    }
    if(FpInPic1)
    {
        fclose(FpInPic1);
        FpInPic1 = NULL;
    }
    if(FpOutPic)
    {
        fclose(FpOutPic);
        FpOutPic = NULL;
    }

    printf("====================================================================== \n");
    printf("Quit the deinterlacedemo program, goodbey!\n");
    printf("====================================================================== \n");
}
