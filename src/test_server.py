import websockets
import asyncio
import json
import unittest
import tc_server as server
import tc_sessions as sessions
import os


class TestCls(unittest.TestCase):

    class FakeWebsocket:

        def __init__(self, j):
            if type(j) is list:
                self.json = [json.dumps(jj) for jj in j]
            elif type(j) is dict:
                self.json = json.dumps(j)
            self.closed = False
            self.sent = []
            self.url = ''

        async def recv(self):
            if type(self.json) is str:
                return self.json
            # else it is a list
            if len(self.json) == 0:
                # can't send more,
                # sleep
                while True:
                    await asyncio.sleep(2)
            ret = self.json.pop(0)
            return ret

        async def send(self, buf):
            self.sent.append(buf)

        def close(self):
            pass

        async def connect(url):
            self.url = url

    def run_coro(self, coro, *args, **kwargs):
        res = self.loop.run_until_complete(coro(*args, **kwargs))
        return res

    def setUp(self):
        self.loop = asyncio.get_event_loop()
        server.socketDict = {}
        sessions.idDict = {}
        sessions.queue = []
        self.asyncResult = None
        self.loopIsClosed = False

    def tearDown(self):
        if len(self.loop._scheduled):
            # there are still tasks running, wait for loop to be closed inside a coro
            # ugly
            self.loop.stop()
        pass

    def test_open_session_ClientSession(self):
        session, _ = self.run_coro(server.open_session, self.FakeWebsocket({'open':'client', 'from':'test.onion', 'extension':'client'}))
        self.assertIs(type(session), sessions.ClientSession)
        self.assertEqual(len(server.socketDict), 0)

    def test_open_session_messageSession(self):
        session, _ = self.run_coro(server.open_session, self.FakeWebsocket({'open':'message', 'from':'test.onion'}))
        self.assertIs(type(session), sessions.MessageSession)
        self.assertEqual(len(server.socketDict), 0)

    def test_open_invalid_session(self):
        session, _ = self.run_coro(server.open_session, self.FakeWebsocket({'open':'message', 'from':'test.onion', 'other':'test'}))
        self.assertIs(session, None)
        self.assertEqual(len(server.socketDict), 0)

    def test_get_session(self):
        # test a valid ClientSession JSON
        # a valid MessageSession JSON
        # an invalid JSON
        # and check that in socketDict there are the right number of websockets
        session, _ = self.run_coro(server.get_session, self.FakeWebsocket({'open':'client', 'from':'test.onion', 'extension':'client'}))
        self.assertIs(type(session), sessions.ClientSession)
        self.assertEqual(len(server.socketDict), 1)
        session, _ = self.run_coro(server.get_session, self.FakeWebsocket({'open':'message', 'from':'test.onion'}))
        self.assertIs(type(session), sessions.MessageSession)
        self.assertEqual(len(server.socketDict), 2)

    def test_get_session_invalid_session(self):
        session, _  = self.run_coro(server.get_session, self.FakeWebsocket({'open':'message', 'from':'test.onion', 'other':'t'}))
        self.assertIs(session, None)
        self.assertEqual(len(server.socketDict), 0)

    def test_communication_with_node_MessageSession(self):

        f = self.FakeWebsocket([{'open':'message','from':'test'},
                          {'msg':'msg1'},
                          {'close':''}])
        session = self.run_coro(server.communication_with_node, f, None)
        self.assertEqual(session.toClose(), True)
        self.assertEqual(session.getErrorState(), '') # no invalid state
        self.assertEqual(f.json, []) # received all elements

    def test_communication_with_node_MessageSession_invalidopening(self):
        # now test an invalid opening
        f = self.FakeWebsocket([{'open':'message','from':'test', 'other':'t'}])
        session  = self.run_coro(server.communication_with_node, f, None)
        self.assertEqual(session, None)

    def test_communication_with_node_MessageSession_invalidmsg(self):
        # now test an invalid msg
        f = self.FakeWebsocket([{'open':'message','from':'test'},
                          {'message':'msg1'},
                          {'close':''}])
        session = self.run_coro(server.communication_with_node, f, None)
        self.assertEqual(session.toClose(), False)
        self.assertNotEqual(session.getErrorState(), '') # invalid state
        self.assertEqual(len(f.json), 1) # received all elements

    def test_Client_getLastMessageFromDaemon(self):
        f = self.FakeWebsocket({})
        session = sessions.ClientSession(json={'open': 'client', 'extension': 'client', 'from': 'test'}, socket=f)
        self.run_coro(session.getLastMessageFromDaemon)
        self.assertEqual(f.sent[0], json.dumps({'cmd': 'getmsg'}))

    def test_Client_executeAction_getmsg(self):
        f = self.FakeWebsocket({})
        session = sessions.ClientSession(json={'open': 'client', 'extension': 'client', 'from': 'test'}, socket=f)
        session.currentJSON = {'cmd': 'getmsg'}
        sessions.queue.append(('test', 'test msg'))
        self.run_coro(session.executeAction)
        self.assertEqual(f.sent, [json.dumps({"id": "test", "msg": "test msg"})])

    def test_Client_executeAction_send(self):
        async def fun(websocket, path):
            recv = await websocket.recv()
            self.asyncResult = recv
            self.loop.stop()

        self.loopIsClosed = True
        f = self.FakeWebsocket({})
        session = sessions.ClientSession(json={'open': 'client', 'extension': 'client', 'from': 'testClient'}, socket=f)
        session.currentJSON = {'cmd': 'send', 'to': 'localhost', 'port': 8686, 'msg': 'test msg'}

        logic = websockets.serve(fun, 'localhost', 8686)
        self.loop.run_until_complete(logic)
        self.loop.run_until_complete(session.executeAction())
        self.loop.run_forever()
        self.assertEqual(self.asyncResult, json.dumps({"msg": "test msg"}))
