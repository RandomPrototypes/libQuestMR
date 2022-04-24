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