import json
import socket
import readline
# import thread

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
    j = create_json (cmd='GET_PEERS')
    resp = send_to_mongoose (j, portno) 
    peerList = resp['msg'].split (',')
    if peerList[0] == '':
        print ("No peers")
        exit (0)
    else:
        i = 0
        for id in peerList:
            print (str (i) + '.', peerList[i])
            i+=1
        print ("Choose one id: ", end = '')
        i = input ()
        while True:
            j = create_json (cmd='UPDATE', msg=peerList[int (i)])
            resp = send_to_mongoose (j, portno)
            print (resp) 
            if resp['cmd'] == 'END':
                break










if __name__ == '__main__':
    from sys import argv
    main (argv[1])
