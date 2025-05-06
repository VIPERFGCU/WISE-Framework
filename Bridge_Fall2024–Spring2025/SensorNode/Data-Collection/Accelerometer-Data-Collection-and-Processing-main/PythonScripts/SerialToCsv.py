import serial
import csv
import time


def read_serial_data(port='COM5', baudrate=115200, duration=60, output_file='sensor_data.csv'):
    try:
        with serial.Serial(port, baudrate, timeout=.01) as ser, open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["Time", "AccelX", "AccelY", "AccelZ"])

            start_time = time.time()
            next_sample_time = start_time

            while time.time() - start_time < duration:
                if ser.in_waiting:
                    try:
                        raw_data = ser.readline().strip().decode('ascii')
                        values = [float(v) for v in raw_data.split(',')]

                        if len(values) == 3:  # Expecting 3-axis data
                            elapsed_time = round(time.time() - start_time, 3)
                            writer.writerow([elapsed_time] + values)
                    except (ValueError, UnicodeDecodeError):
                        pass  # skip bad data

                # Enforce 100 Hz (10 ms intervals)
                next_sample_time += 0.01
                sleep_time = next_sample_time - time.time()
                if sleep_time > 0:
                    time.sleep(sleep_time)

    except serial.SerialException as e:
        print(f"Failed to open serial port: {e}")


if __name__ == "__main__":
    read_serial_data()
