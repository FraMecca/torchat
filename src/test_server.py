import websockets
import asyncio
import json
import unittest
import tc_server as server
import tc_sessions as sessions
import os


class TestCls(unittest.TestCase):

    def run_coro(self, coro, *args, **kwargs):
        res = self.loop.run_until_complete(coro(*args, **kwargs))
        return res

    def setUp(self):
        self.loop = asyncio.get_event_loop()
        server.socketDict = {}
        sessions.idDict = {}

    def tearDown(self):
        # self.loop.close()
        pass

    def test_open_session(self):
        class FakeWebsocket:
            def __init__(self, j):
                self.json = json.dumps(j)
            async def recv(self):
                return self.json

        session = self.run_coro(server.open_session, FakeWebsocket({'open':'client', 'from':'test.onion'}))
        self.assertIs(type(session), sessions.ClientSession)
        self.assertEqual(len(server.socketDict), 0)
        session = self.run_coro(server.open_session, FakeWebsocket({'open':'message', 'from':'test.onion'}))
        self.assertIs(type(session), sessions.MessageSession)
        self.assertEqual(len(server.socketDict), 0)
        session = self.run_coro(server.open_session, FakeWebsocket({'open':'message', 'from':'test.onion', 'other':'test'}))
        self.assertIs(session, None)
        self.assertEqual(len(server.socketDict), 0)

    def test_get_session(self):
        # test a valid ClientSession JSON
        # a valid MessageSession JSON
        # an invalid JSON
        # and check that in socketDict there are the right number of websockets
        class FakeWebsocket:
            def __init__(self, j):
                self.json = json.dumps(j)
            async def recv(self):
                return self.json

        session = self.run_coro(server.get_session, FakeWebsocket({'open':'client', 'from':'test.onion'}))
        self.assertIs(type(session), sessions.ClientSession)
        self.assertEqual(len(server.socketDict), 1)
        session = self.run_coro(server.get_session, FakeWebsocket({'open':'message', 'from':'test.onion'}))
        self.assertIs(type(session), sessions.MessageSession)
        self.assertEqual(len(server.socketDict), 2)
        session = self.run_coro(server.get_session, FakeWebsocket({'open':'message', 'from':'test.onion', 'other':'t'}))
        self.assertIs(session, None)
        self.assertEqual(len(server.socketDict), 2)

    def test_communication_with_node_MessageSession(self):

        class FakeWebsocket:
            def __init__(self, j1, j2, j3):
                self.json = [json.dumps(j1), json.dumps(j2), json.dumps(j3)]
                self.closed = False
                self.sent = []
            async def recv(self):
                ret = self.json.pop(0)
                return ret

            def close(self):
                pass

        f = FakeWebsocket({'open':'message','from':'test'},
                          {'msg':'msg1'},
                          {'close':''})
        session = self.run_coro(server.communication_with_node, f, None)
        self.assertEqual(session.toClose(), True)
        self.assertEqual(session.getErrorState(), '') # no invalid state
        self.assertEqual(f.json, []) # received all elements
