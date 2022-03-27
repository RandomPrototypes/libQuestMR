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

#include <libQuestMR/frame.h>
#include <libQuestMR/log.h>
#include <string.h>

namespace libQuestMR
{

class FrameCollectionPrivateData
{
public:
	std::chrono::time_point<std::chrono::system_clock> m_firstFrameTime;
	std::vector<uint8_t> m_scratchPad;
	std::list<std::shared_ptr<Frame>> m_frames;
	std::mutex m_frameMutex;

	FrameCollectionPrivateData()
	{
		m_scratchPad.reserve(16 * 1024 * 1024);
	}
};

FrameCollection::FrameCollection()
	: m_hasError(false)
{
	recordingFile = NULL;
	timestampFile = NULL;
	privateData = new FrameCollectionPrivateData();
}

FrameCollection::~FrameCollection()
{
	Reset();
	delete privateData;
}

void FrameCollection::setRecording(const char *folder, const char *filenameWithoutExt)
{
	if(folder == NULL || filenameWithoutExt == NULL || strlen(filenameWithoutExt) == 0)
	{
		if(recordingFile != NULL)
		{
			fclose(recordingFile);
			fclose(timestampFile);
			recordingFile = NULL;
			timestampFile = NULL;
		}
		return;
	}
	std::string baseFilename = folder;
	baseFilename += "/";
	baseFilename += filenameWithoutExt;
	recordingFile = fopen((baseFilename+".questMRVideo").c_str(), "wb");
	timestampFile = fopen((baseFilename+"Timestamp.txt").c_str(), "wb");
}

void FrameCollection::Reset()
{
	std::lock_guard<std::mutex> lock(privateData->m_frameMutex);

	m_hasError = false;
	privateData->m_scratchPad.clear();
	privateData->m_frames.clear();
	m_firstFrameTimeSet = false;

	if(recordingFile != NULL)
	{
		fclose(recordingFile);
		recordingFile = NULL;
	}
	if(timestampFile != NULL)
	{
		fclose(timestampFile);
		timestampFile = NULL;
	}
}

void FrameCollection::AddData(const uint8_t* data, uint32_t len)
{
	std::lock_guard<std::mutex> lock(privateData->m_frameMutex);

	if (m_hasError)
	{
		return;
	}

#if _DEBUG
	OM_LOG(LOG_DEBUG, "FrameCollection::AddData, len = %u", len);
#endif
    if(recordingFile != NULL)
		fwrite(data, len, 1, recordingFile);
	
	privateData->m_scratchPad.insert(privateData->m_scratchPad.end(), data, data + len);

	while (privateData->m_scratchPad.size() >= sizeof(FrameHeader))
	{
		const FrameHeader* frameHeader = (const FrameHeader*)privateData->m_scratchPad.data();
		if (frameHeader->Magic != Magic)
		{
			OM_LOG(LOG_ERROR, "Frame magic mismatch: expected 0x%08x get 0x%08x", Magic, frameHeader->Magic);
			m_hasError = true;
			return;
		}
		uint32_t frameLengthExcludingMagic = frameHeader->TotalDataLengthExcludingMagic;
		if (privateData->m_scratchPad.size() >= sizeof(uint32_t) + frameLengthExcludingMagic)
		{
			if (frameHeader->PayloadLength != frameHeader->TotalDataLengthExcludingMagic + sizeof(uint32_t) - sizeof(FrameHeader))
			{
				OM_LOG(LOG_ERROR, "Frame length mismatch: length %u, payload length %u", frameHeader->TotalDataLengthExcludingMagic, frameHeader->PayloadLength);
				m_hasError = true;
				return;
			}

			std::shared_ptr<Frame> frame = std::make_shared<Frame>();
			frame->m_type = (Frame::PayloadType)frameHeader->PayloadType;
			//frame->m_secondsSinceEpoch = frameHeader->SecondsSinceEpoch;
			auto first = privateData->m_scratchPad.begin() + sizeof(FrameHeader);
			auto last = first + frameHeader->PayloadLength;

			if (privateData->m_scratchPad.size() >= sizeof(uint32_t) + frameHeader->TotalDataLengthExcludingMagic + sizeof(uint32_t))
			{
				uint32_t* magic = (uint32_t*)&privateData->m_scratchPad.at(sizeof(uint32_t) + frameHeader->TotalDataLengthExcludingMagic);
				if (*magic != Magic)
				{
					OM_LOG(LOG_ERROR, "Will have magic number error in next frame: current frame type %d, frame length %d, scratchPad size %d",
						frame->m_type,
						sizeof(uint32_t) + frameHeader->TotalDataLengthExcludingMagic,
						privateData->m_scratchPad.size());
				}
			}

			frame->m_payload.assign(first, last);
			privateData->m_frames.push_back(frame);

			auto current_time = std::chrono::system_clock::now();
			auto seconds_since_epoch = std::chrono::duration<double>(current_time.time_since_epoch()).count();
            frame->localTimestamp = (uint64_t)(seconds_since_epoch*1000);

			if(timestampFile != NULL)
                fprintf(timestampFile, "%llu\n", static_cast<unsigned long long>(frame->localTimestamp));

			if (!m_firstFrameTimeSet)
			{
				m_firstFrameTimeSet = true;
				privateData->m_firstFrameTime = std::chrono::system_clock::now();
			}
#if _DEBUG
			std::chrono::duration<double> timePassed = std::chrono::system_clock::now() - privateData->m_firstFrameTime;

			static int frameIndex = 0;
			OM_LOG(LOG_DEBUG, "[%f] new frame(%d) pushed, type %u, payload %u bytes", timePassed.count(), frameIndex++, frame->m_type, frame->m_payload.size());
#endif
			privateData->m_scratchPad.erase(privateData->m_scratchPad.begin(), last);
		}
		else
		{
			break;
		}
	}
}

bool FrameCollection::HasCompletedFrame()
{
	std::lock_guard<std::mutex> lock(privateData->m_frameMutex);

	return privateData->m_frames.size() > 0;
}

std::shared_ptr<Frame> FrameCollection::PopFrame()
{
	std::lock_guard<std::mutex> lock(privateData->m_frameMutex);

	if (privateData->m_frames.size() > 0)
	{
		auto result = privateData->m_frames.front();
		privateData->m_frames.pop_front();
		return result;
	}
	else
	{
		return std::shared_ptr<Frame>(nullptr);
	}
}

double FrameCollection::GetNbTickSinceFirstFrame() const
{
	if (!m_firstFrameTimeSet)
		return 0;
	std::chrono::duration<double> timePassed = std::chrono::system_clock::now() - privateData->m_firstFrameTime;
	return timePassed.count();
}

}
