/*
 * Copyright (c) 2008-2017 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : kun_media_player.c
 * Description : KunOS media player,output width weston alsa
 * History :
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <sys/select.h>

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

//#include "iniparserapi.h"

#include <cdx_config.h>
#include <cdx_log.h>
#include <xplayer.h>
#include <CdxTypes.h>
#include <memoryAdapter.h>
#include <deinterlace.h>
//typedef unsigned long uintptr_t ;

extern LayerCtrl* LayerCreate();
extern SoundCtrl* SoundDeviceCreate();
extern SubCtrl* SubtitleCreate();
extern Deinterlace* DeinterlaceCreate();
extern LayerCtrl* LayerCreate_Weston();

static const int STATUS_STOPPED   = 0;
static const int STATUS_PREPARING = 1;
static const int STATUS_PREPARED  = 2;
static const int STATUS_PLAYING   = 3;
static const int STATUS_PAUSED    = 4;
static const int STATUS_SEEKING   = 5;

typedef struct DemoPlayerContext
{
    struct sigaction sigint;
    XPlayer*        mAwPlayer;
    int             mPreStatus;
    int             mStatus;
    int             mSeekable;
    int             mError;
    pthread_mutex_t mMutex;
    int             mVideoFrameNum;
    int             bQuit;
}DemoPlayerContext;


//* define commands for user control.
typedef struct Command
{
    const char* strCommand;
    int         nCommandId;
    const char* strHelpMsg;
}Command;

#define COMMAND_HELP            0x1     //* show help message.
#define COMMAND_QUIT            0x2     //* quit this program.

#define COMMAND_SET_SOURCE      0x101   //* set url of media file.
#define COMMAND_PLAY            0x102   //* start playback.
#define COMMAND_PAUSE           0x103   //* pause the playback.
#define COMMAND_STOP            0x104   //* stop the playback.
#define COMMAND_SEEKTO          0x105   //* seek to posion, in unit of second.
#define COMMAND_SHOW_MEDIAINFO  0x106   //* show media information.
#define COMMAND_SHOW_DURATION   0x107   //* show media duration, in unit of second.
#define COMMAND_SHOW_POSITION   0x108   //* show current play position, in unit of second.
#define COMMAND_SWITCH_AUDIO    0x109   //* switch autio track.
#define COMMAND_SETSPEED        0x10a


static const Command commands[] =
{
    {"help",            COMMAND_HELP,               "show this help message."},
    {"quit",            COMMAND_QUIT,               "quit this program."},
    {"set url",         COMMAND_SET_SOURCE,
        "set url of the media, for example, set url: ~/testfile.mkv."},
    {"play",            COMMAND_PLAY,               "start playback."},
    {"pause",           COMMAND_PAUSE,              "pause the playback."},
    {"stop",            COMMAND_STOP,               "stop the playback."},
    {"set speed",       COMMAND_SETSPEED,      "stop the playback."},
    {"seek to",         COMMAND_SEEKTO,
        "seek to specific position to play, position is in unit of second, ex, seek to: 100."},
    {"show media info", COMMAND_SHOW_MEDIAINFO,  "show media information of the media file."},
    {"show duration",   COMMAND_SHOW_DURATION,   "show duration of the media file."},
    {"show position",   COMMAND_SHOW_POSITION,   "show current play position,in unit of second."},
    {"switch audio",    COMMAND_SWITCH_AUDIO,
        "switch audio to a track, for example, switch audio: 2, track is start counting from 0."},
    {NULL, 0, NULL}
};

static void showHelp(void)
{
    int     i;

    printf("\n");
    printf("******************************************************************************\n");
    printf("* This is a simple media player, when it is started, you can input commands to tell\n");
    printf("* what you want it to do.\n");
    printf("* Usage: \n");
    printf("*   # ./demoPlayer\n");
    printf("*   # set url: http://www.allwinner.com/ald/al3/testvideo1.mp4\n");
    printf("*   # show media info\n");
    printf("*   # play\n");
    printf("*   # pause\n");
    printf("*   # stop\n");
    printf("*\n");
    printf("* Command and it param is seperated by a colon, param is optional, as below:\n");
    printf("*     Command[: Param]\n");
    printf("*\n");
    printf("* here are the commands supported:\n");

    for(i=0; ; i++)
    {
        if(commands[i].strCommand == NULL)
            break;
        printf("*    %s:\n", commands[i].strCommand);
        printf("*\t\t%s\n",  commands[i].strHelpMsg);
    }
    printf("*\n");
    printf("********************************************************\n");
}

static int readCommand(char* strCommandLine, int nMaxLineSize)
{
    int            nMaxFds;
    fd_set         readFdSet;
    int            result;
    char*          p;
    unsigned int   nReadBytes;

    printf("\ndemoPlayer# ");
    fflush(stdout);

    nMaxFds    = 0;
    FD_ZERO(&readFdSet);
    FD_SET(STDIN_FILENO, &readFdSet);

    result = select(nMaxFds+1, &readFdSet, NULL, NULL, NULL);
    if(result > 0)
    {
        printf(" result: %d", result);
        if(FD_ISSET(STDIN_FILENO, &readFdSet))
        {
            nReadBytes = read(STDIN_FILENO, &strCommandLine[0], nMaxLineSize);
            printf("-===== nReadBytes: %d", nReadBytes);
            if(nReadBytes > 0)
            {
                p = strCommandLine;
                while(*p != 0)
                {
                    if(*p == 0xa)
                    {
                        *p = 0;
                        break;
                    }
                    p++;
                }
            }

            return 0;
        }
    }

    return -1;
}

static void formatString(char* strIn)
{
    char* ptrIn;
    char* ptrOut;
    int   len;
    int   i;

    if(strIn == NULL || (len=strlen(strIn)) == 0)
        return;

    ptrIn  = strIn;
    ptrOut = strIn;
    i      = 0;
    while(*ptrIn != '\0')
    {
        //* skip the beginning space or multiple space between words.
        if(*ptrIn != ' ' || (i!=0 && *(ptrOut-1)!=' '))
        {
            *ptrOut++ = *ptrIn++;
            i++;
        }
        else
            ptrIn++;
    }

    //* skip the space at the tail.
    if(i==0 || *(ptrOut-1) != ' ')
        *ptrOut = '\0';
    else
        *(ptrOut-1) = '\0';

    return;
}


//* return command id,
static int parseCommandLine(char* strCommandLine, int* pParam)
{
    char* strCommand;
    char* strParam;
    int   i;
    int   nCommandId;
    char  colon = ':';

    if(strCommandLine == NULL || strlen(strCommandLine) == 0)
    {
        return -1;
    }

    strCommand = strCommandLine;
    strParam   = strchr(strCommandLine, colon);
    if(strParam != NULL)
    {
        *strParam = '\0';
        strParam++;
    }

    formatString(strCommand);
    formatString(strParam);

    nCommandId = -1;
    for(i=0; commands[i].strCommand != NULL; i++)
    {
        if(strcmp(commands[i].strCommand, strCommand) == 0)
        {
            nCommandId = commands[i].nCommandId;
            break;
        }
    }

    if(commands[i].strCommand == NULL)
        return -1;

    switch(nCommandId)
    {
        case COMMAND_SET_SOURCE:
            if(strParam != NULL && strlen(strParam) > 0)
                *pParam = (uintptr_t)strParam;        //* pointer to the url.
            else
            {
                printf("no url specified.\n");
                nCommandId = -1;
            }
            break;

        case COMMAND_SEEKTO:
            if(strParam != NULL)
            {
                *pParam = (int)strtol(strParam, (char**)NULL, 10);  //* seek time in unit of second.
                if(errno == EINVAL || errno == ERANGE)
                {
                    printf("seek time is not valid.\n");
                    nCommandId = -1;
                }
            }
            else
            {
                printf("no seek time is specified.\n");
                nCommandId = -1;
            }
            break;

        case COMMAND_SETSPEED:
            if(strParam != NULL)
            {
                *pParam = (int)strtol(strParam, (char**)NULL, 10);  //* seek speed.
                if(errno == EINVAL || errno == ERANGE)
                {
                    printf("seek time is not valid.\n");
                    nCommandId = -1;
                }
            }
            else
            {
                printf("no seek time is specified.\n");
                nCommandId = -1;
            }
            break;

        case COMMAND_SWITCH_AUDIO:
            if(strParam != NULL)
            {
                *pParam = (int)strtol(strParam, (char**)NULL, 10);
                if(errno == EINVAL || errno == ERANGE)
                {
                    printf("audio stream index is not valid.\n");
                    nCommandId = -1;
                }
            }
            else
            {
                printf("no audio stream index is specified.\n");
                nCommandId = -1;
            }
            break;

        default:
            break;
    }


    return nCommandId;
}


//* a callback for awplayer.
int CallbackForAwPlayer(void* pUserData, int msg, int ext1, void* param)
{
    DemoPlayerContext* pDemoPlayer = (DemoPlayerContext*)pUserData;

    switch(msg)
    {
        case AWPLAYER_MEDIA_INFO:
        {
            switch(ext1)
            {
                case AW_MEDIA_INFO_NOT_SEEKABLE:
                {
                    pDemoPlayer->mSeekable = 0;
                    printf("info: media source is unseekable.\n");
                    break;
                }
                case AW_MEDIA_INFO_RENDERING_START:
                {
                    printf("info: start to show pictures.\n");
                    break;
                }
            }
            break;
        }

        case AWPLAYER_MEDIA_ERROR:
        {
            pthread_mutex_lock(&pDemoPlayer->mMutex);
            pDemoPlayer->mStatus = STATUS_STOPPED;
            pDemoPlayer->mPreStatus = STATUS_STOPPED;
            printf("error: open media source fail.\n");
            pthread_mutex_unlock(&pDemoPlayer->mMutex);
            pDemoPlayer->mError = 1;

            loge(" error : how to deal with it");
            break;
        }

        case AWPLAYER_MEDIA_PREPARED:
        {
            logd("info : preared");
            pDemoPlayer->mPreStatus = pDemoPlayer->mStatus;
            pDemoPlayer->mStatus = STATUS_PREPARED;
            printf("info: prepare ok.\n");
            break;
        }

        case AWPLAYER_MEDIA_BUFFERING_UPDATE:
        {
            int nTotalPercentage;
            int nBufferPercentage;
            int nLoadingPercentage;

            nTotalPercentage   = ((int*)param)[0];   //* read positon to total file size.
            nBufferPercentage  = ((int*)param)[1];   //* cache buffer fullness.
            nLoadingPercentage = ((int*)param)[2];   //* loading percentage to start play.

            int nBufferedFilePos;
            int nBufferFullness;

            break;
        }

        case AWPLAYER_MEDIA_PLAYBACK_COMPLETE:
        {
            //* stop the player.
            //* TODO
            break;
        }

        case AWPLAYER_MEDIA_SEEK_COMPLETE:
        {
            pthread_mutex_lock(&pDemoPlayer->mMutex);
            pDemoPlayer->mStatus = pDemoPlayer->mPreStatus;
            printf("info: seek ok.\n");
            pthread_mutex_unlock(&pDemoPlayer->mMutex);
            break;
        }
        case AWPLAYER_MEDIA_SET_VIDEO_SIZE:
        {
            int w, h;
            w   = ((int*)param)[0];   //* read positon to total file size.
            h  = ((int*)param)[1];   //* cache buffer fullness.
            printf("++++ video width: %d, height: %d \n", w, h);
            break;
        }

        default:
        {
            //printf("warning: unknown callback from AwPlayer.\n");
            break;
        }
    }

    return 0;
}



#define ProbeDataLen (2*1024)
enum CdxStreamStatus
{
    STREAM_IDLE,
    STREAM_CONNECTING,
    STREAM_SEEKING,
    STREAM_READING,
};

typedef struct StreamingSource StreamingSource;
struct StreamingSource
{
    CdxStreamT base;
    cdx_uint32 attribute;
    enum CdxStreamStatus status;
    cdx_int32 ioState;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    cdx_int32 forceStop;
    CdxStreamProbeDataT probeData;

    StreamingSource * mAwStreamingSource;
};

static CdxStreamProbeDataT *StreamingSourceGetProbeData(CdxStreamT *stream)
{
    StreamingSource *streamingSource = (StreamingSource *)stream;
    return &streamingSource->probeData;
}

cdx_int32 StreamingSourceGetIOState(CdxStreamT *stream)
{
	logd("StreamingSourceGetIOState");
   	StreamingSource *streamingSource = (StreamingSource *)stream;
    return streamingSource->ioState;
}

cdx_uint32 StreamingSourceAttribute(CdxStreamT *stream)
{
	//logd("StreamingSourceAttribute");
    StreamingSource *streamingSource = (StreamingSource *)stream;
    return streamingSource->attribute;
}

static cdx_int32 GetCacheState(StreamingSource *streamingSource,
                             struct StreamCacheStateS *cacheState)
{
    //memset(cacheState, 0, sizeof(struct StreamCacheStateS));
    //cacheState->nCacheCapacity = (cdx_int32)streamingSource->mAwStreamingSource->getCacheCapacity();
    //cacheState->nCacheSize = (cdx_int32)streamingSource->mAwStreamingSource->getCachedSize();
    return 0;
}

static cdx_int32 StreamingSourceForceStop(CdxStreamT *stream)
{
    /*StreamingSource *streamingSource = (StreamingSource *)stream;
    pthread_mutex_lock(&streamingSource->lock);
    streamingSource->forceStop = 1;
    while(streamingSource->status != STREAM_IDLE)
    {
        pthread_cond_wait(&streamingSource->cond, &streamingSource->lock);
    }
    pthread_mutex_unlock(&streamingSource->lock);*/
    return 0;
}

