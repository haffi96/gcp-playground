from pydantic import BaseModel


class Participant(BaseModel):
    client_id: str
    sid: str
    name: str
    role: str


class Room(BaseModel):
    room_id: str
    participants: list[Participant] | None = []
