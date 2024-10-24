# FishFoxFederation's VOX

## [Documentation](https://fishfoxfederation.github.io/ft_vox/)

## Quickstart

Clone the repo  
`git clone https://github.com/FishFoxFederation/ft_vox.git`  
install [vulkan](https://vulkan.lunarg.com/sdk/home)
`apt install vulkan-sdk`

then you can build the project, you must have g++11 installed

`cd ft_vox && sh build.sh`

once build is complete you can launch the server using

`./build/server [port]`

and you can run the client using this command

`./build/ft_vox [ip] [port] [userid]`

by default the ip is localhost and the port is 4245
and you cna use any userid you want as long as there isnt another player on the server with the same userid otherwise the server will reject the connection

# How to play

Press C to bind the mouse to the window  
WASD to move  
SPACE to jump or fly up  
1 to stop flying  
2 to start flying

