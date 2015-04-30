/****************************************************************************************************
 * Copyright (C) 2015/04/29, Genvision
 *
 * File Name   : mp4v2_264_aac_mp4.h
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
#ifndef __MP4V2_264_AAC_MP4_H__
#define __MP4V2_264_AAC_MP4_H__
#include "fifo_operation.h"

typedef enum packet_type {
	PACKET_TYPE_ID,
	PACKET_TYPE_VIDEO,
	PACKET_TYPE_AUDIO,
}T_PacketType;

struct stream_packet_header{
	unsigned int uiSize;
	unsigned int uiPacketType;
	unsigned int uiFlag;
	unsigned int uiTimeStamp;
};

struct mp4v2_info{
	/*mp4 file info*/
	char *pcMp4FileNamePtr;
	/*video frame info*/
	unsigned int uiFrameWidth;
	unsigned int uiFrameHeight;
	unsigned int uiFrameRate;
	unsigned int uiAVCProfileIndication;
	unsigned int uiProfileCompat;
	unsigned int uiAVCLevelIndication;
	/*audio frame info*/
	unsigned int uiSampleRate;
	unsigned int uiChannel;
	/*queue info*/
	queue *pDataQuePtr;
	/*end flag*/
	unsigned int uiAudioEndFlag;
	unsigned int uiVideoEndFlag;
};

struct mp4v2_info *mp4v2_info_init(char *file_name, unsigned int video_width,\
		unsigned int video_height, unsigned video_framerate, unsigned int audio_channel,\
		unsigned int audio_samplerate, int queue_maxsize);

void mp4v2_info_deinit(struct mp4v2_info *mp4v2_info_ptr);

void *mp4v2_h264_aac_to_mp4(void *arg);
#endif //end ifndef __MP4V2_264_AAC_MP4_H__
