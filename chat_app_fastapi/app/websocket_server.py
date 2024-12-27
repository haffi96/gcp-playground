import logging

import requests
import socketio
from typing import TYPE_CHECKING


from app.config import APP_CONFIG

if TYPE_CHECKING:
    from app.room_manager import RoomManager

from app.schemas import Participant


logger = logging.getLogger(__name__)

sio_server = socketio.AsyncServer(
    async_mode="asgi",
    cors_allowed_origins=[],  # Needs to be empty list
)

sio_app = socketio.ASGIApp(
    socketio_server=sio_server,
    socketio_path="/ws/socket.io",
)


async def background_task():
    """Example of how to send server generated events to clients."""
    logger.info("Background task started")
    count = 0
    while True:
        await sio_server.sleep(10)
        count += 1
        await sio_server.emit("my_response", {"data": "Server generated event"})


async def sio_app_context(sio_server: socketio.AsyncServer, sid: str) -> "RoomManager":
    environ = sio_server.get_environ(sid=sid)
    app = environ["asgi.scope"]["app"]
    return app.state.room_manager


@sio_server.event
async def my_event(sid, message):
    logger.info("received my event: %s", message)
    await sio_server.emit("my_response", {"data": message["data"]}, room=sid)


@sio_server.event
async def my_broadcast_event(sid, message):
    logger.info("received broadcast event: %s", message)
    await sio_server.emit("my_response", {"data": message["data"]})


@sio_server.event
async def join(sid, message):
    # Get room manager from app context
    logger.info("Client joined room: %s", message["room"])
    room_manager = await sio_app_context(sio_server, sid)

    token = requests.get(
        f"{APP_CONFIG.AUTH_SERVICE_URL}/token",
    ).json()["access_token"]

    room_id = message["room"]

    await sio_server.enter_room(sid, room_id)

    client_id = room_manager.get_client_id_from_sid(sid)

    if room_manager.rooms and room_manager.rooms.get(room_id):
        room_manager.add_client_to_room(
            room_id=room_id,
            participant=Participant(
                client_id=client_id,
                sid=sid,
                name="User",
                role="user",
            ),
        )
        await sio_server.emit(
            "my_response",
            {"data": "Client added to existing room", "room": room_id},
            room=sid,
        )
        return

    room_manager.create_room_for_client(room_id, client_id)

    await sio_server.emit(
        "my_response", {"data": "Client added to new room: " + room_id}, room=sid
    )


@sio_server.event
async def leave(sid, message):
    await sio_server.leave_room(sid, message["room"])
    await sio_server.emit(
        "my_response", {"data": "Left room: " + message["room"]}, room=sid
    )


@sio_server.event
async def close_room(sid, message):
    await sio_server.emit(
        "my_response",
        {"data": "Room " + message["room"] + " is closing."},
        room=message["room"],
    )
    await sio_server.close_room(message["room"])


@sio_server.event
async def my_room_event(sid, message):
    await sio_server.emit(
        "my_response", {"data": message["data"]}, room=message["room"]
    )


@sio_server.event
async def disconnect_request(sid):
    await sio_server.disconnect(sid)


@sio_server.event
async def connect(sid, environ, auth):
    if auth["token"] != "my_token":
        return False

    # Get room manager from app context
    room_manager = await sio_app_context(sio_server, sid)

    token = auth["token"]
    client_id = auth["clientId"]

    room_manager.save_client_connection(sid, client_id)

    logger.info("Client connected: %s", sid)
    await sio_server.emit("my_response", {"data": "Connected", "count": 0}, room=sid)


@sio_server.event
async def disconnect(sid):
    # Get room manager from app context
    room_manager = await sio_app_context(sio_server, sid)

    room_manager.remove_client_from_room(sid)
    room_manager.remove_client_connection(sid)
    logger.info("Client disconnected")