static FILE* inFd=NULL;
cdx_int32 StreamingSourceRead(CdxStreamT *stream, void *buf, cdx_uint32 len)
{
    StreamingSource *streamingSource = (StreamingSource *)stream;
    pthread_mutex_lock(&streamingSource->lock);

    streamingSource->status = STREAM_READING;
    pthread_mutex_unlock(&streamingSource->lock);

    char *data = (char *)buf;
    int ret = 0;

	//printf("read len=%d\n",len);

	if(NULL == inFd){
    	inFd = fopen("/tmp/a.mp4", "rb");
	}

	if(inFd != NULL)
    {
        static int cnt = 10;
		memset(data,0,len);
		ret = fread(data, 1, len, inFd);
		if(ret == 0){
			fseek(inFd,0,SEEK_SET);
			ret = fread(data, 1, len, inFd);
			printf("fseek to the begin,data=0x%x,ret=%d\n",data[0],ret);
			cnt = 10;
		}
		else{
			if(cnt-- >0)
				printf("len=%d,buf[0]=0x%x\n",len,data[0]);
		}
        //fclose(inFd);
    }else{
		printf("open/tmp/a.mp4 failed\n");
	}

#if SAVE_FILE
    if(ret > 0)
    {
        if (file)
        {
            fwrite(data, 1, ret, file);
            sync();
        }
        else
        {
            CDX_LOGW("save file = NULL");
        }
    }
#endif

	if(ret == 0){
		printf("StreamingSourceRead return 0,eos\n");
	}

	return ret;
}

