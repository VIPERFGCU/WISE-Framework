import csv
import matplotlib.pyplot as plt

# Read the CSV file and extract the data
time_data = []
accel_x = []
accel_y = []
accel_z = []

# Read data from CSV
with open('sensor_data.csv', 'r') as csvfile:
    reader = csv.reader(csvfile)
    next(reader)  # Skip the header row
    for row in reader:
        # Extract values from each row and append to the lists
        time_data.append(float(row[0]))  # Time is the first column
        accel_x.append(float(row[1]))   # X acceleration data
        accel_y.append(float(row[2]))   # Y acceleration data
        accel_z.append(float(row[3]))   # Z acceleration data

# Plotting
plt.figure(figsize=(10, 6))

# Plot each acceleration axis
plt.plot(time_data, accel_x, label='Accel X', color='r')
plt.plot(time_data, accel_y, label='Accel Y', color='g')
plt.plot(time_data, accel_z, label='Accel Z', color='b')

# Add labels and title
plt.xlabel('Time (s)')
plt.ylabel('Acceleration (m/s^2)')
plt.title('Acceleration vs Time')
plt.legend()

# Show the plot
plt.grid(True)
plt.show()
