#include "QuestVideoMngr.h"
#include <fcntl.h>
#include "log.h"

std::string GetAvErrorString(int errNum)
{
	char buf[1024];
	std::string result = av_make_error_string(buf, 1024, errNum);
	return result;
}



QuestVideoMngr::QuestVideoMngr()
{
    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec)
    {
        OM_BLOG(LOG_ERROR, "Unable to find decoder");
    }
    else
    {
        OM_BLOG(LOG_INFO, "Codec found. Capabilities 0x%x", m_codec->capabilities);
    }
}


void QuestVideoMngr::setRecording(const char *folder, const char *filenameWithoutExt)
{
    m_frameCollection.setRecording(folder, filenameWithoutExt);
}

void QuestVideoMngr::StartDecoder()
{
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
}

void QuestVideoMngr::StopDecoder()
{
    if (m_codecContext)
    {
        avcodec_close(m_codecContext);
        avcodec_free_context(&m_codecContext);
        OM_BLOG(LOG_INFO, "m_codecContext freed");
    }

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
                printf("data\n");
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
            if(recordedTimestampFile != NULL)
                fscanf(recordedTimestampFile, "%llu\n", &(frame->localTimestamp));

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
                        std::chrono::duration<double> timePassed = std::chrono::system_clock::now() - m_frameCollection.GetFirstFrameTime();
                        OM_BLOG(LOG_DEBUG, "[%f][VIDEO_DATA] size %d width %d height %d format %d", timePassed.count(), packet->size, picture->width, picture->height, picture->format);
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
                std::chrono::duration<double> timePassed = std::chrono::system_clock::now() - m_frameCollection.GetFirstFrameTime();
                OM_BLOG(LOG_DEBUG, "[%f][AUDIO_DATA] timestamp %llu", timePassed.count());
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



QuestVideoSourceSocket::~QuestVideoSourceSocket()
{
    Disconnect();
}


void QuestVideoSourceSocket::Connect()
{
    if (m_connectSocket != INVALID_SOCKET)
    {
        OM_BLOG(LOG_ERROR, "Already connected");
        return;
    }

    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints = { 0 };

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int iResult;
    iResult = getaddrinfo(m_ipaddr.c_str(), std::to_string(m_port).c_str(), &hints, &result);
    if (iResult != 0)
    {
        OM_BLOG(LOG_ERROR, "getaddrinfo failed: %d", iResult);
    }

    ptr = result;
    m_connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (m_connectSocket == INVALID_SOCKET)
    {
        OM_BLOG(LOG_ERROR, "Error at socket(): ");//, WSAGetLastError());
        freeaddrinfo(result);
    }

    if (m_connectSocket != INVALID_SOCKET)
    {
        // put socked in non-blocking mode...
        u_long block = 1;
        /*if (ioctlsocket(m_connectSocket, FIONBIO, &block) == SOCKET_ERROR)
        {
            OM_BLOG(LOG_ERROR, "Unable to put socket to unblocked mode");
        }*/

        iResult = connect(m_connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

        //fcntl(m_connectSocket, F_SETFL, fcntl(m_connectSocket, F_GETFL, 0) | O_NONBLOCK);

        bool hasError = true;
        if (iResult == SOCKET_ERROR)
        {
            printf("iResult == SOCKET_ERROR\n");
            if (WSAGetLastError() == WSAEWOULDBLOCK)
            {
                fd_set setW, setE;

                FD_ZERO(&setW);
                FD_SET(m_connectSocket, &setW);
                FD_ZERO(&setE);
                FD_SET(m_connectSocket, &setE);

                timeval time_out = { 0 };
                time_out.tv_sec = 2;
                time_out.tv_usec = 0;

                int ret = select(0, NULL, &setW, &setE, &time_out);
                if (ret > 0 && !FD_ISSET(m_connectSocket, &setE))
                {
                    hasError = false;
                }
            }
        } else {
            printf("iResult != SOCKET_ERROR\n");
            hasError = false;
        }

        if (hasError)
        {
            OM_BLOG(LOG_ERROR, "Unable to connect");
            printf("Please verify the Quest IP address, and if MRC-enabled game is running on Quest.\n\nReboot the headset and re-launch the game if the issue remains.\n");
            closesocket(m_connectSocket);
            m_connectSocket = INVALID_SOCKET;
        }
        else
        {
            OM_BLOG(LOG_INFO, "Socket connected to %s:%d", m_ipaddr.c_str(), m_port);
            block = 0;
            if (ioctlsocket(m_connectSocket, FIONBIO, &block) == SOCKET_ERROR)
            {
                OM_BLOG(LOG_ERROR, "Unable to put socket to blocked mode");
            }
        }
    }

    freeaddrinfo(result);
}

ssize_t QuestVideoSourceSocket::recv(char *buf, size_t bufferSize)
{
    return ::recv(m_connectSocket, buf, bufferSize, 0);
}

bool QuestVideoSourceSocket::isValid()
{
    return m_connectSocket != INVALID_SOCKET;
}

void QuestVideoSourceSocket::Disconnect()
{
    int ret = closesocket(m_connectSocket);
    if (ret == INVALID_SOCKET)
    {
        //OM_BLOG(LOG_ERROR, "closesocket error %d", WSAGetLastError());
        OM_BLOG(LOG_ERROR, "closesocket error");
    }
    m_connectSocket = INVALID_SOCKET;
    OM_BLOG(LOG_INFO, "Socket disconnected");
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

ssize_t QuestVideoSourceFile::recv(char *buf, size_t bufferSize)
{
    return ::fread(buf, 1, bufferSize, file);
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