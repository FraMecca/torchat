from server import AbstractSession
import json
import websockets
import asyncio

queue = list()
idDict = dict()

class AbstractSession:
    def __init__(self, json, websocket):
        self.currentJSON = ""
        self.type = json['type']
        self.websocket = websocket
        self.toclose = False
        self.errorState = ''
        self.acceptable = True
        self.nodeId = json['from']
        # store in idDict the new opened connection
        global idDict
        idDict[self.nodeId] = self

    def isAcceptable (self):
        raise NotImplementedError()

    def toClose (self):
        return self.toclose

    def errorState(self):
        return self.errorState

    async def waitForAnotherJMU (self. websocket):
        buf = await self.websocket.recv()
        try:
            self.currentJSON = json.loads(buf)
            if d is not dict():
                raise ValueError("json is not a dictionary")
        except ValueError as e:
            self.acceptable = False
            self.toclose = True
            self.errorState = 'not valid JSON'

    def shutdown (self):
        raise NotImplementedError


class MessageSession(AbstractSession):

    def isAcceptables(self):
        j = self.currentJSON
        if j is not dict() or 'msg' not in j or len(j) != 1:
            self.errorState = 'not a message json'
            return False
        else:
            # json is acceptable and contains only 'msg' field
            self.currentJSON = j
            return True

    def waitForAnotherJMU(self, websocket):
        super().waitForAnotherJMU(websocket)
        self.storeMessage()

    def send(self, msg):
        d = {'msg': msg}
        j = json.dumps(d)
        self.websocket.send(j) 

    def self.storeMessage(self):
        global queue
        queue.append(self.formatMsg())

    def formatMsg(self):
        return {self.nodeId: self.currentJSON['msg']}

    def shutdown(self):
        self.websocket.close

class ClientSession(AbstractSession):

    def isAcceptables(self):
        # j = self.currentJSON
        # if j is not dict() or 'msg' not in j or len(j) != 1:
            # self.errorState = 'not a message json'
            # return False
        # else:
            # # json is acceptable and contains only 'msg' field
            # self.currentJSON = j
            return True

    def waitForAnotherJMU(self, websocket):
        super().waitForAnotherJMU(websocket)
        self.executeAction()

    def executeAction(self):
        j = self.currentJSON
        cmd = j['cmd']
        if cmd == 'getmsg':
            global queue:
            sendBack = queue.pop(0)
        elif cmd == 'send':
            # TODO: not only message sessions
            global idDict
            if j['to'] not in idDict:
                d = {'from': j['to'], 'type': 'msg'}
                session = MessageSession(json.dumps(d), websocket)

            session = idDict[j['to']]
            session.send(j['msg'])


    def formatMsg(self):
        return {self.nodeId: self.currentJSON['msg']}

    def shutdown(self):
        self.websocket.close
