#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <RPCameraInterface/CameraInterface.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include "RPCam_helper.h"

using namespace RPCameraInterface;

void testChromaKey()
{
	
	int maxImgSize = 1280;

	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	
	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    cv::Size size = cv::Size(dstFormat.width, dstFormat.height);

	cv::Mat backgroundYCrCb;

	int hardThresh = 20;
	int softThresh = 50;
	
    std::shared_ptr<ImageData> imgData2 = createImageData();

	printf("start\n");
    while(true) 
	{
		//Obtain the frame
        std::shared_ptr<ImageData> imgData = cam->getNewFrame(true);
        //Conver to the output format (BGR 720x480)
        converter.convertImage(imgData, imgData2);
        //Create OpenCV Mat for visualization
        cv::Mat frame(imgData2->getImageFormat().height, imgData2->getImageFormat().width, CV_8UC3, imgData2->getDataPtr());
		if (frame.empty()) {
            printf("error : empty frame grabbed");
            break;
        }

		if(backgroundYCrCb.empty())
			cv::cvtColor(frame, backgroundYCrCb, cv::COLOR_BGR2YCrCb);

		cv::Mat frameYCrCb;
		cv::cvtColor(frame, frameYCrCb, cv::COLOR_BGR2YCrCb);
        
		cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);

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

        if(frame.cols > maxImgSize || frame.rows > maxImgSize) {
        	float ratio = static_cast<float>(maxImgSize) / std::max(frame.cols, frame.rows);
        	cv::resize(mask, mask, cv::Size(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio)));
        }
        cv::imshow("img", mask);
        int key = cv::waitKey(20);
        if(key > 0)
        	break;
        
        
	}
}

int main(int argc, char **argv) 
{
	testChromaKey();
    return 0;
}
