import socket
import sys
from time import sleep

def torchat_send (sock, buf, size):
    sizeSt = chr (size & 0xFF) + chr (size >> 8)
    buf = sizeSt + buf
    print ("sending " + buf)
    return sock.send (buf.encode ('utf-8'))

def torchat_recv (sock):
    size = s.recv (2)
    # print (size, end = ':')
    sizeToRead = size[0] | size[1] << 8
    print (sizeToRead, end = ':')
    print (s.recv (sizeToRead))

while True:
    s  = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect (('localhost', int (sys.argv[1])))
    # input ()
    sleep (0.1)
    torchat_send (s, sys.argv[2], int (sys.argv[3]))
    torchat_recv (s)
