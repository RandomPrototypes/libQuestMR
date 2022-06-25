#!/bin/bash

libQuestMR_branch=$(git symbolic-ref --short HEAD)
if [ $libQuestMR_branch = "master" ]; then
	BufferedSocket_branch="master"
	RPCameraInterface_branch="master"
	onnxruntime_branch="v1.10.0"
elif [ $libQuestMR_branch = "dev" ]; then
	BufferedSocket_branch="dev"
	RPCameraInterface_branch="dev"
	onnxruntime_branch="v1.10.0"
else
	BufferedSocket_branch="v1.0.0"
	RPCameraInterface_branch="v1.0.0"
	onnxruntime_branch="v1.10.0"
fi
