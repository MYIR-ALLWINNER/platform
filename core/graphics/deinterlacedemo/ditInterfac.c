#include "ditInterface.h"
#include "deinterlace_new.h"
#include "cdx_log.h"

Deinterlace* CdxDiCreate(int type)
{
    switch(type)
    {
        case DI_TYPE_DI300:
            return DI300Create();
        case DI_TYPE_DI200:
            //return DI200Create();
            return NULL;
        default:
            return NULL;
    }
}

void CdxDiDestory(Deinterlace* pSelf, int type)
{
    switch(type)
    {
        case DI_TYPE_DI300:
            DI300Destory(pSelf);
            break;
        case DI_TYPE_DI200:
            //DI200Destory(pSelf);
            break;
        default:
            return;
    }

}

int CdxDiSetParameter(Deinterlace* di, DIParam pParam)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->setParamter);
    return di->ops->setParamter(di, pParam);
}

int CdxDiReset(Deinterlace* di)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->reset);
    return di->ops->reset(di);
}

int CdxDiGetInfo(Deinterlace* di, DeInterlaceInfo* pInfo)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->getInfo);
    return di->ops->getInfo(di, pInfo);
}

int CdxDiProcess(Deinterlace* di, InputFrame* input, OutputFrame* output)
{
    CDX_CHECK(di);
    CDX_CHECK(di->ops);
    CDX_CHECK(di->ops->process);
    return di->ops->process(di, input, output);
}
