#include <libQuestMR/BackgroundSubtractor.h>

#ifdef LIBQUESTMR_USE_OPENCV

namespace libQuestMR
{

BackgroundSubtractor::~BackgroundSubtractor()
{
}

BackgroundSubtractorBase::~BackgroundSubtractorBase()
{
}

int BackgroundSubtractorBase::getParameterCount() const
{
    return listParams.size();
}
int BackgroundSubtractorBase::getParameterId(const char *name) const
{
    for(size_t i = 0; i < listParams.size(); i++) {
        if(listParams[i].name.str() == name)
            return i;
    }
    return -1;
}
PortableString BackgroundSubtractorBase::getParameterName(int id) const
{
    return listParams[id].name;
}
BackgroundSubtractorParamType BackgroundSubtractorBase::getParameterType(int id) const
{
    return listParams[id].type;
}

std::string toHexString(unsigned int val, int length = -1)
{
    std::string result;
    while(val > 0) {
        unsigned char tmp = val & 0xF;
        if(tmp >= 10)
            result.insert(0, 1, 'A'+tmp-10);
        else result.insert(0, 1, '0'+tmp);
        val = (val >> 4);
    }
    while(result.size() < length)
        result.insert(0, 1, '0');
    return result;
}

unsigned int hexToUInt(const char *val, int length = -1)
{
    unsigned int result = 0;
    int i = 0;
    while((length < 0 || i < length) && val[i] != '\0')
    {
        if(val[i] >= 'A' && val[i] <= 'F') {
            result = (result<<4) + (val[i]-'A'+10);
        } else if(val[i] >= 'a' && val[i] <= 'f') {
            result = (result<<4) + (val[i]-'a'+10);
        } else if(val[i] >= '0' && val[i] <= '9') {
            result = (result<<4) + (val[i]-'0');
        } else {
            break;
        }
    }
    return result;
}

PortableString BackgroundSubtractorBase::getParameterVal(int id) const
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            return toPortableString((*(bool*)listParams[id].param) ? "true" : "false");
        case BackgroundSubtractorParamType::ParamTypeInt:
            return toPortableString(std::to_string(*(int*)listParams[id].param));
        case BackgroundSubtractorParamType::ParamTypeDouble:
            return toPortableString(std::to_string(*(double*)listParams[id].param));
        case BackgroundSubtractorParamType::ParamTypeString:
            return *(PortableString*)listParams[id].param;
        case BackgroundSubtractorParamType::ParamTypeColor:
            unsigned char r,g,b;
            getParameterValAsRGB(id, &r, &g, &b);
            return toPortableString("#"+toHexString(r,2)+toHexString(g,2)+toHexString(b,2));
    }
    return "";
}



bool BackgroundSubtractorBase::getParameterValAsBool(int id) const
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            return (*(bool*)listParams[id].param);
        case BackgroundSubtractorParamType::ParamTypeInt:
            return (*(int*)listParams[id].param) > 0;
        case BackgroundSubtractorParamType::ParamTypeDouble:
            return cvRound(*(double*)listParams[id].param) > 0;
        default:
            printf("BackgroundSubtractor::getParameterValAsInt wrong type\n");
            return false;
    }
}
int BackgroundSubtractorBase::getParameterValAsInt(int id) const
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            return (*(bool*)listParams[id].param) ? 1 : 0;
        case BackgroundSubtractorParamType::ParamTypeInt:
            return *(int*)listParams[id].param;
        case BackgroundSubtractorParamType::ParamTypeDouble:
            return cvRound(*(double*)listParams[id].param);
        default:
            printf("BackgroundSubtractor::getParameterValAsInt wrong type\n");
            return 0;
    }
}
double BackgroundSubtractorBase::getParameterValAsDouble(int id) const
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            return (*(bool*)listParams[id].param) ? 1 : 0;
        case BackgroundSubtractorParamType::ParamTypeInt:
            return *(int*)listParams[id].param;
        case BackgroundSubtractorParamType::ParamTypeDouble:
            return (*(double*)listParams[id].param);
        default:
            printf("BackgroundSubtractor::getParameterValAsDouble wrong type\n");
            return 0;
    }
}
bool BackgroundSubtractorBase::getParameterValAsRGB(int id, unsigned char *r, unsigned char *g, unsigned char *b) const
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeColor:
            *b = (*(unsigned int*)listParams[id].param) & 0xFF;
            *g = ((*(unsigned int*)listParams[id].param)>>8) & 0xFF;
            *r = ((*(unsigned int*)listParams[id].param)>>16) & 0xFF;
            return true;
        default:
            printf("BackgroundSubtractor::getParameterValAsRGB wrong type\n");
            return false;
    }
}

