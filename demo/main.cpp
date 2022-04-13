#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <thread>
#include <queue>

#include <libQuestMR/QuestVideoMngr.h>
#include <libQuestMR/QuestCalibData.h>
#include <libQuestMR/QuestFrameData.h>
#include <libQuestMR/QuestCommunicator.h>
#include <libQuestMR/QuestStringUtil.h>

#define DEFAULT_IP_ADDRESS "192.168.10.105"

using namespace libQuestMR;

std::string mat2str(cv::Mat mat)
{
    std::string str;
    str += std::to_string(mat.cols) + "x" + std::to_string(mat.rows) + ", type: " + std::to_string(mat.type()) +"\n";
    for(int i = 0; i < mat.rows; i++)
    {
        if(mat.type() == CV_64F)
        {
            double *ptr = mat.ptr<double>(i);
            for(int j = 0; j < mat.cols; j++)
            {
                str += std::to_string(ptr[j]);
                if(j+1 < mat.cols)
                    str += ", ";
            }
            str += "\n";
        } else if(mat.type() == CV_32F) {
            float *ptr = mat.ptr<float>(i);
            for(int j = 0; j < mat.cols; j++)
            {
                str += std::to_string(ptr[j]);
                if(j+1 < mat.cols)
                    str += ", ";
            }
            str += "\n";
        }
    }
    return str+"\n";
}

void sample1()
{
    std::shared_ptr<BufferedSocket> questCom = createBufferedSocket();
    std::vector<char> data;
    std::vector<char> message;
    if(questCom->connect(DEFAULT_IP_ADDRESS, 25671))
    {
        while(true){
            char buf[255];
            int length = questCom->readData(buf, 255);
            if(length <= 0)
                break;
            addToBuffer(data, buf, length);
            int nextMessageStart = libQuestMR::findMessageStart(&data[0], (int)data.size(), 1);
            while(nextMessageStart > 0)
            {
                extractFromStart(message, data, nextMessageStart);
                message.push_back('\0');
                for(size_t i = 4; i < message.size() && i < 12; i++)
                    printf("%02x ", message[i]);
                printf(", size : %d\n", (int)message.size() - 1 - 12);
                nextMessageStart = libQuestMR::findMessageStart(&data[0], (int)data.size(), 1);
            }
        }
    }
}

void sample2()
{
    std::shared_ptr<QuestCommunicator> questCom = createQuestCommunicator();
    if(questCom->connect(DEFAULT_IP_ADDRESS, 25671))
    {
        while(true)
        {
            QuestCommunicatorMessage message;
            if(!questCom->readMessage(&message))
                break;
            int size = static_cast<int>(message.data.size());
            printf("message type:%u size:%d\n", message.type, size);
            if(message.type == 33)
            {
                QuestFrameData frame;
                frame.parse(message.data.c_str(), size);
                printf("frame data:\n%s\n", frame.toString().c_str());
            }
            else printf("raw data:\n%.*s\n\n", size, message.data.c_str());
        }
    }
}

