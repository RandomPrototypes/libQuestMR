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
        cv::Mat img = image.getMat();
        cv::Mat &mask = fgmask.getMatRef();
        mask.create(image.size(), CV_8UC1);
        cv::Rect ROI2 = getROI();
        if(ROI2.empty())
            ROI2 = cv::Rect(0,0,mask.cols,mask.rows);
        if(ROI2.size() != mask.size())
            mask.setTo(cv::Scalar(0));
    	pBackSub->apply(img(ROI2), mask(ROI2), needReset ? 1:learningRate);
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
