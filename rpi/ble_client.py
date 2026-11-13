import asyncio
from bleak import BleakClient, BleakGATTCharacteristic
from bleak.exc import BleakError

address = "C4:9E:7E:18:A6:A1"
CHARACTERISTIC_UUID = "05ef3990-a968-4b05-81b0-550f9a8a1344"
NOTIFICATION_TIMEOUT_SECONDS = 3

async def main(address):
    while True:
        notification_received = asyncio.Event()

        def on_notification(sender: BleakGATTCharacteristic, data: bytearray):
            value = int.from_bytes(data, "little", signed=False)
            print(f"{sender.uuid}: {value}")
            notification_received.set()

        try:
            async with BleakClient(address) as client:
                print(f"Connected to {address}")
                await client.start_notify(CHARACTERISTIC_UUID, on_notification)
                print("Subscribed to notifications")

                while True:
                    try:
                        await asyncio.wait_for(
                            notification_received.wait(),
                            timeout=NOTIFICATION_TIMEOUT_SECONDS,
                        )
                    except TimeoutError:
                        print(f"No notifications received after {NOTIFICATION_TIMEOUT_SECONDS} seconds")
                        break

                    notification_received.clear()
        except (BleakError, OSError, EOFError) as error:
            print(f"BLE connection error: {error!r}")

        print("Retrying connection in 2 seconds...")
        await asyncio.sleep(2)

try:
    asyncio.run(main(address))
except KeyboardInterrupt:
    print("Disconnected.")
