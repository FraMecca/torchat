import json
import socket
import readline
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

def update_routine(peerList, i, portno, ui):
    # this function queries the server for unread messages
    # it runs until no messages from the given peer are left
    # then waits half a second and queries again
    global lock
    while True:
        j = create_json (cmd='UPDATE', msg=peerList[int (i)])
        resp = send_to_mongoose (j, portno, wait=True)
        # the json is not printed if no messages are received
        if resp['cmd'] == 'END':
            sleep(0.5)
        else: 
            lock.acquire()
            print_line_cur (resp['msg'], ui, 3) 
            lock.release()

def elaborate_command (line, portno):
    # if line == '/help':
        # print ('Command list: ')
        # TODO
    # this sends an exit to the client AND to the server
    if line == '/exit':
        j = create_json(cmd='EXIT', msg='')
        send_to_mongoose(j, portno, wait=False)
        exit ()
    return

def input_routine (onion, portno, ui):
    global lock
    c = Completer (['/help', '/exit'])
    # readline.set_completer (c.complete)
    # readline.parse_and_bind ("tab: complete")
    # readline.parse_and_bind ("set editing-mode vi")
    while True:
        # the input is taken from the bottom window in the ui
        # and printed directly (it is actually send below)
        line = ui.wait_input(completer = c)
        print_line_cur (line, ui, 2)
        # here we send to mongoose / tor
        if len (line) > 0 and line[0] != '/':
            # clearly the default action if the user does not input a command is
            # to send the message
            j = create_json(cmd='SEND', msg=line, id=onion, portno = 80)
            resp = send_to_mongoose(j, portno)
            c.update ([line])
        else:
            # the user input a command,
            # they start with /
            elaborate_command (line, portno)


def create_json (cmd='', msg='', id='localhost', portno=8000):
    # create a dictionary and populate it, ready to be converted to a json string
    if cmd == '':
        exit (1)
    else:
        j = dict ()
        j['id'] = id
        j['portno'] = portno
        j['msg'] = msg
        j['cmd'] = cmd
        j['date'] = "90" # TODO
        return j

def send_to_mongoose (j, portno, wait=False):
    s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
    s.connect (("localhost", int (portno)))
    s.send (bytes (json.dumps (j), 'utf-8'))
    # wait for response only when needed (not for SEND)
    if wait:
        resp = json.loads (s.recv (5000).decode ('utf-8')) # a dictionary
        return resp

def main (stdscr,portno):

    # create a semaphore
    # ask for a list of peers with pending messages
    j = create_json (cmd='GET_PEERS')
    resp = send_to_mongoose (j, portno, wait=True)
    peerList = resp['msg'].split (',')

    # initiate the ui
    stdscr.clear() 
    ui = ChatUI(stdscr)

    if peerList[0] == '': # no peers have written you! 
        print ("No peers, give me an onion address:")
        i = 0 
        peerList[0] = ui.wait_input("Onion Address: ")
        ui.userlist.append(peerList[0])
        ui.redraw_userlist()
    else:
        for userid in peerList: # print them all with an integer id associated
            ui.userlist.append(userid)
        ui.redraw_userlist() # this redraws only the user panel
        i = int(ui.wait_input("Peer Id: ")) - 1
    
    # here we use one thread to update unread messages in background,
    # the foreground one gets the input
    # they both work on the same buffer (printBuf) and thus a 
    # semaphore is needed to prevent race conditions
    t1 = Thread(target=update_routine, args=(peerList, i, portno, ui))
    t1.start()
    input_routine (peerList[i], portno, ui)

# the wrapper is a curses function which performs a standard init process
# the ui init is then continued by the call to the ChatUi class (see main)
if __name__ == '__main__':
    from sys import argv
    wrapper(main, argv[1])
