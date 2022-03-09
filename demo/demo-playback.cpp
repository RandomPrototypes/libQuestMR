#include <stdio.h>

#include <libQuestMR/QuestVideoMngr.h>

using namespace libQuestMR;

void playbackCapturedData(std::string outputFolder, std::string name)
{
    QuestVideoMngr mngr;
    QuestVideoSourceFile videoSrc;
    videoSrc.open((outputFolder+"/"+name+".questMRVideo").c_str());
    mngr.attachSource(&videoSrc);
    mngr.setRecordedTimestampSource((outputFolder+"/"+name+"Timestamp.txt").c_str());
    while(true)
    {
        if(!videoSrc.isValid())
            break;
        printf("VideoTickImpl\n");
        mngr.VideoTickImpl();
        uint64_t timestamp;
        cv::Mat img = mngr.getMostRecentImg(&timestamp);
        if(!img.empty())
        {
            cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
            cv::imshow("img", img);
            int key = cv::waitKey(10);
            if(key > 0)
                break;
        }
    }
}

int main(int argc, char** argv) 
{
    if(argc < 2) {
		printf("usage: demo-playback output_folder (name)\n");
	} else {
		playbackCapturedData(argv[1], argc >= 2 ? argv[2] : "capturedData");
    }
    return 0;
}
