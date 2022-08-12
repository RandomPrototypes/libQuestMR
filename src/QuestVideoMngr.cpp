/*
 * Based on GPL code from Facebook ( https://github.com/facebookincubator/obs-plugins ), modified and imported into libQuestMR
 *
 * Copyright (C) 2019-present, Facebook, Inc.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/QuestVideoTimestampRectifier.h>
#include <fcntl.h>
#include <thread>
#include <fstream>
#include <chrono>
#include "log.h"
#include "frame.h"
#include <BufferedSocket/DataPacket.h>


namespace libQuestMR
{

uint64_t getTimestampMs()
{
    auto currentTime = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(currentTime).count();
}

#ifdef LIBQUESTMR_USE_FFMPEG
std::string GetAvErrorString(int errNum)
{
	char buf[1024];
	std::string result = av_make_error_string(buf, 1024, errNum);
	return result;
}
#endif

class LQMR_EXPORTS QuestAudioDataImpl : public QuestAudioData
{
public:
    QuestAudioDataImpl(uint64_t deviceTimestamp, uint64_t localTimestamp, int nbChannels, uint32_t sampleRate, unsigned char *data, int dataLength);
	virtual ~QuestAudioDataImpl();
	virtual uint64_t getLocalTimestamp() const;//timestamp when received the data
	virtual uint64_t getDeviceTimestamp() const;//timestamp recorded on the device before transfer
	virtual int getNbChannels() const;//number of channels
    virtual uint32_t getSampleRate() const;//sample rate
	virtual const unsigned char *getData() const;//raw audio data
    virtual int getDataLength() const;//length (in bytes) of the audio data
protected:
    uint64_t deviceTimestamp, localTimestamp;
    int nbChannels;
    unsigned char *data;
    int dataLength;
    uint32_t sampleRate;
};

QuestAudioData::~QuestAudioData()
{
}

QuestAudioDataImpl::QuestAudioDataImpl(uint64_t deviceTimestamp, uint64_t localTimestamp, int nbChannels, uint32_t sampleRate, unsigned char *data, int dataLength)
    :deviceTimestamp(deviceTimestamp), localTimestamp(localTimestamp), nbChannels(nbChannels), sampleRate(sampleRate), dataLength(dataLength)
{
    if(data == NULL)
        this->data = NULL;
    else {
        this->data = new unsigned char[dataLength];
        memcpy(this->data, data, dataLength);
    }
}

QuestAudioDataImpl::~QuestAudioDataImpl()
{
    if(data != NULL)
        delete [] data;
}

uint64_t QuestAudioDataImpl::getLocalTimestamp() const
{
    return localTimestamp;
}

uint64_t QuestAudioDataImpl::getDeviceTimestamp() const
{
    return deviceTimestamp;
}

int QuestAudioDataImpl::getNbChannels() const
{
    return nbChannels;
}

uint32_t QuestAudioDataImpl::getSampleRate() const 
{
    return sampleRate;
}

const unsigned char *QuestAudioDataImpl::getData() const
{
    return data;
}

int QuestAudioDataImpl::getDataLength() const
{
    return dataLength;
}

class QuestVideoMngrImpl : public QuestVideoMngr
{
public:
    QuestVideoMngrImpl();
    virtual ~QuestVideoMngrImpl();
    virtual void StartDecoder();
    virtual void StopDecoder();

    virtual bool isRecording() const;//return true if the stream is currently recorded
	virtual void setRecording(const char *folder, const char *filenameWithoutExt);//set folder and filename (without extension) for recording 
	virtual void setRecordedTimestampFile(const char *filename, bool use_rectifyTimestamps = true);//set timestamp file (for playback)
    virtual void setRecordedTimestamp(const std::vector<uint64_t>& listTimestamp);//set timestamps (for playback)
	virtual void setVideoDecoding(bool videoDecoding);//to disable video decoding (useful if we want to record without preview)

    virtual void ReceiveData();
    virtual void VideoTickImpl(bool skipOldFrames = false);//process the received data
    virtual void attachSource(std::shared_ptr<QuestVideoSource> videoSource);//attach the data source (socket, file,...)
    virtual void detachSource();//detach the data source

    virtual uint32_t getWidth();//get img width

	virtual uint32_t getHeight();//get img height
	
	#ifdef LIBQUESTMR_USE_OPENCV
    virtual cv::Mat getMostRecentImg(uint64_t *timestamp = NULL, int *frameId = NULL);
	#endif
    virtual int getMostRecentAudio(QuestAudioData*** listAudioData);

private:
    void clearMostRecentAudioFrameList();
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
    std::vector<QuestAudioData*> mostRecentAudioFrames;
    uint64_t mostRecentTimestamp;

	std::shared_ptr<QuestVideoSource> videoSource = NULL;

    FILE *debugAudioFile;
    FILE *debugAudioHeaderFile;
    FILE *debugAudioTimestampFile;
};

QuestVideoMngr::~QuestVideoMngr()
{
}

QuestVideoMngrImpl::QuestVideoMngrImpl()
{
	videoDecoding = true;
	#ifdef LIBQUESTMR_USE_FFMPEG
    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec)
    {
        OM_BLOG(LOG_ERROR, "Unable to find decoder");
    }
    else
    {
        OM_BLOG(LOG_INFO, "Codec found. Capabilities 0x%x", m_codec->capabilities);
    }
    #endif
    //debugAudioFile = fopen("debugAudio.pcm", "w");
    //debugAudioHeaderFile = fopen("debugAudio.csv", "w");
    //debugAudioTimestampFile = fopen("debugAudioTimestamp.csv", "w");
}

QuestVideoMngrImpl::~QuestVideoMngrImpl()
{
    //fclose(debugAudioFile);
    //fclose(debugAudioHeaderFile);
    //fclose(debugAudioTimestampFile);
}

void QuestVideoMngrImpl::clearMostRecentAudioFrameList()
{
    for(size_t i = 0; i < mostRecentAudioFrames.size(); i++)
        delete mostRecentAudioFrames[i];
    mostRecentAudioFrames.clear();
}

bool QuestVideoMngrImpl::isRecording() const
{
    return m_frameCollection.isRecording();
}

void QuestVideoMngrImpl::setRecording(const char *folder, const char *filenameWithoutExt)
{
    m_frameCollection.setRecording(folder, filenameWithoutExt);
}

void QuestVideoMngrImpl::StartDecoder()
{
	#ifdef LIBQUESTMR_USE_FFMPEG
    if (m_codecContext != nullptr)
    {
        OM_BLOG(LOG_ERROR, "Decoder already started");
        return;
    }

    if (!m_codec)
    {
        OM_BLOG(LOG_ERROR, "m_codec not initalized");
        return;
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext)
    {
        OM_BLOG(LOG_ERROR, "Unable to create codec context");
        return;
    }

    AVDictionary* dict = nullptr;
    int ret = avcodec_open2(m_codecContext, m_codec, &dict);
    av_dict_free(&dict);
    if (ret < 0)
    {
        OM_BLOG(LOG_ERROR, "Unable to open codec context");
        avcodec_free_context(&m_codecContext);
        return;
    }

    OM_BLOG(LOG_INFO, "m_codecContext constructed and opened");
    #endif
}

void QuestVideoMngrImpl::StopDecoder()
{
	#ifdef LIBQUESTMR_USE_FFMPEG
    if (m_codecContext)
    {
        avcodec_close(m_codecContext);
        avcodec_free_context(&m_codecContext);
        OM_BLOG(LOG_INFO, "m_codecContext freed");
    }
    #endif

    m_frameCollection.Reset();

    /*if (m_temp_texture)
    {
        obs_enter_graphics();
        gs_texture_destroy(m_temp_texture);
        obs_leave_graphics();
        m_temp_texture = nullptr;
    }*/
}

