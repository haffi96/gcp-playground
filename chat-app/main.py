from aiohttp import web
import requests

import socketio

sio = socketio.AsyncServer(async_mode="aiohttp")
app = web.Application()
sio.attach(app)

AUTH_SERVICE_URL = "http://127.0.0.1:9000/auth"


async def background_task():
    """Example of how to send server generated events to clients."""
    count = 0
    while True:
        await sio.sleep(10)
        count += 1
        await sio.emit("my_response", {"data": "Server generated event"})


async def index(request):
    with open("app.html") as f:
        return web.Response(text=f.read(), content_type="text/html")


@sio.event
async def my_event(sid, message):
    await sio.emit("my_response", {"data": message["data"]}, room=sid)


@sio.event
async def my_broadcast_event(sid, message):
    print("received broadcast event", message)
    await sio.emit("my_response", {"data": message["data"]})


@sio.event
async def join(sid, message):
    token = requests.get(f"{AUTH_SERVICE_URL}/token").json()["access_token"]
    print(token)

    await sio.enter_room(sid, message["room"])
    await sio.emit(
        "my_response", {"data": "Entered room: " + message["room"]}, room=sid
    )


@sio.event
async def leave(sid, message):
    await sio.leave_room(sid, message["room"])
    await sio.emit("my_response", {"data": "Left room: " + message["room"]}, room=sid)


@sio.event
async def close_room(sid, message):
    await sio.emit(
        "my_response",
        {"data": "Room " + message["room"] + " is closing."},
        room=message["room"],
    )
    await sio.close_room(message["room"])


@sio.event
async def my_room_event(sid, message):
    await sio.emit("my_response", {"data": message["data"]}, room=message["room"])


@sio.event
async def disconnect_request(sid):
    await sio.disconnect(sid)


@sio.event
async def connect(sid, environ):
    await sio.emit("my_response", {"data": "Connected", "count": 0}, room=sid)


@sio.event
def disconnect(sid):
    print("Client disconnected")


app.router.add_static("/static", "static")
app.router.add_get("/", index)


async def init_app():
    sio.start_background_task(background_task)
    return app


if __name__ == "__main__":
    web.run_app(init_app(), host="0.0.0.0", port=8000)
