#include <stdio.h>

#include <libQuestMR/QuestVideoMngr.h>

using namespace libQuestMR;

void captureFromQuest(const char *ipAddr, const char *outputFolder, const char *name)
{
    QuestVideoMngr mngr;
    QuestVideoSourceSocket videoSrc;
    videoSrc.Connect(ipAddr);
    mngr.attachSource(&videoSrc);
    if(outputFolder != NULL) {
    	printf("recording folder : %s, name : %s\n", outputFolder, name);
    	mngr.setRecording(outputFolder, name);
    }
    while(true)
    {
        mngr.VideoTickImpl();
        cv::Mat img = mngr.getMostRecentImg();
        if(!img.empty())
        {
            cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
            cv::imshow("img", img);
            int key = cv::waitKey(10);
            if(key > 0)
                break;
        }
    }
    mngr.detachSource();
    videoSrc.Disconnect();
}

int main(int argc, char** argv) 
{
	if(argc < 2) {
		printf("usage: demo-capture ipAddr (output_folder) (name)\n");
	} else {
		captureFromQuest(argv[1], argc >= 2 ? argv[2] : NULL, argc >= 3 ? argv[3] : "capturedData");
    }
    return 0;
}