void QuestVideoMngrImpl::ReceiveData()
{
    if (videoSource != NULL && videoSource->isValid())
    {
        for (;;)
        {
            /*fd_set socketSet = { 0 };
            FD_ZERO(&socketSet);
            FD_SET(m_connectSocket, &socketSet);

            timeval t = { 0, 0 };
            int num = select(0, &socketSet, nullptr, nullptr, &t);*/
            //if (num >= 1)
            {
                const int bufferSize = 65536;
                uint8_t buf[bufferSize];
                uint64_t timestamp;
                int iResult = videoSource->recv((char*)buf, bufferSize, &timestamp);
                if (iResult < 0)
                {
                    OM_BLOG(LOG_ERROR, "recv error %d, closing socket", iResult);
                    detachSource();
                    break;
                }
                else if (iResult == 0)
                {
                    OM_BLOG(LOG_INFO, "recv 0 bytes, closing socket");
                    detachSource();
                    break;
                }
                else
                {
                    OM_BLOG(LOG_INFO, "recv: %d bytes received", iResult);
                    m_frameCollection.AddData(buf, iResult, timestamp);
                    break;
                }
            }
            /*else
            {
                printf("no data\n");
                break;
            }*/
        }
    }
}

void QuestVideoMngrImpl::setRecordedTimestampFile(const char *filename, bool use_rectifyTimestamps)
{
    std::vector<uint64_t> recordedTimestamp;
    std::vector<uint32_t> listType;
    std::vector<uint32_t> listSize;
    std::vector<std::vector<std::string> > listExtraData;
    loadQuestRecordedTimestamps(filename, &recordedTimestamp, &listType, &listSize, &listExtraData);
    //loadQuestRecordedTimestamps(filename, &recordedTimestamp, &listType);
    if(use_rectifyTimestamps)
        recordedTimestamp = rectifyTimestamps(recordedTimestamp, listType, listSize, listExtraData);
    setRecordedTimestamp(recordedTimestamp);
}

