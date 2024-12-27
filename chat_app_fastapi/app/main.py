import logging

from app.websocket_server import background_task, sio_server, sio_app
from app.room_manager import RoomManager
from dataclasses import dataclass
from fastapi import FastAPI
from contextlib import asynccontextmanager
from fastapi.middleware.cors import CORSMiddleware
from app.handlers import router


logging.basicConfig(level=logging.DEBUG)

logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan_context(app: FastAPI):
    room_manager = RoomManager()

    app.state.room_manager = room_manager

    yield


app = FastAPI(lifespan=lifespan_context)

allow_all_origins = ["*"]

app.add_middleware(
    CORSMiddleware,
    allow_origins=allow_all_origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# Attach Socket.IO server to the application
sio_server.start_background_task(background_task)
app.mount("/ws", sio_app)

# Routes
app.include_router(router)
