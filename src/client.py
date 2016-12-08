import socket
import readline


def main (portno):

        # client connects and updates unread messages 
        portUpdate = 42000
        s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
        s.connect (("localhost", int (portno)))

        jsonStr = ''.join (['{"cmd": "', "UPDATE", '","id":"', "i", '", "portno": ', str(portUpdate), ',"msg": "', "a", '"}'])
        print (s.send (bytes (jsonStr, 'utf-8')))
        sUp = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
        sUp.connect (("localhost", portUpdate))
        while True:
            msg = sUp.recv(8192)
            if len(msg) == 0:
                break
            print(msg.decode('utf-8'))
        # then talking business
        print("Cosa vuoi fare? (SEND, RECV)")
        cmd = input ()
        if cmd == "SEND":
            print ("Dammi ip")
            ip = input ()
            print ("Dammi porta")
            portno = 80
        elif cmd == 'RECV':
            ip = "CLIENTPY"
        elif cmd == 'EXIT':
            ip = "CLIENTPY"


        print ("Dammi il messaggio: ")
        msg = input ()
        jsonStr = ''.join (['{"cmd": "', cmd, '","id":"', ip, '", "portno": ', str(portno), ',"msg": "', msg, '"}'])
        print ("Sending encoded json: ", jsonStr)

        # jsonStr = b'{"cmd": ' + cmd + b',"id":"Clientpy", "porto": ' + bytes(portno) + b',"msg": ' + msg + b'}'
        print (s.send (bytes (jsonStr, 'utf-8')))



if __name__ == '__main__':
    from sys import argv
    import time
    portno = 80
    if len (argv) > 2:
       while (1):
            main (argv[1])
    s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
    s.connect (("localhost", int (8000)))
    cmd = "SEND"
    ip = "ld74fqvoxpu5yi73.onion"
    msg = str (time.time ())
    jsonStr = ''.join (['{"cmd": "', cmd, '","id":"', ip, '", "portno": ', str(portno), ',"msg": "', msg, '"}'])
    print ("Sending encoded json: ", jsonStr)

# jsonStr = b'{"cmd": ' + cmd + b',"id":"Clientpy", "porto": ' + bytes(portno) + b',"msg": ' + msg + b'}'
    print (s.send (bytes (jsonStr, 'utf-8')))