bool BackgroundSubtractorBase::getParameterValAsYCrCb(int id, unsigned char *y, unsigned char *cr, unsigned char *cb) const
{
    unsigned char r,g,b;
    bool ret = getParameterValAsRGB(id, &r, &g, &b);
    float delta = 128;
    float y_float = 0.299 * r + 0.587 * g + 0.114 * b;
    *y = cv::saturate_cast<unsigned char>(cvRound(y_float));
    *cr = cv::saturate_cast<unsigned char>(cvRound((r-y_float) * 0.713 + delta));
    *cb = cv::saturate_cast<unsigned char>(cvRound((b-y_float) * 0.564 + delta));
    return ret;
}
void BackgroundSubtractorBase::setParameterVal(int id, const char *val)
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            if(!strcmp(val, "true")) {
                (*(bool*)listParams[id].param) = true;
            } else if(!strcmp(val, "false")) {
                (*(bool*)listParams[id].param) = false;
            } else {
                (*(bool*)listParams[id].param) = (std::stoi(val) > 0);
            }
            break;
        case BackgroundSubtractorParamType::ParamTypeInt:
            (*(int*)listParams[id].param) = std::stoi(val);
            break;
        case BackgroundSubtractorParamType::ParamTypeDouble:
            (*(double*)listParams[id].param) = std::stod(val);
            break;
        case BackgroundSubtractorParamType::ParamTypeString:
            (*(PortableString*)listParams[id].param) = val;
            break;
        case BackgroundSubtractorParamType::ParamTypeColor:
            if(val[0] == '#') {
                unsigned int rgb = hexToUInt(val+1, 6);
                setParameterValRGB(id, (rgb>>16) & 0xFF, (rgb>>8) & 0xFF, rgb & 0xFF);
            }
            break;
    }
}
void BackgroundSubtractorBase::setParameterVal(int id, bool val)
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            (*(bool*)listParams[id].param) = val;
            break;
        case BackgroundSubtractorParamType::ParamTypeInt:
            (*(int*)listParams[id].param) = val ? 1 : 0;
            break;
        case BackgroundSubtractorParamType::ParamTypeDouble:
            (*(double*)listParams[id].param) = val ? 1 : 0;
            break;
        case BackgroundSubtractorParamType::ParamTypeString:
            (*(PortableString*)listParams[id].param) = toPortableString(val ? "true" : "false");
            break;
    }
}
void BackgroundSubtractorBase::setParameterVal(int id, int val)
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            (*(bool*)listParams[id].param) = (val > 0);
            break;
        case BackgroundSubtractorParamType::ParamTypeInt:
            (*(int*)listParams[id].param) = val;
            break;
        case BackgroundSubtractorParamType::ParamTypeDouble:
            (*(double*)listParams[id].param) = val;
            break;
        case BackgroundSubtractorParamType::ParamTypeString:
            (*(PortableString*)listParams[id].param) = toPortableString(std::to_string(val));
            break;
    }
}
void BackgroundSubtractorBase::setParameterVal(int id, double val)
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeBool:
            (*(bool*)listParams[id].param) = (val > 0);
            break;
        case BackgroundSubtractorParamType::ParamTypeInt:
            (*(int*)listParams[id].param) = cvRound(val);
            break;
        case BackgroundSubtractorParamType::ParamTypeDouble:
            (*(double*)listParams[id].param) = val;
            break;
        case BackgroundSubtractorParamType::ParamTypeString:
            (*(PortableString*)listParams[id].param) = toPortableString(std::to_string(val));
            break;
    }
}

void BackgroundSubtractorBase::setParameterValRGB(int id, unsigned char r, unsigned char g, unsigned char b)
{
    switch(listParams[id].type)
    {
        case BackgroundSubtractorParamType::ParamTypeColor:
            (*(unsigned int*)listParams[id].param) = (r<<16 | g<<8 | b);
            break;
    }
}
void BackgroundSubtractorBase::setParameterValYCrCb(int id, unsigned char y, unsigned char cr, unsigned char cb)
{
    int delta = 128;
    unsigned char r = cv::saturate_cast<unsigned char>(cvRound(y + 1.403 * (cr -delta)));
    unsigned char g = cv::saturate_cast<unsigned char>(cvRound(y - 0.714*(cr-delta) - 0.344*(cb-delta)));
    unsigned char b = cv::saturate_cast<unsigned char>(cvRound(y + 1.773*(cb-delta)));
    setParameterValRGB(id, r, g, b);
}


