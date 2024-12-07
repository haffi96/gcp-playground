import logging
from aiohttp import web

from handlers import get_rooms_for_client_id, get_participants_for_room
from websocket_server import sio_server
from room_manager import RoomManager
from dataclasses import dataclass


logging.basicConfig(level=logging.DEBUG)

logger = logging.getLogger(__name__)


@dataclass
class AppContext:
    room_manager: RoomManager


async def background_task():
    """Example of how to send server generated events to clients."""
    count = 0
    while True:
        await sio_server.sleep(10)
        count += 1
        await sio_server.emit("my_response", {"data": "Server generated event"})


async def index(request):
    with open("app.html") as f:
        return web.Response(text=f.read(), content_type="text/html")


app = web.Application()


# Application context
app["ctx"] = AppContext(
    # Dependencies
    room_manager=RoomManager(),
)

# Attach Socket.IO server to the application
sio_server.attach(app)

# Routes
app.router.add_get("/", index)
app.router.add_get("/room/{client_id}", get_rooms_for_client_id)
app.router.add_get("/room/{room_id}/participants", get_participants_for_room)


async def init_app():
    sio_server.start_background_task(background_task)
    return app


if __name__ == "__main__":
    web.run_app(init_app(), host="0.0.0.0", port=8000)
