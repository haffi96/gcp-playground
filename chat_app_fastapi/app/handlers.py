from app.websocket_server import sio_server
from fastapi import APIRouter, Request, Response

from app.room_manager import RoomManager
from fastapi.responses import JSONResponse
from starlette.responses import FileResponse


router = APIRouter()


async def app_context(request: Request) -> "RoomManager":
    return request.app.room_manager


@router.get("/")
async def read_index():
    return FileResponse("app/index.html")


@router.get("/rooms/{client_id}")
async def get_rooms_for_client_id(
    request: Request,
    client_id: str,
) -> Response:
    # Get room manager from context
    room_manager = await app_context(request)

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
        return JSONResponse({"rooms": server_rooms})

    return JSONResponse(
        {"message": "Rooms do not match"},
        status_code=500,
    )


@router.get("/rooms/{room_id}/participants")
async def get_participants_for_room(
    request: Request,
    room_id: str,
) -> Response:
    # Get room manager from context
    room_manager = await app_context(request)

    room = room_manager.rooms.get(room_id)

    partipiants_json = [participant.model_dump() for participant in room.participants]

    part = sio_server.manager.get_participants(room=room_id, namespace="/")

    if room:
        return JSONResponse({"participants": partipiants_json})

    return JSONResponse({"message": "Room not found"}, status=404)
