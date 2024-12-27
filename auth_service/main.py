from fastapi import FastAPI

app = FastAPI()


@app.get("/token")
async def get_token():
    return {"access_token": "1234567890"}
