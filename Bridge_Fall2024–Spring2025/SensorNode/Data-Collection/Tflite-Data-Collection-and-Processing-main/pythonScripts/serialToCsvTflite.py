import serial
import csv
import time
import argparse

def read_serial_data(port='COM5', baudrate=115200, duration=60, output_file='sensor_data.csv'):
    try:
        with serial.Serial(port, baudrate, timeout=1) as ser, open(output_file, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["Time", "RawX", "DenoisedX", "RMS", "RMSE", "MSE"])

            start_time = time.time()
            while time.time() - start_time < duration:
                line = ser.readline().decode('ascii', errors='ignore').strip()
                if not line:
                    continue
                parts = line.split(',')
                try:
                    if len(parts) == 5:
                        raw_x, denoised_x, rms, rmse, mse = map(float, parts)
                        elapsed = round(time.time() - start_time, 3)
                        writer.writerow([elapsed, raw_x, denoised_x, rms, rmse, mse])
                except ValueError:
                    # skip lines that don't parse
                    continue

    except serial.SerialException as e:
        print(f"Failed to open serial port: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Read denoised accelerometer data from serial and log to CSV"
    )
    parser.add_argument('--port',    type=str, default='COM5',    help='Serial port')
    parser.add_argument('--baud',    type=int, default=115200,    help='Baud rate')
    parser.add_argument('--duration',type=int, default=60,        help='Duration in seconds')
    parser.add_argument('--output',  type=str, default='sensor_data.csv', help='Output CSV file')
    args = parser.parse_args()

    read_serial_data(
        port=args.port,
        baudrate=args.baud,
        duration=args.duration,
        output_file=args.output
    )
