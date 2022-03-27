#pragma once

#include "config.h"

#ifdef LIBQUESTMR_USE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/error.h>
}
#endif

#include "frame.h"

#include "BufferedSocket.h"

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

#define OM_DEFAULT_WIDTH (1920*2)
#define OM_DEFAULT_HEIGHT 1080
#define OM_DEFAULT_AUDIO_SAMPLERATE 48000
//#define OM_DEFAULT_IP_ADDRESS "192.168.10.105" //"192.168.0.1"
#define OM_DEFAULT_PORT 28734

namespace libQuestMR
{

LQMR_EXPORTS uint64_t getTimestampMs();

class LQMR_EXPORTS QuestVideoSource
{
public:
	virtual ~QuestVideoSource(){}
	virtual bool isValid() = 0;
	virtual int recv(char *buf, size_t bufferSize) = 0;
};

class LQMR_EXPORTS QuestVideoSourceBufferedSocket : public QuestVideoSource
{
public:
	virtual ~QuestVideoSourceBufferedSocket();
	virtual bool isValid();
	virtual int recv(char *buf, size_t bufferSize);

    bool Connect(std::string ipaddr, uint32_t port = OM_DEFAULT_PORT);
    void Disconnect();

    BufferedSocket m_connectSocket;
};

class LQMR_EXPORTS QuestVideoSourceFile : public QuestVideoSource
{
public:
	virtual ~QuestVideoSourceFile();
	virtual bool isValid();
	virtual int recv(char *buf, size_t bufferSize);

    void open(const char *filename);
    void close();

    FILE *file = NULL;
};

class LQMR_EXPORTS QuestVideoMngr
{
public:
#ifdef LIBQUESTMR_USE_FFMPEG
    const AVCodec* m_codec = nullptr;
	AVCodecContext* m_codecContext = nullptr;
	SwsContext* m_swsContext = nullptr;
	AVPixelFormat m_swsContext_SrcPixelFormat = AV_PIX_FMT_NONE;
#endif

    FrameCollection m_frameCollection;
    
    bool videoDecoding;

	int m_swsContext_SrcWidth = 0;
	int m_swsContext_SrcHeight = 0;
	int m_swsContext_DestWidth = 0;
	int m_swsContext_DestHeight = 0;

	std::vector<std::pair<int, std::shared_ptr<Frame>>> m_cachedAudioFrames;
	int m_audioFrameIndex = 0;
	int m_videoFrameIndex = 0;

    uint32_t m_width = OM_DEFAULT_WIDTH;
	uint32_t m_height = OM_DEFAULT_HEIGHT;
    uint32_t m_audioSampleRate = OM_DEFAULT_AUDIO_SAMPLERATE;

#ifdef LIBQUESTMR_USE_OPENCV
	cv::Mat mostRecentImg;
	cv::Mat m_temp_texture;
#endif
    uint64_t mostRecentTimestamp;

	QuestVideoSource *videoSource = NULL;
	FILE *recordedTimestampFile = NULL;

    QuestVideoMngr();
    void StartDecoder();
    void StopDecoder();

	void setRecording(const char *folder, const char *filenameWithoutExt);//set folder and filename (without extension) for recording 
	void setRecordedTimestampSource(const char *filename);//set timestamp file (for playback)
	void setVideoDecoding(bool videoDecoding);//to disable video decoding (useful if we want to record without preview)

    void ReceiveData();
    void VideoTickImpl(bool skipOldFrames = false);//process the received data
    void attachSource(QuestVideoSource *videoSource);//attach the data source (socket, file,...)
    void detachSource();//detach the data source

    uint32_t GetWidth()//get img width
	{
		return m_width;
	}

	uint32_t GetHeight()//get img height
	{
		return m_height;
	}

#ifdef LIBQUESTMR_USE_OPENCV
    cv::Mat getMostRecentImg(uint64_t *timestamp = NULL)
	{
		if(timestamp != NULL)
			*timestamp = mostRecentTimestamp;
		return mostRecentImg;
	}
#endif
};

}
