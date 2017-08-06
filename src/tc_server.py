import argparse
import json
import asyncio
import websockets
import time
import signal
from tc_sessions import MessageSession

socketDict = {} # map all websockets to related classes

def get_hostname ():
    raise NotImplementedError("TOR Hostname")

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

async def get_session(websocket):
    # grab the session related to communication over websocket
    # if the communication was opened by another node
    # open a new session and store it in socketDict
    global socketDict
    if websocket not in socketDict:
        # the websocket was not opened by the local daemon
        session = await open_session(websocket)
        socketDict[websocket] = session
    return socketDict[websocket]

async def communication_with_node(websocket, path):
    session = await get_session(websocket)
    while session.isAcceptable() and not session.toClose():
        await session.waitForAnotherJMU() # TODO await?
        print (session)
    # check if the session has to be closed for an error
    if session.errorState():
        session.sendError()
    session.shutdown()

def assign_args(argv):
    parser = argparse.ArgumentParser(description="TORchat Daemon")
    parser.add_argument('--port', '-p', nargs='?', const=int, dest='port')
    parser.add_argument('--host', '-H', nargs='?', const=str, dest='host')
    args = vars(parser.parse_args(argv)) # arguments are mapped to dict
    print (args)
    port = 8000 if args['port'] is None else args['port']
    host = 'localhost' if args['host'] is None else args['host']

    return host, port

def main(argv):

    host, port = assign_args(argv)

    logic = websockets.serve(communication_with_node, host, port)
    try:
        loop = asyncio.get_event_loop()
        loop.run_until_complete(logic)
        loop.run_forever()
    except KeyboardInterrupt: #TODO: remove?
        print ("KeyboardInterrupt")
        logic.close()

if __name__ == '__main__':
    import sys
    main(sys.argv[1:])