void sample3()
{
    bool use_recording = false;

    std::shared_ptr<QuestVideoMngr> mngr = createQuestVideoMngr();
    if(!use_recording)
    {
        std::shared_ptr<QuestVideoSourceBufferedSocket> videoSrc = createQuestVideoSourceBufferedSocket();
        videoSrc->Connect(DEFAULT_IP_ADDRESS);
        mngr->attachSource(videoSrc);
        mngr->setRecording("output", "test");
        while(true)
        {
            mngr->VideoTickImpl();
            cv::Mat img = mngr->getMostRecentImg();
            if(!img.empty())
            {
                cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
                cv::imshow("img", img);
                int key = cv::waitKey(10);
                if(key > 0)
                    break;
            }
        }
        mngr->detachSource();
        videoSrc->Disconnect();
    } else {
        mngr->setRecording(NULL, NULL);


        std::shared_ptr<QuestVideoSourceFile> videoSrc2 = createQuestVideoSourceFile();
        videoSrc2->open("output/test.questMRVideo");
        mngr->attachSource(videoSrc2);
        mngr->setRecordedTimestampSource("output/testTimestamp.txt");
        while(true)
        {
            if(!videoSrc2->isValid())
                break;
            printf("VideoTickImpl\n");
            mngr->VideoTickImpl();
            uint64_t timestamp;
            cv::Mat img = mngr->getMostRecentImg(&timestamp);
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
}

void sample4()
{
    QuestCalibData calibData;
    calibData.loadXMLFile("example_calib_data.xml");
    std::cout << "result:\n" << calibData.generateXMLString().str();
    std::cout << "\n\nend\n";
}

void sample5()
{
    //QuestCalibData calibData;
    //calibData.loadXMLFile("example_calib_data2.xml");

    cv::Mat img;
    cv::VideoCapture cap;
    int deviceID = 0;
    int apiID = cv::CAP_ANY;
    cap.open(deviceID, apiID);
    if (!cap.isOpened()) {
        std::cout << "ERROR! Unable to open camera\n";
        return ;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,1080);

    std::shared_ptr<QuestCommunicator> questCom = createQuestCommunicator();
    if(!questCom->connect(DEFAULT_IP_ADDRESS, 25671))
    {
        printf("can not connect to the quest\n");
        return ;
    }

    std::shared_ptr<QuestCommunicatorThreadData> questComData = createQuestCommunicatorThreadData(questCom);
    std::thread questComThread(QuestCommunicatorThreadFunc, questComData.get());

    QuestFrameData frame;
    bool hasFrameData = false;

    int nbRecordedFrame = 0;

    FILE *recordFile = fopen("record/record.txt", "w");

    std::string calibData;

    uint32_t currentTriggerCount = 0;
    while(true)
    {
        cap.read(img);
        if (img.empty()) {
            std::cout << "ERROR! blank frame grabbed\n";
            break;
        }
        if(questComData->hasNewFrameData())
        {
            while(questComData->hasNewFrameData())
                questComData->getFrameData(&frame);
            hasFrameData = true;
        }
        if(hasFrameData && currentTriggerCount < questComData->getTriggerCount())
        {
            cv::imwrite("record/img"+std::to_string(nbRecordedFrame)+".png", img);
            fprintf(recordFile, "%d\n", nbRecordedFrame);
            fprintf(recordFile, "%.8lf,%.8lf,%.8lf\n", frame.head_pos[0], frame.head_pos[1], frame.head_pos[2]);
            fprintf(recordFile, "%.8lf,%.8lf,%.8lf,%.8lf\n", frame.head_rot[0], frame.head_rot[1], frame.head_rot[2], frame.head_rot[3]);
            fprintf(recordFile, "%.8lf,%.8lf,%.8lf\n", frame.right_hand_pos[0], frame.right_hand_pos[1], frame.right_hand_pos[2]);
            fprintf(recordFile, "%.8lf,%.8lf,%.8lf,%.8lf\n", frame.right_hand_rot[0], frame.right_hand_rot[1], frame.right_hand_rot[2], frame.right_hand_rot[3]);
            fprintf(recordFile, "%.8lf,%.8lf,%.8lf\n", frame.raw_pos[0], frame.raw_pos[1], frame.raw_pos[2]);
            fprintf(recordFile, "%.8lf,%.8lf,%.8lf,%.8lf\n", frame.raw_rot[0], frame.raw_rot[1], frame.raw_rot[2], frame.raw_rot[3]);

            nbRecordedFrame++;
            currentTriggerCount = questComData->getTriggerCount();
        }
        if(calibData.empty())
        {
            calibData = questComData->getCalibData().str();
            if(!calibData.empty())
            {
                FILE *file = fopen("record/calib.xml", "w");
                for(int i = 0; i < calibData.size(); i++)
                    fprintf(file, "%c", calibData[i]);
                fclose(file);
            }
        }
        /*if(hasFrameData)
        {
            cv::Point3d headPos(frame.head_pos[0], frame.head_pos[1], frame.head_pos[2]);
            cv::Point2d p = calibData.projectToCam(headPos);
            cv::circle(img, p, 8, cv::Scalar(0,0,255), 4);
        }*/

        cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
        cv::imshow("img", img);

        printf("%dx%d\n", img.cols, img.rows);
        int key = cv::waitKey(10);
        if(key > 0)
            break;
    }
    fclose(recordFile);
    questComData->setFinishedVal(true);
    questComThread.join();
}

class MouseData
{
public:
    int x,y;
    bool leftDown;

};


void MouseCallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    MouseData *data = (MouseData*)userdata;
    if(event == cv::EVENT_LBUTTONDOWN)
    {
        printf("leftDown\n");
        data->leftDown = true;
    }
    else if(event == cv::EVENT_LBUTTONUP)
    {
        printf("leftUp\n");
        data->leftDown = false;
    }
    data->x = x;
    data->y = y;
    //printf("%d,%d\n", x, y);
}

/*void sample6()
{
    QuestCalibData calibData;
    calibData.loadXMLFile("record1/calib.xml");
    std::cout << calibData.generateXMLString() << "\n";
    cv::namedWindow("img");
    MouseData mouseData;
    cv::setMouseCallback("img", MouseCallBackFunc, &mouseData);
    FILE *recordFile = fopen("record1/record.txt", "r");
    //FILE *gtFile = fopen("record/gt.txt", "w");
    FILE *gtFile = fopen("record1/gt.txt", "r");

    std::vector<cv::Mat> listImages;
    std::vector<cv::Point3d> listPoints3D;
    std::vector<cv::Point3d> listHand3D;
    std::vector<cv::Point2d> listPoints2D;
    std::vector<cv::Point3d> listRawPos;
    std::vector<cv::Quat<double> > listRawRot;
    while(!feof(recordFile))
    {
        int frameId;
        double head_pos[3];
        double head_rot[4];
        double right_hand_pos[3];
        double right_hand_rot[4];
        double raw_pos[3];
        double raw_rot[4];
        fscanf(recordFile, "%d\n", &frameId);
        fscanf(recordFile, "%lf,%lf,%lf\n", &head_pos[0], &head_pos[1], &head_pos[2]);
        fscanf(recordFile, "%lf,%lf,%lf,%lf\n", &head_rot[0], &head_rot[1], &head_rot[2], &head_rot[3]);
        //fscanf(recordFile, "%lf,%lf,%lf\n", &right_hand_pos[0], &right_hand_pos[1], &right_hand_pos[2]);
        //fscanf(recordFile, "%lf,%lf,%lf,%lf\n", &right_hand_rot[0], &right_hand_rot[1], &right_hand_rot[2], &right_hand_rot[3]);
        fscanf(recordFile, "%lf,%lf,%lf\n", &raw_pos[0], &raw_pos[1], &raw_pos[2]);
        fscanf(recordFile, "%lf,%lf,%lf,%lf\n", &raw_rot[0], &raw_rot[1], &raw_rot[2], &raw_rot[3]);

        cv::Mat img = cv::imread("record1/img"+std::to_string(frameId)+".png");
        cv::imshow("img", img);
        //while(!mouseData.leftDown)
        //    cv::waitKey(10);
        //while(mouseData.leftDown)
        //    cv::waitKey(10);
        fscanf(gtFile, "%d,%d,%d\n", &frameId, &(mouseData.x), &(mouseData.y));
        listImages.push_back(img);
        listPoints2D.push_back(cv::Point2d(mouseData.x, mouseData.y));
        listPoints3D.push_back(cv::Point3d(head_pos[0], head_pos[1], head_pos[2]));
        listHand3D.push_back(cv::Point3d(right_hand_pos[0], right_hand_pos[1], right_hand_pos[2]));
        listRawPos.push_back(cv::Point3d(raw_pos[0], raw_pos[1], raw_pos[2]));
        listRawRot.push_back(cv::Quat<double>(raw_rot[3], raw_rot[0], raw_rot[1], raw_rot[2]));
        //fprintf(gtFile, "%d,%d,%d\n", frameId, mouseData.x, mouseData.y);
    }
    fclose(recordFile);
    fclose(gtFile);
    cv::Mat K = calibData.getCameraMatrix();
    cv::Mat distCoeffs = calibData.getDistCoeffs();
    cv::Mat rvec, tvec;
    //cv::solvePnP(listPoints3D, listPoints2D, K, distCoeffs, rvec, tvec);
    rvec = quaternion2rvec(calibData.getRotation());
    tvec = calibData.getTranslation();
    //cv::Mat Rt = cv::Mat::eye(4,4,CV_64F);
    cv::Mat r = quaternion2rotationMat(calibData.getRotation());
    cv::Mat R = cv::Mat::eye(4,4,CV_64F);
    r.copyTo(R(cv::Rect(0,0,3,3)));
    cv::Mat T = cv::Mat::eye(4,4,CV_64F);
    calibData.getTranslation().copyTo(T(cv::Rect(3,0,1,3)));

    cv::Mat r_raw = quaternion2rotationMat(calibData.getRawRotation());
    cv::Mat R_raw = cv::Mat::eye(4,4,CV_64F);
    r_raw.copyTo(R_raw(cv::Rect(0,0,3,3)));
    cv::Mat T_raw = cv::Mat::eye(4,4,CV_64F);
    calibData.getRawTranslation().copyTo(T_raw(cv::Rect(3,0,1,3)));

    printf("K: %s\n", mat2str(K).c_str());

    printf("R: %s\nt: %s\nR_raw: %s\nT_raw: %s\n", mat2str(R).c_str(), mat2str(T).c_str(), mat2str(R_raw).c_str(), mat2str(T_raw).c_str());
    printf("r: %s\nr_raw: %s\n", mat2str(r).c_str(), mat2str(r_raw).c_str());

    //for(int k : {222,223,254,255,322,323,354,355})
    //int k = 114;
    int k=3;
    //for(int k = 0; k < 512; k++)
    {
        cv::Mat R1 = k%2      == 0 ? R : R.inv();
        cv::Mat T1 = (k/2)%2  == 0 ? T : T.inv();
        cv::Mat Rt1 = (k/4)%2 == 0 ? R1*T1 : T1*R1;
        cv::Mat R2 = (k/8)%2  == 0 ? R_raw : R_raw.inv();
        cv::Mat T2 = (k/16)%2 == 0 ? T_raw : T_raw.inv();
        cv::Mat Rt2 = (k/32)%2== 0 ? R2*T2 : T2*R2;
        //cv::Mat Rt = (k/64)%2 == 0 ? Rt1 : Rt2;//Rt1 * Rt2 : Rt2 * Rt1;
        cv::Mat Rt = calibData.getExtrinsicMat();//(R.inv()*T.inv());//(T_raw*R_raw).inv() * (T*R).inv();
        //printf("rvec: %s\ntvec: %s\n", mat2str(rvec).c_str(), mat2str(tvec).c_str());

        std::string R1_str = (k%2 == 0) ? "R" : "R^-1";
        std::string T1_str = ((k/2)%2 == 0) ? "t" : "t^-1";
        std::string Rt1_str = "(" + ((k/4)%2 == 0 ? R1_str + "*" + T1_str : T1_str + "*" + R1_str) + ")";
        std::string R2_str = ((k/8)%2 == 0) ? "R_raw" : "R_raw^-1";
        std::string T2_str = ((k/16)%2 == 0) ? "t_raw" : "t_raw^-1";
        std::string Rt2_str = "(" + ((k/32)%2 == 0 ? R2_str + "*" + T2_str : T2_str + "*" + R2_str) + ")";
        std::string Rt_str = "(" + ((k/64)%2 == 0 ? Rt1_str + "*" + Rt2_str : Rt2_str + "*" + Rt1_str) + ")";
        if((k/128) % 2 == 0)
            Rt_str += ", X flip";
        if((k/256) % 2 == 0)
            Rt_str += ", Y flip";

        cv::Mat A = cv::Mat::zeros(listImages.size()*2, 4, CV_64F);
        cv::Mat B = cv::Mat::zeros(listImages.size()*2, 1, CV_64F);
        for(int i = 0; i < listImages.size(); i++)
        {
            //(x,1,0,0) * (fx,cx,fy,cy) = x'
            //(0,0,y,1) * (fx,cx,fy,cy) = y'
            cv::Mat vec(4,1,CV_64F);
            vec.at<double>(0,0) = listPoints3D[i].x;
            vec.at<double>(1,0) = listPoints3D[i].y;
            vec.at<double>(2,0) = listPoints3D[i].z;
            vec.at<double>(3,0) = 1;
            vec = Rt(cv::Rect(0,0,4,3)) * vec;
            vec /= vec.at<double>(2,0);
            A.at<double>(i*2,0) = vec.at<double>(0,0);
            A.at<double>(i*2,1) = 1;
            A.at<double>(i*2+1,2) = vec.at<double>(1,0);
            A.at<double>(i*2+1,3) = 1;
            B.at<double>(i*2,0) = listPoints2D[i].x;
            B.at<double>(i*2+1,0) = listPoints2D[i].y;
        }
        cv::Mat C;
        cv::solve(A,B,C, cv::DECOMP_SVD);
        //K.at<double>(0,0) = C.at<double>(0,0);
        //K.at<double>(0,2) = C.at<double>(1,0);
        //K.at<double>(1,1) = C.at<double>(2,0);
        //K.at<double>(1,2) = C.at<double>(3,0);
        
        int count = 0;
        for(int i = 0; i < listImages.size(); i++)
        {
            cv::Mat img = listImages[i].clone();
            //cv::Mat vec(4,1,CV_64F);
            //vec.at<double>(0,0) = listPoints3D[i].x;
            //vec.at<double>(1,0) = listPoints3D[i].y;
            //vec.at<double>(2,0) = listPoints3D[i].z;
            //vec.at<double>(3,0) = 1;
            //vec = K * Rt(cv::Rect(0,0,4,3)) * vec;
            //vec /= vec.at<double>(2,0);
            //cv::Point2d p(vec.at<double>(0,0), vec.at<double>(1,0));
            //if((k/128) % 2 == 0)
            //    p.x = img.cols - p.x;
            cv::Point2d p = calibData.projectToCam(listPoints3D[i]);
            //if((k/256) % 2 == 0)
            //    p.y = img.rows - p.y;
            //p.x -= 200;
            //p.y -= 200;

            //cv::Mat vec2(4,1,CV_64F);
            //vec2.at<double>(0,0) = listHand3D[i].x;
            //vec2.at<double>(1,0) = listHand3D[i].y;
            //vec2.at<double>(2,0) = listHand3D[i].z;
            //vec2.at<double>(3,0) = 1;
            //vec2 = K * Rt(cv::Rect(0,0,4,3)) * vec2;
            //vec2 /= vec2.at<double>(2,0);
            //cv::Point2d p2(vec2.at<double>(0,0), vec2.at<double>(1,0));
            //if((k/128) % 2 == 0)
            //    p2.x = img.cols - p2.x;
            cv::Point2d p2 = calibData.projectToCam(listHand3D[i]);
            //if((k/256) % 2 == 0)
            //    p2.y = img.rows - p2.y;

            cv::Point2d diff = p - listPoints2D[i];

            cv::circle(img, p, 8, cv::Scalar(0,0,255), 4);
            cv::circle(img, p2, 8, cv::Scalar(0,255,0), 4);
            cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
            cv::imshow("img", img);
            //printf("%lf : %lf %lf => %lf %lf\n", sqrt(diff.dot(diff)), p.x, p.y, listPoints2D[i].x, listPoints2D[i].y);
            if(sqrt(diff.dot(diff)) < 200)
            {
                count++;
                printf("k: %d, dist %lf, count %d\n", k, sqrt(diff.dot(diff)), count);
                cv::waitKey(300);
            } else {
                //cv::waitKey(0);
            }
            cv::waitKey(0);
        }
        if(count >= 16)
            printf("k: %d, %s\n", k, Rt_str.c_str());
    }
}*/

void sample7()
{
    cv::Mat img;
    cv::VideoCapture cap;
    int deviceID = 0;
    int apiID = cv::CAP_ANY;
    cap.open(deviceID, apiID);
    if (!cap.isOpened()) {
        std::cout << "ERROR! Unable to open camera\n";
        return ;
    }
    cap.set(cv::CAP_PROP_FRAME_WIDTH,1920);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,1080);

    cv::Ptr<cv::BackgroundSubtractor> pBackSub;
    pBackSub = cv::createBackgroundSubtractorKNN();

    cv::Mat fgMask;
    while(true)
    {
        cap.read(img);
        if (img.empty()) {
            std::cout << "ERROR! blank frame grabbed\n";
            break;
        }

        pBackSub->apply(img, fgMask);

        cv::imshow("Frame", img);
        cv::imshow("FG Mask", fgMask);

        int key = cv::waitKey(10);
        if(key > 0)
            break;
    }
}

