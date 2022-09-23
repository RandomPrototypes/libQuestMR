cmake_minimum_required(VERSION 3.11.0)
project(libQuestMR VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_DEBUG_POSTFIX "d")

include(FetchContent)

include(CTest)
enable_testing()

set(EXTERNAL_INSTALL_LOCATION deps)

FetchContent_Populate(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 1dee28e51f9175a31955b9791c74c430fe13dc82
)

find_package(BufferedSocket REQUIRED)
message("BufferedSocket dir: ${BufferedSocket_DIR}")
set(BufferedSocket_INCLUDE_DIR ${BufferedSocket_DIR}/../../../include)
include_directories(${BufferedSocket_INCLUDE_DIR})

SET(tinyxml2 ${tinyxml2_SOURCE_DIR})

option(BUILD_DEMO "Build DEMO" OFF)
option(USE_OPENCV "Use OpenCV" ON)
option(USE_DEPTHAI "Use DepthAI" OFF)
option(USE_RPCameraInterface "Use RPCameraInterface" ON)
option(USE_FFMPEG "Use FFMPEG" ON)
option(USE_ONNX_RUNTIME "Use ONNX Runtime" ON)
option(DEBUG_OCULUS_MRC_SOURCE "Debug Oculus MRC Source" OFF)

if(USE_ONNX_RUNTIME)
	option(USE_ONNX_RUNTIME_CUDA "Use ONNX Runtime with CUDA" OFF)
	option(USE_ONNX_RUNTIME_DIRECTML "Use ONNX Runtime with DirectML" OFF)
	find_path(ONNX_RUNTIME_SESSION_INCLUDE_DIRS onnxruntime_cxx_api.h HINTS /usr/local/include/onnxruntime/core/session)
	find_library(ONNX_RUNTIME_LIB onnxruntime HINTS /usr/local/lib)
	include_directories(${ONNX_RUNTIME_SESSION_INCLUDE_DIRS})
endif()

if(USE_DEPTHAI)
	find_package(depthai CONFIG REQUIRED)
endif()

if(USE_OPENCV)
	find_package(OpenCV REQUIRED video dnn calib3d)
	set(OpenCV_LIBS_minimal ${OpenCV_LIBS})
	if(BUILD_DEMO)
		find_package(OpenCV REQUIRED highgui)
	endif()
endif()

if(USE_RPCameraInterface)
	find_package(RPCameraInterface REQUIRED)
	message("RPCameraInterface include dir: ${RPCameraInterface_DIR}")
	set(RPCameraInterface_INCLUDE_DIR ${RPCameraInterface_DIR}/../../../include)
	include_directories(${RPCameraInterface_INCLUDE_DIR})
endif()

if(USE_FFMPEG)
	if(WIN32)
	   SET(FFMPEG_DIR "" CACHE STRING "ffmpeg dir")
	   SET(FFMPEG_INCLUDE_DIR "" CACHE STRING "ffmpeg include dir")
	   SET(FFMPEG_LIB_DIR "" CACHE STRING "ffmpeg lib dir")
	   if(FFMPEG_DIR AND (NOT FFMPEG_INCLUDE_DIR))
		  set(FFMPEG_INCLUDE_DIR ${FFMPEG_DIR}/include CACHE STRING "ffmpeg include dir" FORCE)
	   endif()
	   if(FFMPEG_DIR AND (NOT FFMPEG_LIB_DIR))
		  set(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib CACHE STRING "ffmpeg lib dir" FORCE)
	   endif()
	   include_directories(${FFMPEG_INCLUDE_DIR})
	   link_directories(${FFMPEG_LIB_DIR})
	else()
	   find_package(PkgConfig REQUIRED)
	   pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec libswresample libswscale libavutil)
	   #get_cmake_property(_variableNames VARIABLES)
	   #list (SORT _variableNames)
	   #foreach (_variableName ${_variableNames})
	   #message(STATUS "${_variableName}=${${_variableName}}")
	   #endforeach()
	   include_directories(${LIBAV_INCLUDE_DIRS})
	endif()
endif()

SET (LIB_INCLUDE
             include/libQuestMR/config.h
             src/frame.h
             src/log.h
             include/libQuestMR/QuestVideoMngr.h
             include/libQuestMR/QuestVideoTimestampRectifier.h
             include/libQuestMR/QuestCalibData.h
             include/libQuestMR/QuestFrameData.h
             include/libQuestMR/QuestCommunicator.h
             include/libQuestMR/QuestStringUtil.h
             include/libQuestMR/BackgroundSubtractor.h
             include/libQuestMR/PortableTypes.h
             ${tinyxml2}/tinyxml2.h)

