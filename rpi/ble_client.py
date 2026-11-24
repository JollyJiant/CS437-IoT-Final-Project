import asyncio
import struct
from bleak import BleakClient, BleakGATTCharacteristic
from bleak.exc import BleakError

address = "C4:9E:7E:18:A6:A1"
CHARACTERISTIC_UUID = "05ef3990-a968-4b05-81b0-550f9a8a1344"
NOTIFICATION_TIMEOUT_SECONDS = 3
PACKET_FORMAT = "<HH3f3f3f"

async def main(address):
    while True:
        notification_received = asyncio.Event()

        def on_notification(sender: BleakGATTCharacteristic, data: bytearray):
            notification_received.set()

            for values in struct.iter_unpack(PACKET_FORMAT, data):
                (
                    batch_id, sample_id,
                    accel_x, accel_y, accel_z,
                    gyro_x, gyro_y, gyro_z,
                    mag_x, mag_y, mag_z,
                ) = values
                print(f"Batch ID: {batch_id}, Sample ID: {sample_id}")
                print(f"Accel X: {accel_x:.2f}  Y: {accel_y:.2f}  Z: {accel_z:.2f} m/s^2",)
                print(f"Gyro X: {gyro_x:.2f}  Y: {gyro_y:.2f}  Z: {gyro_z:.2f} rad/s",)
                print(f"Mag X: {mag_x:.2f}  Y: {mag_y:.2f}  Z: {mag_z:.2f} uT",)
                print()
            print("END BATCH")

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
