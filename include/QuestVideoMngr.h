#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/error.h>
}

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "frame.h"

#include "config.h"

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/opencv.hpp>
#endif

#define SOCKET int
#define INVALID_SOCKET          ((SOCKET)(~0))
#define SOCKET_ERROR            (-1)
#define WSAEWOULDBLOCK          EWOULDBLOCK 
#define closesocket close
#define ioctlsocket ioctl

#define OM_DEFAULT_WIDTH (1920*2)
#define OM_DEFAULT_HEIGHT 1080
#define OM_DEFAULT_AUDIO_SAMPLERATE 48000
#define OM_DEFAULT_IP_ADDRESS "192.168.10.105" //"192.168.0.1"
//"192.168.10.105"
#define OM_DEFAULT_PORT 28734

inline int WSAGetLastError()
{
    return errno;
}

class QuestVideoSource
{
public:
	virtual ~QuestVideoSource(){}
	virtual bool isValid() = 0;
	virtual ssize_t recv(char *buf, size_t bufferSize) = 0;
};

class QuestVideoSourceSocket : public QuestVideoSource
{
public:
	virtual ~QuestVideoSourceSocket();
	virtual bool isValid();
	virtual ssize_t recv(char *buf, size_t bufferSize);

    void Connect();
    void Disconnect();

    SOCKET m_connectSocket = INVALID_SOCKET;
    std::string m_ipaddr = OM_DEFAULT_IP_ADDRESS;
	uint32_t m_port = OM_DEFAULT_PORT;
};

class QuestVideoSourceFile : public QuestVideoSource
{
public:
	virtual ~QuestVideoSourceFile();
	virtual bool isValid();
	virtual ssize_t recv(char *buf, size_t bufferSize);

    void open(const char *filename);
    void close();

    FILE *file = NULL;
};

class QuestVideoMngr
{
public:
    cv::Mat m_temp_texture;

    AVCodec* m_codec = nullptr;
	AVCodecContext* m_codecContext = nullptr;

    FrameCollection m_frameCollection;

    SwsContext* m_swsContext = nullptr;
	int m_swsContext_SrcWidth = 0;
	int m_swsContext_SrcHeight = 0;
	AVPixelFormat m_swsContext_SrcPixelFormat = AV_PIX_FMT_NONE;
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
#endif
	unsigned long long mostRecentTimestamp;

	QuestVideoSource *videoSource = NULL;
	FILE *recordedTimestampFile = NULL;

    QuestVideoMngr();
    void StartDecoder();
    void StopDecoder();

	void setRecording(const char *folder, const char *filenameWithoutExt);
	void setRecordedTimestampSource(const char *filename);

    void ReceiveData();
    void VideoTickImpl(bool skipOldFrames = false);
    void attachSource(QuestVideoSource *videoSource);
    void detachSource();

    uint32_t GetWidth()
	{
		return m_width;
	}

	uint32_t GetHeight()
	{
		return m_height;
	}

	cv::Mat getMostRecentImg(unsigned long long *timestamp = NULL)
	{
		if(timestamp != NULL)
			*timestamp = mostRecentTimestamp;
		return mostRecentImg;
	}
};