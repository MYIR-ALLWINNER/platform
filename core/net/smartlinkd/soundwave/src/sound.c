#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <alsa/asoundlib.h>
#include <time.h>
#include "adt.h"
#include <uci.h>
#include "aw_smartlinkd_connect.h"
//#define UCI_CONFIG_FILE "/etc/config/smartlinkd"
#define RECORDFILE "/tmp/smrecord.pcm"

#ifdef __cplusplus
extern "C" {
#endif
const int gsample_rate = 16000;

static int f_send_finished = 0;
#ifdef RECORDFILE
int recordfd;
#endif
/*wav Audio head format */
typedef struct _wave_pcm_hdr
{
    char        riff[4];                // = "RIFF"
    int         size_8;                 // = FileSize - 8
    char        wave[4];                // = "WAVE"
    char        fmt[4];                 // = "fmt "
    int         fmt_size;               // = The size of the next structure : 16

    short int   format_tag;             // = PCM : 1
    short int   channels;               // = channels num : 1
    int         samples_per_sec;        // = Sampling Rate : 6000 | 8000 | 11025 | 16000
    int         avg_bytes_per_sec;      // = The number if bytes per second : samples_per_sec * bits_per_sample / 8
    short int   block_align;            // = Number if bytes per sample point : wBitsPerSample / 8
    short int   bits_per_sample;        // = Quantization bit number: 8 | 16

    char        data[4];                // = "data";
    int         data_size;              // = Pure data length : FileSize - 44
} wave_pcm_hdr;

/* default wav audio head data */
wave_pcm_hdr default_wav_hdr =
{
    { 'R', 'I', 'F', 'F' },
    0,
    {'W', 'A', 'V', 'E'},
    {'f', 'm', 't', ' '},
    16,
    1,
    1,
    16000,
    32000,
    2,
    16,
    {'d', 'a', 't', 'a'},
    0
};

int signal_exit=0;
void localsigroutine(int dunno){
	LOGE("sig: %d coming!\n",dunno);
	switch(dunno){
		case SIGINT:
		case SIGQUIT:
		case SIGHUP:
		{
            signal_exit =1;
			//exit(0);
            break;
		}
		case SIGPIPE:
		{
			//When the client is closed after start scaning and parsing,
			//this signal will come, ignore it!
			LOGD("do nothings for PIPE signal\n");
            break;
		}
	}
}

snd_pcm_t* alsa_init(char *device, int sample_rate){
    int err;

    LOGD("device: %s sample_rate is %d\n", device, sample_rate);

    snd_pcm_t *capture_handle = NULL;
    snd_pcm_hw_params_t *hw_params = NULL;

    if((err = snd_pcm_open(&capture_handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0){
        LOGE("cannot open audio device (%s)\n",  snd_strerror(err));
        return NULL;
    }

    if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        LOGE("cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
        goto fail;
    }

    if((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
        LOGE("cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
        goto fail;
    }

    if((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        LOGE("cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
        goto fail;
    }

    if((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
        LOGE("cannot allocate hardware parameter structure (%s)\n",snd_strerror(err));
        goto fail;
    }

    if((err = snd_pcm_hw_params_set_rate(capture_handle, hw_params, sample_rate, 0)) < 0) {
        LOGE("cannot set sample rate (%s)\n", snd_strerror(err));
        goto fail;
    }

    if((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, 1)) <0) {
        LOGE("cannot set channel count (%s)\n", snd_strerror(err));
        goto fail;
    }

    if((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
        LOGE("cannot set parameters (%s)\n", snd_strerror(err));
        goto fail;
    }

    snd_pcm_hw_params_free(hw_params);

    return capture_handle;
fail:
    LOGE("close dev\n");
    snd_pcm_close(capture_handle);
    LOGE("Fail\n");
    return NULL;
}
void alsa_release(snd_pcm_t* capture_handle){
    snd_pcm_close(capture_handle);
}
int alsa_record(snd_pcm_t* capture_handle, char *buf, int frames)
{
	int bufbyte;
    int r;
    r = snd_pcm_readi(capture_handle, buf, frames);
    if (r == -EPIPE) {
        LOGE("overrun occurred\n");
        snd_pcm_prepare(capture_handle);
    } else if (r < 0) {
        LOGE("error from read: %s\n", snd_strerror(r));
    } else if (r > 0) {
        // 2 bytes/sample, 1 or 2 channels, r: frames
    }
#ifdef RECORDFILE
	bufbyte=(default_wav_hdr.fmt_size/8)*(default_wav_hdr.channels)*frames;
	if (write(recordfd,buf,bufbyte) != bufbyte) {
	    fprintf(stderr, "Error SNDWAV_Record[write]/n");
	    exit(-1);
	}
#endif
    return r;
}

void usage(){
    printf("---------------------------------------------------------------------------------\n");
	printf("Available options:\n");
	printf("-D,\tselect the device, default 'default'.\n");
	printf("-t,\tset timeouts(s),default 60.\n");
	printf("-f,\tset frequency type(0-LOW,1-MIDDLE,2-HIGH),default 0\n");
	printf("-r,\tset simple rate,default 16000\n");
	printf("-s,\tWhether to connect smartlinkd server(yes or no),default yes\n");
	printf("-h,\tthis help\n\n");

	printf("example:\n");
	printf("smartlinkd_adt -f 2 -r 44100 -s \"no\" \n");
	printf("---------------------------------------------------------------------------------\n");
}

int thread_handle(char *buf, int length)
{
    int ret;

    ret = strcmp(buf,"OK");
    if(0 == ret)
    {
	f_send_finished = 1;
	printf("thread_handle of sunxiadt: recieve OK from server!\n");
	return THREAD_EXIT;
    }
    return THREAD_CONTINUE;
}
static void decoder_info(config_decoder_t config_decoder)
{
	printf("---------------------------------------------------------------------------------\n");
	switch(config_decoder.freq_type)
	{
		case 0:printf("frequency type:    \tFREQ_TYPE_LOW\n");break;
		case 1:printf("frequency type:    \tFREQ_TYPE_MIDDLE\n");break;
		case 2:printf("frequency type:    \tFREQ_TYPE_HIGH\n");break;
		default:printf("frequency type:    \tERROR\n");break;
	}
	printf("sample rate:     \t%d\n",config_decoder.sample_rate);
	printf("max string length:\t%d\n",config_decoder.max_strlen);
	printf("grounp symbol num:\t%d\n",config_decoder.group_symbol_num);
	if(config_decoder.error_correct==1){
		printf("Error correction num:\t%d\n",config_decoder.error_correct_num);
	}
	printf("---------------------------------------------------------------------------------\n");
}
/*
static int load_config(const char *section,const char *option,int value)
{
	struct uci_context *ctx =NULL;
	struct uci_package *pkg=NULL;
	struct uci_element *e=NULL ;

	char cmd[100];
	ctx = uci_alloc_context();	//Allocate a new uci context
	if(UCI_OK !=uci_load(ctx,UCI_CONFIG_FILE,&pkg)){
		printf("opps!! uci_load error");
		goto cleanup;
	}

	uci_foreach_element(&pkg->sections, e) // loop through a list of uci elements
	{
		struct uci_section *s = uci_to_section(e);
		if(!strcmp(section,s->e.name)){
			struct uci_option *o = uci_lookup_option(ctx, s, option);  //look up an option

			if ((NULL != o) && (UCI_TYPE_LIST == o->type)){
				struct uci_element *e=NULL;
				uci_foreach_element(&o->v.list, e)
				{
					if(value >= 0)
						sprintf(cmd,"%s %d",e->name,value);
					else
						sprintf(cmd,"%s",e->name);
					system(cmd);
				}

			}
		}
	}
	uci_unload(ctx, pkg);
cleanup:
	uci_free_context(ctx);
	ctx = NULL;
}
*/
int main(int argc, char** argv){

    int timeouts = 60;
    int ret_dec;
	int feq_type=FREQ_TYPE_LOW;
    int items, bsize,i;
    void* decoder;
    short* buffer;
    char out[ADT_STR];
    char device[20];
    int rate = 16000;
	char server_state[5]="yes";

    strncpy(device,"default",sizeof(device));

	signal(SIGHUP,localsigroutine);
	signal(SIGQUIT,localsigroutine);
	signal(SIGINT,localsigroutine);
	signal(SIGPIPE,localsigroutine);

    for (;;) {
        int c = getopt(argc, argv, "hD:t:r:f:s:");
        if (c < 0) {
            break;
        }
        switch (c) {
            case 'D':
                strncpy(device,optarg,sizeof(device));
                break;
            case 't':
                timeouts = atoi(optarg);
                break;
			case 'f':
				feq_type = atoi(optarg);
				break;
            case 'r':
                rate = atoi(optarg);
                break;
			case 's':
                strncpy(server_state,optarg,sizeof(server_state));
                break;
            case 'h':
                usage();
                return 0;
            default:
                usage();
                return 0;
        }
    }
    struct _cmd c;
    struct _info *info = &c.info;
    memset(info,0,sizeof(struct _info));
    info->protocol = AW_SMARTLINKD_PROTO_FAIL;
    /* ADT param */
    ret_dec = RET_DEC_NORMAL;
    config_decoder_t config_decoder;
    config_decoder.max_strlen = ADT_STR - 1;
    config_decoder.freq_type = feq_type;
    config_decoder.sample_rate = rate;
    config_decoder.group_symbol_num = 10;
    config_decoder.error_correct = 1;
    config_decoder.error_correct_num = 4;
	decoder_info(config_decoder);
	//load_config("record","record_on",-1);   //open record device
#ifdef RECORDFILE
    if ((recordfd = open(RECORDFILE, O_WRONLY | O_CREAT, 0644)) == -1) {
        fprintf(stderr, "Error open: [%s]/n", RECORDFILE);
        return -1;
    }
#endif
    /* create decoder handle */
    decoder = decoder_create(& config_decoder);
    if(decoder == NULL)
    {
        LOGE("allocate handle error !\n");
        return -1;
    }
    /* get buffer size and allocate buffer */
    bsize = decoder_getbsize(decoder);
    buffer = (short*)malloc(sizeof(short)*bsize);
    if(buffer == NULL)
    {
        LOGE("allocate buffer error !\n");
        goto finish;
    }
    LOGD("bsize: %d\n",bsize);
    snd_pcm_t *capture_handle = alsa_init(device, config_decoder.sample_rate);
    if(capture_handle == NULL)
        goto finish;

    timeouts = timeouts * config_decoder.sample_rate/bsize;

    /* decoding loop */
    while(signal_exit == 0 && ret_dec == RET_DEC_NORMAL && timeouts--)
    {
        /*samples get from ADC */

        items = alsa_record(capture_handle,(char*)buffer,bsize);
        if(items < 0 && items != -EPIPE){

        }

        /*padding with zeros if items is less than bsize samples*/
        for(i = items; i< bsize; i++)
        {
            buffer[i] = 0;
        }
        /* input the pcm data to decoder */
        ret_dec = decoder_fedpcm(decoder, buffer);
    }
    /* check if we can get the output string */
    if(ret_dec != RET_DEC_ERROR)
    {
        /* get the decoder output */
        ret_dec = decoder_getstr(decoder, out);
        LOGD("ret_dec of get the decoder output: %d\n", ret_dec);
        
        if(ret_dec == RET_DEC_NORMAL)
        {
            /* this is the final decoding output */
            LOGD("************recv outchar: %s ****************\n", out);

	    info->protocol = AW_SMARTLINKD_PROTO_ADT;
            strncpy(info->adt_str,out,sizeof(info->adt_str));
        }else
        {
            /* decoding have not done, so nothing output */
            LOGD("decoder output nothing! \n");
        }
    }else
    {
        LOGE("decoder error \n");
    }

    alsa_release(capture_handle);
	//load_config("record","record_off",-1);

finish:
    /* free handle */
    decoder_destroy(decoder);

	if(signal_exit == 1){
        printf("signal exit!!\n");
        exit(0);
    }
	if(strcmp(server_state,"yes")==0){
		aw_smartlinkd_init(0,thread_handle);
		if(aw_easysetupfinish(&c)!= sizeof(struct _cmd)){
			printf("main of adt: failed to send the information!\n");
		}
		while(f_send_finished == 0){
			sleep(1);
		}
	}
    return 0;
}
