import json
import socket
from time import localtime, strftime
import rlcompleter
from time import sleep
import os
import sys
from cleverbot import Cleverbot

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
def check_correct_json (resp):
    if resp == None:
        return True
    # resp : string to bool
    j = json.loads (resp)
    if j['cmd'] == "ERR":
        return false
    else:
        return true


def update_routine(portno, currId):
    # this function queries the server for unread messages
    # it runs until no messages from the given peer are left
    # then waits half a second and queries again
    while True:
        j = create_json (cmd='UPDATE', msg=currId)
        resp = send_to_mongoose (j, portno, wait=True)
        print_json (resp)
        # print (resp)
        if resp['cmd'] == 'END':
            # sleep(0.5)
            return
        else:
            response = ask_cleverbot (resp['msg'])
            while True:
                j = create_json(cmd='SEND', msg=response, id=currId, portno = 80)
                resp = send_to_mongoose(j, portno)
                if check_correct_json (resp):
                    # try to send again if error in transmission
                    break

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
        try:
            resp = json.loads (s.recv (5000).decode ('utf-8')) # a dictionary
            return resp
        except:
            pass

def get_peers(portno, hostname):
    # ask for a list of peers with pending messages
    j = create_json (cmd='GET_PEERS')
    resp = send_to_mongoose (j, portno, wait=True)
    peerList = resp['msg'].split (',')

    if peerList[0] == '': # no peers have written you! 
        return [], 0
    else:
        # print (peerList)
        return peerList, len (peerList)

def get_hostname(portno):
    j = create_json(cmd="HOST")
    resp = send_to_mongoose(j, portno, wait=True)
    hostname = resp["msg"]
    return hostname

def main (portno):

    global currid
    hostname = get_hostname(portno)
    while True:
        peerlist, i = get_peers(portno, hostname)
        for userid in peerlist:
#             currid = userid
            update_routine (portno, userid)
        sleep (1)


# the wrapper is a curses function which performs a standard init process
# the ui init is then continued by the call to the ChatUi class (see main)
if __name__ == '__main__':
    main (sys.argv[1])
