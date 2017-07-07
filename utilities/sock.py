import socket
import sys
from time import sleep

def torchat_send (sock, buf, size):
    # sizeSt = str (size & 0xFF) +  str(size >> 8) +  str(size >> 16) +  str(size >> 24)
    sizeSt = format (size, '04x')[::-1]

    buf = sizeSt + buf
    print ("sending " + buf)
    return sock.send (buf.encode ('utf-8'))

while True:
    s  = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect (('localhost', int (sys.argv[1])))
    # input ()
    sleep (0.1)
    torchat_send (s, sys.argv[2], int (sys.argv[3]))
    print (s.recv (400))
