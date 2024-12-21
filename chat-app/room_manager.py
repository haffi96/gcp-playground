import json
from schemas import Participant, Room
import logging
from config import APP_CONFIG
from enum import StrEnum
from redis import asyncio as aioredis

logger = logging.getLogger(__name__)


class RedisHashKeys(StrEnum):
    ROOMS = "rooms"
    CLIENT_ROOMS = "client_rooms_map"
    CLIENT_CONNECTIONS = "client_connections"
    SOCKET_CONNECTIONS = "socket_connections"


class RoomManager:
    def __init__(self):
        self.redis: aioredis.Redis = aioredis.Redis.from_url(
            APP_CONFIG.REDIS_URL,
        )

        self.client_rooms_map: dict[str, list[str]] = {}
        self.rooms: dict[str, Room] = {}
        self.client_connections = {}
        self.socket_connections = {}

    async def create_new_room(self, room_id: str):
        await self.redis.hset(
            RedisHashKeys.ROOMS,
            room_id,
            Room(room_id=room_id).model_dump_json(),
        )
        # self.rooms[room_id] = Room(room_id=room_id)

    async def save_client_connection(self, sid: str, client_id: str):
        try:
            await self.redis.hset(
                RedisHashKeys.CLIENT_CONNECTIONS,
                sid,
                client_id,
            )

            await self.redis.hset(
                RedisHashKeys.SOCKET_CONNECTIONS,
                client_id,
                sid,
            )
            # self.client_connections[sid] = client_id
            # self.socket_connections[client_id] = sid
        except Exception as e:
            logger.error(f"Error saving client connection:\n {e}")

    async def remove_client_connection(self, sid: str):
        try:
            client_id = await self.redis.hget(RedisHashKeys.CLIENT_CONNECTIONS, sid)
            # client_id = self.client_connections[sid]
            await self.redis.hdel(RedisHashKeys.CLIENT_CONNECTIONS, sid)
            await self.redis.hdel(RedisHashKeys.SOCKET_CONNECTIONS, client_id)
            # del self.client_connections[sid]
            # del self.socket_connections[client_id]
        except Exception as e:
            logger.error(f"Error removing client connection:\n {e}")

    async def get_client_id_from_sid(self, sid: str):
        result = await self.redis.hget(RedisHashKeys.CLIENT_CONNECTIONS, sid)
        if not result:
            return None

        return result.decode("utf-8")
        # return self.client_connections.get(sid)

    async def get_sid_from_client_id(self, client_id: str):
        result = await self.redis.hget(RedisHashKeys.SOCKET_CONNECTIONS, client_id)

        if not result:
            return None

        return result.decode("utf-8")
        # return self.socket_connections.get(client_id)

    async def get_client_room_ids(self, client_id: str):
        result = await self.redis.hget(RedisHashKeys.CLIENT_ROOMS, client_id)

        if not result:
            return None

        return json.loads(result.decode("utf-8"))

    async def get_room(self, room_id: str):
        room = await self.redis.hget(RedisHashKeys.ROOMS, room_id)

        if not room:
            return None

        return Room.model_validate_json(room)

    async def add_client_to_room(self, room_id: str, participant: Participant):
        room = await self.get_room(room_id)

        participants = room.participants

        participants.append(participant)

        await self.redis.hset(
            RedisHashKeys.ROOMS,
            room_id,
            room.model_dump_json(),
        )
        # self.rooms[room_id].participants.append(participant)

        client_id = participant.client_id

        client_rooms = await self.get_client_room_ids(client_id)

        if not client_rooms:  # First room for client
            await self.redis.hset(
                RedisHashKeys.CLIENT_ROOMS,
                client_id,
                json.dumps([room_id]),
            )
            # self.client_rooms_map[client_id] = [room_id]
        elif client_rooms and room_id not in client_rooms:  # Add room for client
            client_rooms.append(room_id)
            await self.redis.hset(
                RedisHashKeys.CLIENT_ROOMS,
                client_id,
                json.dumps(client_rooms),
            )
            # self.client_rooms_map[client_id].append(room_id)
        else:
            logger.info("Client already in room")

    async def create_room_for_client(self, room_id: str, client_id: str):
        await self.create_new_room(room_id)

        client_sid = await self.get_sid_from_client_id(client_id)

        await self.add_client_to_room(
            room_id=room_id,
            participant=Participant(
                client_id=client_id,
                sid=client_sid,
                name="User",
                role="admin",
            ),
        )

    async def remove_client_from_room(self, sid: str):
        # client_id = self.client_connections[sid]
        client_id = await self.get_client_id_from_sid(sid)

        client_rooms = await self.get_client_room_ids(client_id)

        # Find the room to remove the client from
        room_to_remove = None
        for client_room_id in client_rooms:
            # room = self.rooms[client_room_id]
            room = await self.get_room(client_room_id)

            if room:
                for participant in room.participants:
                    if participant.client_id == client_id:
                        room_to_remove = room
                        break

        # Update participant list on the room
        room_to_remove.participants = [
            participant
            for participant in room_to_remove.participants
            if participant.client_id != client_id
        ]
        await self.redis.hset(
            RedisHashKeys.ROOMS,
            room_to_remove.room_id,
            room_to_remove.model_dump_json(),
        )

        # Update client rooms map
        await self.redis.hset(
            RedisHashKeys.CLIENT_ROOMS,
            client_id,
            json.dumps(
                [
                    room_id
                    for room_id in client_rooms
                    if room_id != room_to_remove.room_id
                ]
            ),
        )
