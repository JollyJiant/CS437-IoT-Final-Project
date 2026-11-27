import asyncio
import sqlite3
import struct
import uuid
from bleak import BleakClient, BleakGATTCharacteristic, BleakScanner
from bleak.exc import BleakError
from datetime import datetime, UTC

address = "C4:9E:7E:18:A6:A1"
device_id = None
session_id = None
CHARACTERISTIC_UUID = "05ef3990-a968-4b05-81b0-550f9a8a1344"
CONTROL_CHARACTERISTIC_UUID = "b4ca5883-0b7f-4f90-bd7d-9f876d85c821"
NOTIFICATION_TIMEOUT_SECONDS = 3
PACKET_FORMAT = "<HH3f3f3f"

async def main(address):
    db = sqlite3.connect("db_files/posture.db")
    try:
        while True:
            notification_received = asyncio.Event()

            def on_notification(sender: BleakGATTCharacteristic, data: bytearray):
                notification_received.set()

                rows = []
                for values in struct.iter_unpack(PACKET_FORMAT, data):
                    (
                        batch_id, sample_id,
                        accel_x, accel_y, accel_z,
                        gyro_x, gyro_y, gyro_z,
                        mag_x, mag_y, mag_z,
                    ) = values
                    placeholder_timestamp = datetime.now(UTC).isoformat(timespec="milliseconds")
                    row = (
                        device_id, session_id, sample_id,
                        placeholder_timestamp, # TODO still need to backcalculate sample timestamps for the column "estimated_at_utc"
                        accel_x, accel_y, accel_z,
                        gyro_x, gyro_y, gyro_z,
                        mag_x, mag_y, mag_z,
                    )
                    rows.append(row)
                print(f"END BATCH #{batch_id} for {device_id} with session ID {session_id}")
                with db:
                    statement = "INSERT INTO imu_samples VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)"
                    db.executemany(statement, rows)
                

            try:
                device = await BleakScanner.find_device_by_address(address, timeout=5)
                if device is None:
                    raise BleakError(f"BLE device not found: {address}")
                async with BleakClient(address) as client:
                    device_id = device.name
                    session_id = str(uuid.uuid4())

                    print(f"Connected to {address}")
                    print(f"Device ID: {device_id}")
                    print(f"Session ID: {session_id}")

                    await client.start_notify(CHARACTERISTIC_UUID, on_notification)
                    print("Subscribed to notifications")

                    await client.write_gatt_char(
                        CONTROL_CHARACTERISTIC_UUID, b"READY", response=True
                    )

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
    finally:
        db.close
try:
    asyncio.run(main(address))
except KeyboardInterrupt:
    print("Disconnected.")
