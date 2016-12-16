import json
import socket
import readline
from time import sleep
from threading import Thread 

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
            print (resp) # we NEED a function that prints the json in a nicer way

def input_routine(): # tutta tua mecca
    while True:
        print ("mecca ghei")
        sleep(2)

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
    resp = json.loads (s.recv (5000).decode ('utf-8')) # a dictionary
    return resp

def main (portno):

    # first of all, ask for a list of peers with pending messages
    j = create_json (cmd='GET_PEERS')
    resp = send_to_mongoose (j, portno) 
    peerList = resp['msg'].split (',')

    if peerList[0] == '': # no peers have written you! 
        print ("No peers")
        exit (0)
    else:
        i = 0
        for id in peerList: # print them all with an integer id associated
            print (str (i) + '.', peerList[i])
            i+=1

    # here we use one thread to update unread messages, another that sends
    print ("Choose one id: ", end = '')
    i = input ()
    t1 = Thread(target=update_routine, args=(peerList, i, portno))
    t2 = Thread(target=input_routine, args=()) #mecca metti qui tutti gli args che ti servono in input_routine separati da vigola
    t1.start()
    t2.start()

if __name__ == '__main__':
    from sys import argv
    main (argv[1])
