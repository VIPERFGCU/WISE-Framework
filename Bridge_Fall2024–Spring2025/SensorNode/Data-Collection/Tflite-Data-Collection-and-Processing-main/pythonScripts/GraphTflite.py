import csv
import matplotlib.pyplot as plt

# Lists to hold data
time_data = []
raw_x = []
denoised_x = []
rms_vals = []
rmse_vals = []
mse_vals = []

# Read data from CSV
with open('sensor_data.csv', 'r') as csvfile:
    reader = csv.reader(csvfile)
    next(reader)  # Skip header: Time,RawX,DenoisedX,RMS,RMSE,MSE
    for row in reader:
        time_data.append(float(row[0]))
        raw_x.append(float(row[1]))
        denoised_x.append(float(row[2]))
        rms_vals.append(float(row[3]))
        rmse_vals.append(float(row[4]))
        mse_vals.append(float(row[5]))

# Plotting
plt.figure(figsize=(10, 6))

# Plot raw and denoised X
plt.plot(time_data, raw_x,      label='Raw X',       linestyle='-', marker='o')
plt.plot(time_data, denoised_x, label='Denoised X',  linestyle='-', marker='x')

# Plot metrics
plt.plot(time_data, rms_vals,   label='RMS',   linestyle='--')
plt.plot(time_data, rmse_vals,  label='RMSE',  linestyle='--')
plt.plot(time_data, mse_vals,   label='MSE',   linestyle='--')

# Labels & title
plt.xlabel('Time (s)')
plt.ylabel('Value')
plt.title('Raw vs Denoised X and Metrics over Time')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
