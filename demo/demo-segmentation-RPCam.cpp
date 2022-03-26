#include <onnxruntime_cxx_api.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn/dnn.hpp>
#include <RPCameraInterface/CameraInterface.h>
#include <RPCameraInterface/ImageFormatConverter.h>
#include "RPCam_helper.h"

using namespace RPCameraInterface;

void testSegmentation()
{
	bool use_CUDA = true;
	Ort::Env env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "segmentation");
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(1);
    
    if(use_CUDA) {
    	OrtCUDAProviderOptions cuda_options;
    	sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
    }
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
	Ort::Session session(env, "../rvm_mobilenetv3_fp32.onnx", sessionOptions);
	Ort::IoBinding io_binding(session);
	
	Ort::AllocatorWithDefaultOptions allocator;
	
	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	
	int maxImgSize = 1280;
	
	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, srcFormat.width, srcFormat.height);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    cv::Size size = cv::Size(dstFormat.width, dstFormat.height);
	
	printf("create tensors\n");

	
	Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
	
	Ort::MemoryInfo memoryInfoCuda("Cuda", OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
	
	std::vector<float> src_data(dstFormat.width * dstFormat.height * 3);
	std::vector<int64_t> src_dims = {1, 3, dstFormat.height, dstFormat.width};
	Ort::Value src_tensor = Ort::Value::CreateTensor<float>(memoryInfo, src_data.data(), src_data.size(), src_dims.data(), 4);
	
	float downsample_ratio = 0.25f;
	int64_t downsample_ratio_dims[] = {1};
	Ort::Value downsample_ratio_tensor = Ort::Value::CreateTensor<float>(memoryInfo, &downsample_ratio, 1, downsample_ratio_dims, 1);
	
	float rec_data = 0.0f;
	int64_t rec_dims[] = {1, 1, 1, 1};
	Ort::Value r1i = Ort::Value::CreateTensor<float>(memoryInfo, &rec_data, 1, rec_dims, 4);
	
	io_binding.BindOutput("fgr", memoryInfoCuda);
	io_binding.BindOutput("pha", memoryInfo);
	io_binding.BindOutput("r1o", memoryInfoCuda);
	io_binding.BindOutput("r2o", memoryInfoCuda);
	io_binding.BindOutput("r3o", memoryInfoCuda);
	io_binding.BindOutput("r4o", memoryInfoCuda);
    
    std::shared_ptr<ImageData> imgData2 = std::make_shared<ImageData>();
    
	io_binding.BindInput("r1i", r1i);
    io_binding.BindInput("r2i", r1i);
    io_binding.BindInput("r3i", r1i);
    io_binding.BindInput("r4i", r1i);
    io_binding.BindInput("downsample_ratio", downsample_ratio_tensor);
        
	printf("start\n");
    while(true) 
	{
		printf("iter\n");
		//Obtain the frame
        std::shared_ptr<ImageData> imgData = cam->getNewFrame(true);
        //Conver to the output format (BGR 720x480)
        converter.convertImage(imgData, imgData2);
        //Create OpenCV Mat for visualization
        cv::Mat frame(imgData2->imageFormat.height, imgData2->imageFormat.width, CV_8UC3, imgData2->data);
		if (frame.empty()) {
            printf("error : empty frame grabbed");
            break;
        }
        
        cv::Mat blobMat;
        cv::dnn::blobFromImage(frame, blobMat);
        
        src_data.assign(blobMat.begin<float>(), blobMat.end<float>());
        for(size_t i = 0; i < src_data.size(); i++)
        	src_data[i] /= 255;
        
        io_binding.BindInput("src", src_tensor);
        session.Run(Ort::RunOptions{nullptr}, io_binding);
        
        std::vector<std::string> outputNames = io_binding.GetOutputNames();
        std::vector<Ort::Value> outputValues = io_binding.GetOutputValues();
        
        cv::Mat mask(size.height, size.width, CV_8UC1);
        for(int i = 0; i < outputNames.size(); i++) {
        	if(outputNames[i] == "pha") {
        		const cv::Mat outputImg(size.height, size.width, CV_32FC1, const_cast<float*>(outputValues[i].GetTensorData<float>()));
        		outputImg.convertTo(mask, CV_8UC1, 255.0);
        	} else if(outputNames[i] == "r1o") {
        		io_binding.BindInput("r1i", outputValues[i]);
        	} else if(outputNames[i] == "r2o") {
        		io_binding.BindInput("r2i", outputValues[i]);
        	} else if(outputNames[i] == "r3o") {
        		io_binding.BindInput("r3i", outputValues[i]);
        	} else if(outputNames[i] == "r4o") {
        		io_binding.BindInput("r4i", outputValues[i]);
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

/*void testSegmentation()
{
	bool use_CUDA = true;
	Ort::Env env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "segmentation");
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(1);
    
    if(use_CUDA) {
    	OrtCUDAProviderOptions cuda_options;
    	sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
    }
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
	Ort::Session session(env, "../rvm_mobilenetv3_fp32.onnx", sessionOptions);
	Ort::IoBinding io_binding(session);
	
	Ort::AllocatorWithDefaultOptions allocator;

    size_t numInputNodes = session.GetInputCount();
    size_t numOutputNodes = session.GetOutputCount();

    std::cout << "Number of Input Nodes: " << numInputNodes << std::endl;
    std::cout << "Number of Output Nodes: " << numOutputNodes << std::endl;
    
    std::vector<const char*> inputNames(numInputNodes);
	std::vector<const char*> outputNames(numOutputNodes);
	
    for(size_t i = 0; i < numInputNodes; i++)
    {
		const char* inputName = session.GetInputName(i, allocator);
		inputNames[i] = inputName;
		std::cout << "Input Name: " << inputName << std::endl;
		
		Ort::TypeInfo inputTypeInfo = session.GetInputTypeInfo(i);
		auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();

		ONNXTensorElementDataType inputType = inputTensorInfo.GetElementType();
		std::cout << "Input Type: " << inputType << std::endl;

		std::vector<int64_t> inputDims = inputTensorInfo.GetShape();
		std::cout << "Input Dimensions: ";
		for(int64_t val : inputDims)
			std::cout << val  << " ";
		std::cout << std::endl;
    }

	for(size_t i = 0; i < numOutputNodes; i++)
    {
		const char* outputName = session.GetOutputName(i, allocator);
		outputNames[i] = outputName;
		std::cout << "Output Name: " << outputName << std::endl;

		Ort::TypeInfo outputTypeInfo = session.GetOutputTypeInfo(i);
		auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();

		ONNXTensorElementDataType outputType = outputTensorInfo.GetElementType();
		std::cout << "Output Type: " << outputType << std::endl;
		
		std::vector<int64_t> outputDims = outputTensorInfo.GetShape();
		std::cout << "Output Dimensions: ";
		for(int64_t val : outputDims)
			std::cout << val  << " ";
		std::cout << std::endl;
		
    }
	
	std::shared_ptr<CameraInterface> cam;
	ImageFormat srcFormat;
	if(!configureCamera(cam, &srcFormat)) {
		printf("error configuring camera!!\n");
		return ;
	}
	
	int maxImgSize = 1280;
	
	//Choose the conversion format and initialize the converter
    ImageFormat dstFormat(ImageType::BGR24, 640, 480);
    ImageFormatConverter converter(srcFormat, dstFormat);
    
    cv::Size size = cv::Size(dstFormat.width, dstFormat.height);
    std::vector<int> listInputChannels = {3, 16, 20, 40, 64, 1};
    std::vector<cv::Size> listInputSize = {cv::Size(size.width, size.height), cv::Size(size.width/4, size.height/4), cv::Size(size.width/8, size.height/8), cv::Size(size.width/16, size.height/16), cv::Size(size.width/32, size.height/32), cv::Size(1,1)};
    std::vector<int> listOutputChannels = {3, 1, 16, 20, 40, 64};
    std::vector<cv::Size> listOutputSize = {listInputSize[0], listInputSize[0], listInputSize[1], listInputSize[2], listInputSize[3], listInputSize[4]};
    std::vector<std::vector<float> > listInputTensorBuf(numInputNodes);
	std::vector<std::vector<float> > listOutputTensorBuf(numOutputNodes);
	std::vector<std::vector<int64_t> > listInputDims(numInputNodes);
	std::vector<std::vector<int64_t> > listOutputDims(numOutputNodes);
	
	std::vector<Ort::Value> inputTensors;
	std::vector<Ort::Value> outputTensors;
	
	printf("create tensors\n");
	
	

//r1o: 1,120,160,16
//r2o: 1,60,80,20
//r3o: 1,30,40,40
//r4o: 1,15,20,64
//fgr: 1,480,640,3
//pha: 1,480,640,1
//src: 1,480,640,3
//r1i: 1,120,160,16
//r2i: 1,60,80,20
//r3i: 1,30,40,40
//r4i: 1,15,20,64
//downsample_ratio:

	
	Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
	for(size_t i = 0; i < numInputNodes; i++) {
		printf("input %d : %s %d %d %d %d\n", i, inputNames[i], 1, listInputChannels[i], listInputSize[i].height, listInputSize[i].width);
		listInputTensorBuf[i].resize(listInputChannels[i] * listInputSize[i].width * listInputSize[i].height, 0);
		if(listInputSize[i].height == 1 && listInputSize[i].width == 1 && listInputChannels[i] == 1)
			listInputDims[i] = {1};
		else listInputDims[i] = {1, listInputChannels[i], listInputSize[i].height, listInputSize[i].width};
		inputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo, listInputTensorBuf[i].data(), listInputTensorBuf[i].size(), listInputDims[i].data(), listInputDims[i].size()));
	}
	listInputTensorBuf[5][0] = 0.5;
	for(size_t i = 0; i < numOutputNodes; i++) {
		printf("output %d : %s %d %d %d %d\n", i, outputNames[i], 1, listOutputChannels[i], listOutputSize[i].height, listOutputSize[i].width);
		listOutputTensorBuf[i].resize(listOutputChannels[i] * listOutputSize[i].width * listOutputSize[i].height);
		listOutputDims[i] = {1, listOutputChannels[i], listOutputSize[i].height, listOutputSize[i].width};
		outputTensors.push_back(Ort::Value::CreateTensor<float>(memoryInfo, listOutputTensorBuf[i].data(), listOutputTensorBuf[i].size(), listOutputDims[i].data(), listOutputDims[i].size()));
	}
    
    std::shared_ptr<ImageData> imgData2 = std::make_shared<ImageData>();
    
    
	printf("start\n");
    while(true) 
	{
		printf("iter\n");
		//Obtain the frame
        std::shared_ptr<ImageData> imgData = cam->getNewFrame(true);
        //Conver to the output format (BGR 720x480)
        converter.convertImage(imgData, imgData2);
        //Create OpenCV Mat for visualization
        cv::Mat frame(imgData2->imageFormat.height, imgData2->imageFormat.width, CV_8UC3, imgData2->data);
		if (frame.empty()) {
            printf("error : empty frame grabbed");
            break;
        }
        
        cv::Mat blobMat;
        cv::dnn::blobFromImage(frame, blobMat);
        
        listInputTensorBuf[0].assign(blobMat.begin<float>(), blobMat.end<float>());
        for(size_t i = 0; i < listInputTensorBuf[0].size(); i++)
        	listInputTensorBuf[0][i] /= 255;
        
        session.Run(Ort::RunOptions{nullptr}, inputNames.data(),
                inputTensors.data(), numInputNodes, outputNames.data(),
                outputTensors.data(), numOutputNodes);
        
        cv::Mat outputImg(imgData2->imageFormat.height, imgData2->imageFormat.width, CV_8UC3);
        for(int c = 0; c < 3; c++) {
        	for(int i = 0; i < imgData2->imageFormat.height; i++) {
        		float *src = &(listOutputTensorBuf[1][0]) + (0*imgData2->imageFormat.height + i)*imgData2->imageFormat.width;
        		unsigned char *dst = outputImg.ptr<unsigned char>(i);
        		for(int j = 0; j < imgData2->imageFormat.width; j++)
        		{
        			dst[j*3+c] = cv::saturate_cast<unsigned char>(src[j]*255);
        		}
        	}
        }
        
        
        cv::Mat img = outputImg.clone();

        if(frame.cols > maxImgSize || frame.rows > maxImgSize) {
        	float ratio = static_cast<float>(maxImgSize) / std::max(frame.cols, frame.rows);
        	cv::resize(img, img, cv::Size(cvRound(frame.cols * ratio), cvRound(frame.rows * ratio)));
        }
        cv::imshow("img", img);
        int key = cv::waitKey(20);
        if(key > 0)
        	break;
        
        for(int i = 0; i < 4; i++)
        	listInputTensorBuf[i+1].assign(listOutputTensorBuf[i+2].begin(), listOutputTensorBuf[i+2].end());
	}
}*/

int main(int argc, char **argv) 
{
	testSegmentation();
    return 0;
}
