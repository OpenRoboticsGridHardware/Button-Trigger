import asyncio
import websockets

async def test_ws():
    url = "ws://<ESP32_IP>:932/ws" 
    try:
        async with websockets.connect(url) as websocket:
            print("Connected to the WebSocket server")
            while True:
                message = await websocket.recv()
                print(f"Received message: {message}")

    except Exception as e:
        print(f"An error occurred: {e}")
asyncio.get_event_loop().run_until_complete(test_ws())
