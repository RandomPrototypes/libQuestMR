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
        addParameter("useSingleColor", &useSingleColor);
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
        cv::Mat frameYCrCb;
		cv::cvtColor(image, frameYCrCb, cv::COLOR_BGR2YCrCb);
    	if(backgroundYCrCb.empty())
			backgroundYCrCb = frameYCrCb;
        cv::Mat &mask = _fgmask.getMatRef();
        mask.create(frameYCrCb.size(), CV_8UC1);
        if(useSingleColor) {
            for(int i = 0; i < mask.rows; i++)
            {
                unsigned char *dst = mask.ptr<unsigned char>(i);
                unsigned char *src = frameYCrCb.ptr<unsigned char>(i);
                for(int j = 0; j < mask.cols; j++) {
                    int diff = abs(src[1] - backgroundCr) + abs(src[2] - backgroundCb);
                    if(diff < hardThresh)
                        *dst = 0;
                    else if(diff > softThresh)
                        *dst = 255;
                    else *dst = (diff-hardThresh) * 255 / (softThresh-hardThresh);

                    dst++;
                    src += 3;
                }
            }
        } else {
            for(int i = 0; i < mask.rows; i++)
            {
                unsigned char *dst = mask.ptr<unsigned char>(i);
                unsigned char *src = frameYCrCb.ptr<unsigned char>(i);
                unsigned char *back = backgroundYCrCb.ptr<unsigned char>(i);
                for(int j = 0; j < mask.cols; j++) {
                    int diff = abs(src[1] - back[1]) + abs(src[2] - back[2]);
                    if(diff < hardThresh)
                        *dst = 0;
                    else if(diff > softThresh)
                        *dst = 255;
                    else *dst = (diff-hardThresh) * 255 / (softThresh-hardThresh);

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
