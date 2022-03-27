/*
 * Based on https://github.com/facebookincubator/obs-plugins
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
#include <libQuestMR/log.h>
#include <fcntl.h>

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


QuestVideoMngr::QuestVideoMngr()
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
}


void QuestVideoMngr::setRecording(const char *folder, const char *filenameWithoutExt)
{
    m_frameCollection.setRecording(folder, filenameWithoutExt);
}

void QuestVideoMngr::StartDecoder()
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

void QuestVideoMngr::StopDecoder()
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

void QuestVideoMngr::ReceiveData()
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
                int iResult = videoSource->recv((char*)buf, bufferSize);
                if (iResult < 0)
                {
                    OM_BLOG(LOG_ERROR, "recv error %d, closing socket", iResult);
                    detachSource();
                }
                else if (iResult == 0)
                {
                    OM_BLOG(LOG_INFO, "recv 0 bytes, closing socket");
                    detachSource();
                }
                else
                {
                    OM_BLOG(LOG_INFO, "recv: %d bytes received", iResult);
                    m_frameCollection.AddData(buf, iResult);
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

void QuestVideoMngr::setRecordedTimestampSource(const char *filename)
{
    recordedTimestampFile = fopen(filename, "r");
}

void QuestVideoMngr::setVideoDecoding(bool videoDecoding)
{
	this->videoDecoding = videoDecoding;
}

void QuestVideoMngr::VideoTickImpl(bool skipOldFrames)
{
    if (videoSource != NULL && videoSource->isValid())
    {
        if(!m_frameCollection.HasCompletedFrame())
            ReceiveData();

        if (!videoSource->isValid())	// socket disconnected
            return;

        //std::chrono::time_point<std::chrono::system_clock> startTime = std::chrono::system_clock::now();
        while (m_frameCollection.HasCompletedFrame())
        {
            //std::chrono::duration<double> timePassed = std::chrono::system_clock::now() - startTime;
            //if (timePassed.count() > 0.05)
            //	break;

            auto frame = m_frameCollection.PopFrame();
            if(recordedTimestampFile != NULL){
                unsigned long long tmp;
                fscanf(recordedTimestampFile, "%llu\n", &tmp);
                frame->localTimestamp = tmp;
            }

            //auto current_time = std::chrono::system_clock::now();
            //auto seconds_since_epoch = std::chrono::duration<double>(current_time.time_since_epoch()).count();
            //double latency = seconds_since_epoch - frame->m_secondsSinceEpoch;

            if (frame->m_type == Frame::PayloadType::VIDEO_DIMENSION)
            {
                struct FrameDimension
                {
                    int w;
                    int h;
                };
                const FrameDimension* dim = (const FrameDimension*)frame->m_payload.data();
                m_width = dim->w;
                m_height = dim->h;

                OM_BLOG(LOG_INFO, "[VIDEO_DIMENSION] width %d height %d", m_width, m_height);
            }
            else if (frame->m_type == Frame::PayloadType::VIDEO_DATA)
            {
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

		                    while (m_cachedAudioFrames.size() > 0 && m_cachedAudioFrames[0].first <= m_videoFrameIndex)
		                    {
		                        std::shared_ptr<Frame> audioFrame = m_cachedAudioFrames[0].second;

		                        struct AudioDataHeader {
		                            uint64_t timestamp;
		                            int channels;
		                            int dataLength;
		                        };
		                        AudioDataHeader* audioDataHeader = (AudioDataHeader*)(audioFrame->m_payload.data());

		                        if (audioDataHeader->channels == 1 || audioDataHeader->channels == 2)
		                        {
		                            /*obs_source_audio audio = { 0 };
		                            audio.data[0] = (uint8_t*)audioFrame->m_payload.data() + sizeof(AudioDataHeader);
		                            audio.frames = audioDataHeader->dataLength / sizeof(float) / audioDataHeader->channels;
		                            audio.speakers = audioDataHeader->channels == 1 ? SPEAKERS_MONO : SPEAKERS_STEREO;
		                            audio.format = AUDIO_FORMAT_FLOAT;
		                            audio.samples_per_sec = m_audioSampleRate;
		                            audio.timestamp = audioDataHeader->timestamp;
		                            obs_source_output_audio(m_src, &audio);*/
		                        }
		                        else
		                        {
		                            OM_BLOG(LOG_ERROR, "[AUDIO_DATA] unimplemented audio channels %d", audioDataHeader->channels);
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

                if(skipOldFrames)
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

void QuestVideoMngr::attachSource(QuestVideoSource *videoSource)
{
    this->videoSource = videoSource;
    m_frameCollection.Reset();

    m_audioFrameIndex = 0;
    m_videoFrameIndex = 0;
    m_cachedAudioFrames.clear();

    if (videoSource->isValid())
        StartDecoder();
}

void QuestVideoMngr::detachSource()
{
    if (videoSource == NULL || !videoSource->isValid())
    {
        OM_BLOG(LOG_ERROR, "Not connected");
        return;
    }

    StopDecoder();

    videoSource = NULL;
}


QuestVideoSourceFile::~QuestVideoSourceFile()
{
    close();
}

void QuestVideoSourceFile::open(const char *filename)
{
    if (file != NULL)
    {
        OM_BLOG(LOG_ERROR, "Already opened");
        return;
    }

    file = fopen(filename, "rb");
}

int QuestVideoSourceFile::recv(char *buf, size_t bufferSize)
{
    return static_cast<int>(::fread(buf, 1, bufferSize, file));
}

bool QuestVideoSourceFile::isValid()
{
    return file != NULL && !feof(file);
}

void QuestVideoSourceFile::close()
{
    if(file != NULL)
        fclose(file);
    file = NULL;
}







QuestVideoSourceBufferedSocket::~QuestVideoSourceBufferedSocket()
{
    Disconnect();
}


bool QuestVideoSourceBufferedSocket::Connect(std::string ipaddr, uint32_t port)
{
    return m_connectSocket.connect(ipaddr, port);
}

int QuestVideoSourceBufferedSocket::recv(char *buf, size_t bufferSize)
{
    return m_connectSocket.readData(buf, bufferSize);
}

bool QuestVideoSourceBufferedSocket::isValid()
{
    return m_connectSocket.isConnected();
}

void QuestVideoSourceBufferedSocket::Disconnect()
{
    m_connectSocket.disconnect();
}

}
