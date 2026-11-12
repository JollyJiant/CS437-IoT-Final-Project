import asyncio
from bleak import BleakClient

address = "C4:9E:7E:18:A6:A1"
CHARACTERISTIC_UUID = "05ef3990-a968-4b05-81b0-550f9a8a1344"

async def main(address):
    async with BleakClient(address) as client:
        characteristic_value = await client.read_gatt_char(CHARACTERISTIC_UUID)
        print(f"{characteristic_value.decode()}")

asyncio.run(main(address))
