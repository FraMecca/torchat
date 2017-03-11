import json
import socket
from time import localtime, strftime
import rlcompleter
from time import sleep
import os
import sys
from cleverbot import Cleverbot
from libpy import Torchat

exitFlag = False
cb = None

def ask_cleverbot (msg):
    global cb
    if cb == None:
        print ("Starting cleverbot istance")
        cb = Cleverbot ("TORchat")
    resp = cb.ask (msg)
    sleep (1)
    print ('HOST:       ', resp)
    return resp

def print_json (j):
    print (j['id'], end=': ')
    if j['cmd'] == 'SEND':
        print (j['msg'])
    elif j['cmd'] == 'UPDATE':
        print (j['msg'])
    else:
        pass

def update_routine(t, currId):
    # this function queries the server for unread messages
    # it runs until no messages from the given peer are left
    # then waits half a second and queries again
    while True:
        resp = t.check_new_messages_single (currId)
        if resp == None:
            # sleep(0.5)
            return
        else:
            response = ask_cleverbot (resp['msg'])
            while True:
                j = t.create_json(cmd='SEND', msg=response, id=currId, portno = 80)
                resp = t.send_to_mongoose(j, portno)
                if not t.check_error (resp):
                    # try to send again if error in transmission
                    break

def main (portno):
    # main routine
    t = Torchat ('localhost', portno)
    while True:
        peerlist = t.get_peers ()
        for userid in peerlist:
#             currid = userid
            update_routine (t, userid)
        sleep (1)


# the wrapper is a curses function which performs a standard init process
# the ui init is then continued by the call to the ChatUi class (see main)
if __name__ == '__main__':
    main (sys.argv[1])
