
#ifdef MOVIETEXTURE_FFMPEG
// http://dranger.com/ffmpeg/tutorial01.html
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#define inline   //someone in ffmpeg put a bit of cpp in their headers, this seemed to fix it
//#include "libavutil/avstring.h"
//#include "libavutil/colorspace.h"
//#include "libavutil/mathematics.h"
//#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
//#include "libavutil/pixfmt.h"
//#include "libavutil/dict.h"
//#include "libavutil/parseutils.h"
//#include "libavutil/samplefmt.h"
//#include "libavutil/avassert.h"
//#include "libavutil/time.h"
#include "libavformat/avformat.h"
//#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
//#include "libswresample/swresample.h"
//#include "libavutil/opt.h"
//#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

#include "internal.h"
#include "Vector.h"
#include "../opengl/textures.h"
void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname);
// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif


//from ffmpeg tutorial01.c
//save to .ppm imge format for debugging, which gimp will read but only if RGB24 / nchan==3
void SaveFrame(AVFrame *pFrame, int width, int height, int nchan, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  // Open file
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*nchan, pFile);
  
  // Close file
  fclose(pFile);
}
float fwroundf(float val){
	//some math.h don't have round. here's dug9 version.
	int ival;
	float singv, valv;
	singv = val < 0.0f ? -1.0f : 1.0f;
	valv = fabsf(val);
	valv = valv + .5f;
	ival = (int)valv;
	valv = (float)ival;
	valv *= singv;
	return valv;
}
//our opaque pointer is a struct:
struct fw_movietexture {
	//AVFormatContext *pFormatCtx; //don't need to save for decode-on-load
	//AVCodecContext *pVideoCodecCtx; //don't need to save for decode-on-load
	//video and audio section:
	double duration;
	//video section:
	int width,height,nchan,nframes,fps;
	unsigned char **frames;
	//audio section:
	unsigned char *audio_buf;
	int audio_buf_size;
	int channels;
	int freq;
	int bits_per_channel;
};
int movie_load_from_file(char *fname, void **opaque){
	static int once = 0;
	struct fw_movietexture fw_movie;
	AVFormatContext *pFormatCtx;
	int i, videoStream, audioStream;
	AVCodecContext *pCodecCtxOrig;
	AVCodecContext *pCodecCtx;
	AVCodecContext  *aCodecCtxOrig;
	AVCodecContext  *aCodecCtx;
	AVCodec         *aCodec;
	AVFrame			*aFrame;
	AVFrame			*aFrameB;
	//uint8_t *audio_pkt_data = NULL;
	//int audio_pkt_size = 0;
	unsigned int audio_buf_size;
	unsigned int audio_buf_index;
	uint8_t * audio_buf;
	SwrContext *swr; 
	int audio_resample_target_fmt;
	int do_audio_resample;
	struct SwsContext *sws_ctx;
	int frameFinished;
	AVPacket packet;
	AVFrame *pFrame;
	AVCodec *pCodec;
	Stack *fw_framequeue;
	AVFrame *pFrameRGB;
	int nchan;
	uint8_t *buffer;



	*opaque = NULL;
	//initialize ffmpeg libs once per process
	if(once == 0){
		av_register_all(); //register all codecs - will filter in the future for patent non-expiry
		once = 1;
	}
	pFormatCtx = NULL;

	// Open video file
	if(avformat_open_input(&pFormatCtx, fname, NULL, NULL)!=0)
		return -1; // Couldn't open file

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return -1; // Couldn't find stream information

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx, 0, fname, 0);
	//fw_movie.pFormatCtx = pFormatCtx;

	pCodecCtxOrig = NULL;
	pCodecCtx = NULL;

	// Find the first video stream
	videoStream=-1;
	audioStream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++){
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO && videoStream < 0) {
			videoStream=i;
		}
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO && audioStream < 0) {
			audioStream=i;
		}
	}
	if(videoStream==-1 && audioStream == -1)
		return -1; // Didn't find either video or audio stream



	//audio and video prep
	memset(&fw_movie,0,sizeof(struct fw_movietexture));
	fw_movie.frames = NULL;
	fw_movie.nframes = 0;
	fw_movie.audio_buf = NULL;
	fw_movie.audio_buf_size = 0;

	//audio function-scope variables
	aCodecCtxOrig = NULL;
	aCodecCtx = NULL;
	aCodec = NULL;
	aFrame = NULL;
	aFrameB = NULL;
	//uint8_t *audio_pkt_data = NULL;
	//int audio_pkt_size = 0;
	audio_buf_size = 1000000;
	audio_buf_index = 0;
	audio_buf = NULL;
	swr = NULL; 
	audio_resample_target_fmt = 0;
	do_audio_resample = FALSE;

	//audio prep
	if(audioStream > -1){
		AVCodecParameters *aparams;

		aCodecCtxOrig=pFormatCtx->streams[audioStream]->codec;
		aCodec = avcodec_find_decoder(aCodecCtxOrig->codec_id);
		if(!aCodec) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1;
		}

		// Copy context
		aCodecCtx = avcodec_alloc_context3(aCodec);
		aparams = avcodec_parameters_alloc();
		avcodec_parameters_from_context(aparams, aCodecCtxOrig);
		avcodec_parameters_to_context(aCodecCtx,aparams);
		avcodec_parameters_free(&aparams);
		//if(avcodec_copy_context(aCodecCtx, aCodecCtxOrig) != 0) {
		//	fprintf(stderr, "Couldn't copy codec context");
		//	return -1; // Error copying codec context
		//}

		// Set audio settings from codec info
		fw_movie.channels = aCodecCtx->channels;
		fw_movie.freq = aCodecCtx->sample_rate;
		fw_movie.bits_per_channel = aCodecCtx->bits_per_raw_sample; 

		//printf("audio sample format %d\n",aCodecCtx->sample_fmt);
		// online I found request_sample_fmt is for older versions 1.1 and down, use swresample now
		//aCodecCtx->request_sample_fmt = AV_SAMPLE_FMT_FLTP; //AV_SAMPLE_FMT_S16P; //AV_SAMPLE_FMT_S16;

		printf("bits per coded channel=%d\n",aCodecCtx->bits_per_coded_sample);


		if(avcodec_open2(aCodecCtx, aCodec, NULL) < 0){
			fprintf(stderr, "Could not open codec\n");
			return -1;
		}
	

		audio_buf = malloc(audio_buf_size);
		aFrame=av_frame_alloc();
		aFrameB=av_frame_alloc();

		//assuming we resample to what we want:
		audio_resample_target_fmt = aCodecCtx->sample_fmt;
		if(aCodecCtx->sample_fmt != AV_SAMPLE_FMT_S16) {
			fw_movie.channels = 2;
			fw_movie.freq = 44100;
			fw_movie.bits_per_channel = 16; 
			audio_resample_target_fmt = AV_SAMPLE_FMT_S16;
			do_audio_resample = TRUE;

			// win32 openAL has problems with FLTP (float) audio format  
			// and android openSLES says when queuing chunks can only use PCM
			// recent versions of libavcodec convert mp4 audio to FLTP
			// so we will convert to an older S16 or S16P format
			//swresample didn't work for me, hand-coded did
			//// Set up SWR context once you've got codec information
			//swr = swr_alloc();
			//av_opt_set_int(swr, "in_channel_layout",  aCodecCtx->channel_layout, 0);
			//av_opt_set_int(swr, "out_channel_layout", aCodecCtx->channel_layout,  0);
			//av_opt_set_int(swr, "in_sample_rate",     aCodecCtx->sample_rate, 0);
			//av_opt_set_int(swr, "out_sample_rate",    aCodecCtx->sample_rate, 0);
			//av_opt_set_sample_fmt(swr, "in_sample_fmt",  aCodecCtx->sample_fmt, 0);
			//av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
			// https://www.ffmpeg.org/doxygen/3.2/group__lswr.html#details

			swr = swr_alloc_set_opts(NULL,  // we're allocating a new context
				AV_CH_LAYOUT_STEREO,  // out_ch_layout
				AV_SAMPLE_FMT_S16,    // out_sample_fmt
				44100,                // out_sample_rate
				aCodecCtx->channel_layout, // in_ch_layout
				aCodecCtx->sample_fmt,   // in_sample_fmt
				aCodecCtx->sample_rate,   // in_sample_rate
				0,                    // log_offset
				NULL);                // log_ctx
			swr_init(swr);
		}

	}

	//video function-scope variables
	sws_ctx = NULL;
	pFrame = NULL;
	pCodec = NULL;
	fw_framequeue = NULL;
	pFrameRGB = NULL;
	buffer = NULL;
	//video prep
	if(videoStream > -1){
		AVCodecParameters *vparams;		
		int numBytes;
		int av_pix_fmt;

		// Get a pointer to the codec context for the video stream
		pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;


		// Find the decoder for the video stream
		pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
		if(pCodec==NULL) {
			fprintf(stderr, "Unsupported codec!\n");
			return -1; // Codec not found
		}
		// Copy context
		pCodecCtx = avcodec_alloc_context3(pCodec);
		vparams = avcodec_parameters_alloc();
		avcodec_parameters_from_context(vparams, pCodecCtxOrig);
		avcodec_parameters_to_context(pCodecCtx, vparams);
		avcodec_parameters_free(&vparams);
		//if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		//	fprintf(stderr, "Couldn't copy codec context");
		//	return -1; // Error copying codec context
		//}
		// Open codec
		if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
			return -1; // Could not open codec
		//fw_movie.pVideoCodecCtx = pCodecCtx;


		// Allocate video frame
		pFrame=av_frame_alloc();

		// Allocate an AVFrame structure
		pFrameRGB=av_frame_alloc();
		if(pFrameRGB==NULL)
			return -1;

		// Determine required buffer size and allocate buffer
		if(0){
			nchan = 3;
			av_pix_fmt = AV_PIX_FMT_RGB24;
		}else{
			nchan = 4;
			av_pix_fmt = AV_PIX_FMT_RGBA;
		}

		fw_movie.nchan = nchan; //RGB24 == 3, RGBA == 4
		fw_movie.width = pCodecCtx->width;
		fw_movie.height = pCodecCtx->height;

		//numBytes=avpicture_get_size(av_pix_fmt, pCodecCtx->width,  //AV_PIX_FMT_RGB24, AV_PIX_FMT_RGBA
		//							pCodecCtx->height);
		numBytes = av_image_get_buffer_size(av_pix_fmt, pCodecCtx->width, pCodecCtx->height,1); //in ffmpeg code I see 1, 16, 32 for align
		buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));


		// Assign appropriate parts of buffer to image planes in pFrameRGB
		// Note that pFrameRGB is an AVFrame, but AVFrame is a superset
		// of AVPicture
		//avpicture_fill((AVPicture *)pFrameRGB, buffer,av_pix_fmt, //AV_PIX_FMT_RGBA, //AV_PIX_FMT_RGB24,
		//	pCodecCtx->width, pCodecCtx->height);
		av_image_fill_arrays(pFrameRGB->data,pFrameRGB->linesize,buffer,av_pix_fmt,pCodecCtx->width, pCodecCtx->height,1);

		// initialize SWS context for software scaling
		sws_ctx = sws_getContext(pCodecCtx->width,
			pCodecCtx->height,
			pCodecCtx->pix_fmt,
			pCodecCtx->width,
			pCodecCtx->height,
			av_pix_fmt, //AV_PIX_FMT_RGBA, //AV_PIX_FMT_RGB24,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL
			);

		//if( METHOD_DECODE_ON_LOAD ) - decodes all frames in resource thread when loading the file
		fw_framequeue = newStack(unsigned char *); //I like stack because stack_push will realloc
	}

	//video and audo decoded in combined loop (could split for decode-on-load)
	i=0;
	while(av_read_frame(pFormatCtx, &packet)>=0) {
		// Is this a packet from the video stream?
		if(packet.stream_index==videoStream) {
			// Decode video frame
			//avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			avcodec_send_packet(pCodecCtx,&packet);
			frameFinished = avcodec_receive_frame(pCodecCtx,pFrame) == 0? TRUE : FALSE;
			// Did we get a video frame?
			if(frameFinished) {
				// Convert the image from its native format to RGB
				unsigned char * fw_frame;
				int k;
				sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
					pFrame->linesize, 0, pCodecCtx->height,
					pFrameRGB->data, pFrameRGB->linesize);
	
				// Save the frame to disk
				++i;
				//if(++i<=5)
				if(0) if(i<=5){
					SaveFrame(pFrameRGB, pCodecCtx->width, 
							pCodecCtx->height, nchan, i);
				}
				//printf("saving frame %d %d %d\n",pCodecCtx->width,pCodecCtx->height, i);
				//printf("linesize = %d \n",pFrameRGB->linesize[0]);

				fw_frame = malloc(fw_movie.height * fw_movie.width *  nchan); //assumes width == linesize[0]
				
				for(k=0;k<pCodecCtx->height;k++){
					int kd,ks,kk;
					unsigned char *src;
					kk = pCodecCtx->height - k - 1; //flip y-down to y-up for opengl
					ks = k*pFrame->linesize[0]*nchan;
					kd = kk * fw_movie.width * nchan;
					src = ((unsigned char *)pFrameRGB->data[0]) + ks;
					memcpy(&fw_frame[kd],src,fw_movie.width * nchan);
				}
				stack_push(unsigned char *,fw_framequeue,fw_frame);
			}
			//av_free_packet(&packet);
		} else if(packet.stream_index==audioStream) {
			// http://open-activewrl.sourceforge.net/data/OpenAL_PGuide.pdf
			// page 5:
			// "Fill the buffers with PCM data using alBufferData."
			// alBufferData(g_Buffers[0],format,data,size,freq); 
			// Goal: PCM data
			// taking code from decode_audio_frame in ffmpeg tutorial03.c
			int buf_size;
			int got_frame = 0;
			int data_size = 0;
			//int len1;
			//len1 = avcodec_decode_audio4(aCodecCtx, aFrame, &got_frame, &packet);
			avcodec_send_packet(aCodecCtx, &packet);
			got_frame = avcodec_receive_frame(aCodecCtx, aFrame) == 0 ? TRUE : FALSE;

			buf_size = audio_buf_size - audio_buf_index;
			if(got_frame) {
				//aFrameOut->format = aCodecCtx->sample_fmt;
				if(aFrame->nb_samples > 0){
					data_size = av_samples_get_buffer_size(NULL, 
											aFrame->channels, //aCodecCtx->channels,
											aFrame->nb_samples,
											aFrame->format, //AV_SAMPLE_FMT_S16P, //aCodecCtx->sample_fmt,
											1);
					//printf("aCodecCtx->sample_fmt= %d channels=%d samples=%d",aCodecCtx->sample_fmt,aCodecCtx->channels,aFrame->nb_samples);
					//if this chunk's reformatted output will be bigger than the room we have left
					// in our allocated big audio buffer, then realloc the big audio buffer * 2
					if(data_size * 2 > buf_size){
						audio_buf = realloc(audio_buf,audio_buf_size *2);
						audio_buf_size *= 2;
					}
					if (do_audio_resample) //TRUE && aCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
					{
						if(0){
							//hand-coded FLTP to S16 
							// works with apple1984veryshort.mp4 on win32 openAL, but not generally trusted, just as hacker code
							//http://stackoverflow.com/questions/14989397/how-to-convert-sample-rate-from-av-sample-fmt-fltp-to-av-sample-fmt-s16
							int i,c;
							int nb_samples = aFrame->nb_samples;
							int channels = aFrame->channels;
							int outputBufferLen = nb_samples * channels * 2;
							short* outputBuffer = (short*)&audio_buf[audio_buf_index];

							for (i = 0; i < nb_samples; i++)
							{
								 for (c = 0; c < channels; c++)
								 {
									 float* extended_data = (float*)aFrame->extended_data[c];
									 float sample = extended_data[i];
									 if (sample < -1.0f) sample = -1.0f;
									 else if (sample > 1.0f) sample = 1.0f;
									 outputBuffer[i * channels + c] = (short)fwroundf(sample * 32767.0f);
								 }
							}
							audio_buf_index += outputBufferLen;
						}
						else if(1){
							//swresample module > swr_convert - works uwp and win32
							//should convert non PCM 16 formats to PCM 16bit/channel stereo, 44100Hz
							uint8_t *output;
							int in_samples = aFrame->nb_samples;

							int out_samples = (int)av_rescale_rnd(swr_get_delay(swr, aCodecCtx->sample_rate) + in_samples, 44100, aCodecCtx->sample_rate, AV_ROUND_UP);
							av_samples_alloc(&output, NULL, 2, out_samples,	AV_SAMPLE_FMT_S16, 0);
							out_samples = swr_convert(swr,&output,out_samples, aFrame->extended_data, aFrame->nb_samples);  
							memcpy(&audio_buf[audio_buf_index],output, out_samples * 2 * 2);
							audio_buf_index +=  out_samples * 2 * 2;
							av_freep(&output);
						}
					}else{
						//works when incoming audio is already in s16 format and decoder doesn't change it
						//ie mpgsys.mpg
						//(but for mp4 audio, libav gives FLP/float format, and using this simple
						// memcpy it comes out junk/noise in openAL H: openal can't handle float, just s16)
						memcpy(&audio_buf[audio_buf_index], aFrame->data[0], data_size);
						audio_buf_index += data_size;
					}
				}
			}

		} else {
			// Free the packet that was allocated by av_read_frame
			//av_free_packet(&packet);
		}
	}

	//video fin
	if(videoStream > -1){
		fw_movie.frames = fw_framequeue->data;
		fw_movie.nframes = fw_framequeue->n;
		fw_movie.duration = (double)(fw_movie.nframes) / 30.0; //s = frames / fps 

		if(0){
			//write out frames in .web3dit image format for testing
			int k;
			textureTableIndexStruct_s ttipp, *ttip;
			ttip = &ttipp;
			ttip->x = fw_movie.width;
			ttip->y = fw_movie.height;
			ttip->z = 1;
			ttip->hasAlpha = 1;
			ttip->channels = nchan;

			for(k=0;k<fw_movie.nframes;k++){
				char namebuf[100];
				ttip->texdata = fw_movie.frames[k];
				sprintf(namebuf,"%s%d.web3dit","ffmpeg_frame_",k);
				saveImage_web3dit(ttip, namebuf);
			}
		}
		//IF(METHOD_DECODE_ON_LOAD)
		//   GARBAGE COLLECT FFMPEG STUFF
		// Free the RGB image
		av_free(buffer);
		av_frame_free(&pFrameRGB);
  
		// Free the YUV frame
		av_frame_free(&pFrame);
  
		// Close the codecs
		avcodec_close(pCodecCtx);
		avcodec_close(pCodecCtxOrig);
	}
	//audio fin
	if(audioStream > -1){
		fw_movie.audio_buf = audio_buf;
		fw_movie.audio_buf_size = audio_buf_index;
		fw_movie.duration = (double)(fw_movie.nframes) / 30.0; //s = frames / fps 

		avcodec_close(aCodecCtxOrig);
		avcodec_close(aCodecCtx);
	}

	//audio and video fin
		// Close the video file
		avformat_close_input(&pFormatCtx);
		*opaque = malloc(sizeof(struct fw_movietexture));
		memcpy(*opaque,&fw_movie,sizeof(struct fw_movietexture));
	

	return 1;
}
double movie_get_duration(void *opaque){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	return fw_movie->duration;
}