void QuestVideoMngrImpl::setRecordedTimestamp(const std::vector<uint64_t>& listTimestamp)
{
    m_frameCollection.setRecordedTimestamp(listTimestamp);
}

void QuestVideoMngrImpl::setVideoDecoding(bool videoDecoding)
{
	this->videoDecoding = videoDecoding;
}

void QuestVideoMngrImpl::VideoTickImpl(bool skipOldFrames)
{
    if (videoSource != NULL && videoSource->isValid())
    {
        if(!m_frameCollection.HasCompletedFrame())
            ReceiveData();

        if (videoSource == NULL || !videoSource->isValid())	// socket disconnected
            return;

        //std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
        while (m_frameCollection.HasCompletedFrame())
        {
            //std::chrono::duration<double> timePassed = std::chrono::system_clock::now() - startTime;
            //if (timePassed.count() > 0.05)
            //	break;

            auto frame = m_frameCollection.PopFrame();

            //auto current_time = std::chrono::system_clock::now();
            //auto seconds_since_epoch = std::chrono::duration<double>(current_time.time_since_epoch()).count();
            //double latency = seconds_since_epoch - frame->m_secondsSinceEpoch;

            if (frame->m_type == Frame::PayloadType::VIDEO_DIMENSION)
            {
                m_width = convertBytesToInt32(frame->m_payload.data(), false);
                m_height = convertBytesToInt32(frame->m_payload.data() + 4, false);

                OM_BLOG(LOG_INFO, "[VIDEO_DIMENSION] width %d height %d", m_width, m_height);
            }
            else if (frame->m_type == Frame::PayloadType::VIDEO_DATA)
            {
                //auto start = std::chrono::steady_clock::now();
		
            	if(videoDecoding)
            	{
		        	#ifdef LIBQUESTMR_USE_FFMPEG
		            OM_BLOG(LOG_ERROR, "[VIDEO_DATA]");
		            AVPacket* packet = av_packet_alloc();
		            AVFrame* picture = av_frame_alloc();

		            av_new_packet(packet, (int)frame->m_payload.size());
		            assert(packet->data);
		            memcpy(packet->data, frame->m_payload.data(), frame->m_payload.size());

		            int ret = avcodec_send_packet(m_codecContext, packet);
		            if (ret < 0)
		            {
		                OM_BLOG(LOG_ERROR, "avcodec_send_packet error %s", GetAvErrorString(ret).c_str());
		            }
		            else
		            {
		                ret = avcodec_receive_frame(m_codecContext, picture);
		                if (ret < 0)
		                {
		                    OM_BLOG(LOG_ERROR, "avcodec_receive_frame error %s", GetAvErrorString(ret).c_str());
		                }
		                else
		                {
	#if _DEBUG
		                    double timePassed = m_frameCollection.GetNbTickSinceFirstFrame();
		                    OM_BLOG(LOG_DEBUG, "[%lf][VIDEO_DATA] size %d width %d height %d format %d", timePassed, packet->size, picture->width, picture->height, picture->format);
	#endif

                            clearMostRecentAudioFrameList();
		                    while (m_cachedAudioFrames.size() > 0 /*&& m_cachedAudioFrames[0].first <= m_videoFrameIndex*/)
		                    {
		                        std::shared_ptr<Frame> audioFrame = m_cachedAudioFrames[0].second;

		                        struct AudioDataHeader {
		                            uint64_t timestamp;
		                            int channels;
		                            int dataLength;
		                        };
                                AudioDataHeader audioDataHeader;
                                audioDataHeader.timestamp = convertBytesToUInt64(audioFrame->m_payload.data(), false);
                                audioDataHeader.channels = convertBytesToInt32(audioFrame->m_payload.data() + 8, false);
                                audioDataHeader.dataLength = convertBytesToInt32(audioFrame->m_payload.data() + 12, false);

                                //fprintf(debugAudioHeaderFile, "%lf,%d\n", (double)audioDataHeader.timestamp, audioDataHeader.dataLength);

		                        if (audioDataHeader.channels == 1 || audioDataHeader.channels == 2)
		                        {
		                            /*obs_source_audio audio = { 0 };
		                            audio.data[0] = (uint8_t*)audioFrame.m_payload.data() + sizeof(AudioDataHeader);
		                            audio.frames = audioDataHeader.dataLength / sizeof(float) / audioDataHeader.channels;
		                            audio.speakers = audioDataHeader.channels == 1 ? SPEAKERS_MONO : SPEAKERS_STEREO;
		                            audio.format = AUDIO_FORMAT_FLOAT;
		                            audio.samples_per_sec = m_audioSampleRate;
		                            audio.timestamp = audioDataHeader.timestamp;
		                            obs_source_output_audio(m_src, &audio);*/
                                    //fprintf(debugAudioTimestampFile, "%llu,%d\n", (unsigned long long)audioFrame->localTimestamp, audioDataHeader.dataLength);
                                    mostRecentAudioFrames.push_back(new QuestAudioDataImpl(audioDataHeader.timestamp, audioFrame->localTimestamp, audioDataHeader.channels, m_audioSampleRate, (uint8_t*)audioFrame->m_payload.data() + sizeof(AudioDataHeader), audioDataHeader.dataLength));
                                    //FILE *debugTimestamp = fopen("debugTimestamp2.csv", "a");
                                    //fprintf(debugTimestamp, "%llu,%llu\n", audioDataHeader.timestamp, audioFrame.localTimestamp);
                                    //fclose(debugTimestamp);
		                        }
		                        else
		                        {
		                            OM_BLOG(LOG_ERROR, "[AUDIO_DATA] unimplemented audio channels %d", audioDataHeader.channels);
		                        }

		                        m_cachedAudioFrames.erase(m_cachedAudioFrames.begin());
		                    }

		                    ++m_videoFrameIndex;

		                    if (m_swsContext != nullptr)
		                    {
		                        if (m_swsContext_SrcWidth != m_codecContext->width ||
		                            m_swsContext_SrcHeight != m_codecContext->height ||
		                            m_swsContext_SrcPixelFormat != m_codecContext->pix_fmt ||
		                            m_swsContext_DestWidth != m_codecContext->width ||
		                            m_swsContext_DestHeight != m_codecContext->height)
		                        {
		                            OM_BLOG(LOG_DEBUG, "Need recreate m_swsContext");
		                            sws_freeContext(m_swsContext);
		                            m_swsContext = nullptr;
		                        }
		                    }

		                    if (m_swsContext == nullptr)
		                    {
		                        m_swsContext = sws_getContext(
		                            m_codecContext->width,
		                            m_codecContext->height,
		                            m_codecContext->pix_fmt,
		                            m_codecContext->width,
		                            m_codecContext->height,
		                            AV_PIX_FMT_BGR24,
		                            SWS_POINT,
		                            nullptr, nullptr, nullptr
		                        );
		                        m_swsContext_SrcWidth = m_codecContext->width;
		                        m_swsContext_SrcHeight = m_codecContext->height;
		                        m_swsContext_SrcPixelFormat = m_codecContext->pix_fmt;
		                        m_swsContext_DestWidth = m_codecContext->width;
		                        m_swsContext_DestHeight = m_codecContext->height;
		                        OM_BLOG(LOG_DEBUG, "sws_getContext(%d, %d, %d)", m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt);
		                    }

		                    assert(m_swsContext);

		                    m_temp_texture = cv::Mat(m_codecContext->height, m_codecContext->width, CV_8UC3);
		                    //uint8_t* data[1] = { new uint8_t[m_codecContext->width * m_codecContext->height * 4] };
		                    uint8_t* data[1] = { m_temp_texture.ptr<uint8_t>(0) };
		                    int stride[1] = { (int)m_codecContext->width * 3 };
		                    sws_scale(m_swsContext, picture->data,
		                        picture->linesize,
		                        0,
		                        picture->height,
		                        data,
		                        stride);
		                
		                    //cv::cvtColor(m_temp_texture, m_temp_texture, cv::COLOR_RGBA2BGR);
		                    cv::flip(m_temp_texture, m_temp_texture, 0);

		                    mostRecentImg = m_temp_texture;
		                    mostRecentTimestamp = frame->localTimestamp;

		                    //cv::imshow("img", m_temp_texture);
		                    //cv::waitKey(10);

		                    /*obs_enter_graphics();
		                    if (m_temp_texture)
		                    {
		                        gs_texture_destroy(m_temp_texture);
		                        m_temp_texture = nullptr;
		                    }
		                    m_temp_texture = gs_texture_create(m_codecContext->width,
		                        m_codecContext->height,
		                        GS_RGBA,
		                        1,
		                        const_cast<const uint8_t**>(&data[0]),
		                        0);
		                    obs_leave_graphics();

		                    delete data[0];*/
		                }
		            }

		            av_frame_free(&picture);
		            av_packet_free(&packet);
		            
		            #endif
		        } else {
		        	++m_videoFrameIndex;
		        }

                //auto end = std::chrono::steady_clock::now();
		        //printf("decode frame: %lf ms\n", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()/1000.0);

                if(!skipOldFrames)
                    break;
            }
            else if (frame->m_type == Frame::PayloadType::AUDIO_SAMPLERATE)
            {
                m_audioSampleRate = *(uint32_t*)(frame->m_payload.data());
                OM_BLOG(LOG_DEBUG, "[AUDIO_SAMPLERATE] %d", m_audioSampleRate);
            }
            else if (frame->m_type == Frame::PayloadType::AUDIO_DATA)
            {
                m_cachedAudioFrames.push_back(std::make_pair(m_audioFrameIndex, frame));
                //fwrite(frame->m_payload.data(), 1, frame->m_payload.size(), debugAudioFile);
                ++m_audioFrameIndex;
#if _DEBUG
                double timePassed = m_frameCollection.GetNbTickSinceFirstFrame();
                OM_BLOG(LOG_DEBUG, "[%lf][AUDIO_DATA] timestamp", timePassed);
#endif
            }
            else
            {
                OM_BLOG(LOG_ERROR, "Unknown payload type: %u", frame->m_type);
            }
        }
    }
}

