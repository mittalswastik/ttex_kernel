import pandas as pd
import matplotlib.pyplot as plt

# Replace 'your_csv_url' with the link to your Google Sheets CSV file
csv_url = 'ioctl_log.csv'
df = pd.read_csv(csv_url)

# Assuming your CSV has columns named 'Column1' and 'Column2'
y_values1 = df['getpid']
y_values2 = df['ioctl']

# Plotting the line graph
plt.plot(y_values1, label='getpid')
plt.plot(y_values2, label='ioctl')
plt.yticks(range(500, int(max(max(y_values1), max(y_values2))) + 1, 2000))
plt.xlabel('Iterations')
plt.ylabel('Execution Time(ns)')
plt.title('getpid vs ioctl call over 10000 iterations')
plt.legend()
plt.show()