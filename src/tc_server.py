#!/usr/bin/env python
import json
import asyncio
import websockets
import time
from tc_sessions import MessageSession

socketDict = {} # map all websockets to related classes

def get_hostname ():
    return "TOR Hostname"

async def open_session (websocket):
    buf = await websocket.recv()
    print("[received] " + buf)
    # if json not valid,explicit exception and connection closed
    j = json.loads(buf)
    if j['open'] == 'message':
        session = MessageSession(j, websocket)
    elif j['open'] == 'client':
        print ("client")
    elif j['open'] == 'file':
        print ("file")
    else:
        return None
    # session = AbstractSession(j,websocket)
    return session


async def communication_with_node(websocket, path):
    global socketDict
    if websocket not in socketDict:
        # the websocket was not opened by the local daemon
        session = await open_session(websocket)
        socketDict[websocket] = session
    session = socketDict[websocket]
    # iterate till websocket shutdown
    while session.isAcceptable() and not session.toClose():
        session.waitForAnotherJMU()
        print (session)
    session.shutdown()

logic = websockets.serve(communication_with_node, 'localhost', 8000)

asyncio.get_event_loop().run_until_complete(logic)
asyncio.get_event_loop().run_forever()
