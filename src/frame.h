/*

Source code from Facebook ( https://github.com/facebookincubator/obs-plugins ), modified and imported into libQuestMR


Copyright (C) 2019-present, Facebook, Inc.
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <libQuestMR/config.h>

#include <stdint.h>
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <cassert>

namespace libQuestMR
{

struct FrameHeader
{
	uint32_t Magic;
	uint32_t TotalDataLengthExcludingMagic;
	//double SecondsSinceEpoch;
	uint32_t PayloadType;
	uint32_t PayloadLength;
};

struct Frame
{
	enum class PayloadType : uint32_t {
		VIDEO_DIMENSION = 10,
		VIDEO_DATA = 11,
		AUDIO_SAMPLERATE = 12,
		AUDIO_DATA = 13,
	};
	PayloadType m_type;
	//double m_secondsSinceEpoch;
    uint64_t localTimestamp;
	std::vector<uint8_t> m_payload;
};

//typedef std::vector<uint8_t> Frame;

class FrameCollectionPrivateData;

class FrameCollection
{
public:
	FrameCollection();
	~FrameCollection();
	FrameCollection(FrameCollection const&) = delete;
    FrameCollection& operator=(FrameCollection const&) = delete;
    
    FrameHeader readFrameHeader(unsigned char *data) const;

	void Reset();

	void setRecording(const char *folder, const char *filenameWithoutExt);

	void setRecordedTimestamp(const std::vector<uint64_t>& listTimestamp);

	void AddData(const uint8_t* data, uint32_t len, uint64_t recv_timestamp);

	bool HasCompletedFrame();

	std::shared_ptr<Frame> PopFrame();

	bool HasError() const
	{
		return m_hasError;
	}

	bool HasFirstFrame() const
	{
		return m_firstFrameTimeSet && !m_hasError;
	}

	double GetNbTickSinceFirstFrame() const;

private:
	uint32_t Magic = 0x2877AF94;

	bool m_firstFrameTimeSet = false;

	bool m_hasError;

	FILE *recordingFile;
	FILE *timestampFile;

	std::vector<uint64_t> recordedTimestamp;
    int recordedTimestampId;

	std::chrono::time_point<std::chrono::system_clock> m_firstFrameTime;
	std::vector<uint8_t> m_scratchPad;
	std::list<std::shared_ptr<Frame>> m_frames;
	std::mutex m_frameMutex;
};

}
