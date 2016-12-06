import socket






def main (portno):
        s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
        s.connect (("localhost", int (portno)))
        print ("Cosa vuoi fare? (SEND, RECV)")
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
    while (1): 
        main (argv[1])
