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
        needReset = true;
    }

    virtual ~BackgroundSubtractorOpenCV()
    {
    }

    virtual void restart()
    {
        needReset = true;
    }
    
    virtual void apply(cv::InputArray image, cv::OutputArray fgmask, double learningRate=-1)
    {
    	pBackSub->apply(image, fgmask, needReset ? 1:learningRate);
        needReset = false;
    }
    
    cv::Ptr<cv::BackgroundSubtractor> pBackSub;
    bool needReset;
};

LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorOpenCVRawPtr(cv::Ptr<cv::BackgroundSubtractor> pBackSub)
{
    return new BackgroundSubtractorOpenCV(pBackSub);
}


}
#endif
