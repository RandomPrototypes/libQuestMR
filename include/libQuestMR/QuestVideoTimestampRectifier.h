#pragma once

#include <libQuestMR/config.h>

namespace libQuestMR
{

LQMR_EXPORTS std::vector<uint64_t> rectifyTimestamps(const std::vector<uint64_t>& listTimestamp, const std::vector<uint32_t>& listType);
LQMR_EXPORTS std::vector<uint64_t> rectifyTimestamps(const std::vector<uint64_t>& listTimestamp,
													 const std::vector<uint32_t>& listType, 
													 const std::vector<uint32_t>& listDataLength, 
													 const std::vector<std::vector<std::string> >& listExtraData);


LQMR_EXPORTS void rectifyTimestamps(const char *filename, const char *outputFilename);



}