uint32_t QuestVideoMngrImpl::getWidth()//get img width
{
	return m_width;
}

uint32_t QuestVideoMngrImpl::getHeight()//get img height
{
	return m_height;
}

#ifdef LIBQUESTMR_USE_OPENCV
cv::Mat QuestVideoMngrImpl::getMostRecentImg(uint64_t *timestamp, int *frameId)
{
	if(timestamp != NULL)
		*timestamp = mostRecentTimestamp;
    if(frameId != NULL)
        *frameId = m_videoFrameIndex;
	return mostRecentImg;
}
#endif

int QuestVideoMngrImpl::getMostRecentAudio(QuestAudioData*** listAudioData)
{
    *listAudioData = &mostRecentAudioFrames[0];
    return (int)mostRecentAudioFrames.size();
}


void QuestVideoMngrImpl::attachSource(std::shared_ptr<QuestVideoSource> videoSource)
{
    this->videoSource = videoSource;
    m_frameCollection.Reset();

    m_audioFrameIndex = 0;
    m_videoFrameIndex = 0;
    m_cachedAudioFrames.clear();

    if (videoSource->isValid())
        StartDecoder();
}

void QuestVideoMngrImpl::detachSource()
{
    if (videoSource == NULL)// || !videoSource->isValid())
    {
        OM_BLOG(LOG_ERROR, "Not connected");
        return;
    }

    StopDecoder();

    videoSource = NULL;
}