SET (LIB_SRC
            src/frame.cpp
            src/QuestVideoMngr.cpp
            src/QuestVideoTimestampRectifier.cpp
            src/QuestCalibData.cpp
            src/QuestFrameData.cpp
            src/QuestCommunicator.cpp
            src/QuestStringUtil.cpp
            src/BackgroundSubtractor.cpp
            src/BackgroundSubtractorOpenCV.cpp
            src/BackgroundSubtractorChromaKey.cpp
            src/BackgroundSubtractorRobustVideoMattingONNX.cpp
            src/PortableTypes.cpp
            ${tinyxml2}/tinyxml2.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${BufferedSocket} ${tinyxml2})

add_library(libQuestMR SHARED ${LIB_SRC} ${LIB_INCLUDE})
target_compile_definitions(libQuestMR PRIVATE "LIBQUESTMR_EXPORTS")
set_target_properties(libQuestMR PROPERTIES VERSION ${PROJECT_VERSION})
#set_target_properties(libQuestMR PROPERTIES PUBLIC_HEADER ${LIB_INCLUDE})

if(USE_ONNX_RUNTIME)
    target_compile_definitions(libQuestMR PRIVATE "USE_ONNX_RUNTIME")
    target_link_libraries(libQuestMR PRIVATE ${ONNX_RUNTIME_LIB})
    if(USE_ONNX_RUNTIME_CUDA)
        target_compile_definitions(libQuestMR PRIVATE "USE_ONNX_RUNTIME_CUDA")
    endif()
    if(USE_ONNX_RUNTIME_DIRECTML)
        target_compile_definitions(libQuestMR PRIVATE "USE_ONNX_RUNTIME_DIRECTML")
    endif()
endif()

target_compile_definitions(libQuestMR PRIVATE "_CRT_SECURE_NO_WARNINGS")

target_link_libraries(libQuestMR PRIVATE BufferedSocket)

if(DEBUG_OCULUS_MRC_SOURCE)
	target_compile_definitions(libQuestMR PRIVATE DEBUG_OCULUS_MRC_SOURCE)
endif()

