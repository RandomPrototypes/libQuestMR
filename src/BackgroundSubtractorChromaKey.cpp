#include <libQuestMR/BackgroundSubtractor.h>

#ifdef LIBQUESTMR_USE_OPENCV

namespace libQuestMR
{


class BackgroundSubtractorChromaKey : public BackgroundSubtractorBase
{
public:
    BackgroundSubtractorChromaKey(int _hardThresh, int _softThresh, bool _useSingleColor, bool _usrYCrCb, int _backgroundCol1, int _backgroundCol2, int _backgroundCol3)
        :hardThresh(_hardThresh), softThresh(_softThresh), useSingleColor(_useSingleColor), useYCrCb(_usrYCrCb)
    {
        addParameter("hardThresh", &hardThresh);
        addParameter("softThresh", &softThresh);
        addParameterColor("backgroundColor", &backgroundColor);
        if(_useSingleColor) {
            if(_usrYCrCb) {
                setParameterValYCrCb("backgroundColor", _backgroundCol1, _backgroundCol2, _backgroundCol3);
            } else {
                setParameterValRGB("backgroundColor", _backgroundCol1, _backgroundCol2, _backgroundCol3);
            }
        }
        //addParameter("backgroundCr", &backgroundCr);
        //addParameter("backgroundCb", &backgroundCb);
    }

    virtual ~BackgroundSubtractorChromaKey()
    {
    }

    virtual void restart()
    {
        backgroundImg = cv::Mat();
    }

    template<bool singleColor, bool ycrcb>
    void process(cv::Mat &mask, const cv::Mat& frame, int backgroundCol1, int backgroundCol2, int backgroundCol3, cv::Rect ROI2)
    {
        int softThresh2 = softThresh*softThresh;
        int hardThresh2 = hardThresh*hardThresh;
        unsigned char *back = NULL;
        for(int i = ROI2.y; i < ROI2.y + ROI2.height; i++)
        {
            unsigned char *dst = mask.ptr<unsigned char>(i) + ROI2.x;
            const unsigned char *src = frame.ptr<unsigned char>(i) + ROI2.x * 3;
            if(!singleColor)
                back = backgroundImg.ptr<unsigned char>(i) + ROI2.x * 3;
            for(int j = 0; j < ROI2.width; j++) {
                int diff_col1, diff_col2, diff_col3;
                if(singleColor) {
                    diff_col1 = ycrcb ? 0 : (src[0] - backgroundCol1);
                    diff_col2 = (src[1] - backgroundCol2);
                    diff_col3 = (src[2] - backgroundCol3);
                } else {
                    diff_col1 = ycrcb ? 0 : (src[0] - back[0]);
                    diff_col2 = (src[1] - back[1]);
                    diff_col3 = (src[2] - back[2]);
                }
                int diff2 = diff_col2*diff_col2 + diff_col3*diff_col3;
                if(!ycrcb)
                    diff2 += diff_col1*diff_col1;
                if(diff2 <= hardThresh2)
                    *dst = 0;
                else if(diff2 >= softThresh2)
                    *dst = 255;
                else *dst = static_cast<unsigned char>((sqrt(diff2)-hardThresh) * 255 / (softThresh-hardThresh));

                dst++;
                src += 3;
                if(!singleColor)
                    back += 3;
            }
        }
    }

    virtual void apply(cv::InputArray image, cv::OutputArray _fgmask, double learningRate=-1)
    {
        unsigned char backgroundCol1 = 0, backgroundCol2 = 0, backgroundCol3 = 0;
        if(useSingleColor) {
            if(useYCrCb) {
                getParameterValAsYCrCb("backgroundColor", &backgroundCol1, &backgroundCol2, &backgroundCol3);
            } else {
                getParameterValAsRGB("backgroundColor", &backgroundCol1, &backgroundCol2, &backgroundCol3);
            }
        }
        if(softThresh <= hardThresh)
            softThresh = hardThresh + 1;
        cv::Mat frame;
        if(useYCrCb)
		    cv::cvtColor(image, frame, cv::COLOR_BGR2YCrCb);
        else frame = image.getMat();
    	if(backgroundImg.empty())
			backgroundImg = frame.clone();
        cv::Mat &mask = _fgmask.getMatRef();
        mask.create(frame.size(), CV_8UC1);
        cv::Rect ROI2 = getROI();
        if(ROI2.empty())
            ROI2 = cv::Rect(0,0,mask.cols,mask.rows);
        if(ROI2.size() != mask.size())
            mask.setTo(cv::Scalar(0));
        if(useSingleColor) {
            if(useYCrCb)
                process<true, true>(mask, frame, backgroundCol1, backgroundCol2, backgroundCol3, ROI2);
            else process<true, false>(mask, frame, backgroundCol1, backgroundCol2, backgroundCol3, ROI2);
        } else {
            if(useYCrCb)
                process<false, true>(mask, frame, backgroundCol1, backgroundCol2, backgroundCol3, ROI2);
            else process<false, false>(mask, frame, backgroundCol1, backgroundCol2, backgroundCol3, ROI2);
        }
    }

    int hardThresh, softThresh;
    bool useSingleColor;
    bool useYCrCb;
    unsigned int backgroundColor;
    //int backgroundCr, backgroundCb;
    cv::Mat backgroundImg;
};

LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorChromaKeyRawPtr(int _hardThresh, int _softThresh, bool _useSingleColor, bool _useYCrCb, int _backgroundCol1, int _backgroundCol2, int _backgroundCol3)
{
    return new BackgroundSubtractorChromaKey(_hardThresh, _softThresh, _useSingleColor, _useYCrCb, _backgroundCol1, _backgroundCol2, _backgroundCol3);
}

}
#endif