QuestVideoSource::~QuestVideoSource()
{
}

class QuestVideoSourceFileImpl : public QuestVideoSourceFile
{
public:
	QuestVideoSourceFileImpl();
	virtual ~QuestVideoSourceFileImpl();
	virtual bool isValid();
	virtual int recv(char *buf, size_t bufferSize, uint64_t *timestamp);

    void open(const char *filename);
    void close();

private:
    FILE *file = NULL;
};

QuestVideoSourceFile::~QuestVideoSourceFile()
{
}

QuestVideoSourceFileImpl::QuestVideoSourceFileImpl()
{
}

QuestVideoSourceFileImpl::~QuestVideoSourceFileImpl()
{
    close();
}

void QuestVideoSourceFileImpl::open(const char *filename)
{
    if (file != NULL)
    {
        OM_BLOG(LOG_ERROR, "Already opened");
        return;
    }

    file = fopen(filename, "rb");
}

int QuestVideoSourceFileImpl::recv(char *buf, size_t bufferSize, uint64_t *timestamp)
{
    if(feof(file))
        return 0;
    int sizeRead = static_cast<int>(::fread(buf, 1, bufferSize, file));
    if(timestamp != NULL)
        *timestamp = getTimestampMs();
    return sizeRead;
}

bool QuestVideoSourceFileImpl::isValid()
{
    return file != NULL && !feof(file);
}

