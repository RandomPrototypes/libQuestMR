#include <libQuestMR/BackgroundSubtractor.h>
#ifdef USE_ONNX_RUNTIME
    #include <onnxruntime_cxx_api.h>
    
    #ifdef USE_ONNX_RUNTIME_DIRECTML
        #include <dml_provider_factory.h>
    #endif
#endif

#ifdef LIBQUESTMR_USE_OPENCV
#include <opencv2/dnn/dnn.hpp>


namespace libQuestMR
{

#ifdef USE_ONNX_RUNTIME
class BackgroundSubtractorRobustVideoMattingONNX : public BackgroundSubtractorBase
{
public:
    BackgroundSubtractorRobustVideoMattingONNX(const char *onnxModelFilename, bool use_GPU)
        :memoryInfo(nullptr), memoryInfoCuda(nullptr), src_tensor(nullptr), downsample_ratio_tensor(nullptr), r1i(nullptr)
    {
        firstFrame = true;

        int deviceID = 0;
	
        //creates the onnx runtime environment
        env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "segmentation");
        sessionOptions = Ort::SessionOptions();
        sessionOptions.SetIntraOpNumThreads(1);
        
        //activates the CUDA backend
        if(use_GPU) {
            #if defined(USE_ONNX_RUNTIME_CUDA)
                OrtCUDAProviderOptions cuda_options;
                sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
            #elif defined(USE_ONNX_RUNTIME_DIRECTML)
                OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);
                sessionOptions.DisableMemPattern();
                sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
            #endif
        }
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        #ifdef _WIN32
            size_t filename_size = strlen(onnxModelFilename);
            wchar_t *wc_filename = new wchar_t[filename_size+1];
            size_t convertedChars = 0;
            mbstowcs_s(&convertedChars, wc_filename, filename_size+1, onnxModelFilename, _TRUNCATE);
            session = new Ort::Session(env, wc_filename, sessionOptions);
            delete [] wc_filename;
        #else
            session = new Ort::Session(env, onnxModelFilename, sessionOptions);
        #endif
        io_binding = new Ort::IoBinding(*session);
        
        memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
	
        //Not sure if this really allocate on the GPU, there is currently no documentation on it...
        memoryInfoCuda = Ort::MemoryInfo("Cuda", OrtAllocatorType::OrtDeviceAllocator, 0, OrtMemType::OrtMemTypeDefault);
        
        rec_data = 0.0f;
        for(int i = 0; i < 4; i++)
            rec_dims[i] = 1;
        r1i = Ort::Value::CreateTensor<float>(memoryInfo, &rec_data, 1, rec_dims, 4);
    }

    virtual ~BackgroundSubtractorRobustVideoMattingONNX()
    {
        delete io_binding;
        delete session;
    }

    virtual void restart()
    {
        firstFrame = true;
        delete io_binding;
        io_binding = new Ort::IoBinding(*session);
    }

    virtual void apply(cv::InputArray image, cv::OutputArray _fgmask, double learningRate=-1)
    {
        const cv::Mat &img = image.getMat();
        cv::Rect ROI2 = getROI();
        if(ROI2.empty())
            ROI2 = cv::Rect(0,0,img.cols,img.rows);
        if(firstFrame) {
            downsample_ratio = 0.25f * 1080 / img.rows;
            downsample_ratio_dims[0] = 1;
            downsample_ratio_tensor = Ort::Value::CreateTensor<float>(memoryInfo, &downsample_ratio, 1, downsample_ratio_dims, 1);

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

            src_data = std::vector<float>(ROI2.width * ROI2.height * 3);
            src_dims[0] = 1; src_dims[1] = 3; src_dims[2] = ROI2.height; src_dims[3] = ROI2.width;
            src_tensor = Ort::Value::CreateTensor<float>(memoryInfo, src_data.data(), src_data.size(), src_dims, 4);
            firstFrame = false;
        }
        cv::Mat blobMat;
        cv::dnn::blobFromImage(img(ROI2), blobMat);
        
        src_data.assign(blobMat.begin<float>(), blobMat.end<float>());
        for(size_t i = 0; i < src_data.size(); i++)
        	src_data[i] /= 255;
        
        io_binding->BindInput("src", src_tensor);
        session->Run(Ort::RunOptions{nullptr}, *io_binding);
        
        std::vector<std::string> outputNames = io_binding->GetOutputNames();
        std::vector<Ort::Value> outputValues = io_binding->GetOutputValues();
        
        cv::Mat &mask = _fgmask.getMatRef();
        mask.create(image.size(), CV_8UC1);
        if(ROI2.size() != mask.size())
            mask.setTo(cv::Scalar(0));
        for(int i = 0; i < outputNames.size(); i++) {
        	if(outputNames[i] == "pha") {
        		const cv::Mat outputImg(ROI2.height, ROI2.width, CV_32FC1, const_cast<float*>(outputValues[i].GetTensorData<float>()));
        		outputImg.convertTo(mask(ROI2), CV_8UC1, 255.0);
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

LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorRobustVideoMattingONNXRawPtr(const char *onnxModelFilename, bool use_GPU)
{
    return new BackgroundSubtractorRobustVideoMattingONNX(onnxModelFilename, use_GPU);
}

#else
LQMR_EXPORTS BackgroundSubtractor *createBackgroundSubtractorRobustVideoMattingONNXRawPtr(const char *onnxModelFilename, bool use_GPU)
{
    printf("BackgroundSubtractorRobustVideoMattingONNX unavailable, rebuild with ONNX runtime\n");
    return NULL;
}

#endif

}
#endif

