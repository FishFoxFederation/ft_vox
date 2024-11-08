#!/bin/bash

ROOT_DIR=$(pwd)
TRACY_MULTICAPTURE_DIR=$ROOT_DIR/external/tracy-experimental/capture
TRACY_MERGE_DIR=$ROOT_DIR/external/tracy-experimental/multiprocess
TRACY_PROFILER_DIR=$ROOT_DIR/external/tracy/profiler


if [ ! -d ./tracy ]
then
	echo "building"
	mkdir -p tracy

	# build tracy-multicapture
	cd $TRACY_MULTICAPTURE_DIR
	cmake -B build
	cmake --build build
	mv build/tracy-multicapture $ROOT_DIR/tracy

	# build tracy-merge
	cd $TRACY_MERGE_DIR
	cmake -B build
	cmake --build build
	mv build/tracy-merge $ROOT_DIR/tracy

	# build tracy-profiler
	cd $TRACY_PROFILER_DIR
	cmake -B build -D LEGACY=ON
	cmake --build build
	mv build/tracy-profiler $ROOT_DIR/tracy
fi

# launch multicapture
cd $ROOT_DIR/tracy
./tracy-multicapture -o output -M -f &

PID_MULTICAPTURE=$!

# launch server
$ROOT_DIR/build/server &
PID_SERVER=$!

sleep 1
# launch client ( from root dir)
cd $ROOT_DIR
./build/ft_vox localhost 4245 5 2> /dev/null
cd $ROOT_DIR/tracy

# now we wait for client to stop exec
#( we do nothing because its not launched async )
# when client stop kill server

kill -int $PID_SERVER
wait $PID_MULTICAPTURE

# merge traces

./tracy-merge -o wonder.tracy -f output*

# launch tracy
./tracy-profiler wonder.tracy
