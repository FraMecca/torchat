import json
import socket
import readline
from time import localtime, strftime
import rlcompleter
from curses import wrapper
import curses
from ui import ChatUI # many thanks to https://github.com/calzoneman/python-chatui.git
                      # for this curses implementation of a chat UI
from time import sleep
import os
from threading import Thread, Lock
import threading

printBuf = list ()
lock = Lock() # a binary semaphore
exitFlag = False
currId = ""

def print_line_cur (line, ui, color):
    # append sent messages and received messages to the buffer
    # then send them to the ui and pop them one by one
    global printBuf
    printBuf.append (line)
    for l in printBuf:
        ui.chatbuffer_add(l, color)
        printBuf.pop()


class Completer(object):
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

def update_routine(portno, ui):
    # this function queries the server for unread messages
    # it runs until no messages from the given peer are left
    # then waits half a second and queries again
    global lock
    while True:
        if exitFlag:
            ui.close_ui()
            exit()
        j = create_json (cmd='UPDATE', msg=currId)
        resp = send_to_mongoose (j, portno, wait=True)
        # the json is not printed if no messages are received
        if resp['cmd'] == 'END':
            sleep(0.5)
        else:
            lock.acquire()
            print_line_cur ('[' + resp['date'] + '] ' + resp['msg'], ui, 3) 
            lock.release()

def elaborate_command (line, portno, ui):
    global exitFlag
    global currId
    # if line == '/help':
        # print ('Command list: ')
        # TODO
    # this sends an exit to the client AND to the server
    if line == '/exit':
        j = create_json(cmd='EXIT', msg='')
        send_to_mongoose(j, portno, wait=False)
        exitFlag = True
        exit ()
    elif line == '/quit':
        exitFlag = True
        exit()
    elif line == '/peer':
        peerList, i = get_peers(portno, ui)
        currId = peerList[i]
        return peerList, i

def input_routine (portno, ui):
    c = Completer (['/help', '/exit', '/quit', '/peer'])
    # readline.set_completer (c.complete)
    # readline.parse_and_bind ("tab: complete")
    # readline.parse_and_bind ("set editing-mode vi")
    while True:
        # the input is taken from the bottom window in the ui
        # and printed directly (it is actually send below)
        line = ui.wait_input(completer = c)
        # here we send to mongoose / tor
        if len (line) > 0 and line[0] != '/':
            # clearly the default action if the user does not input a command is
            # to send the message
            print_line_cur (line, ui, 2)
            j = create_json(cmd='SEND', msg=line, id=currId, portno = 80)
            resp = send_to_mongoose(j, portno, wait=True)
            if resp['cmd'] == 'ERR':
                print_line_cur(resp['msg'], ui, 1)
            c.update ([line])
        elif line != "":
            # the user input a command,
            # they start with /
            peerList, i = elaborate_command (line, portno, ui)
            # if it gets here, is because we changed peer
            ui.chatbuffer = []
            ui.linebuffer = []
            ui.redraw_ui(i)


def create_json (cmd='', msg='', id='localhost', portno=8000):
    # create a dictionary and populate it, ready to be converted to a json string
    t = localtime()
    if cmd == '':
        exit (1)
    else:
        j = dict ()
        j['id'] = id
        j['portno'] = portno
        j['msg'] = msg
        j['cmd'] = cmd
        j['date'] = strftime("[%H:%M] ", t)
        return j

def send_to_mongoose (j, portno, wait=False):
    s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
    s.connect (("localhost", int (portno)))
    s.send (bytes (json.dumps (j), 'utf-8'))
    # wait for response only when needed (not for SEND)
    if wait:
            resp = json.loads (s.recv (5000).decode ('utf-8')) # a dictionary
            return resp

def get_peers(portno, ui, hostname):
    # ask for a list of peers with pending messages
    j = create_json (cmd='GET_PEERS')
    resp = send_to_mongoose (j, portno, wait=True)
    peerList = resp['msg'].split (',')
    rightId = False
    ui.userlist = list()

    if peerList[0] == '': # no peers have written you! 
        i = 0
        peerList[0] = ui.wait_input("Onion Address: ")
        ui.userlist.append(peerList[0])
        ui.redraw_userlist(i, hostname)# this redraws only the user panel
        if currId != "":
            ui.userlist.append(currId)
    else:
        for userid in peerList: # print them all with an integer id associated
            ui.userlist.append(userid)
        if not currId in peerList and currId != "":
            ui.userlist.append(currId)
            peerList.append(currId)
        ui.redraw_userlist(None, hostname) # this redraws only the user panel

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
        ui.redraw_userlist(i, hostname) # this redraws only the user panel
    return peerList, i

def get_hostname(portno):
    j = create_json(cmd="HOST")
    resp = send_to_mongoose(j, portno, wait=True)
    hostname = resp["msg"]
    return hostname

def main (stdscr,portno):

    global currId
    # initiate the ui
    stdscr.clear() 
    ui = ChatUI(stdscr)
    
    hostname = get_hostname(portno)
    peerList, i = get_peers(portno, ui, hostname)
    currId = peerList[i]

    # here we use one thread to update unread messages in background,
    # the foreground one gets the input
    # they both work on the same buffer (printBuf) and thus a
    # semaphore is needed to prevent race conditions
    t1 = Thread(target=update_routine, args=(portno, ui))
    t1.start()
    input_routine (portno, ui)

# the wrapper is a curses function which performs a standard init process
# the ui init is then continued by the call to the ChatUi class (see main)
if __name__ == '__main__':
    from sys import argv
    wrapper(main, argv[1])
