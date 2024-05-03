# Network

[TOC]

## Intro

## Layers

There are a few layers to our network stack  
- the basic socket and queueing of message / polling is abstracted in differents classes
- A ( to be determined ) class handles higher level things, reading queues, detecting disconnections and instanciate commands classes when a packet has been fully received
- The command factory deserializes the complete packets and returns instances of Commands using polymorphism 
- The commands are then executed/dispatched on the update thread

### Sockets, Connections and Poller

Not much to say here just some classes to RAII and abstract away the not so fun stuff of unix and tcp sockets

### Commands

#### Command Factory

It is used when data has been received to detect complete packets and instanciate the correct command from it

#### 
