import pdb
import asyncio
import random
import sys
import struct
import os
import time
import ctypes
import win32event
import json
from dataclasses import asdict
from multiprocessing.shared_memory import SharedMemory
from copilot import CopilotClient, PermissionHandler
from copilot.tools import define_tool
from copilot.generated.session_events import SessionEventType
from pydantic import BaseModel, Field

%s

def is_json_direct(myjson,key):
    try:
        buf2 = json.loads(myjson)
        buf = buf2[key]
    except ValueError as e:
        return False
    except KeyError as e:
        return False
    return True

async def main():
    %s
    shm_in = SharedMemory(name="shm_in_%S", create=True, size=1024*1024)
    shm_out = SharedMemory(name="shm_out_%S", create=True, size=1024*1024)
    ev_in = win32event.CreateEvent(None, 0, 0, "ev_in_%S")
    ev_out = win32event.CreateEvent(None, 0, 0, "ev_out_%S")
    ev_cancel = win32event.CreateEvent(None, 0, 0, "ev_cancel_%S")
    buf = shm_out.buf
    struct.pack_into("<I", buf, 0, 0)         # write_index
    struct.pack_into("<I", buf, 4, 0)         # read_index
    struct.pack_into("<I", buf, 8, 1024*1024)  # capacity
    await client.start()

%s

    session = await client.create_session(session_config)
    print("\033[33mModel: ", session_config['model'],"\033[0m")
    # if there is a system message, print it
    if 'system_message' in session_config and session_config['system_message']:
        print("\033[33mSystem: ", session_config['system_message'],"\033[0m")

    def ring_write_final(payload: bytes):
        buf = shm_out.buf
        w = struct.unpack_from("<I", buf, 0)[0]
        r = struct.unpack_from("<I", buf, 4)[0]
	    
        capacity = struct.unpack_from("<I", buf, 8)[0]
        data_off = 12
	    
        msg_len = 4 + len(payload)
	    
        # free space
        if w >= r:
            free = capacity - (w - r)
        else:
            free = r - w
	    
        if free <= msg_len:
            # FULL drop
            return False
	    
        pos = data_off + w;
        # write size
        struct.pack_into("<I", buf, pos, len(payload))
        pos += 4	
        # payload (wrap safe)
        end = min(len(payload), capacity - (w + 4))
        buf[pos:pos+end] = payload[:end]
        if end < len(payload):
            buf[data_off:data_off+(len(payload)-end)] = payload[end:]
	    
        # advance write
        struct.pack_into("<I", buf, 0, (w + msg_len) %% capacity)
        return True

    def ring_write_status(payload: bytes, status: int):
        # first byte of payload is status
        payload = bytes([status]) + payload
        return ring_write_final(payload)

    def ring_write(payload: bytes):
        return ring_write_status(payload,1)

    start_reasoning = 0
    def handle_event(event):
        nonlocal start_reasoning
        if event.type == SessionEventType.ASSISTANT_REASONING:
            if (start_reasoning == 1):
                print("\033[0m")
                start_reasoning = 0
        if event.type == SessionEventType.ASSISTANT_REASONING_DELTA:
            payload = event.data.delta_content.encode("utf-8")
            if (start_reasoning == 0):
                print("\033[35m",end='')
                start_reasoning = 1
            print(event.data.delta_content,end='')
            ring_write_status(payload,3)
            # if ev_cancel is set, stop
            wait = win32event.WaitForSingleObject(ev_cancel, 0)
            if wait == win32event.WAIT_OBJECT_0:
                asyncio.get_running_loop().create_task(session.abort())
            win32event.SetEvent(ev_out)
        if event.type == SessionEventType.ASSISTANT_MESSAGE_DELTA:
            if (start_reasoning == 1):
                print("\033[0m")
                start_reasoning = 0
            payload = event.data.delta_content.encode("utf-8")
            # print it
            print(event.data.delta_content, end='', flush=True)
            ring_write(payload)
            # if ev_cancel is set, stop
            wait = win32event.WaitForSingleObject(ev_cancel, 0)
            if wait == win32event.WAIT_OBJECT_0:
                asyncio.get_running_loop().create_task(session.abort())
            # set event
            win32event.SetEvent(ev_out)

    session.on(handle_event)

    loop = asyncio.get_running_loop()
    while True:
        await loop.run_in_executor(
            None,
            win32event.WaitForSingleObject,
            ev_in,
            win32event.INFINITE
            )
        # first 4 bytes = size
        buf = shm_in.buf
        size = struct.unpack_from("<I", buf, 0)[0]
        if size == 0 or size > len(buf) - 4:
            continue  # corrupted / empty
        payload = bytes(buf[4:4+size])
        user_input = payload.decode("utf-8").rstrip("\r\n")
        if user_input == "/exit":
            print("\033[91m",user_input,"\033[0m",sep='')
            print("Exiting interactive session.")
            break
        if user_input == "/quit":
            print("\033[91m",user_input,"\033[0m",sep='')
            print("Exiting interactive session.")
            break
        if user_input == "/ping":
            print("\033[91m",user_input,"\033[0m",sep='')
            pong = await client.ping("")
            ring_write(bytes(pong.message, 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        if user_input == "/authstate":
            print("\033[91m",user_input,"\033[0m",sep='')
            pong = await client.get_auth_status()
            ring_write(bytes(json.dumps(pong.isAuthenticated), 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        if user_input == "/state":
            print("\033[91m",user_input,"\033[0m",sep='')
            pong = client.get_state()
            ring_write(bytes(pong, 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        if user_input == "/models":
            print("\033[91m",user_input,"\033[0m",sep='')
            models = await client.list_models()
            payload = json.dumps([asdict(m) for m in models], indent=2, ensure_ascii=False)
            ring_write(bytes(payload, 'utf-8'))
            end_payload = "--end--".encode("utf-8")
            ring_write_status(end_payload,2)
            win32event.SetEvent(ev_out)
            continue
        print("\033[92m",user_input,"\033[0m",end='',sep='')
        print('')
        # check if user passed a direct message
        isjd = is_json_direct(user_input,"direct")
        if isjd:
			# parse direct message
             await session.send_and_wait(json.loads(user_input), timeout=None)
        else:
            await session.send_and_wait({"prompt": user_input}, timeout=None)
        # also send --end--
        end_payload = "--end--".encode("utf-8")
        print("");
        ring_write_status(end_payload,2)
        win32event.SetEvent(ev_out)

    shm_in.close()
    shm_out.close()
    shm_in.unlink()
    shm_out.unlink()       
    
asyncio.run(main())