PortableString BackgroundSubtractorBase::getParameterVal(const char *paramName) const
{
    return getParameterVal(getParameterId(paramName));
}
bool BackgroundSubtractorBase::getParameterValAsBool(const char *paramName) const
{
    return getParameterValAsBool(getParameterId(paramName));
}
int BackgroundSubtractorBase::getParameterValAsInt(const char *paramName) const
{
    return getParameterValAsInt(getParameterId(paramName));
}
double BackgroundSubtractorBase::getParameterValAsDouble(const char *paramName) const
{
    return getParameterValAsDouble(getParameterId(paramName));
}
bool BackgroundSubtractorBase::getParameterValAsRGB(const char *paramName, unsigned char *r, unsigned char *g, unsigned char *b) const
{
    return getParameterValAsRGB(getParameterId(paramName), r, g, b);
}
bool BackgroundSubtractorBase::getParameterValAsYCrCb(const char *paramName, unsigned char *y, unsigned char *cr, unsigned char *cb) const
{
    return getParameterValAsYCrCb(getParameterId(paramName), y, cr, cb);
}
void BackgroundSubtractorBase::setParameterVal(const char *paramName, const char *val)
{
    setParameterVal(getParameterId(paramName), val);
}
void BackgroundSubtractorBase::setParameterVal(const char *paramName, bool val)
{
    setParameterVal(getParameterId(paramName), val);
}
void BackgroundSubtractorBase::setParameterVal(const char *paramName, int val)
{
    setParameterVal(getParameterId(paramName), val);
}
void BackgroundSubtractorBase::setParameterVal(const char *paramName, double val)
{
    setParameterVal(getParameterId(paramName), val);
}
void BackgroundSubtractorBase::setParameterValRGB(const char *paramName, unsigned char r, unsigned char g, unsigned char b)
{
    setParameterValRGB(getParameterId(paramName), r, g, b);
}
void BackgroundSubtractorBase::setParameterValYCrCb(const char *paramName, unsigned char y, unsigned char cr, unsigned char cb)
{
    setParameterValYCrCb(getParameterId(paramName), y, cr, cb);
}



void BackgroundSubtractorBase::addParameter(const char *name, bool *val)
{
    listParams.push_back(BackgroundSubtractorParam(name, BackgroundSubtractorParamType::ParamTypeBool, val));
}
void BackgroundSubtractorBase::addParameter(const char *name, int *val)
{
    listParams.push_back(BackgroundSubtractorParam(name, BackgroundSubtractorParamType::ParamTypeInt, val));
}
void BackgroundSubtractorBase::addParameter(const char *name, double *val)
{
    listParams.push_back(BackgroundSubtractorParam(name, BackgroundSubtractorParamType::ParamTypeDouble, val));
}
void BackgroundSubtractorBase::addParameter(const char *name, PortableString *val)
{
    listParams.push_back(BackgroundSubtractorParam(name, BackgroundSubtractorParamType::ParamTypeString, val));
}
void BackgroundSubtractorBase::addParameterColor(const char *name, unsigned int *val)
{
    listParams.push_back(BackgroundSubtractorParam(name, BackgroundSubtractorParamType::ParamTypeColor, val));
}


LQMR_EXPORTS void deleteBackgroundSubtractorRawPtr(BackgroundSubtractor *backgroundSubtractor)
{
    delete backgroundSubtractor;
}


typedef BackgroundSubtractor *(*BackgroundSubtractorFnPtr)();

std::vector<std::pair<std::string, BackgroundSubtractorFnPtr> > getBackgroundSubtractorList()
{
    std::vector<std::pair<std::string, BackgroundSubtractorFnPtr> > list;
    list.push_back(std::make_pair("OpenCV_KNN", [](){ return createBackgroundSubtractorOpenCVRawPtr(cv::createBackgroundSubtractorKNN());}));
    list.push_back(std::make_pair("OpenCV_MOG2", [](){ return createBackgroundSubtractorOpenCVRawPtr(cv::createBackgroundSubtractorMOG2());}));
    list.push_back(std::make_pair("Greenscreen", [](){ return createBackgroundSubtractorChromaKeyRawPtr(22, 35, true, 104, 117);}));
    list.push_back(std::make_pair("ChromaKey_diffFirstFrame", [](){ return createBackgroundSubtractorChromaKeyRawPtr(22, 35, false, 0, 0);}));
    list.push_back(std::make_pair("ONNX_RobustVideoMatting", [](){ return createBackgroundSubtractorRobustVideoMattingONNXRawPtr("rvm_mobilenetv3_fp32.onnx", false);}));
    list.push_back(std::make_pair("ONNX_RobustVideoMatting_CUDA", [](){ return createBackgroundSubtractorRobustVideoMattingONNXRawPtr("rvm_mobilenetv3_fp32.onnx", true);}));
    return list;
}

LQMR_EXPORTS int getBackgroundSubtractorCount()
{
    return getBackgroundSubtractorList().size();
}
LQMR_EXPORTS PortableString getBackgroundSubtractorName(int id)
{
    auto list = getBackgroundSubtractorList();
    if(id < 0 || id >= list.size())
        return "";
    return toPortableString(list[id].first);
}
LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorRawPtr(int id)
{
    auto list = getBackgroundSubtractorList();
    if(id < 0 || id >= list.size())
        return NULL;
    return list[id].second();
}

}
#endif