/*void sample8()
{
    QuestCalibData calibData;
    calibData.loadXMLFile("record1/calib.xml");
    std::cout << calibData.generateXMLString() << "\n";
    cv::namedWindow("img");
    MouseData mouseData;
    cv::setMouseCallback("img", MouseCallBackFunc, &mouseData);
    FILE *recordFile = fopen("record1/record.txt", "r");
    //FILE *gtFile = fopen("record/gt.txt", "w");
    FILE *gtFile = fopen("record1/gt.txt", "r");

    std::vector<cv::Mat> listImages;
    std::vector<cv::Point3d> listPoints3D;
    std::vector<cv::Point3d> listHand3D;
    std::vector<cv::Point2d> listPoints2D;
    std::vector<cv::Point3d> listRawPos;
    std::vector<cv::Quat<double> > listRawRot;
    while(!feof(recordFile))
    {
        int frameId;
        double head_pos[3];
        double head_rot[4];
        double right_hand_pos[3];
        double right_hand_rot[4];
        double raw_pos[3];
        double raw_rot[4];
        fscanf(recordFile, "%d\n", &frameId);
        fscanf(recordFile, "%lf,%lf,%lf\n", &head_pos[0], &head_pos[1], &head_pos[2]);
        fscanf(recordFile, "%lf,%lf,%lf,%lf\n", &head_rot[0], &head_rot[1], &head_rot[2], &head_rot[3]);
        //fscanf(recordFile, "%lf,%lf,%lf\n", &right_hand_pos[0], &right_hand_pos[1], &right_hand_pos[2]);
        //fscanf(recordFile, "%lf,%lf,%lf,%lf\n", &right_hand_rot[0], &right_hand_rot[1], &right_hand_rot[2], &right_hand_rot[3]);
        fscanf(recordFile, "%lf,%lf,%lf\n", &raw_pos[0], &raw_pos[1], &raw_pos[2]);
        fscanf(recordFile, "%lf,%lf,%lf,%lf\n", &raw_rot[0], &raw_rot[1], &raw_rot[2], &raw_rot[3]);

        cv::Mat img = cv::imread("record1/img"+std::to_string(frameId)+".png");
        fscanf(gtFile, "%d,%d,%d\n", &frameId, &(mouseData.x), &(mouseData.y));
        listImages.push_back(img);
        listPoints2D.push_back(cv::Point2d(mouseData.x, mouseData.y));
        listPoints3D.push_back(cv::Point3d(head_pos[0], head_pos[1], head_pos[2]));
        listHand3D.push_back(cv::Point3d(right_hand_pos[0], right_hand_pos[1], right_hand_pos[2]));
        listRawPos.push_back(cv::Point3d(raw_pos[0], raw_pos[1], raw_pos[2]));
        listRawRot.push_back(cv::Quat<double>(raw_rot[3], raw_rot[0], raw_rot[1], raw_rot[2]));
        //fprintf(gtFile, "%d,%d,%d\n", frameId, mouseData.x, mouseData.y);
    }
    fclose(recordFile);
    fclose(gtFile);
    calibData.calibrateCamPose(listPoints3D, listPoints2D);
    
    for(int i = 0; i < listImages.size(); i++)
    {
        cv::Mat img = listImages[i].clone();
        cv::Point2d p = calibData.projectToCam(listPoints3D[i]);
        cv::Point2d p2 = calibData.projectToCam(listHand3D[i]);

        cv::circle(img, p, 8, cv::Scalar(0,0,255), 4);
        cv::circle(img, p2, 8, cv::Scalar(0,255,0), 4);
        cv::resize(img, img, cv::Size(img.cols/2, img.rows/2));
        cv::imshow("img", img);
        cv::waitKey(0);
    }
}*/

int main(int, char**) 
{
    sample3();
    //sample4();
    return 0;
}
