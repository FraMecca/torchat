import json
import socket
import readline
from time import localtime, strftime
import rlcompleter
from curses import wrapper
import curses
from time import sleep
import os
from threading import Thread, Lock
import threading

from libpy import Torchat
from ui import ChatUI
# many thanks to https://github.com/calzoneman/python-chatui.git
# for this curses implementation of a chat UI

printBuf = list ()
lock = Lock() # a binary semaphore
exitFlag = False
currId = ""

class Completer(object):
    # this is a completer that works on the input buffer
    'The completer class for gnu readline'

    def __init__(self, options):
        self.options = sorted(options)

    def update (self, options):
        # use this method to add options to the completer
        self.options.extend (options)

    def complete(self, text, state=0):
        response = None
        if state == 0:
            # This is the first time for this text, so build a match list.
            if text:
                self.matches = [s for s in self.options if s and s.startswith(text)]
            else:
                self.matches = self.options[:]

        # Return the state'th item from the match list,
        # if we have that many.
        try:
            response = self.matches[state]
        except IndexError:
            response = ''
        return response

def print_line_cur (line, ui, color):
    # append sent messages and received messages to the buffer
    # then send them to the ui and pop them one by one
    global printBuf
    printBuf.append (line)
    for l in printBuf:
        ui.chatbuffer_add(l, color)
        printBuf.pop()


def update_routine(t, ui):
    # this function queries the server for unread messages
    # it runs until no messages from the given peer are left
    # then waits half a second and queries again
    global lock
    while True:
        if exitFlag:
            ui.close_ui()
            exit()
        resp = t.send_message (command="UPDATE", line=currId, currentId="localhost")
        # the json is not printed if no messages are received
        if resp['cmd'] == 'END':
            sleep(0.5)
        else:
            lock.acquire()
            print_line_cur ('[' + resp['date'] + '] ' + resp['msg'], ui, 3) 
            lock.release()

def send_input_message (msg, t, ui):
    # send message is multithread because socket recv is blocking
    resp = t.send_message(command="SEND", line=msg, currentId=currId, sendPort=80)
    if resp['cmd'] == 'ERR':
        print_line_cur(resp['msg'], ui, 1)

def elaborate_command (line, t, ui):
    global exitFlag
    global currId
    if line == '/exit':
    # this sends an exit to the client AND to the server
        t.send_message(command='EXIT', line='', id="localhost")
        exitFlag = True
        exit ()
    elif line == '/quit': 
        # only the client exits here
        exitFlag = True
        exit()
    elif line == '/peer': 
        # update peers list, possibly select a new one
        peerList, i = get_peers(t, ui)
        currId = peerList[i]
        ui.chatbuffer = []
        ui.linebuffer = []
        ui.redraw_ui(i)
    elif line == '/fileup': 
        # upload files: start by requiring a random port to the peer
        resp = t.send_message(command='FILEALLOC', line=currId, currentId="localhost")
        port = resp["msg"]
        print_line_cur ("You can send at port: " + port, ui, 2)


def input_routine (t, ui):
    c = Completer (['/help', '/exit', '/quit', '/peer', '/fileup'])
    while True:
        # the input is taken from the bottom window in the ui
        # and printed directly (it is actually sent below)
        line = ui.wait_input(completer = c)
        if len (line) > 0 and line[0] != '/':
            # here we send to mongoose / tor
            # if the user does not input a command send the message (done on a separate thread)
            print_line_cur (line, ui, 2)
            td = Thread(target=send_input_message, args=(line, t, ui))
            td.start ()
            c.update ([line])
        elif line != "":
            # the user input a command,
            # they start with /
            elaborate_command(line, t, ui)


def get_peers(t, ui):
    # ask for a list of peers with pending messages
    peerList = t.get_peers()
    rightId = False
    ui.userlist = list()

    # this part is the peer list UI management
    if peerList[0] == '': # no peers have written you!
        i = 0
        peerList[0] = ui.wait_input("Onion Address: ")
        ui.userlist.append(peerList[0])
        ui.redraw_userlist(i, t.onion)# this redraws only the user panel
        if currId != "":
            ui.userlist.append(currId)
    else:
        for userid in peerList: # print them all with an integer id associated
            ui.userlist.append(userid)
        if not currId in peerList and currId != "":
            ui.userlist.append(currId)
            peerList.append(currId)
        ui.redraw_userlist(None, t.onion) # this redraws only the user panel

        # this avoids error crashing while selecting an ID
        while not rightId: 
            choice = ui.wait_input("Peer Id (a number): ")
            try:
                i = int(choice) - 1
                if i >= len(peerList) or i < 0:
                    rightId = False
                else:
                    rightId = True
            except:
                rightId = False
        ui.redraw_userlist(i, t.onion) # this redraws only the user panel
    return peerList, i

def main (stdscr, portno):
    global currId

    # initialize UI class
    stdscr.clear()
    ui = ChatUI(stdscr)

    # initialize Torchat class
    t = Torchat('localhost', portno)
    peerList, i = get_peers(t, ui)
    currId = peerList[i]

    # here we use one thread to update unread messages in background,
    # the foreground one gets the input
    # they both work on the same buffer (printBuf) and thus a
    # semaphore is needed to prevent race conditions
    t1 = Thread(target=update_routine, args=(t, ui))
    t1.start()
    input_routine (t, ui)

# the wrapper is a curses function which performs a standard init process
# the ui init is then continued by the call to the ChatUi class (see main)
if __name__ == '__main__':
    from sys import argv
    wrapper(main, argv[1])
