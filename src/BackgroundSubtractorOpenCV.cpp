#include <libQuestMR/BackgroundSubtractor.h>

#ifdef LIBQUESTMR_USE_OPENCV

namespace libQuestMR
{

class BackgroundSubtractorOpenCV : public BackgroundSubtractorBase
{
public:
    BackgroundSubtractorOpenCV(cv::Ptr<cv::BackgroundSubtractor> pBackSub)
    	:pBackSub(pBackSub)
    {
    }

    virtual ~BackgroundSubtractorOpenCV()
    {
    }
    
    virtual void apply(cv::InputArray image, cv::OutputArray fgmask, double learningRate=-1)
    {
    	pBackSub->apply(image, fgmask, learningRate);
    }
    
    cv::Ptr<cv::BackgroundSubtractor> pBackSub;
};

LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorOpenCVRawPtr(cv::Ptr<cv::BackgroundSubtractor> pBackSub)
{
    return new BackgroundSubtractorOpenCV(pBackSub);
}


}
#endif
