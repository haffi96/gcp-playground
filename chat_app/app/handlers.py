from aiohttp import web
from app.websocket_server import sio_server
from typing import TYPE_CHECKING


if TYPE_CHECKING:
    from app.main import AppContext


async def app_context(request: web.Request) -> "AppContext":
    return request.app["ctx"]


async def get_rooms_for_client_id(request: web.Request) -> web.Response:
    # Get room manager from context
    ctx = await app_context(request)
    room_manager = ctx.room_manager

    # Handler logic
    client_id = request.match_info["client_id"]

    socket_id = room_manager.get_sid_from_client_id(client_id)

    pubsub_rooms = [
        room
        for room in sio_server.manager.get_rooms(
            sid=socket_id,
            namespace="/",
        )
        if room != socket_id
    ]

    server_rooms = room_manager.client_rooms_map.get(client_id, [])

    if pubsub_rooms == server_rooms:
        return web.json_response({"rooms": server_rooms})

    return web.Response(
        {"message": "Rooms do not match"},
        status=500,
    )


async def get_participants_for_room(request: web.Request) -> web.Response:
    # Get room manager from context
    ctx = await app_context(request)
    room_manager = ctx.room_manager

    # Handler logic
    room_id = request.match_info["room_id"]

    room = room_manager.rooms.get(room_id)

    partipiants_json = [participant.model_dump() for participant in room.participants]

    part = sio_server.manager.get_participants(room=room_id, namespace="/")

    if room:
        return web.json_response({"participants": partipiants_json})

    return web.json_response({"message": "Room not found"}, status=404)
