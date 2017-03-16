import json, socket
from time import strftime, localtime, ctime
# this is used for async send/recv on socket
from asyncore import dispatcher

class Torchat:
    def __init__ (self, host, port):
        self.host = host
        self.port = port
        self.onion = self.get_hostname()

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

    def send_to_mongoose (self, j, wait=False):
        s = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
        s.connect ((self.host, int (self.port)))
        s.send (bytes (json.dumps (j), 'utf-8'))
        if wait:
            resp = json.loads (s.recv (5000).decode ('utf-8')) # a dictionary
            return resp

    def get_peers(self):        # returns a list
        # ask for a list of peers with pending messages
        j = self.create_json (cmd='GET_PEERS')
        resp = self.send_to_mongoose (j, wait=True)
        peerList = resp['msg'].split (',')

        return peerList

    def get_hostname(self):
        resp = self.send_message(command="HOST", line="", currentId="localhost", wait=True)
        return resp["msg"]

    def close_server (self):
        j = self.create_json(cmd='EXIT', msg='')
        self.send_to_mongoose(j, wait=True)

    def send_message (self, command, line, currentId, sendPort="", wait=False): # added cmd for fileup needs
        # portno is the one used by the other server, usually 80
        if sendPort == "":
            sendPort = self.port
        j = self.create_json(cmd=command, msg=line, id=currentId, portno = sendPort)
        return self.send_to_mongoose(j, wait)

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
            resp = self.send_to_mongoose (j, wait=True)
            if resp['cmd'] == 'END':
                if msgs:
                    return msgs
                else:
                    return None
            else:
                msgs.append (resp)

    def check_new_messages_single (self, currId):
        j = self.create_json (cmd='UPDATE', msg=currId)
        resp = self.send_to_mongoose (j, wait=True)
        if resp['cmd'] == 'END':
                return None
        else:
            return resp
