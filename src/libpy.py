import json, socket
from time import strftime, localtime, ctime

class Torchat:
    def __init__ (self, host, port):
        self.host = host
        self.port = port

    def create_json (self, cmd='', msg='', id='localhost', portno=None):
        # create a dictionary and populate it, ready to be converted to a json string
        t = localtime()
        if portno == None:
            portno = self.port
        if cmd == '':
            raise ValueError ("NullCommand")
        else:
            j = dict ()
            j['id'] = id
            j['portno'] = int (portno)
            j['date'] = ctime ()[-13:]
            j['msg'] = msg
            j['cmd'] = cmd
            return j

    def send_to_mongoose (self, j):
        s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
        s.connect ((self.host, int (self.port)))
        s.send (bytes (json.dumps (j), 'utf-8'))
        resp = json.loads (s.recv (5000).decode ('utf-8')) # a dictionary
        return resp

    def get_peers(self):        # returns a list
        # ask for a list of peers with pending messages
        j = self.create_json (cmd='GET_PEERS')
        resp = self.send_to_mongoose (j)
        peerList = resp['msg'].split (',')

        if peerList[0] == '': # no peers have written you! 
            return []
        else:
            return peerList

    def get_hostname(self):
        j = self.create_json(cmd="HOST")
        resp = self.send_to_mongoose(j)
        return resp["msg"]

    def close_server (self):
        j = self.create_json(cmd='EXIT', msg='')
        self.send_to_mongoose(j)

    def send_message (self, line, portno, currId):
        # portno is the one used by the other server, usually 80
        j = self.create_json(cmd='SEND', msg=line, id=currId, portno = portno)
        return self.send_to_mongoose(j)

    def check_error (self, j):
        if j['cmd'] == 'ERR':
            return j['msg']
        else:
            return False

    def check_new_messages (self, currId):
        # return List of tuples date, msg
        # could have used an iterator
        msgs = list ()
        while True:
            j = self.create_json (cmd='UPDATE', msg=currId)
            resp = self.send_to_mongoose (j)
            if resp['cmd'] == 'END':
                if msgs:
                    return msgs
                else:
                    return None
            else:
                msgs.append (resp)

    def check_new_message_single (self, currId):
        j = self.create_json (cmd='UPDATE', msg=currId)
        resp = self.send_to_mongoose (j)
        if resp['cmd'] == 'END':
                return None
        else:
            return resp
