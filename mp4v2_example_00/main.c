/****************************************************************************************************
 * Copyright (C) 2015/04/28, Genvision
 *
 * File Name   : main.c
 * File Mark   :
 * Description : test file
 * Others      :
 * Verision    : 1.0
 * Date        : Apr 28, 2015
 * Author      : Peter Zhang
 * Email       : zfy31415926.love@163.com OR zhangfy@genvision.cn
 *
 * History 1   :
 *     Date    :
 *     Version :
 *     Author  :
 *     Modification:
 * History 2   : ...
 ****************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <mp4v2_264_aac_mp4.h>
#include <pthread.h>
#include <signal.h>

#define H264_FILE_PATH "./test_source_file/test.264"
#define AAC_FILE_PATH "./test_source_file/test.aac"
#define MP4_FILE_PATH "./test_source_file/test.mp4"

#define STREAMING_PACKET_HEADER_LENGTH 16 
#define UCBUF_LEN (1920*1080)
#define DEFAULT_FLAG (0)

#define GS_FIFOFULL_PRINT_SKIP  (10000)

static unsigned int g_ExitFlag = 0;

static unsigned int gs_fifofull_print_flag = 0;


int packet_push(unsigned char *buf, int size, unsigned int type,\
		unsigned int timestamp, struct mp4v2_info *ptr)
{
	struct stream_packet_header *PacketHeaderPtr = NULL;
	unsigned int DataLen = STREAMING_PACKET_HEADER_LENGTH + size;

	PacketHeaderPtr = (struct stream_packet_header *)malloc(sizeof(unsigned char)*DataLen);
	memset(PacketHeaderPtr,0,DataLen);

	PacketHeaderPtr->uiSize = size;
	PacketHeaderPtr->uiPacketType = type;
	PacketHeaderPtr->uiFlag = DEFAULT_FLAG;
	PacketHeaderPtr->uiTimeStamp = timestamp;

	memcpy((void *)((struct stream_packet_header *)PacketHeaderPtr + 1),(void *)buf,size);
    memset(buf,0,size);

	if (fifo_full(ptr->pDataQuePtr))
	{
		int popSize = 0;
		void *data = fifo_pop(ptr->pDataQuePtr, &popSize);
		free(data);
		if ((gs_fifofull_print_flag%GS_FIFOFULL_PRINT_SKIP) == 0)
		{
			fprintf(stderr,"queue is full\n");
			gs_fifofull_print_flag = 1;
		}
		else
		{
			gs_fifofull_print_flag ++;
		}
	}
	else
	{
		if (gs_fifofull_print_flag != 0)
		{
			gs_fifofull_print_flag = 0;
		}
	}

	return fifo_push(ptr->pDataQuePtr,PacketHeaderPtr,DataLen);
}

void my_sleep(int millsec)
{
	struct timeval t_timeval;
	t_timeval.tv_sec = 0;
	t_timeval.tv_usec = millsec*1000;
	select(0,0,0,0,&t_timeval);
}

unsigned int get_current_tick_count(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

void *audio_streaming(void *arg)
{
	struct mp4v2_info *ThreadParam = (struct mp4v2_info *)arg;
	FILE *aac_fd;
	unsigned char ucBuf[UCBUF_LEN];
	unsigned int uiCount;
	unsigned int uiFlag;

	unsigned int uiFrameNum = 0;
	unsigned int uiTimeStamp = 0;
	unsigned int uiFrameType = 0;

	unsigned int now_tick = 0;
	unsigned int next_tick = 0;

    aac_fd = fopen(AAC_FILE_PATH, "r");
	if (aac_fd == NULL)
	{
		fprintf(stderr,"Error : File %s not found\n",AAC_FILE_PATH);
		return NULL;
	}

	/*FIND THE FIRST AUDIO AAC HEADER*/
	memset(ucBuf,0,sizeof(ucBuf));
	uiFlag = 0;
	uiCount = 0;
	while(1){
		uiCount = fread(ucBuf,sizeof(unsigned char),UCBUF_LEN,aac_fd);
		if (uiCount)
		{
			int i;
			for (i = 0; i < uiCount; i ++)
			{
				if (ucBuf[i]==0xff && ucBuf[i+1]==0xf1)
				{
					uiFlag = 1;
					break;
				}
			}
			if (uiFlag == 1)
			{
				fseek(aac_fd,-1*uiCount,SEEK_CUR);
				fseek(aac_fd,i,SEEK_CUR);
				break;
			}
			else
			{
				continue;
			}
		}
		else
		{
			fprintf(stderr,"Warning : Don't find the first audio aac header int the file %s\n",AAC_FILE_PATH);
			return NULL;
		}
	}//end while(1)

	/*PUSH THE AUDIO DATA*/
	while (1)
	{
		now_tick = get_current_tick_count();
		next_tick = now_tick + (1024*1000/44100);
		uiCount = fread(ucBuf,sizeof(unsigned char),UCBUF_LEN,aac_fd);
		if (uiCount)
		{
			int i;
			for (i = 1; i < uiCount; i ++)
			{
				if (ucBuf[i]==0xff && ucBuf[i+1]==0xf1)
				{
					break;
				}
			}
			fseek(aac_fd,-1*uiCount,SEEK_CUR);
			fseek(aac_fd,i,SEEK_CUR);
			uiFrameType = PACKET_TYPE_AUDIO;
			uiTimeStamp = 1024*1000/44100*uiFrameNum;
			packet_push(ucBuf,i,uiFrameType,uiTimeStamp,ThreadParam);
			uiFrameNum ++;
			now_tick = get_current_tick_count();
			if (next_tick > now_tick)
			{
				my_sleep(next_tick - now_tick);
			}
		}
		else
		{
			break;
		}
		if (g_ExitFlag == 1)
		{
			break;
		}
	}//end while(1)

	fclose(aac_fd);
	ThreadParam->uiAudioEndFlag = 1;

	pthread_exit(NULL);
}
void *video_streaming(void *arg)
{
	struct mp4v2_info *ThreadParam = (struct mp4v2_info *)arg;
	FILE *h264_fd;
	unsigned char ucBuf[UCBUF_LEN];
	unsigned int uiCount;
	unsigned int uiFlag;

	unsigned int uiFrameNum = 0;
	unsigned int uiTimeStamp = 0;
	unsigned int uiFrameType;

	unsigned int now_tick = 0;
	unsigned int next_tick = 0;

	h264_fd = fopen(H264_FILE_PATH, "r");
	if (h264_fd == NULL)
	{
		fprintf(stderr,"Error : File %s not found\n",H264_FILE_PATH);
		return NULL;
	}

	/*FIND THE FIRST VIDEO SPS FRAME*/
	memset(ucBuf,0,sizeof(ucBuf));
	uiFlag = 0;
	uiCount = 0;
	while (1)
	{
		uiCount = fread(ucBuf,sizeof(unsigned char),UCBUF_LEN,h264_fd);
		if (uiCount)
		{
			int i;
			for (i = 0; i < uiCount; i ++)
			{
				if (ucBuf[i]==0x00 && ucBuf[i+1]==0x00 && ucBuf[i+2]==0x00 && ucBuf[i+3]==0x01 && (ucBuf[i+4]&0x1f)==7)
				{
					uiFlag = 1;
					mp4v2_set_videotrack_avcinfo(ThreadParam,ucBuf[i+5],ucBuf[i+6],ucBuf[i+7]);
					break;
				}
			}
			if (uiFlag == 1)
			{
				fseek(h264_fd,-1*uiCount,SEEK_CUR);
				fseek(h264_fd,i,SEEK_CUR);
				break;
			}
			else
			{
				continue;
			}
		}
		else
		{
			fprintf(stderr,"Warning : Don't find the first video SPS frame in the file %s\n",H264_FILE_PATH);
			return NULL;
		}
	}//end while(1)

	/*PUSH THE VIDEO DATA*/
	unsigned int uitempCnt = 0;
	while (1)
	{
		now_tick = get_current_tick_count();
		uiCount = fread(ucBuf,sizeof(unsigned char),UCBUF_LEN,h264_fd);
		if (uiCount)
		{
			int i;
			for (i = 1; i < uiCount; i ++)
			{
				if (ucBuf[i]==0x00 && ucBuf[i+1]==0x00 && ucBuf[i+2]==0x00 && ucBuf[i+3]==0x01)
				{
					break;
				}
			}
			fseek(h264_fd,-1*uiCount,SEEK_CUR);
			fseek(h264_fd,i,SEEK_CUR);
			uiFrameType = PACKET_TYPE_VIDEO;
			packet_push(ucBuf,i,uiFrameType,uiTimeStamp,ThreadParam);
			uiFrameNum ++;

			if (uitempCnt == 0 || uitempCnt == 1)
			{
				next_tick = now_tick + 33;
				uiTimeStamp += 33;
				uitempCnt ++;
			}
			else if (uitempCnt == 2)
			{
				next_tick = now_tick + 34;
				uiTimeStamp += 34;
				uitempCnt = 0;
			}
			else
			{
				uitempCnt = 0;
			}
			now_tick = get_current_tick_count();
			if (next_tick > now_tick)
			{
				my_sleep(next_tick - now_tick);
			}
			usleep(100);
		}
		else 
		{
			break;
		}
		if (g_ExitFlag == 1)
		{
			break;
		}
	}//end while(1)

	fclose(h264_fd);
	ThreadParam->uiVideoEndFlag = 1;

	pthread_exit(NULL);
}
void sigint_handle(int signal)
{
	g_ExitFlag = 1;
}
int main(int argc, const char *argv[])
{
	struct mp4v2_info *tMp4v2 =NULL;
    pthread_t AudioPid;
	pthread_t VideoPid;
	pthread_t Mp4v2Pid;
	int iRetVal = 0;
    
	tMp4v2 = mp4v2_info_init(MP4_FILE_PATH,1920,1080,30,2,44100,3*1024);

	signal(SIGINT,sigint_handle);

    iRetVal = pthread_create(&AudioPid,NULL,audio_streaming,(void *)tMp4v2);
	if (iRetVal < 0)
	{
		fprintf(stderr,"Create the thread named audio_streaming failed\n");
		goto EXIT_0;
	}

	iRetVal = pthread_create(&VideoPid,NULL,video_streaming,(void *)tMp4v2);
	if (iRetVal < 0)
	{
		fprintf(stderr,"Create the thread named video_streaming failed\n");
		goto EXIT_1;
	}

	iRetVal = pthread_create(&Mp4v2Pid,NULL,mp4v2_h264_aac_to_mp4,(void *)tMp4v2);
	if (iRetVal < 0)
	{
		fprintf(stderr,"Create the thread named mp4v2_h264_aac_to_mp4 failed\n");
		goto EXIT_2;
	}

	pthread_join(Mp4v2Pid, NULL);
	printf("mp4v2_h264_aac_to_mp4 joined\n");
EXIT_2:
	pthread_join(VideoPid, NULL);
	printf("video_streaming joined\n");
EXIT_1:
	pthread_join(AudioPid, NULL);
	printf("audio_streaming joined\n");
EXIT_0:
	mp4v2_info_deinit(tMp4v2);
	return 0;
}