if(BUILD_DEMO)
	add_executable(libQuestMRDemo-connectToMRC ${LIB_INCLUDE} demo/demo-connectToMRC.cpp)
	add_executable(libQuestMRDemo-connectToMRC-raw ${LIB_INCLUDE} demo/demo-connectToMRC-raw.cpp)
	add_executable(libQuestMRDemo-capture ${LIB_INCLUDE} demo/demo-capture.cpp)
	add_executable(libQuestMRDemo-playback ${LIB_INCLUDE} demo/demo-playback.cpp)
	add_executable(libQuestMRDemo-loadQuestCalib ${LIB_INCLUDE} demo/demo-loadQuestCalib.cpp)
	add_executable(libQuestMRDemo-loadAndUploadQuestCalib ${LIB_INCLUDE} demo/demo-loadAndUploadQuestCalib.cpp)
	add_executable(libQuestMRDemo-uploadQuestCalib ${LIB_INCLUDE} demo/demo-uploadQuestCalib.cpp)
	add_executable(libQuestMRDemo-calibrateCameraIntrinsic-cv ${LIB_INCLUDE} demo/demo-calibrateCameraIntrinsic-cv.cpp demo/calibration_helper.h demo/calibration_helper.cpp)
	if(USE_RPCameraInterface)
		add_executable(libQuestMRDemo-calibrateCameraIntrinsic-RPCam ${LIB_INCLUDE} demo/demo-calibrateCameraIntrinsic-RPCam.cpp demo/calibration_helper.h demo/calibration_helper.cpp)
		add_executable(libQuestMRDemo-calibrateCameraExtrinsic-RPCam ${LIB_INCLUDE} demo/demo-calibrateCameraExtrinsic-RPCam.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-calibrateCameraFull-RPCam ${LIB_INCLUDE} demo/demo-calibrateCameraFull-RPCam.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-testCalib-RPCam ${LIB_INCLUDE} demo/demo-testCalib-RPCam.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-capture-RPCam ${LIB_INCLUDE} demo/demo-capture-RPCam.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-captureRaw-RPCam ${LIB_INCLUDE} demo/demo-captureRaw-RPCam.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-processRawCapture ${LIB_INCLUDE} demo/demo-processRawCapture.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-processRawCaptureAudio ${LIB_INCLUDE} demo/demo-processRawCaptureAudio.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-chromakey-RPCam ${LIB_INCLUDE} demo/demo-chromakey-RPCam.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
		add_executable(libQuestMRDemo-segmentation-RPCam ${LIB_INCLUDE} demo/demo-segmentation-RPCam.cpp demo/RPCam_helper.h demo/RPCam_helper.cpp)
	endif()
endif()

if(USE_OPENCV)
	target_link_libraries(libQuestMR PRIVATE ${OpenCV_LIBS_minimal})
endif()

if(WIN32)
	target_link_libraries(libQuestMR PRIVATE ws2_32)
endif()

if(USE_FFMPEG)
	if(WIN32)
		if(MSVC)
			target_link_libraries(libQuestMR PRIVATE avdevice avfilter avformat avcodec swresample swscale avutil)
		else()
			target_link_libraries(libQuestMR PRIVATE libavdevice libavfilter libavformat libavcodec libswresample libswscale libavutil)
		endif()
	else()
		target_link_libraries(libQuestMR PRIVATE PkgConfig::LIBAV)
	endif()
endif()

if(BUILD_DEMO)
	target_link_libraries(libQuestMRDemo-connectToMRC PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo-connectToMRC LINK_PUBLIC libQuestMR BufferedSocket)
	target_link_libraries(libQuestMRDemo-connectToMRC-raw PRIVATE ${OpenCV_LIBS})
   	target_link_libraries(libQuestMRDemo-connectToMRC-raw LINK_PUBLIC libQuestMR BufferedSocket)
   	target_link_libraries(libQuestMRDemo-capture PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo-capture LINK_PUBLIC libQuestMR BufferedSocket)
	target_link_libraries(libQuestMRDemo-playback PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo-playback LINK_PUBLIC libQuestMR BufferedSocket)
	target_link_libraries(libQuestMRDemo-loadQuestCalib PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo-loadQuestCalib LINK_PUBLIC libQuestMR BufferedSocket)
	target_link_libraries(libQuestMRDemo-loadAndUploadQuestCalib PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo-loadAndUploadQuestCalib LINK_PUBLIC libQuestMR BufferedSocket)
	target_link_libraries(libQuestMRDemo-uploadQuestCalib PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo-uploadQuestCalib LINK_PUBLIC libQuestMR BufferedSocket)
	target_link_libraries(libQuestMRDemo-calibrateCameraIntrinsic-cv PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo-calibrateCameraIntrinsic-cv LINK_PUBLIC libQuestMR BufferedSocket)
	if(USE_RPCameraInterface)
		target_link_libraries(libQuestMRDemo-calibrateCameraIntrinsic-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-calibrateCameraIntrinsic-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-calibrateCameraExtrinsic-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-calibrateCameraExtrinsic-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-calibrateCameraFull-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-calibrateCameraFull-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-testCalib-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-testCalib-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-capture-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-capture-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-captureRaw-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-captureRaw-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-processRawCapture PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-processRawCapture LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-processRawCaptureAudio PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-processRawCaptureAudio LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-chromakey-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-chromakey-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		target_link_libraries(libQuestMRDemo-segmentation-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(libQuestMRDemo-segmentation-RPCam LINK_PUBLIC libQuestMR BufferedSocket RPCameraInterface)
		if(USE_ONNX_RUNTIME)
			target_link_libraries(libQuestMRDemo-segmentation-RPCam PRIVATE ${ONNX_RUNTIME_LIB})
		endif()
	endif()
	install(TARGETS libQuestMRDemo-connectToMRC RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-connectToMRC-raw RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-capture RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-playback RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-loadQuestCalib RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-loadAndUploadQuestCalib RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-uploadQuestCalib RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-calibrateCameraIntrinsic-cv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-calibrateCameraIntrinsic-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-calibrateCameraExtrinsic-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-calibrateCameraFull-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-testCalib-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-capture-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-captureRaw-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-processRawCapture RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-processRawCaptureAudio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-chromakey-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(TARGETS libQuestMRDemo-segmentation-RPCam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS libQuestMR  EXPORT libQuestMRConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})
#install(TARGETS libQuestMR DESTINATION lib/ EXPORT libQuestMRConfig)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION . PATTERN "*.h")	
install(EXPORT libQuestMRConfig DESTINATION lib/cmake/libQuestMR)
export(TARGETS libQuestMR FILE libQuestMRConfig.cmake)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

if(NOT MSVC)
	set(CMAKE_THREAD_LIBS_INIT "-lpthread")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

include(CPack)