cdx_int32 StreamingSourceControl(CdxStreamT *stream, cdx_int32 cmd, void *param)
{
	logd("StreamingSourceControl");
	#if 0
    StreamingSource *streamingSource = (StreamingSource *)stream;
    switch(cmd)
    {
        case STREAM_CMD_GET_CACHESTATE:
            return GetCacheState(streamingSource, (struct StreamCacheStateS *)param);
        case STREAM_CMD_SET_FORCESTOP:
            return StreamingSourceForceStop(stream);
        default:
            CDX_LOGW("not implement cmd(%d)", cmd);
            break;
    }
    #endif

    return 0;
}


cdx_int32 StreamingSourceClose(CdxStreamT *stream)
{
	logd("StreamingSourceClose");
	if(inFd){
		fclose(inFd);
		inFd = NULL;
	}
	#if 0
    StreamingSource *streamingSource = (StreamingSource *)stream;
    StreamingSourceForceStop(stream);
    if(streamingSource->probeData.buf)
    {
        free(streamingSource->probeData.buf);
    }
    streamingSource->mAwStreamingSource->stop();
    streamingSource->mAwStreamingSource.clear();
#if SAVE_FILE
    if (file)
    {
        fclose(file);
        file = NULL;
    }
#endif
    pthread_mutex_destroy(&streamingSource->lock);
    pthread_cond_destroy(&streamingSource->cond);
    free(streamingSource);
    #endif
    return 0;
}