unsigned char *movie_get_frame_by_fraction(void *opaque, float fraction, int *width, int *height, int *nchan){
	int iframe;
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	if(!fw_movie) return NULL;

	iframe = (int)(fraction * ((float)(fw_movie->nframes -1) + .5f));
	iframe = max(0,iframe);
	iframe = min(fw_movie->nframes -1,iframe);
	*width = fw_movie->width;
	*height = fw_movie->height;
	*nchan = fw_movie->nchan;
	return fw_movie->frames[iframe];
}
unsigned char * movie_get_audio_PCM_buffer(void *opaque,int *freq, int *channels, int *size, int *bits){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	if(!fw_movie) return NULL;
	if(!fw_movie->audio_buf) return NULL;
	*freq = fw_movie->freq;
	*channels = fw_movie->channels;
	*size = fw_movie->audio_buf_size;
	*bits = fw_movie->bits_per_channel;
	return fw_movie->audio_buf;
}
void movie_free(void *opaque){
	struct fw_movietexture *fw_movie = (struct fw_movietexture *)opaque;
	if(fw_movie) {
		int k;
		for(k=0;k<fw_movie->nframes;k++){
			FREE_IF_NZ(fw_movie->frames[k]);
		}
		free(opaque);
	}
}

#endif //MOVIETEXTURE_FFMPEG