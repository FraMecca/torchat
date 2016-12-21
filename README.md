# TORchat

- [Status](#Status)
- [Features](#Features)
- [How it works](#How-it-works)
- [Design](#Design)
- [Building](#Building)
- [Usage](#Usage)
- [Development](#Development)
- [Disclaimer](#Disclaimer)
- [Todo](#Todo)

A simple chat client for the TOR network using [mongoose](cesanta.org/mongoose).
Inspired by [TorChat](https://github.com/prof7bit/TorChat)
It is written in C with bindings for C++ libraries (json and loguru)

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

## Design

TORchat is not a protocol and uses standard JSON messages for communication.

TORchat is divided into a daemon and a client completely independent of each other.
The daemon continuosly runs and gathers messages from other peers and stores them in a volatile hash table (and logs, if configured accordingly).
The client may connect at any time, read the received messages and chat with peers.
The client send commands to the server using JSON.
For a list of possible commands, check the [Development section](#Development)

## Building

TORchat has no external dependencies and just requires a C++11 compatible compiler.

Just run make and set LD_LIBRARY_PATH as the build directory.

## Usage

~~ still no cli arguments ~~

## Development
===

#### Daemon

The daemon aims to be as  small as possible. It has no external dependency and is written in less that 1000 loc. 
Currently it supports only Linux and aims to do so.

The daemon uses [mongoose](cesanta.org/mongoose) to manage events, TOR as a socks5 proxy, [loguru](github.com) to mantain logs and [json](github.com/nlohmann/json) for communication.

The core of the daemon is written in C with bindings to embedded libraries in C++.

Until the exit procedure is called, the daemon waits for messages from peers or clients (mg_mgr_poll) and acts accordingly to the JSON received.

An [hash table](troydhanson/uthash)is mantained and used to store all the unread messages from the peers. As soon as a client connects, the read messages are removed from the hash table.

The daemon only mantains two logs: one for the messages, one for the errors.
Separate functions which enable to parse and divide the logs are provided.

There is an ongoing discussion about the possibility of adding encryption (maybe OTR) on top of the TOR layer.

#### Client

Clients are indipendent of the daemon.
Currently a small curses python client is provided.

#### JSON

JSON is used for communication, both IPC and sockets.
One possible JSON may be:

```
/*
 * json j = {
 * {"cmd" = SEND},
 * {"portno" = 80},
 * {"id" = "ld74fqvoxpu5yi73.onion" },
 * {"date" = "31-10-2016"},
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
* HISTORY : the client is asking the daemon for the previous n ("msg" field) lines of conversation with a peer ("id" field);
* END : the daemon notifies that the previous command has no response;
* EXIT : starts exit procedure (clean datastructs and exit cleanly).

## Disclaimer

Please note that TORchat is produced *independently* from the Tor® anonymity software, I am not related with or sponsored by torproject.org. TORchat is making use of the Tor® client software but TorChat itself is a completely separate project developed by totally different people, so if you instead want to buy the developers of Tor® from torproject.org a beer (they deserve it even more than me and without their great Tor software my little program would not have been possible) then please consider doing so at the following address:

* https://www.torproject.org/donate/donate.html.en

### Todo

* group chats
* parse log files
* file upload