static cdx_int32 PrepareThread(CdxStreamT *stream)
{
	logd("PrepareThread");
	#if 1
    CDX_LOGI("PrepareThread start");
    StreamingSource *streamingSource = (StreamingSource *)stream;
    pthread_mutex_lock(&streamingSource->lock);

    streamingSource->status = STREAM_CONNECTING;
    pthread_mutex_unlock(&streamingSource->lock);

	#if 1
    FILE* inFd = fopen("/tmp/a.mp4", "rb");
    if(inFd != NULL)
    {
        printf("open /tmp/a.mp4 success\n");
        fread(streamingSource->probeData.buf, 1, ProbeDataLen, inFd);
        fclose(inFd);
    }else{
		printf("open /tmp/a.mp4 failed\n");
	}

	#endif

    streamingSource->ioState = CDX_IO_STATE_OK;
    pthread_mutex_lock(&streamingSource->lock);
    streamingSource->status = STREAM_IDLE;
    pthread_mutex_unlock(&streamingSource->lock);
    pthread_cond_signal(&streamingSource->cond);
    CDX_LOGI("PrepareThread succeed");
    return 0;
_exit:
    streamingSource->ioState = CDX_IO_STATE_ERROR;
    pthread_mutex_lock(&streamingSource->lock);
    streamingSource->status = STREAM_IDLE;
    pthread_mutex_unlock(&streamingSource->lock);
    pthread_cond_signal(&streamingSource->cond);
    CDX_LOGI("PrepareThread end");
	#else
	    CDX_LOGI("PrepareThread start");
    StreamingSource *streamingSource = (StreamingSource *)stream;
    pthread_mutex_lock(&streamingSource->lock);
    if(streamingSource->forceStop)
    {
        pthread_mutex_unlock(&streamingSource->lock);
        return -1;
    }
    streamingSource->status = STREAM_CONNECTING;
    pthread_mutex_unlock(&streamingSource->lock);

    cdx_uint32 ret;
    while((ret = streamingSource->mAwStreamingSource->getCachedSize()) < ProbeDataLen)
    {
        //CDX_LOGI("getCachedSize(%d)", ret);
        if(streamingSource->forceStop || streamingSource->mAwStreamingSource->isReceiveEOS())
        {
            goto _exit;
        }
        usleep(20000);
    }
    streamingSource->mAwStreamingSource->copyAccessData(streamingSource->probeData.buf,
                                         ProbeDataLen);
    streamingSource->ioState = CDX_IO_STATE_OK;
    pthread_mutex_lock(&streamingSource->lock);
    streamingSource->status = STREAM_IDLE;
    pthread_mutex_unlock(&streamingSource->lock);
    pthread_cond_signal(&streamingSource->cond);
    CDX_LOGI("PrepareThread succeed");
    return 0;
_exit:
    streamingSource->ioState = CDX_IO_STATE_ERROR;
    pthread_mutex_lock(&streamingSource->lock);
    streamingSource->status = STREAM_IDLE;
    pthread_mutex_unlock(&streamingSource->lock);
    pthread_cond_signal(&streamingSource->cond);
    CDX_LOGI("PrepareThread end");
    #endif
    return 0;
}


