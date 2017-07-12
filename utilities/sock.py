import socket
import sys
import json
from time import sleep

class Torchat:
    def __init__(self):
        self.sock  = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect (('localhost', int (sys.argv[1])))
        # self.sock.settimeout(1)

    def _build_json_msg_ (self, msg, id):
        d = {"to":id, "message":msg, "auth":"banane", "port":8080}
        return json.loads(d.__str__().replace("'", '"'))

    def send_message (self, msg, id):
        # build msg json
        j = self._build_json_msg_(msg, id)
        buf = json.dumps(j) 
        # add size and send
        size = len (buf)
        sizeSt = chr (size & 0xFF) + chr (size >> 8)
        buf = sizeSt + buf
        print ("sending " + buf)
        return self.sock.send (buf.encode ('utf-8'))

    def recv (self):
        size = self.sock.recv (2)
        # print (size, end = ':')
        sizeToRead = size[0] | size[1] << 8
        print (sizeToRead, end = ':')
        print (self.sock.recv (sizeToRead))

torch = Torchat ()
while True:
    # sleep (60)
    # torchat_send (s, sys.argv[2], int (sys.argv[3]))
    torch.send_message ("ciao", "127.0.0.1")
    # break
    try:
        torch.recv ()
    except socket.timeout:
        print ('timeout of socket ')
    input ("Wait for input")
