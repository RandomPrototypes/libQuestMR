#include <libQuestMR/BackgroundSubtractor.h>
#ifdef USE_ONNX_RUNTIME
#include <onnxruntime_cxx_api.h>
#endif

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/dnn/dnn.hpp>


namespace libQuestMR
{

#ifdef USE_ONNX_RUNTIME
class BackgroundSubtractorRobustVideoMattingONNX : public BackgroundSubtractorBase
{
public:
    BackgroundSubtractorRobustVideoMattingONNX(const char *onnxModelFilename, bool use_CUDA)
        :memoryInfo(nullptr), memoryInfoCuda(nullptr), src_tensor(nullptr), downsample_ratio_tensor(nullptr), r1i(nullptr)
    {
        firstFrame = true;

        int deviceID = 0;
	
        //creates the onnx runtime environment
        env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "segmentation");
        sessionOptions = Ort::SessionOptions();
        sessionOptions.SetIntraOpNumThreads(1);
        
        //activates the CUDA backend
        if(use_CUDA) {
            OrtCUDAProviderOptions cuda_options;
            sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
        }
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        session = new Ort::Session(env, onnxModelFilename, sessionOptions);
        io_binding = new Ort::IoBinding(*session);
        
        memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
	
        //Not sure if this really allocate on the GPU, there is currently no documentation on it...
        memoryInfoCuda = Ort::MemoryInfo("Cuda", OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
        
        downsample_ratio = 0.25f;
        downsample_ratio_dims[0] = 1;
        downsample_ratio_tensor = Ort::Value::CreateTensor<float>(memoryInfo, &downsample_ratio, 1, downsample_ratio_dims, 1);
        
        rec_data = 0.0f;
        for(int i = 0; i < 4; i++)
            rec_dims[i] = 1;
        r1i = Ort::Value::CreateTensor<float>(memoryInfo, &rec_data, 1, rec_dims, 4);
        
        io_binding->BindOutput("fgr", memoryInfoCuda);
        io_binding->BindOutput("pha", memoryInfo);
        io_binding->BindOutput("r1o", memoryInfoCuda);
        io_binding->BindOutput("r2o", memoryInfoCuda);
        io_binding->BindOutput("r3o", memoryInfoCuda);
        io_binding->BindOutput("r4o", memoryInfoCuda);
        
        io_binding->BindInput("r1i", r1i);
        io_binding->BindInput("r2i", r1i);
        io_binding->BindInput("r3i", r1i);
        io_binding->BindInput("r4i", r1i);
        io_binding->BindInput("downsample_ratio", downsample_ratio_tensor);

    }

    virtual ~BackgroundSubtractorRobustVideoMattingONNX()
    {
        delete io_binding;
        delete session;
    }

    virtual void apply(cv::InputArray image, cv::OutputArray _fgmask, double learningRate=-1)
    {
        const cv::Mat &img = image.getMat();
        if(firstFrame) {
            src_data = std::vector<float>(img.cols * img.rows * 3);
            src_dims[0] = 1; src_dims[1] = 3; src_dims[2] = img.rows; src_dims[3] = img.cols;
            src_tensor = Ort::Value::CreateTensor<float>(memoryInfo, src_data.data(), src_data.size(), src_dims, 4);
            firstFrame = false;
        }
        cv::Mat blobMat;
        cv::dnn::blobFromImage(img, blobMat);
        
        src_data.assign(blobMat.begin<float>(), blobMat.end<float>());
        for(size_t i = 0; i < src_data.size(); i++)
        	src_data[i] /= 255;
        
        io_binding->BindInput("src", src_tensor);
        session->Run(Ort::RunOptions{nullptr}, *io_binding);
        
        std::vector<std::string> outputNames = io_binding->GetOutputNames();
        std::vector<Ort::Value> outputValues = io_binding->GetOutputValues();
        
        cv::Mat& mask = _fgmask.getMatRef();
        for(int i = 0; i < outputNames.size(); i++) {
        	if(outputNames[i] == "pha") {
        		const cv::Mat outputImg(img.rows, img.cols, CV_32FC1, const_cast<float*>(outputValues[i].GetTensorData<float>()));
        		outputImg.convertTo(mask, CV_8UC1, 255.0);
        	} else if(outputNames[i] == "r1o") {
        		io_binding->BindInput("r1i", outputValues[i]);
        	} else if(outputNames[i] == "r2o") {
        		io_binding->BindInput("r2i", outputValues[i]);
        	} else if(outputNames[i] == "r3o") {
        		io_binding->BindInput("r3i", outputValues[i]);
        	} else if(outputNames[i] == "r4o") {
        		io_binding->BindInput("r4i", outputValues[i]);
        	}
        }
    }

private:
    bool firstFrame; 
    Ort::Env env;
    Ort::SessionOptions sessionOptions;
    Ort::Session *session;
    Ort::MemoryInfo memoryInfo;
    Ort::MemoryInfo memoryInfoCuda;
    Ort::IoBinding *io_binding;
    std::vector<float> src_data;
    int64_t src_dims[4];
    Ort::Value src_tensor;
    float downsample_ratio;
    int64_t downsample_ratio_dims[1];
    Ort::Value downsample_ratio_tensor;
    float rec_data;
    int64_t rec_dims[4];
    Ort::Value r1i;
};

LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorRobustVideoMattingONNXRawPtr(const char *onnxModelFilename, bool use_CUDA)
{
    return new BackgroundSubtractorRobustVideoMattingONNX(onnxModelFilename, use_CUDA);
}

#else
LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorRobustVideoMattingONNXRawPtr(const char *onnxModelFilename, bool use_CUDA)
{
    printf("BackgroundSubtractorRobustVideoMattingONNX unavailable, rebuild with OONX runtime\n");
    return NULL;
}

#endif

}
#endif