CdxStreamT *StreamingSourceOpen(char ** source,
                                size_t numBuffer, size_t bufferSize)
{
    StreamingSource *streamingSource = (StreamingSource *)malloc(sizeof(StreamingSource));
    if(!streamingSource)
    {
        CDX_LOGE("malloc fail!");
        return NULL;
    }
    memset(streamingSource, 0x00, sizeof(StreamingSource));

    int ret = pthread_mutex_init(&streamingSource->lock, NULL);
    CDX_CHECK(!ret);
    ret = pthread_cond_init(&streamingSource->cond, NULL);
    CDX_CHECK(!ret);

    streamingSource->probeData.len = ProbeDataLen;
    streamingSource->probeData.buf = (cdx_char *)malloc(ProbeDataLen);
    CDX_CHECK(streamingSource->probeData.buf);


    //streamingSource->mAwStreamingSource = new awStreamingSource(source);
    //CDX_CHECK(streamingSource->mAwStreamingSource);
    //streamingSource->mAwStreamingSource->start(numBuffer, bufferSize);

    streamingSource->ioState = CDX_IO_STATE_INVALID;
    streamingSource->attribute = CDX_STREAM_FLAG_NET;
    streamingSource->status = STREAM_IDLE;
    streamingSource->base.ops = (struct CdxStreamOpsS *)malloc(sizeof(struct CdxStreamOpsS));
    CDX_CHECK(streamingSource->base.ops);
    memset(streamingSource->base.ops, 0, sizeof(struct CdxStreamOpsS));
    streamingSource->base.ops->getProbeData = StreamingSourceGetProbeData;
    streamingSource->base.ops->read = StreamingSourceRead;
    streamingSource->base.ops->close = StreamingSourceClose;
    streamingSource->base.ops->getIOState = StreamingSourceGetIOState;
    streamingSource->base.ops->attribute = StreamingSourceAttribute;
    streamingSource->base.ops->control = StreamingSourceControl;
    streamingSource->base.ops->connect = PrepareThread;

#if SAVE_FILE
    file = fopen("/data/camera/save.dat", "wb+");
    if (!file)
    {
        loge("open file failure errno(%d)", errno);
    }
#endif
    return &streamingSource->base;
}



