# import functions_framework
import json

# from markupsafe import escape

import logging
import uuid
import socketio
import asyncio
import time
import statistics

logging.basicConfig(level=logging.DEBUG)

logger = logging.getLogger(__name__)


class AsyncLatencyTester:
    def __init__(
        self, url="https://chat-app-fastapi-909066611110.europe-west4.run.app"
    ):
        self.sio = socketio.AsyncClient()
        self.url = url
        self.latencies = []
        self.pending_messages = {}

    async def setup_handlers(self):
        @self.sio.on("connect")
        def on_connect():
            logger.info("Connected to server")

        @self.sio.on("my_response")
        def on_response(data):
            message_id = data.get("data")
            if message_id in self.pending_messages:
                end_time = time.time()
                start_time = self.pending_messages[message_id]
                latency = (end_time - start_time) * 1000
                self.latencies.append(latency)
                del self.pending_messages[message_id]
                logger.info(f"Latency: {latency:.2f}ms")

    async def connect(self):
        await self.setup_handlers()
        latency_test_id = f"latency_test_{uuid.uuid4()}"
        await self.sio.connect(
            self.url,
            socketio_path="/ws/socket.io",
            auth={
                "clientId": latency_test_id,
                "token": "my_token",
            },
        )

    async def measure_latency(self, num_messages=100, delay=0.1):
        for i in range(num_messages):
            message_id = f"latency_test_{i}"
            self.pending_messages[message_id] = time.time()
            await self.sio.emit("my_event", {"data": message_id})
            await asyncio.sleep(delay)

    def calculate_statistics(self):
        if not self.latencies:
            return None

        stats = {
            "min": min(self.latencies),
            "max": max(self.latencies),
            "avg": statistics.mean(self.latencies),
            "median": statistics.median(self.latencies),
            "stdev": statistics.stdev(self.latencies) if len(self.latencies) > 1 else 0,
        }
        return stats

    async def run_test(self, num_messages=1000, delay=0.1):
        try:
            await self.connect()
            logger.info(f"Starting latency test with {num_messages} messages...")
            await self.measure_latency(num_messages, delay)
            await asyncio.sleep(1)
            stats = self.calculate_statistics()
            if stats:
                return json.dumps(stats)  # Convert stats to JSON string
            else:
                return json.dumps({"message": "No latency measurements available"})
        finally:
            await self.sio.disconnect()


# @functions_framework.http
# def hello_http(request):
#     """HTTP Cloud Function.
#     Runs latency test and returns averages in JSON format.
#     """
#     tester = AsyncLatencyTester()
#     response_json = asyncio.run(tester.run_test())
#     return response_json


if __name__ == "__main__":
    tester = AsyncLatencyTester()
    result = asyncio.run(tester.run_test())

    print(result)