void QuestVideoSourceFileImpl::close()
{
    if(file != NULL)
        fclose(file);
    file = NULL;
}

class BufferWithTimestamp
{
public:
    int length;
    char *buffer;
    uint64_t timestamp;
};

class QuestVideoSourceBufferedSocketImpl : public QuestVideoSourceBufferedSocket
{
public:
	QuestVideoSourceBufferedSocketImpl();
	virtual ~QuestVideoSourceBufferedSocketImpl();
	virtual bool isValid();
	virtual int recv(char *buf, size_t bufferSize, uint64_t *timestamp);
    virtual void setUseThread(bool useThread, int maxBufferSize);
    virtual int getBufferedDataLength();

    virtual bool Connect(const char *ipaddr, uint32_t port = OM_DEFAULT_PORT);
    virtual void Disconnect();

    void threadFunc();
    void clearBufferedData();

private:
    std::shared_ptr<BufferedSocket> m_connectSocket;
    int threadMaxBufferSize;
    std::thread *threadPtr;
    std::mutex mutex;
    std::vector<BufferWithTimestamp> receivedData;
    volatile int receivedDataSize;
    volatile bool stopped;
};

QuestVideoSourceBufferedSocket::~QuestVideoSourceBufferedSocket()
{
}

QuestVideoSourceBufferedSocketImpl::QuestVideoSourceBufferedSocketImpl()
{
	m_connectSocket = createBufferedSocket();
    threadPtr = NULL;
    threadMaxBufferSize = 500*1024*1024;
    receivedDataSize = 0;
    stopped = true;
}

QuestVideoSourceBufferedSocketImpl::~QuestVideoSourceBufferedSocketImpl()
{
    Disconnect();
}

void QuestVideoSourceBufferedSocketImpl::setUseThread(bool useThread, int maxBufferSize)
{
    threadMaxBufferSize = useThread ? maxBufferSize : 0;
}

int QuestVideoSourceBufferedSocketImpl::getBufferedDataLength()
{
    return receivedDataSize;
}

void QuestVideoSourceBufferedSocketImpl::clearBufferedData()
{
    mutex.lock();
    receivedDataSize = 0;
    for(size_t i = 0; i < receivedData.size(); i++)
        delete [] receivedData[i].buffer;
    receivedData.clear();
    mutex.unlock();
}


bool QuestVideoSourceBufferedSocketImpl::Connect(const char *ipaddr, uint32_t port)
{
    bool ret = m_connectSocket->connect(ipaddr, port);
    clearBufferedData();
    if(ret)
    {
        stopped = false;
        if(threadMaxBufferSize > 0)
            threadPtr = new std::thread(&QuestVideoSourceBufferedSocketImpl::threadFunc, this);
    }
    return ret;
}

