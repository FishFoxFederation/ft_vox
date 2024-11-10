# Network

[TOC]

## Intro

## Layers

There are a few layers to our network stack  
- the basic socket and queueing of message / polling is abstracted in differents classes,  
	This is implemented by the Connection class
- An abstract classes representing a packet that is sometimes serialized to a connection
  or deserialized from a connection
- A PacketFactory class that can check if a packet is complete in a Connection and instanciate the correct packet and read extract that packet from the connection buffer
- A Higher level class that composes a poller as well as some connections to send and receive packets
  The server and the client both have a different one because of different needs ( a client has only one connection )
- A PacketHandler class that queries the queue of incoming packets in the Server or Client class
  to execute logic

### Sockets, Connections and Poller

Socket are wrapped in the Connection class to make them RAII compliant.  
The Connection class has 2 exposed mutexes, it is the responsibility of the user to lock the read mutex when doing operation on the read buffer and to lock the write mutex when doing operations on the write buffer.

The poller class wrapps around epoll to be able to poll through all the Connections


#### Packets and the Packet Factory

TODO create uml graph of a packet's life 
