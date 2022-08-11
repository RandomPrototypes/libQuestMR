#pragma once

#include <libQuestMR/config.h>

#ifdef LIBQUESTMR_USE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/error.h>
}
#endif

#include <BufferedSocket/BufferedSocket.h>

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
	virtual ~QuestVideoSource();
	virtual bool isValid() = 0;
	virtual int recv(char *buf, size_t bufferSize, uint64_t *timestamp) = 0;
};

class LQMR_EXPORTS QuestVideoSourceBufferedSocket : public QuestVideoSource
{
public:
	virtual ~QuestVideoSourceBufferedSocket();
	virtual bool isValid() = 0;
	virtual int recv(char *buf, size_t bufferSize, uint64_t *timestamp) = 0;
	virtual void setUseThread(bool useThread, int maxBufferSize) = 0;
	virtual int getBufferedDataLength() = 0;

    virtual bool Connect(const char *ipaddr, uint32_t port = OM_DEFAULT_PORT) = 0;
    virtual void Disconnect() = 0;
};

class LQMR_EXPORTS QuestVideoSourceFile : public QuestVideoSource
{
public:
	virtual ~QuestVideoSourceFile();
	virtual bool isValid() = 0;
	virtual int recv(char *buf, size_t bufferSize, uint64_t *timestamp) = 0;

    virtual void open(const char *filename) = 0;
    virtual void close() = 0;
};

class LQMR_EXPORTS QuestAudioData
{
public:
	virtual ~QuestAudioData();
	virtual uint64_t getLocalTimestamp() const = 0;//timestamp when received the data
	virtual uint64_t getDeviceTimestamp() const = 0;//timestamp recorded on the device before transfer
	virtual int getNbChannels() const = 0;//number of channels
	virtual uint32_t getSampleRate() const = 0;//sample rate
	virtual const unsigned char *getData() const = 0;//raw audio data
	virtual int getDataLength() const = 0;//length (in bytes) of the audio data
};

class LQMR_EXPORTS QuestVideoMngr
{
public:
    virtual ~QuestVideoMngr();
    virtual void StartDecoder() = 0;
    virtual void StopDecoder() = 0;

	virtual void setRecording(const char *folder, const char *filenameWithoutExt) = 0;//set folder and filename (without extension) for recording 
	virtual void setRecordedTimestampFile(const char *filename, bool use_rectifyTimestamps = true) = 0;//set timestamp file (for playback)
	virtual void setRecordedTimestamp(const std::vector<uint64_t>& listTimestamp) = 0;//set timestamps (for playback)
	virtual void setVideoDecoding(bool videoDecoding) = 0;//to disable video decoding (useful if we want to record without preview)

    virtual void ReceiveData() = 0;
    virtual void VideoTickImpl(bool skipOldFrames = false) = 0;//process the received data
    virtual void attachSource(std::shared_ptr<QuestVideoSource> videoSource) = 0;//attach the data source (socket, file,...)
    virtual void detachSource() = 0;//detach the data source

    virtual uint32_t getWidth() = 0;//get img width

	virtual uint32_t getHeight() = 0;//get img height
	
	#ifdef LIBQUESTMR_USE_OPENCV
    virtual cv::Mat getMostRecentImg(uint64_t *timestamp = NULL, int *frameId = NULL) = 0;
	#endif
	virtual int getMostRecentAudio(QuestAudioData*** listAudioData) = 0;
	
};

class LQMR_EXPORTS QuestVideoMngrThreadData
{
public:
    virtual ~QuestVideoMngrThreadData();

    //Thread-safe function to set if the communication is finished
    virtual void setFinishedVal(bool val) = 0;

    //Thread-safe function to know if the communication is finished
    virtual bool isFinished() = 0;
 
    virtual void threadFunc() = 0;
    
#ifdef LIBQUESTMR_USE_OPENCV
    virtual cv::Mat getMostRecentImg(uint64_t *timestamp, int *frameId = NULL) = 0;
#endif
};

extern "C" 
{
    LQMR_EXPORTS void QuestVideoMngrThreadFunc(QuestVideoMngrThreadData *data);
	LQMR_EXPORTS QuestVideoSourceBufferedSocket *createQuestVideoSourceBufferedSocketRawPtr();
	LQMR_EXPORTS void deleteQuestVideoSourceBufferedSocketRawPtr(QuestVideoSourceBufferedSocket *videoSource);
	LQMR_EXPORTS QuestVideoSourceFile *createQuestVideoSourceFileRawPtr();
	LQMR_EXPORTS void deleteQuestVideoSourceFileRawPtr(QuestVideoSourceFile *videoSource);
	LQMR_EXPORTS QuestVideoMngr *createQuestVideoMngrRawPtr();
	LQMR_EXPORTS void deleteQuestVideoMngrRawPtr(QuestVideoMngr *videoMngr);
	LQMR_EXPORTS QuestVideoMngrThreadData *createQuestVideoMngrThreadDataRawPtr(std::shared_ptr<QuestVideoMngr> mngr);
	LQMR_EXPORTS void deleteQuestVideoMngrThreadDataRawPtr(QuestVideoMngrThreadData *threadData);
	
	LQMR_EXPORTS bool loadQuestRecordedTimestamps(const char *filename, std::vector<uint64_t> *listTimestamp, std::vector<uint32_t> *listType = NULL, std::vector<uint32_t> *listSize = NULL, std::vector<std::vector<std::string> > *listExtraData = NULL);

}

inline std::shared_ptr<QuestVideoSourceBufferedSocket> createQuestVideoSourceBufferedSocket()
{
	return std::shared_ptr<QuestVideoSourceBufferedSocket>(createQuestVideoSourceBufferedSocketRawPtr(), deleteQuestVideoSourceBufferedSocketRawPtr);
}

inline std::shared_ptr<QuestVideoSourceFile> createQuestVideoSourceFile()
{
	return std::shared_ptr<QuestVideoSourceFile>(createQuestVideoSourceFileRawPtr(), deleteQuestVideoSourceFileRawPtr);
}

inline std::shared_ptr<QuestVideoMngr> createQuestVideoMngr()
{
	return std::shared_ptr<QuestVideoMngr>(createQuestVideoMngrRawPtr(), deleteQuestVideoMngrRawPtr);
}

inline std::shared_ptr<QuestVideoMngrThreadData> createQuestVideoMngrThreadData(std::shared_ptr<QuestVideoMngr> mngr)
{
	return std::shared_ptr<QuestVideoMngrThreadData>(createQuestVideoMngrThreadDataRawPtr(mngr), deleteQuestVideoMngrThreadDataRawPtr);
}

#ifdef LIBQUESTMR_USE_OPENCV
	LQMR_EXPORTS cv::Mat composeMixedRealityImg(const cv::Mat& questImg, const cv::Mat& camImg, const cv::Mat& camAlpha);
#endif

}
