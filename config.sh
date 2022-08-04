#!/bin/bash

libQuestMR_branch=$(git symbolic-ref --short HEAD)
if [ $libQuestMR_branch = "master" ]; then
	BufferedSocket_branch="master"
	RPCameraInterface_branch="master"
elif [ $libQuestMR_branch = "dev" ]; then
	BufferedSocket_branch="dev"
	RPCameraInterface_branch="dev"
else
	BufferedSocket_branch="v1.0.0"
	RPCameraInterface_branch="v1.0.0"
fi
onnxruntime_branch="v1.10.0"
onnxruntime_precompiled_archive="https://github.com/microsoft/onnxruntime/releases/download/v1.10.0/onnxruntime-linux-x64-gpu-1.10.0.tgz"
onnxruntime_precompiled_folder="onnxruntime-linux-x64-gpu-1.10.0"
