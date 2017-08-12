import json
import websockets

queue = list()
idDict = dict()


class AbstractSession:

    @staticmethod
    async def isOpenJSONAcceptable(websocket):
        jsonBuf = await websocket.recv()
        # may throw also exceptions from json module
        d = json.loads(jsonBuf)
        if type(d) is not dict or 'open' not in d or 'from' not in d \
           or (len(d) != 2 and 'extension' not in d) or ('extension' in d and len(d) != 3):
            raise ValueError("Not a valid JSON")
        return d

    def __init__(self, socket=None, json=None, id=None, type=None, currentJSON=None):
        self.currentJSON = currentJSON
        self.websocket = socket
        self._toClose = False
        self._errorState = ''
        self._acceptable = True

        if json != None:
            self.nodeId, self.type = json['from'], json['open']
        else:
            self.nodeId, self.type = id, type
        # store in idDict the new opened connection
        global idDict
        idDict[self.nodeId] = self

        if self.websocket is None:
            print ("Initializing session without a websocket. Are you sure?")
            # should not happen

    def isAcceptable(self):
        return self._acceptable

    def toClose(self):
        return self._toClose

    def getErrorState(self):
        return self._errorState

    async def waitForAnotherJMU(self):
        buf = await self.websocket.recv()
        try:
            self.currentJSON = json.loads(buf)
            if type(self.currentJSON) is not dict:
                raise ValueError("json is not a dictionary")
            if 'close' in self.currentJSON:
                self._toClose = True
        except ValueError as e:
            self._acceptable = False
            self._errorState = 'not valid JSON'

    async def sendOpenJMU(self):
        j = {'open': self.type, 'from': self.id}
        await self.websocket.send(json.dumps(j))

    def shutdown(self):
        raise NotImplementedError

    async def sendError(self):
        await self.websocket.send(self._errorState)


class MessageSession(AbstractSession):

    def __init__(self, **kw):
        super().__init__(**kw)
        if self.type != 'message':
            raise TypeError(self.type + ' is not supported as Message Session')

    async def waitForAnotherJMU(self):
        await super().waitForAnotherJMU()
        j = self.currentJSON
        if type(j) is not dict or 'msg' not in j or len(j) != 1:
            if 'close' not in j:
                # it is not a close jmu nor a message jmu
                # there was an error from the other node
                self._errorState = 'not a message json'
                self._acceptable = False
            return  # do not store msg
        self.storeMessage()

    async def send(self, msg):
        d = {'msg': msg}
        j = json.dumps(d)
        await self.websocket.send(j)

    def storeMessage(self):
        global queue
        queue.append(self.formatMsg())

    def formatMsg(self):
        return (self.nodeId, self.currentJSON['msg'])

    def shutdown(self):
        self.websocket.close()


class ClientSession(AbstractSession):

    def isAcceptable(self):
        # j = self.currentJSON
        # if j is not dict() or 'msg' not in j or len(j) != 1:
            # self.errorState = 'not a message json'
            # return False
        # else:
            # # json is acceptable and contains only 'msg' field
            # self.currentJSON = j
            return True

    async def waitForAnotherJMU(self):
        await super().waitForAnotherJMU()
        self.executeAction()

    async def executeAction(self):
        j = self.currentJSON
        cmd = j['cmd']
        if cmd == 'getmsg':
            global queue
            if len(queue) == 0:
                # TODO: correct way to inform queue is empty 
                j = json.dumps(dict())
            else:
                sendBack = queue.pop(0)
                j = json.dumps({'id': sendBack[0], 'msg': sendBack[1]})
            await self.websocket.send(j)

        elif cmd == 'send':
            # TODO: not only message sessions
            global idDict
            if j['to'] not in idDict:
                # TODO: open socket externally using TOR, wait for PR
                url = ''.join(['ws://', j['to'], ':', str(j['port'])])
                websocket = await websockets.connect(url)
                session = MessageSession(id=j['to'], socket=websocket, type='msg')

            session = idDict[j['to']]
            await session.send(j['msg'])

    async def getLastMessageFromDaemon(self):
        j = json.dumps({'cmd': 'getmsg'})
        return await self.websocket.send(j)

    def formatMsg(self):
        return {self.nodeId: self.currentJSON['msg']}

    def shutdown(self):
        self.websocket.close()
