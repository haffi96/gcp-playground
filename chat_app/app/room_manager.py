from app.schemas import Participant, Room
import logging


logger = logging.getLogger(__name__)


class RoomManager:
    def __init__(self):
        self.client_rooms_map: dict[str, list[str]] = {}
        self.rooms: dict[str, Room] = {}
        self.client_connections = {}
        self.socket_connections = {}

    def create_new_room(self, room_id: str):
        self.rooms[room_id] = Room(room_id=room_id)

    def save_client_connection(self, sid: str, client_id: str):
        try:
            self.client_connections[sid] = client_id
            self.socket_connections[client_id] = sid
        except Exception as e:
            logger.error(f"Error saving client connection:\n {e}")

    def remove_client_connection(self, sid: str):
        try:
            client_id = self.client_connections[sid]
            del self.client_connections[sid]
            del self.socket_connections[client_id]
        except Exception as e:
            logger.error(f"Error removing client connection:\n {e}")

    def get_client_id_from_sid(self, sid: str):
        return self.client_connections.get(sid)

    def get_sid_from_client_id(self, client_id: str):
        return self.socket_connections.get(client_id)

    def add_client_to_room(self, room_id: str, participant: Participant):
        self.rooms[room_id].participants.append(participant)

        client_id = participant.client_id
        if client_id not in self.client_rooms_map:
            self.client_rooms_map[client_id] = [room_id]
        else:
            self.client_rooms_map[client_id].append(room_id)

    def create_room_for_client(self, room_id: str, client_id: str):
        self.create_new_room(room_id)

        self.add_client_to_room(
            room_id=room_id,
            participant=Participant(
                client_id=client_id,
                sid=self.get_sid_from_client_id(client_id),
                name="User",
                role="admin",
            ),
        )

    def remove_client_from_room(self, sid: str):
        client_id = self.client_connections[sid]
        client_rooms = self.client_rooms_map.get(client_id)

        # Find the room to remove the client from
        room_to_remove = None
        for client_room_id in client_rooms:
            room = self.rooms[client_room_id]
            logger.info(room)
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

        # Update client rooms map
        self.client_rooms_map[client_id] = [
            room_id
            for room_id in self.client_rooms_map[client_id]
            if room_id != room_to_remove.room_id
        ]

        logger.info("updated room state: %s", self.rooms[client_room_id])
