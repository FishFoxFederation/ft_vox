# Threads Architecture {#threads}

[TOC]

Our design is the fruit of a lot of discussions and headaches.  
It is based on 4 "control" threads + a thread pool for parallel tasking.  
All of the control threads inherit the AThreadWrapper class



## Main thread

**This is the main execution thread.**
- Handle menus
- Listen for key events and update the Input class
- Instanciates other threads when user wants to play

It is not used for any "gameplay" whatsoever it only setups and instanciates other threads to do these tasks

## Update Thread

**This is the "gameplay" thread it runs synced with the render thread.**
- Entity updates
	- Animation
	- AI pathfinding
	- physics ( sand falling )
- Player update
	- position and camera ( using the input class )
	- health
- GUIs update ( inventories, crafting tables etc... )

During all these tasks this thread will modify the scenes classes for the render thread to use. \
The update threads interacts a lot with the World class that contains all the blocks chunks, entities and the player data.

## Render Thread

**This is the thread where the magic happens it runs synced with the update thread.**
- Reads the scene classes to render frames

The render threads interacts with the renderAPI to render frames using the scenes classes as input

## Block Update Thread

**This is the thread where blocks are updated... duh. It is capped at 20 iterations per second**
- Performs block updates ( piston activation, water falling, trees growing )
- loads chunk when player moves
	- from the file save if possible
	- or generates new chunk
- unloads chunks once the player gets too far
- calculates new meshes if a change occured in a chunk

## Thread pool

**For every calculations that can be done in parallel**
- AI pathfinding
- Calculating new meshes etc...

We use [std::async](https://en.cppreference.com/w/cpp/thread/async).  
Wich uses a thread pool internally
