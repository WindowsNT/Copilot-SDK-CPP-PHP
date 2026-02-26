import cmd
from csv import reader
import sys, json, asyncio, pdb
from copilot import CopilotClient
from copilot.generated.session_events import SessionEventType
import os
from dataclasses import asdict

sys.stdout.reconfigure(line_buffering=True)


HOST = "127.0.0.1"
PORT = 8765
CLI_PATH = "/root/copilot"

client = None
current_model = None
current_token = None
session = None
gwriter = None



def on_event(event):
    if event.type == SessionEventType.ASSISTANT_MESSAGE_DELTA:
        payload = event.data.delta_content.encode("utf-8")
        gwriter.write(payload)

async def ensure_client(model,token):
    global client, current_model, current_token, session, CLI_PATH
    if token == "":
        token = None
    if client is None or current_model != model or current_token != token:
        if token is not None:
            client = CopilotClient({"cli_path": CLI_PATH,"github_token": token})
        else:
            client = CopilotClient({"cli_path": CLI_PATH})
        await client.start()
        session = await client.create_session({"model": model,"streaming":True})
        session.on(on_event)
        current_model = model
        current_token = token

async def handle(req,writer):
    global session,gwriter
    gwriter = writer
    model = req.get("model", "gpt-4.1")
    token = req.get("token", None)
    if token == "":
        token = None
    prompt = req["prompt"]
    await ensure_client(model,token)
    await session.send_and_wait({"prompt": prompt, "timeout": 90})    
    gwriter.write(b"\n<<END>>")


async def main2(reader,writer):
    while True:
        global current_token
        global current_model
        line = await reader.readline()
        if not line:
            break
        # if not valid json, ignore
        try:
            req = json.loads(line)
        except json.JSONDecodeError:
            continue
        try:
            # see if prompt is a command
            line = req.get("prompt", None)
            current_model = req.get("model", current_model)
            current_token = req.get("token", current_token)
            # ensure its str
            line = line.encode("utf-8")
            line = line.rstrip(b"\r\n")
            if line == "/quit".encode("utf-8"):
                break
            if line == "/exit".encode("utf-8"):
                break
            if line == "/state".encode("utf-8"):
                await ensure_client(current_model, current_token)
                payload = client.get_state()
                writer.write(payload.encode("utf-8"))
                writer.write(b"\n<<END>>")
                continue
            if line == "/authstate".encode("utf-8"):
                await ensure_client(current_model, current_token)
                pong = await client.get_auth_status()
                writer.write(bytes(json.dumps(pong.isAuthenticated, indent=2, ensure_ascii=False), "utf-8"))
                writer.write(b"\n<<END>>")
                continue
            if line == "/models".encode("utf-8"):
                await ensure_client(current_model, current_token)
                models = await client.list_models()
                payload = json.dumps([asdict(m) for m in models], indent=2, ensure_ascii=False)
                writer.write(payload.encode("utf-8"))
                writer.write(b"\n<<END>>")  
                continue
        except Exception as e:
            writer.write(bytes(f"Error: {str(e)}", "utf-8"))
            writer.write(b"\n<<END>>")
            continue
        await handle(req, writer)

async def main():
    server = await asyncio.start_server(main2, HOST, PORT)
    async with server:
        await server.serve_forever()

asyncio.run(main())