DemoPlayerContext demoPlayer;

static void
signal_int(int signum)
{
    printf("sigaction signum %d\n",signum);
    demoPlayer.bQuit = 1;
}

//* the main method.
int main(int argc, char** argv)
{
    printf("compile time %s %s\n",__DATE__,__TIME__);
    int  nCommandId;
    int  nCommandParam;
    char strCommandLine[1024];

    demoPlayer.sigint.sa_handler = signal_int;
    sigemptyset(&demoPlayer.sigint.sa_mask);
    demoPlayer.sigint.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &demoPlayer.sigint, NULL);

    CEDARX_UNUSE(argc);
    CEDARX_UNUSE(argv);

    printf("\n");
    printf("*****************************************************************************\n");
    printf("* This program implements a simple player, \n");
    printf("* you can type commands to control the player. \n");
    printf("* To show what commands supported, type 'help'.\n");
    printf("* Inplemented by Allwinner ALD-AL3 department.\n");
    printf("****************************************************************************\n");

    //* create a player.
    memset(&demoPlayer, 0, sizeof(DemoPlayerContext));
    pthread_mutex_init(&demoPlayer.mMutex, NULL);
    demoPlayer.mAwPlayer = XPlayerCreate();
    if(demoPlayer.mAwPlayer == NULL)
    {
        printf("can not create AwPlayer, quit.\n");
        exit(-1);
    }

    //* set callback to player.
    XPlayerSetNotifyCallback(demoPlayer.mAwPlayer, CallbackForAwPlayer, (void*)&demoPlayer);

    //* check if the player work.
    if(XPlayerInitCheck(demoPlayer.mAwPlayer) != 0)
    {
        printf("initCheck of the player fail, quit.\n");
        XPlayerDestroy(demoPlayer.mAwPlayer);
        demoPlayer.mAwPlayer = NULL;
        exit(-1);
    }

    //LayerCtrl* layer = LayerCreate();
    //LayerCtrl* layer = LayerCreate_DE();
    LayerCtrl* layer = LayerCreate_Weston(&demoPlayer.bQuit);
    SoundCtrl* sound = SoundDeviceCreate();
    SubCtrl*   sub   = SubtitleCreate();
    Deinterlace* di = DeinterlaceCreate();

    XPlayerSetAudioSink(demoPlayer.mAwPlayer, (void*)sound);
    XPlayerSetVideoSurfaceTexture(demoPlayer.mAwPlayer, (void*)layer);
    XPlayerSetSubCtrl(demoPlayer.mAwPlayer, sub);
    XPlayerSetDeinterlace(demoPlayer.mAwPlayer, di);

    //* read, parse and process command from user.
    demoPlayer.bQuit = 0;
    while(!demoPlayer.bQuit)
    {
        printf("=== readCommand");
        if(demoPlayer.mError)
        {
            XPlayerReset(demoPlayer.mAwPlayer);
            demoPlayer.mError = 0;

            demoPlayer.mPreStatus = STATUS_PREPARED;
            demoPlayer.mStatus    = STATUS_STOPPED;
        }

        //* read command from stdin.
        if(readCommand(strCommandLine, sizeof(strCommandLine)) == 0)
        {
            //* parse command.
            nCommandParam = 0;
            nCommandId = parseCommandLine(strCommandLine, &nCommandParam);

            //* process command.
            switch(nCommandId)
            {
                case COMMAND_HELP:
                {
                    showHelp();
                    break;
                }

                case COMMAND_QUIT:
                {
                    demoPlayer.bQuit = 1;
                    break;
                }

                case COMMAND_SET_SOURCE :   //* set url of media file.
               {
                    char* pUrl;
                    pUrl = (char*)(uintptr_t)nCommandParam;

                    demoPlayer.mSeekable = 0;

				#if 1
                    //* set url to the AwPlayer.
                    if(XPlayerSetDataSourceUrl(demoPlayer.mAwPlayer,
                                 (const char*)pUrl, NULL, NULL) != 0)
                    {
                        printf("error:\n");
                        printf("    AwPlayer::setDataSource() return fail.\n");
                        break;
                    }
                     printf("setDataSource end\n");
				#else
				    int ret;
				    logd("setDataSource(IStreamSource)");

				    unsigned int numBuffer, bufferSize;
				    const char *suffix = "";

			        numBuffer = 16;
			        bufferSize = 4*1024;

					char ** source = NULL;
				    CdxStreamT *stream = StreamingSourceOpen(source, numBuffer, bufferSize);
				    if(stream == NULL)
				    {
				        loge("StreamingSourceOpen fail!");
				        return -1;
				    }

				    char str[128];
				    sprintf(str, "customer://%p%s",stream, suffix);
				    printf("%s\n",str);

					XPlayerConfig_t     mConfigInfo;
					mConfigInfo.appType = APP_STREAMING;
   					XPlayerConfig(demoPlayer.mAwPlayer, &mConfigInfo);

				if(0 != XPlayerSetDataSourceStream(demoPlayer.mAwPlayer, str)){
					printf("XPlayerSetDataSourceStream failed\n");
				}
				 printf("setDataSource end\n");
				#endif
                    //* start preparing.
                    pthread_mutex_lock(&demoPlayer.mMutex);
                    if(XPlayerPrepareAsync(demoPlayer.mAwPlayer) != 0)
                    {
                        printf("error:\n");
                        printf("    AwPlayer::prepareAsync() return fail.\n");
                        pthread_mutex_unlock(&demoPlayer.mMutex);
                        break;
                    }


                    demoPlayer.mPreStatus = STATUS_STOPPED;
                    demoPlayer.mStatus    = STATUS_PREPARING;
                    printf("preparing...\n");
                    pthread_mutex_unlock(&demoPlayer.mMutex);

                    break;
                }

                case COMMAND_PLAY:   //* start playback.
                {
                    if(XPlayerStart(demoPlayer.mAwPlayer) != 0)
                    {
                        printf("error:\n");
                        printf("    AwPlayer::start() return fail.\n");
                        break;
                    }
                    demoPlayer.mPreStatus = demoPlayer.mStatus;
                    demoPlayer.mStatus    = STATUS_PLAYING;
                    printf("playing.\n");
                    break;
                }

                case COMMAND_PAUSE:   //* pause the playback.
                {
                    if(XPlayerPause(demoPlayer.mAwPlayer) != 0)
                    {
                        printf("error:\n");
                        printf("    AwPlayer::pause() return fail.\n");
                        break;
                    }
                    demoPlayer.mPreStatus = demoPlayer.mStatus;
                    demoPlayer.mStatus    = STATUS_PAUSED;
                    printf("paused.\n");

                    break;
                }

                case COMMAND_STOP:   //* stop the playback.
                {
                    if(XPlayerReset(demoPlayer.mAwPlayer) != 0)
                    {
                        printf("error:\n");
                        printf("    AwPlayer::reset() return fail.\n");
                        break;
                    }
                    demoPlayer.mPreStatus = demoPlayer.mStatus;
                    demoPlayer.mStatus    = STATUS_STOPPED;
                    printf("stopped.\n");
                    break;
                }

                case COMMAND_SEEKTO:   //* seek to posion, in unit of second.
                {
                    int nSeekTimeMs;
                    int nDuration;
                    nSeekTimeMs = nCommandParam*1000;

                    if(XPlayerGetDuration(demoPlayer.mAwPlayer, &nDuration) != 0)
                    {
                        printf("getDuration fail, unable to seek!\n");
                        break;
                    }

                    if(nSeekTimeMs > nDuration)
                    {
                        printf("seek time out of range, media duration = %u seconds.\n",
                                nDuration/1000);
                        break;
                    }

                    if(demoPlayer.mSeekable == 0)
                    {
                        printf("media source is unseekable.\n");
                        break;
                    }

                    //* the player will keep the pauded status
                    //* and pause the playback after seek finish.
                    pthread_mutex_lock(&demoPlayer.mMutex);
                    XPlayerSeekTo(demoPlayer.mAwPlayer, nSeekTimeMs, 0);
                    if(demoPlayer.mStatus != STATUS_SEEKING)
                        demoPlayer.mPreStatus = demoPlayer.mStatus;
                    demoPlayer.mStatus = STATUS_SEEKING;
                    pthread_mutex_unlock(&demoPlayer.mMutex);
                    break;
                }

                case COMMAND_SETSPEED:   //* set speed
                {
                    int nSpeed;
                    nSpeed = nCommandParam;

                    if(demoPlayer.mSeekable == 0)
                    {
                        printf("media source is unseekable.\n");
                        break;
                    }

                    XPlayerSetSpeed(demoPlayer.mAwPlayer, nSpeed);
                    logd("===  set speed end");
                    break;
                }

                case COMMAND_SHOW_MEDIAINFO:   //* show media information.
                {
                    printf("show media information.\n");
                    MediaInfo* mMediaInfo = XPlayerGetMediaInfo(demoPlayer.mAwPlayer);
                    if (mMediaInfo == NULL)
                    {
                        printf("get mediainfo fail!\n");
                        break;
                    }
                    VideoStreamInfo *videoMediaInfo = mMediaInfo->pVideoStreamInfo;
                    if (videoMediaInfo != NULL)
                    {
                        printf("video nWidth %d.\n",videoMediaInfo->nWidth);
                        printf("video height %d.\n",videoMediaInfo->nHeight);
                    }

                    break;
                }

                case COMMAND_SHOW_DURATION:   //* show media duration, in unit of second.
                {
                    int nDuration = 0;
                    if(XPlayerGetDuration(demoPlayer.mAwPlayer, &nDuration) == 0)
                        printf("media duration = %u seconds.\n", nDuration/1000);
                    else
                        printf("fail to get media duration.\n");
                    break;
                }

                case COMMAND_SHOW_POSITION:   //* show current play position, in unit of second.
                {
                    int nPosition = 0;
                    if(XPlayerGetCurrentPosition(demoPlayer.mAwPlayer, &nPosition) == 0)
                        printf("current position = %u seconds.\n", nPosition/1000);
                    else
                        printf("fail to get pisition.\n");
                    break;
                }

                case COMMAND_SWITCH_AUDIO:   //* switch autio track.
                {
                    int nAudioStreamIndex;
                    nAudioStreamIndex = nCommandParam;
                    printf("switch audio to the %dth track.\n", nAudioStreamIndex);
                    //* TODO
                    break;
                }

                default:
                {
                    if(strlen(strCommandLine) > 0)
                        printf("invalid command.\n");
                    break;
                }
            }
        }
    }

    printf("destroy AwPlayer.\n");

    if(demoPlayer.mAwPlayer != NULL)
    {
        XPlayerDestroy(demoPlayer.mAwPlayer);
        demoPlayer.mAwPlayer = NULL;
    }

    printf("destroy AwPlayer 1.\n");
    pthread_mutex_destroy(&demoPlayer.mMutex);

    printf("\n");
    printf("********************************************************************\n");
    printf("* Quit the program, goodbye!\n");
    printf("********************************************************************\n");
    printf("\n");

    return 0;
}

