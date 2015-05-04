/****************************************************************************************************
 * Copyright (C) 2015/04/29, Genvision
 *
 * File Name   : mp4v2_264_aac_mp4.c
 * File Mark   :
 * Description : 
 * Others      :
 * Verision    : 1.0
 * Date        : Apr 29, 2015
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
#include <mp4v2/mp4v2.h>
#include <mp4v2_264_aac_mp4.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

struct mp4v2_info *mp4v2_info_init(char *file_name, unsigned int video_width,\
		unsigned int video_height, unsigned video_framerate, unsigned int audio_channel,\
		unsigned int audio_samplerate, int queue_maxsize)
{
	struct mp4v2_info *ptr = malloc(sizeof(struct mp4v2_info));

	if (ptr == NULL)
	{
		return NULL;
	}
	memset(ptr, 0x0, sizeof(struct mp4v2_info));

	ptr->pcMp4FileNamePtr = file_name;
	ptr->uiFrameWidth = video_width;
	ptr->uiFrameHeight = video_height;
	ptr->uiFrameRate = video_framerate;
	ptr->uiSampleRate = audio_samplerate;
	ptr->uiChannel = audio_channel;

	ptr->uiAVCProfileIndication = 0x42;  //sps[1]
	ptr->uiProfileCompat = 0x80; //sps[2]
	ptr->uiAVCLevelIndication = 0x2A; //sps[3]

	ptr->uiAudioEndFlag = 0;
	ptr->uiVideoEndFlag = 0;
	ptr->uiVideoTrackAVCSetFlag = 0;

	ptr->pDataQuePtr = fifo_init(queue_maxsize);
	if (ptr->pDataQuePtr == NULL)
	{
		free(ptr);
		return NULL;
	}

	return ptr;
}

void mp4v2_info_deinit(struct mp4v2_info *mp4v2_info_ptr)
{
	fifo_free(mp4v2_info_ptr->pDataQuePtr);
	free(mp4v2_info_ptr);
}

void mp4v2_set_videotrack_avcinfo(struct mp4v2_info *mp4v2_info_ptr, unsigned int avc_pro_inc,\
		unsigned int pro_compat, unsigned int avc_level_inc)
{
	mp4v2_info_ptr->uiAVCProfileIndication = avc_pro_inc;
	mp4v2_info_ptr->uiProfileCompat = pro_compat;
	mp4v2_info_ptr->uiAVCLevelIndication = avc_level_inc;
	mp4v2_info_ptr->uiVideoTrackAVCSetFlag = 1;
}

void *mp4v2_h264_aac_to_mp4(void *arg)
{
	struct mp4v2_info *param = (struct mp4v2_info *)arg;
	if (param == NULL)
	{
		fprintf(stderr,"parameter is null!\n");
		goto EXIT_0;
	}

	MP4FileHandle file = MP4Create(param->pcMp4FileNamePtr, 0);
	if (file == MP4_INVALID_FILE_HANDLE)
	{
		fprintf(stderr,"open file %s failed\n",param->pcMp4FileNamePtr);
		goto EXIT_0;
	}
	MP4SetTimeScale(file, 90000);

	while (param->uiVideoTrackAVCSetFlag == 0) usleep(10000);

	MP4TrackId video = MP4AddH264VideoTrack(file, 90000, 90000/param->uiFrameRate,\
			param->uiFrameWidth, param->uiFrameHeight, param->uiAVCProfileIndication,\
			param->uiProfileCompat, param->uiAVCLevelIndication, 3);
	if (video == MP4_INVALID_TRACK_ID)
	{
		fprintf(stderr,"add video track failed\n");
		goto EXIT_1;
	}
	MP4SetVideoProfileLevel(file, 0x7f);

	MP4TrackId audio = MP4AddAudioTrack(file, param->uiSampleRate, 1024, MP4_MPEG4_AUDIO_TYPE);
    if (audio == MP4_INVALID_TRACK_ID)
	{
		fprintf(stderr, "add audio track failed\n");
		goto EXIT_1;
	}
	MP4SetAudioProfileLevel(file, 0x02);
    
	system("date");
	while (1)
	{
		unsigned char *data = NULL;
		int size = 0;
		struct stream_packet_header header;

		data = fifo_pop(param->pDataQuePtr, &size);
		if (data != NULL)
		{
			memcpy(&header, data, sizeof(struct stream_packet_header));
			unsigned char *tmp = data + sizeof(struct stream_packet_header);

			if (header.uiPacketType == PACKET_TYPE_VIDEO)
			{
				if (header.uiSize >= 4)
				{
					unsigned int *p = (unsigned int *)tmp;
					*p = htonl(header.uiSize - 4);
				}
				MP4WriteSample(file, video, tmp, header.uiSize, MP4_INVALID_DURATION, 0, 1);
			}
			else if (header.uiPacketType == PACKET_TYPE_AUDIO)
			{
				MP4WriteSample(file, audio, &tmp[7], header.uiSize - 7, MP4_INVALID_DURATION, 0, 1);
			}
			free(data);
		}
		else
		{
			if (param->uiAudioEndFlag == 1 && param->uiVideoEndFlag == 1)
			{
				goto EXIT_1;
			}
			usleep(100000);
		}
	}//end while(1)
	system("date");
EXIT_1:
	MP4Close(file, 0);
	system("date");
EXIT_0:
	pthread_exit(NULL);
}