void QuestVideoSourceBufferedSocketImpl::threadFunc()
{
    const int bufferSize = 50000;
    char buffer[bufferSize];
    while(!stopped)
    {
        while(receivedDataSize >= threadMaxBufferSize) {
            if(stopped) {
                return ;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        int sizeRead = m_connectSocket->readData(buffer, bufferSize);
        if(sizeRead <= 0) {
            stopped = true;
            break;
        }
        BufferWithTimestamp data;
        data.timestamp = getTimestampMs();
        data.buffer = new char[sizeRead];
        memcpy(data.buffer, buffer, sizeRead);
        data.length = sizeRead;
        mutex.lock();
        receivedData.push_back(data);
        receivedDataSize += sizeRead;
        mutex.unlock();
    }
}

int QuestVideoSourceBufferedSocketImpl::recv(char *buf, size_t bufferSize, uint64_t *timestamp)
{
    if(threadPtr != NULL || receivedDataSize > 0) {
        while(receivedDataSize == 0) {
            if(stopped) {
                return 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        mutex.lock();
        BufferWithTimestamp data = receivedData[0];
        if(timestamp != NULL)
            *timestamp = data.timestamp;

        int readSize = 0;
        if(data.length <= bufferSize) {
            receivedDataSize -= data.length;
            receivedData.erase(receivedData.begin());
            readSize = data.length;
            memcpy(buf, data.buffer, data.length);
            delete [] data.buffer;
        } else {
            receivedDataSize -= bufferSize;
            readSize = bufferSize;
            memcpy(buf, data.buffer, bufferSize);
            for(size_t i = 0; i < data.length-bufferSize; i++)
                data.buffer[i] = data.buffer[i+bufferSize];
            data.length -= bufferSize;
        }
        mutex.unlock();
        return readSize;
    } else {
        int sizeRead = m_connectSocket->readData(buf, (int)bufferSize);
        if(timestamp != NULL)
            *timestamp = getTimestampMs();
        return sizeRead;
    }
}

bool QuestVideoSourceBufferedSocketImpl::isValid()
{
    return receivedDataSize > 0 || (!stopped && m_connectSocket->isConnected());
}

void QuestVideoSourceBufferedSocketImpl::Disconnect()
{
    stopped = true;
    if(threadPtr != NULL) {
        threadPtr->join();
        threadPtr = NULL;
    }
    m_connectSocket->disconnect();
}

class QuestVideoMngrThreadDataImpl : public QuestVideoMngrThreadData
{
public:
    QuestVideoMngrThreadDataImpl(std::shared_ptr<QuestVideoMngr> mngr)
		:mngr(mngr)
	{
		finished = false;
        mostRecentTimestamp = 0;
        mostRecentFrameId = -1;
	}

    virtual ~QuestVideoMngrThreadDataImpl()
    {
    }

    //Thread-safe function to set if the communication is finished
    virtual void setFinishedVal(bool val)
    {
        finished = val;
    }

    //Thread-safe function to know if the communication is finished
    virtual bool isFinished()
    {
        return finished;
    }
    
    #ifdef LIBQUESTMR_USE_OPENCV
    virtual cv::Mat getMostRecentImg(uint64_t *timestamp, int *frameId = NULL)
    {
        cv::Mat img;
		mutex.lock();
		img = mostRecentImg;
        if(timestamp != NULL)
            *timestamp = mostRecentTimestamp;
        if(frameId != NULL)
            *frameId = mostRecentFrameId;
		mutex.unlock();
		return img;
    }
    #endif
    
    virtual void threadFunc()
    {
        while(!isFinished())
		{
			mngr->VideoTickImpl();
            uint64_t timestamp;
            int frameId;
            cv::Mat img = mngr->getMostRecentImg(&timestamp, &frameId);
            if(frameId != mostRecentFrameId) {
                mutex.lock();
                mostRecentImg = img.clone();
                mostRecentTimestamp = timestamp;
                mostRecentFrameId = frameId;
                mutex.unlock();
            }
		}
    }
private:
    std::mutex mutex;
	std::shared_ptr<QuestVideoMngr> mngr;
	bool finished;
    cv::Mat mostRecentImg;
    uint64_t mostRecentTimestamp;
    int mostRecentFrameId;
};

QuestVideoMngrThreadData::~QuestVideoMngrThreadData()
{
}


extern "C" 
{
    void QuestVideoMngrThreadFunc(QuestVideoMngrThreadData *data)
    {
        data->threadFunc();
    }

	QuestVideoSourceBufferedSocket *createQuestVideoSourceBufferedSocketRawPtr()
	{
		return new QuestVideoSourceBufferedSocketImpl();
	}
	void deleteQuestVideoSourceBufferedSocketRawPtr(QuestVideoSourceBufferedSocket *videoSource)
	{
		delete videoSource;
	}
	QuestVideoSourceFile *createQuestVideoSourceFileRawPtr()
	{
		return new QuestVideoSourceFileImpl();
	}
	void deleteQuestVideoSourceFileRawPtr(QuestVideoSourceFile *videoSource)
	{
		delete videoSource;
	}
	QuestVideoMngr *createQuestVideoMngrRawPtr()
	{
		return new QuestVideoMngrImpl();
	}
	void deleteQuestVideoMngrRawPtr(QuestVideoMngr *videoMngr)
	{
		delete videoMngr;
	}

    QuestVideoMngrThreadData *createQuestVideoMngrThreadDataRawPtr(std::shared_ptr<QuestVideoMngr> mngr)
	{
		return new QuestVideoMngrThreadDataImpl(mngr);
	}
	
	void deleteQuestVideoMngrThreadDataRawPtr(QuestVideoMngrThreadData *threadData)
	{
		delete threadData;
	}


    bool loadQuestRecordedTimestamps(const char *filename, std::vector<uint64_t> *listTimestamp, std::vector<uint32_t> *listType, std::vector<uint32_t> *listSize, std::vector<std::vector<std::string> > *listExtraData)
    {
        if(listTimestamp != NULL)
            listTimestamp->clear();
        if(listType != NULL)
            listType->clear();
        if(listSize != NULL)
            listSize->clear();
        if(listExtraData != NULL)
            listExtraData->clear();
        
        std::ifstream input(filename);
        if(!input.good())
            return false;
        std::string line;
        while(std::getline(input, line))
        {
            uint64_t timestamp;
            uint32_t type;
            uint32_t size;
            std::string timestampStr, typeStr, sizeStr;
            std::istringstream str(line);
            if(!std::getline(str, timestampStr, ',') || !std::getline(str, typeStr, ',') || !std::getline(str, sizeStr, ','))
                break;

            std::istringstream(timestampStr) >> timestamp;
            std::istringstream(typeStr) >> type;
            std::istringstream(sizeStr) >> size;
            
            if(listTimestamp != NULL)
                listTimestamp->push_back(timestamp);
            if(listType != NULL)
                listType->push_back(type);
            if(listSize != NULL)
                listSize->push_back(size);
            if(listExtraData != NULL) {
                std::vector<std::string> data;
                std::string tmp;
                while(std::getline(str, tmp, ','))
                    data.push_back(tmp);
                listExtraData->push_back(data);
            }
        }
        return true;
    }
}

#ifdef LIBQUESTMR_USE_OPENCV
    cv::Mat composeMixedRealityImg(const cv::Mat& questImg, const cv::Mat& camImg, const cv::Mat& camAlpha)
    {
        cv::Mat result = questImg(cv::Rect(0,0,questImg.cols/2,questImg.rows)).clone();
        cv::Mat fgImg, fgAlpha;
        cv::resize(questImg(cv::Rect(questImg.cols/2,0,questImg.cols/4,questImg.rows)), fgImg, result.size());
        cv::resize(questImg(cv::Rect(questImg.cols*3/4,0,questImg.cols/4,questImg.rows)), fgAlpha, result.size());
        int camAlphaChannelCount = camAlpha.channels();
        for(int i = 0; i < result.rows; i++)
        {
            unsigned char *resultPtr = result.ptr<unsigned char>(i);
            const unsigned char *fgPtr = fgImg.ptr<unsigned char>(i);
            const unsigned char *fgAlphaPtr = fgAlpha.ptr<unsigned char>(i);
            const unsigned char *camImgPtr = camImg.ptr<unsigned char>(i);
            const unsigned char *camAlphaPtr = camAlpha.ptr<unsigned char>(i);

            int width = std::min(result.cols, camImg.cols);
            if(i >= camImg.rows)
                width = 0;
            for(int j = width; j > 0; j--) {
                int alpha = *camAlphaPtr;
                for(int k = 3; k > 0; k--) {
                    *resultPtr = ((255-alpha) * (*resultPtr) + alpha * (*camImgPtr))/255;
                    *resultPtr = ((255-(*fgAlphaPtr)) * (*resultPtr) + (*fgAlphaPtr) * (*fgPtr))/255;
                    resultPtr++;
                    fgPtr++;
                    fgAlphaPtr++;
                    camImgPtr++;
                }
                camAlphaPtr += camAlphaChannelCount;
            }
            for(int j = 3*(result.cols - width); j > 0; j--) {
                *resultPtr = ((255-(*fgAlphaPtr)) * (*resultPtr) + (*fgAlphaPtr) * (*fgPtr))/255;
                resultPtr++;
                fgPtr++;
                fgAlphaPtr++;
            }
        }
        return result;
    }
#endif

}
