cmake_minimum_required(VERSION 3.11.0)
project(libQuestMR VERSION 0.1 LANGUAGES CXX)

include(FetchContent)

include(CTest)
enable_testing()

set(EXTERNAL_INSTALL_LOCATION deps)

FetchContent_Populate(
    BufferedSocket
    GIT_REPOSITORY https://github.com/RandomPrototypes/BufferedSocket.git
    GIT_TAG 6feeef15136bd2157b144936619808db28cca51e
)

FetchContent_Populate(
    tinyxml2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
    GIT_TAG 1dee28e51f9175a31955b9791c74c430fe13dc82
)

SET(BufferedSocket ${bufferedsocket_SOURCE_DIR})
SET(tinyxml2 ${tinyxml2_SOURCE_DIR})

option(USE_OPENCV "Use OpenCV" ON)
option(USE_RPCameraInterface "Use RPCameraInterface" ON)
option(BUILD_DEMO "Build DEMO" ON)

if(USE_OPENCV)
	find_package(OpenCV REQUIRED)
endif()

if(USE_RPCameraInterface)
	find_package(RPCameraInterface REQUIRED)
	message("RPCameraInterface include dir: ${RPCameraInterface_DIR}")
	set(RPCameraInterface_INCLUDE_DIR ${RPCameraInterface_DIR}/../../../include)
	include_directories(${RPCameraInterface_INCLUDE_DIR})
endif()

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
endif()

SET (LIB_INCLUDE
			 include/libQuestMR/config.h
			 include/libQuestMR/frame.h
             include/libQuestMR/log.h
             include/libQuestMR/QuestVideoMngr.h
             include/libQuestMR/QuestCalibData.h
             include/libQuestMR/QuestFrameData.h
             include/libQuestMR/QuestCommunicator.h
             include/libQuestMR/QuestStringUtil.h
             ${tinyxml2}/tinyxml2.h)

SET (LIB_SRC
		    src/frame.cpp
            src/QuestVideoMngr.cpp
            src/QuestCalibData.cpp
            src/QuestFrameData.cpp
            src/QuestCommunicator.cpp
            src/QuestStringUtil.cpp
            ${tinyxml2}/tinyxml2.cpp
            ${BufferedSocket}/BufferedSocket.h
    		${BufferedSocket}/BufferedSocket.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${BufferedSocket} ${tinyxml2})

add_library(libQuestMR SHARED ${LIB_SRC} ${LIB_INCLUDE})
set_target_properties(libQuestMR PROPERTIES VERSION ${PROJECT_VERSION})
#set_target_properties(libQuestMR PROPERTIES PUBLIC_HEADER ${LIB_INCLUDE})

if(BUILD_DEMO)
	add_executable(libQuestMRDemo ${LIB_INCLUDE} demo/main.cpp)
	add_executable(demo-connectToMRC ${LIB_INCLUDE} demo/demo-connectToMRC.cpp)
	add_executable(demo-connectToMRC-raw ${LIB_INCLUDE} demo/demo-connectToMRC-raw.cpp)
	add_executable(demo-capture ${LIB_INCLUDE} demo/demo-capture.cpp)
	add_executable(demo-playback ${LIB_INCLUDE} demo/demo-playback.cpp)
	add_executable(demo-loadQuestCalib ${LIB_INCLUDE} demo/demo-loadQuestCalib.cpp)
	add_executable(demo-calibrateCameraIntrinsic-cv ${LIB_INCLUDE} demo/demo-calibrateCameraIntrinsic-cv.cpp demo/calibration_helper.h demo/calibration_helper.cpp)
	if(USE_RPCameraInterface)
		add_executable(demo-calibrateCameraIntrinsic-RPCam ${LIB_INCLUDE} demo/demo-calibrateCameraIntrinsic-RPCam.cpp demo/calibration_helper.h demo/calibration_helper.cpp)
	endif()
endif()

if(USE_OPENCV)
	target_link_libraries(libQuestMR PRIVATE ${OpenCV_LIBS})
endif()

if(WIN32)
	target_link_libraries(libQuestMR PRIVATE libavdevice libavfilter libavformat libavcodec libswresample libswscale libavutil)
	target_link_libraries(libQuestMR PRIVATE ws2_32)
else()
	target_link_libraries(libQuestMR PRIVATE PkgConfig::LIBAV)
endif()

if(BUILD_DEMO)
	target_link_libraries(libQuestMRDemo PRIVATE ${OpenCV_LIBS})
	target_link_libraries(libQuestMRDemo LINK_PUBLIC libQuestMR)
	target_link_libraries(demo-connectToMRC PRIVATE ${OpenCV_LIBS})
	target_link_libraries(demo-connectToMRC LINK_PUBLIC libQuestMR)
	target_link_libraries(demo-connectToMRC-raw PRIVATE ${OpenCV_LIBS})
   	target_link_libraries(demo-connectToMRC-raw LINK_PUBLIC libQuestMR)
   	target_link_libraries(demo-capture PRIVATE ${OpenCV_LIBS})
	target_link_libraries(demo-capture LINK_PUBLIC libQuestMR)
	target_link_libraries(demo-playback PRIVATE ${OpenCV_LIBS})
	target_link_libraries(demo-playback LINK_PUBLIC libQuestMR)
	target_link_libraries(demo-loadQuestCalib PRIVATE ${OpenCV_LIBS})
	target_link_libraries(demo-loadQuestCalib LINK_PUBLIC libQuestMR)
	target_link_libraries(demo-calibrateCameraIntrinsic-cv PRIVATE ${OpenCV_LIBS})
	target_link_libraries(demo-calibrateCameraIntrinsic-cv LINK_PUBLIC libQuestMR)
	if(USE_RPCameraInterface)
		target_link_libraries(demo-calibrateCameraIntrinsic-RPCam PRIVATE ${OpenCV_LIBS})
		target_link_libraries(demo-calibrateCameraIntrinsic-RPCam LINK_PUBLIC libQuestMR RPCameraInterface)
	endif()
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

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

include(CPack)
