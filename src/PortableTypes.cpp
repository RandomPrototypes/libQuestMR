#include <libQuestMR/PortableTypes.h>

namespace libQuestMR
{

PortableString::PortableString()
{
    data = (void*)new std::string();
}

PortableString::~PortableString()
{
    delete (std::string*)data;
}

PortableString::PortableString(const PortableString& other)
{
    std::string *str = new std::string();
    std::string *otherStr = static_cast<std::string*>(other.data);
    *str = *otherStr;
    data = (void*)str;
}

PortableString& PortableString::operator=(const PortableString& other)
{
    std::string *str = static_cast<std::string*>(data);
    std::string *otherStr = static_cast<std::string*>(other.data);
    *str = *otherStr;
    return *this;
}

PortableString::PortableString(const char *str, size_t size)
{
    data = (void*)new std::string();
    ((std::string*)data)->assign(str, size);
}

PortableString::PortableString(const char *str)
{
    data = (void*)new std::string();
    ((std::string*)data)->assign(str, strlen(str));
}

const char *PortableString::c_str() const
{
    return ((std::string*)data)->c_str();
}

size_t PortableString::size() const
{
    return ((std::string*)data)->size();
}

}