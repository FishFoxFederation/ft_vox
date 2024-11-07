#!/bin/bash

# launch multicapture
./tracy-multicapture -o output -M -f &

PID_MULTICAPTURE=$!

# launch server
./build/server &
PID_SERVER=$!

sleep 1
# launch client
./build/ft_vox localhost 4245 5 2> /dev/null

# now we wait for client to stop exec
#( we do nothing because its not launched async )
# when client stop kill server

kill -int $PID_SERVER
wait $PID_MULTICAPTURE

# merge traces

./tracy-merge -o wonder.tracy -f output*

# launch tracy
./tracy-profiler wonder.tracy
