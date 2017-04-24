# TORchat

- [Status](#Status)
- [Features](#Features)
- [How it works](#How-it-works)
- [Try it](#Try-it)
- [Design](#Design)
- [Building](#Building)
- [Usage](#Usage)
- [Development](#Development)
	- [Daemon](#Daemon)
	- [Client](#Client)
	- [JSON](#JSON)
	- [Protocol](#Protocol)
	- [Coroutines](#Coroutines)
- [Disclaimer](#Disclaimer)
- [Todo](#Todo)

A simple chat client for the TOR network.
Inspired by [TorChat](https://github.com/prof7bit/TorChat).
It is written in C and C++.


## Status

**This is a work in progress.** TORchat should be stable enough to hack on
and test, but has not been tested in production or for any length of time.

Breaking changes are frequent and stability is not guaranteed at current time.

## Features 
TORchat is an experimental P2P chat client that runs on the TOR network and allows you to:

* chat securely with other peers on the network
* send messages without leaving the TOR network
* chat without exposing your identity (or IP address)

## How it works
TORchat is a peer-to-peer instant messaging system built over the TOR Network [hidden services](https://www.torproject.org/docs/hidden-services.html.en). Your identity is your hidden service address, and contacts connect to you without leaving Tor. The rendezvous system makes it extremely hard for anyone to learn your identity from your address.

## Try it

At the current state, the various torrc values are embedded onto .c files. Later options and a shell script will be provided for that.

Move into the TORchat folder and compile:

` git clone http://francescomecca.eu:3000/pesceWanda/torchat `

Or, if you preferer

` git clone https://github.com/framecca/torchat `

Then

` cd torchat `

` bash install.sh `

Now start tor with the provided torrc

` tor -f tor/torrc `

In case TOR complains about folder permission do `chmod 700 ./tor` to set the appropriate permissions.

Now start the server

`./build/main 8000 `

The server listens for incoming connection from the port 8000. TOR redirects the traffic from port 80 of the hidden service to the server, trasparently.

Your peer id is:

` cat tor/hostname `

As a client, you can use [this](https://github.com/GallaFrancesco/Torchat_Client).


## Design

TORchat uses standard JSON messages for communication plus a size indentifier.

TORchat is divided into a daemon and a client completely independent of each other.
The daemon continuosly runs and gathers messages from other peers and stores them in a volatile hash table (and logs, if configured accordingly).
The client may connect at any time, read the received messages and chat with peers.
The client send commands to the server using JSON.
For a list of possible commands, check the [Development section](#Development)

## Building

<!--TORchat has no external dependencies and just requires a C++11 compatible compiler.-->
TORchat requires a C++11 compatible compiler.
To build the standar version (without debug logging), simply run:

` make `

To build the debug version, with improved logging and coredump for crashes, run:

` make debug `

## Usage

At the moment the only command line option that is provided is the daemon mode:

` ./build/main -d 8000 `

Without daemon mode, the server keeps logging on standard output (that is, on the current shell).
With the daemon mode option, it detaches from the shell and continues its execution in background, therefore it can be monitored only through logs, which are kept in the main directory of the repository.

## Development

#### Daemon

The daemon aims to be as  small as possible. <!--It has no external dependency and is written in less that 1000 loc.--> 
Currently it supports only Linux and aims to do so.

The daemon uses a combination of epoll plus [libdill](https://github.com/sustrik/libdill) to manage events, TOR as a socks5 proxy, [loguru](https://github.com/emilk/loguru) to mantain logs, [json](https://github.com/nlohmann/json) for communication and [proxysocket](https://github.com/brechtsanders/proxysocket) to initialize the socks5 proxy connection.

The core of the daemon is written in C with bindings to embedded libraries in C++.

Until the exit procedure is called, the daemon waits for messages from peers or clients (event_poll) and acts accordingly to the JSON received.

An [hash table](https://troydhanson.github.io/uthash/)is mantained and used to store all the unread messages from the peers. As soon as a client connects, the read messages are removed from the hash table.

The daemon only mantains two logs: one for the messages, one for the errors.
Separate functions which enable to parse and divide the logs are provided.

There is an ongoing discussion about the possibility of adding encryption (maybe OTR) on top of the TOR layer.

#### Client

Clients are independent of the daemon. To work properly, a "basic" client must be:
 * Capable of sending messages though sockets
 * Capable of parsing a JSON structure

Currently a small python client is provided [here](https://github.com/GallaFrancesco/Torchat_Client.git).
It is based on curses, specifically on the ui from: [calzoneman/python-chatui](https://github.com/calzoneman/python-chatui.git).
To use it, move to the repository main directory and:

```
python3 client.py localhost 8000
```
*localhost can be replaced with any other host on which the TORchat daemon is running.*

It will ask for a peer (an onion address) to connect with, and then it will support the following actions:
 * To write a message to the peer selected, simply write and press enter;
 * To send a command to the client/server and perform specific actions, head to the command table provided below. Commands are all preceded by a '/' sign.

| 	Command		| 	Action							|
| ------------- | ----------------------------------|
| 	**/peer**	| Change the current peer.  		|
| 	**/exit**	| Close the client and the server.  |
| 	**/quit**	| Close the client only.  			|

#### JSON

JSON is used for communication, both IPC and sockets.
One possible JSON may be:

```
/*
 * json j = {
 * {"date" = "31-10-2016"}, // not always used
 * {"cmd" = SEND},
 * {"portno" = 80},
 * {"id" = "ld74fqvoxpu5yi73.onion" },
 * {"msg" = "Alice says hi"}
 * }
 */
 ```

The `cmd` field is a set of standard commands understood by the daemon that execute different tasks based on that command.
Some commands can only be sent from a client on the same host, not from a peer.
Commands are:

* SEND : the client is trying to reach for a peer ("id" field, "port" field) and send him a message ("msg" field);
* RECV : a peer ("id" field) has contacted the daemon and sent him a message. Store it in the hash table until it is read;
* UPDATE : the client is polling for unread messages from a peer ("id" field);
* GET_PEERS : the client asks the daemon for the id of the peers that wrote one or more messages;
* HISTORY : the client is asking the daemon for the previous n ("msg" field) lines of conversation with a peer ("id" field); // to be implemented
* HOST : the client is asking the daemon for the current hostname, that is, its current onion address;
* END : the daemon notifies that the previous command has succeded and that communication can end;
* EXIT : starts exit procedure (clean datastructs and exit cleanly).
* ERR : in case TOR can't send the message or there is a sock failure, it reports the error;
* FILE... : there are 4 enum values relative to the file upload process, which are exchanged between the servers and the client requesting an upload.  // to be implemented

The `date` field is used only when the daemon communicates with the server. It must not be used when sending message between different hosts.

#### Protocol

TORchat uses raw tcp packets for communication. They are prefixed with a char lenght[4] that specifies the dimension of the message that is being sent. 
The daemon rejects every message that hasn't got a char[4] message size specifier padded from left(e.g.: 0084{...content of json...}).

#### Coroutines

TORchat by design uses structured concurrency because there isn't a real need for thread parallelism.
When epoll detects action in a socket, the main event loop launches a coroutine that yields only when waiting for TOR to successfully open a connection with the endpoint (the other peer's daemon in this case).

Given that sometimes the connection to the endpoint may take up to two minutes, a real thread is spawned in order to estabilish the connection.

## Disclaimer

Please note that TORchat is produced *independently* from the Tor® anonymity software, I am not related with or sponsored by torproject.org. TORchat is making use of the Tor® client software but TorChat itself is a completely separate project developed by totally different people, so if you instead want to buy the developers of Tor® from torproject.org a beer (they deserve it even more than me and without their great Tor software my little program would not have been possible) then please consider doing so at the following address:

* https://www.torproject.org/donate/donate.html.en
