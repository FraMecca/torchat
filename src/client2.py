import json
import socket
import readline
import rlcompleter
from time import sleep
import os
from multiprocessing import Process

printBuf = list ()

def print_line_cur (line):
    rows, columns = os.popen('stty size', 'r').read().split()
    global printBuf
    printBuf.append (line)
    print ("\033[1J")
    print ('\n'.join (printBuf))
    # print ('\033[' + rows + ';' + '0' + 'f>\r')

    while len(printBuf) > int (rows):
        printBuf.pop[0]


class Completer(object):
    'The completer class for gnu readline'

    def __init__(self, options):
        self.options = sorted(options)

    def update (self, options):
        # use this method to add options to the completer
        self.options.extend (options)

    def complete(self, text, state):
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
            response = None
        return response

def update_routine(peerList, i, portno):
    # this function queries the server for unread messages
    # it runs until no messages from the given peer are left
    # then waits half a second and queries again
    while True:
        j = create_json (cmd='UPDATE', msg=peerList[int (i)])
        resp = send_to_mongoose (j, portno)
        # the json is not printed if no messages are received
        if resp['cmd'] == 'END':
            sleep(0.5)
        else: 
            print_line_cur (resp['date']+' '+resp['msg']) # we NEED a function that prints the json in a nicer way

def input_routine(lst, i, portno):
    if lst:
        c = Completer (lst)
    else:
        c = Completer (['TORchat'])
    readline.set_completer (c.complete)
    readline.parse_and_bind ("tab: complete")
    readline.parse_and_bind ("set editing-mode vi")
    while True:
        rows, columns = os.popen('stty size', 'r').read().split()
        escapeSeq = '\033[' + rows + ';' + '0' + 'f\r'
        line = input (escapeSeq + '> ')

        # print (line)
        # here we send to mongoose
        j = create_json(cmd='SEND', msg=line)
        j['id'] = lst[int (i)]
        j['portno'] = 80
        resp = send_to_mongoose(j, portno)
        print_line_cur (line)
        c.update ([line])

def create_json (cmd='', msg=''):
    if cmd == '':
        print ("WUT?")
        exit (1)
    else:
        j = dict ()
        j['id'] = 'localhost'
        j['portno'] = 8000
        j['msg'] = msg
        j['cmd'] = cmd
        j['date'] = "90"
        return j

def send_to_mongoose (j, portno):
    s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
    s.connect (("localhost", int (portno)))
    s.send (bytes (json.dumps (j), 'utf-8'))
    # wait for response only when needed (not for SEND)
    if j['cmd'] == 'GET_PEERS' or j['cmd'] == 'UPDATE':
        resp = json.loads (s.recv (5000).decode ('utf-8')) # a dictionary
        return resp

def main (portno):

    # first of all, ask for a list of peers with pending messages
    j = create_json (cmd='GET_PEERS')
    resp = send_to_mongoose (j, portno) 
    peerList = resp['msg'].split (',')

    if peerList[0] == '': # no peers have written you! 
        print ("No peers, give me an onion address:")
        # exit (0)
        i = 0
        peerList[0] = str(input())
    else:
        i = 0
        for id in peerList: # print them all with an integer id associated
            print (str (i) + '.', peerList[i])
            i+=1
            print ("Choose one id: ", end = '')
            i = input ()

    # here we use one thread to update unread messages, another that sends
    t1 = Process(target=update_routine, args=(peerList, i, portno))
    # t2 = Process(target=input_routine, args=()) #mecca metti qui tutti gli args che ti servono in input_routine separati da vigola
    t1.start()
    # t2.start()
    input_routine (peerList, i, portno)

if __name__ == '__main__':
    from sys import argv
    main (argv[1])
