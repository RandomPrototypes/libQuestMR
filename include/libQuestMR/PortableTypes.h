#pragma once

#include <libQuestMR/config.h>
#include <string>

/**
 * Classes that encapsulate STL to prevent trouble when exporting to dll
 */

namespace libQuestMR
{

class LQMR_EXPORTS PortableString
{
public:
    void *data;

    PortableString();
    ~PortableString();
    PortableString(const PortableString& other);
    PortableString& operator=(const PortableString& other);

    PortableString(const char *str, size_t size);
    PortableString(const char *str);

    const char *c_str() const;

    size_t size() const;
    
    inline std::string str() const
    {
    	std::string result;
		result.assign(c_str(), size());
		return result;
    }
};

inline PortableString toPortableString(const std::string& str)
{
    return PortableString(str.c_str(), str.size());
}


}//libQuestMR
