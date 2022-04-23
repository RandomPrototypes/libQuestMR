#pragma once

#include "config.h"
#include "PortableTypes.h"
#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/opencv.hpp>

namespace libQuestMR
{

enum class BackgroundSubtractorParamType
{
	ParamTypeBool,
	ParamTypeInt,
	ParamTypeDouble,
	ParamTypeString,
};

class LQMR_EXPORTS BackgroundSubtractor
{
public:
	virtual ~BackgroundSubtractor();
	virtual void apply(cv::InputArray image, cv::OutputArray fgmask, double learningRate=-1) = 0;
	
	virtual int getParameterCount() const = 0;
	virtual int getParameterId(const char *name) const = 0;
	virtual PortableString getParameterName(int id) const = 0;
	virtual BackgroundSubtractorParamType getParameterType(int id) const = 0;
	virtual PortableString getParameterVal(int id) const = 0;
	virtual bool getParameterValAsBool(int id) const = 0;
	virtual int getParameterValAsInt(int id) const = 0;
	virtual double getParameterValAsDouble(int id) const = 0;
	virtual void setParameterVal(int id, const char *val) = 0;
	virtual void setParameterVal(int id, bool val) = 0;
	virtual void setParameterVal(int id, int val) = 0;
	virtual void setParameterVal(int id, double val) = 0;
	
	virtual PortableString getParameterVal(const char *paramName) const = 0;
	virtual bool getParameterValAsBool(const char *paramName) const = 0;
	virtual int getParameterValAsInt(const char *paramName) const = 0;
	virtual double getParameterValAsDouble(const char *paramName) const = 0;
	virtual void setParameterVal(const char *paramName, const char *val) = 0;
	virtual void setParameterVal(const char *paramName, bool val) = 0;
	virtual void setParameterVal(const char *paramName, int val) = 0;
	virtual void setParameterVal(const char *paramName, double val) = 0;
};

class BackgroundSubtractorParam
{
public:
    PortableString name;
    BackgroundSubtractorParamType type;
    void *param;

    BackgroundSubtractorParam(const PortableString& name, BackgroundSubtractorParamType type, void *param)
        :name(name), type(type), param(param)
    {
    }
};

class BackgroundSubtractorBase : public BackgroundSubtractor
{
public:
    virtual ~BackgroundSubtractorBase();
	virtual void apply(cv::InputArray image, cv::OutputArray fgmask, double learningRate=-1) = 0;
	virtual int getParameterCount() const;
    virtual int getParameterId(const char *name) const;
	virtual PortableString getParameterName(int id) const;
	virtual BackgroundSubtractorParamType getParameterType(int id) const;
	virtual PortableString getParameterVal(int id) const;
	virtual bool getParameterValAsBool(int id) const;
    virtual int getParameterValAsInt(int id) const;
	virtual double getParameterValAsDouble(int id) const;
	virtual void setParameterVal(int id, const char *val);
    virtual void setParameterVal(int id, bool val);
	virtual void setParameterVal(int id, int val);
	virtual void setParameterVal(int id, double val);

    virtual PortableString getParameterVal(const char *paramName) const;
	virtual bool getParameterValAsBool(const char *paramName) const;
	virtual int getParameterValAsInt(const char *paramName) const;
	virtual double getParameterValAsDouble(const char *paramName) const;
	virtual void setParameterVal(const char *paramName, const char *val);
	virtual void setParameterVal(const char *paramName, bool val);
	virtual void setParameterVal(const char *paramName, int val);
	virtual void setParameterVal(const char *paramName, double val);

    virtual void addParameter(const char *name, bool *val);
    virtual void addParameter(const char *name, int *val);
    virtual void addParameter(const char *name, double *val);
    virtual void addParameter(const char *name, PortableString *val);

private:
    std::vector<BackgroundSubtractorParam> listParams;
};


extern "C" 
{
	LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorOpenCVRawPtr(cv::Ptr<cv::BackgroundSubtractor> pBackSub);
	LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorChromaKeyRawPtr(int _hardThresh, int _softThresh, bool _useSingleColor, int _backgroundCr, int _backgroundCb);
	LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorRobustVideoMattingONNXRawPtr(const char *onnxModelFilename, bool use_CUDA);
	LQMR_EXPORTS void deleteBackgroundSubtractorRawPtr(BackgroundSubtractor *backgroundSubtractor);
	
	LQMR_EXPORTS int getBackgroundSubtractorCount();
	LQMR_EXPORTS PortableString getBackgroundSubtractorName(int id);
	LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorRawPtr(int id);
}

inline std::shared_ptr<BackgroundSubtractor> createBackgroundSubtractorOpenCV(cv::Ptr<cv::BackgroundSubtractor> pBackSub)
{
	return std::shared_ptr<BackgroundSubtractor>(createBackgroundSubtractorOpenCVRawPtr(pBackSub), deleteBackgroundSubtractorRawPtr);
}

inline std::shared_ptr<BackgroundSubtractor> createBackgroundSubtractorChromaKey(int _hardThresh = 20, int _softThresh = 50, bool _useSingleColor = true, int _backgroundCr = 104, int _backgroundCb = 117)
{
	return std::shared_ptr<BackgroundSubtractor>(createBackgroundSubtractorChromaKeyRawPtr(_hardThresh, _softThresh, _useSingleColor, _backgroundCr, _backgroundCb), deleteBackgroundSubtractorRawPtr);
}

inline std::shared_ptr<BackgroundSubtractor> createBackgroundSubtractorRobustVideoMattingONNX(const char *onnxModelFilename, bool use_CUDA)
{
	return std::shared_ptr<BackgroundSubtractor>(createBackgroundSubtractorRobustVideoMattingONNXRawPtr(onnxModelFilename, use_CUDA), deleteBackgroundSubtractorRawPtr);
}

inline std::shared_ptr<BackgroundSubtractor> createBackgroundSubtractor(int id)
{
	return std::shared_ptr<BackgroundSubtractor>(createBackgroundSubtractorRawPtr(id), deleteBackgroundSubtractorRawPtr);
}

}

#endif