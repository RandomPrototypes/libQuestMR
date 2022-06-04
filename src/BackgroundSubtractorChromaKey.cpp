#include <libQuestMR/BackgroundSubtractor.h>

#ifdef LIBQUESTMR_USE_OPENCV

namespace libQuestMR
{


class BackgroundSubtractorChromaKey : public BackgroundSubtractorBase
{
public:
    BackgroundSubtractorChromaKey(int _hardThresh, int _softThresh, bool _useSingleColor, int _backgroundCr, int _backgroundCb)
        :hardThresh(_hardThresh), softThresh(_softThresh), useSingleColor(_useSingleColor)
    {
        addParameter("hardThresh", &hardThresh);
        addParameter("softThresh", &softThresh);
        addParameterColor("backgroundColor", &backgroundColor);
        setParameterValYCrCb("backgroundColor", 128, _backgroundCr, _backgroundCb);
        //addParameter("backgroundCr", &backgroundCr);
        //addParameter("backgroundCb", &backgroundCb);
    }

    virtual ~BackgroundSubtractorChromaKey()
    {
    }

    virtual void restart()
    {
        backgroundYCrCb = cv::Mat();
    }

    virtual void apply(cv::InputArray image, cv::OutputArray _fgmask, double learningRate=-1)
    {
        unsigned char y, cr, cb;
        getParameterValAsYCrCb("backgroundColor", &y, &cr, &cb);
        int backgroundCr = cr;
        int backgroundCb = cb;
        if(softThresh <= hardThresh)
            softThresh = hardThresh + 1;
        int softThresh2 = softThresh*softThresh;
        int hardThresh2 = hardThresh*hardThresh;
        cv::Mat frameYCrCb;
		cv::cvtColor(image, frameYCrCb, cv::COLOR_BGR2YCrCb);
    	if(backgroundYCrCb.empty())
			backgroundYCrCb = frameYCrCb;
        cv::Mat &mask = _fgmask.getMatRef();
        mask.create(frameYCrCb.size(), CV_8UC1);
        cv::Rect ROI2 = getROI();
        if(ROI2.empty())
            ROI2 = cv::Rect(0,0,mask.cols,mask.rows);
        if(ROI2.size() != mask.size())
            mask.setTo(cv::Scalar(0));
        if(useSingleColor) {
            for(int i = ROI2.y; i < ROI2.y + ROI2.height; i++)
            {
                unsigned char *dst = mask.ptr<unsigned char>(i) + ROI2.x;
                unsigned char *src = frameYCrCb.ptr<unsigned char>(i) + ROI2.x * 3;
                for(int j = 0; j < ROI2.width; j++) {
                    int diff_cr = (src[1] - backgroundCr);
                    int diff_cb = (src[2] - backgroundCb);
                    int diff2 = diff_cr*diff_cr + diff_cb*diff_cb;
                    if(diff2 < hardThresh2)
                        *dst = 0;
                    else if(diff2 > softThresh2)
                        *dst = 255;
                    else *dst = (sqrt(diff2)-hardThresh) * 255 / (softThresh-hardThresh);

                    dst++;
                    src += 3;
                }
            }
        } else {
            for(int i = ROI2.y; i < ROI2.y + ROI2.height; i++)
            {
                unsigned char *dst = mask.ptr<unsigned char>(i) + ROI2.x;
                unsigned char *src = frameYCrCb.ptr<unsigned char>(i) + ROI2.x * 3;
                unsigned char *back = backgroundYCrCb.ptr<unsigned char>(i) + ROI2.x * 3;
                for(int j = 0; j < ROI2.width; j++) {
                    int diff_cr = (src[1] - back[1]);
                    int diff_cb = (src[2] - back[2]);
                    int diff2 = diff_cr*diff_cr + diff_cb*diff_cb;
                    if(diff2 < hardThresh2)
                        *dst = 0;
                    else if(diff2 > softThresh2)
                        *dst = 255;
                    else *dst = (sqrt(diff2)-hardThresh) * 255 / (softThresh-hardThresh);

                    dst++;
                    src += 3;
                    back += 3;
                }
            }
        }
    }

    int hardThresh, softThresh;
    bool useSingleColor;
    unsigned int backgroundColor;
    //int backgroundCr, backgroundCb;
    cv::Mat backgroundYCrCb;
};

LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorChromaKeyRawPtr(int _hardThresh, int _softThresh, bool _useSingleColor, int _backgroundCr, int _backgroundCb)
{
    return new BackgroundSubtractorChromaKey(_hardThresh, _softThresh, _useSingleColor, _backgroundCr, _backgroundCb);
}

}
#endif
