from dataclasses import dataclass

from pydantic import BaseModel


# @dataclass
class Participant(BaseModel):
    client_id: str
    sid: str
    name: str
    role: str


# @dataclass
class Room(BaseModel):
    room_id: str
    participants: list[Participant] = []
