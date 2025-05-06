import pandas as pd
import numpy as np

# Load the data
data = pd.read_csv("sensor_data.csv")

# Extract acceleration columns
accel_x = data['AccelX']
accel_y = data['AccelY']
accel_z = data['AccelZ']

# Calculate RMS for each axis
rms_x = np.sqrt(np.mean(accel_x ** 2))
rms_y = np.sqrt(np.mean(accel_y ** 2))
rms_z = np.sqrt(np.mean(accel_z ** 2))

# Print results
print(f"RMS for X-axis: {rms_x:.4f}")
print(f"RMS for Y-axis: {rms_y:.4f}")
print(f"RMS for Z-axis: {rms_z:.4f}")

combined_rms = np.sqrt(np.mean(accel_x**2 + accel_y**2 + accel_z**2))
print(f"Combined RMS Acceleration: {combined_rms}")