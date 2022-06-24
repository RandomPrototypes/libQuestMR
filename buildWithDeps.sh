#!/bin/bash

has_pkg()
{
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' $1|grep "install ok installed")
	if [ "" = "$PKG_OK" ]; then
		false
		return
	else
		true
		return
	fi
}

USE_CUDA=0
for i in "$@"; do
  case $i in
    --cuda)
      USE_CUDA=1
      shift # past argument with no value
      ;;
    -*|--*)
      echo "Unknown option $i"
      exit 1
      ;;
    *)
      ;;
  esac
done


BASE_FOLDER=$(pwd)
mkdir deps
cd deps

DEPS_FOLDER=$BASE_FOLDER/deps
CUDA_HOME=/usr/local/cuda
CUDNN_HOME=/usr/lib/x86_64-linux-gnu/

BUILD_TYPE=Release

declare -a listPkg=("build-essential" "git" "libopencv-dev" "libavdevice-dev" "libavfilter-dev" "libavformat-dev" "libavcodec-dev" "libswresample-dev" "libswscale-dev" "libavutil-dev")

#install package dependencies
for pkgname in "${listPkg[@]}"
do
	if $(has_pkg $pkgname)
	then
		echo "$pkgname detected"
	else
		echo "$pkgname not detected"
		sudo apt-get install "$pkgname"
	fi
done

#install cmake v3.22.3
cd $DEPS_FOLDER
rm cmake-3.22.3.tar.gz*
wget https://github.com/Kitware/CMake/releases/download/v3.22.3/cmake-3.22.3.tar.gz
tar -xf cmake-3.22.3.tar.gz
cd cmake-3.22.3
mkdir install
./bootstrap --prefix=install
make -j8
make install
CUSTOM_CMAKE=$DEPS_FOLDER/cmake-3.22.3/install/bin/cmake


#install onnxruntime
cd $DEPS_FOLDER
git clone --recursive --branch v1.10.0 https://github.com/Microsoft/onnxruntime
cd onnxruntime
mkdir install
if [ $USE_CUDA = 1 ]; then
	./build.sh --cmake_path $CUSTOM_CMAKE --cuda_home $CUDA_HOME --cudnn_home $CUDNN_HOME --use_cuda --config $BUILD_TYPE --build_shared_lib --skip_tests --parallel 8
else
	./build.sh --cmake_path $CUSTOM_CMAKE --config $BUILD_TYPE --build_shared_lib --skip_tests --parallel 8
fi
cd build/Linux/$BUILD_TYPE
make DESTDIR=$DEPS_FOLDER/onnxruntime/install install
ONNX_RUNTIME_DIR=$DEPS_FOLDER/onnxruntime/install/usr/local

#install BufferedSocket
cd $DEPS_FOLDER
git clone https://github.com/RandomPrototypes/BufferedSocket.git
cd $DEPS_FOLDER/BufferedSocket
mkdir build
mkdir install
cd build
$CUSTOM_CMAKE -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
make -j8
make install
BUFFERED_SOCKET_CMAKE_DIR=$DEPS_FOLDER/BufferedSocket/install/lib/cmake/BufferedSocket


#install RPCameraInterface
cd $DEPS_FOLDER
git clone https://github.com/RandomPrototypes/RPCameraInterface.git
cd RPCameraInterface
mkdir build
mkdir install
cd build
$CUSTOM_CMAKE -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBufferedSocket_DIR=$BUFFERED_SOCKET_CMAKE_DIR ..
make -j8
make install
RP_CAMERA_INTERFACE_CMAKE_DIR=$DEPS_FOLDER/RPCameraInterface/install/lib/cmake/RPCameraInterface

#build libQuestMR
cd $BASE_FOLDER
mkdir build
mkdir install
rm -r install/*
cd build
$CUSTOM_CMAKE -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DUSE_FFMPEG=ON -DUSE_OPENCV=ON -DUSE_RPCameraInterface=ON -DUSE_ONNX_RUNTIME=ON -DBufferedSocket_DIR=$BUFFERED_SOCKET_CMAKE_DIR -DRPCameraInterface_DIR=$RP_CAMERA_INTERFACE_CMAKE_DIR -DONNX_RUNTIME_LIB=$ONNX_RUNTIME_DIR/lib/libonnxruntime.so -DONNX_RUNTIME_SESSION_INCLUDE_DIRS=$ONNX_RUNTIME_DIR/include/onnxruntime/core/session ..
make -j8
make install
LIBQUESTMR_INSTALL_DIR=$BASE_FOLDER/install

#copy the onnxruntime libs to the install folder
ln -s $ONNX_RUNTIME_DIR/lib/libonnxruntime.so.1.10.0 $LIBQUESTMR_INSTALL_DIR/lib/libonnxruntime.so.1.10.0
ln -s $LIBQUESTMR_INSTALL_DIR/lib/libonnxruntime.so.1.10.0 $LIBQUESTMR_INSTALL_DIR/lib/libonnxruntime.so
if [ $USE_CUDA = 1 ]; then
	ln -s $LIBQUESTMR_INSTALL_DIR/lib/libonnxruntime_providers_cuda.so $LIBQUESTMR_INSTALL_DIR/lib/libonnxruntime_providers_cuda.so
	ln -s $LIBQUESTMR_INSTALL_DIR/lib/libonnxruntime_providers_shared.so $LIBQUESTMR_INSTALL_DIR/lib/libonnxruntime_providers_shared.so
fi
