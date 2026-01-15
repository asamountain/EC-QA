import os

filename = 'ec_data_log.csv'
new_header = "Timestamp,Temperature,Raw_EC,Sensor_Default_EC,Smart_Calc_EC,Coefficient_Used,Deviation,Distance_from_12_88_Sensor,Distance_from_12_88_Smart,Improvement_Score\n"

if not os.path.exists(filename):
    print("File not found.")
    exit()

valid_lines = []
with open(filename, 'r') as f:
    for line in f:
        # Check if line has 10 columns (9 commas)
        if line.count(',') == 9:
            valid_lines.append(line)

print(f"Found {len(valid_lines)} new data points.")

if len(valid_lines) > 0:
    # Rename old file
    os.rename(filename, filename + '.bak')
    # Write new file
    with open(filename, 'w') as f:
        f.write(new_header)
        for line in valid_lines:
            f.write(line)
    print("Fixed ec_data_log.csv! Old file backed up to .bak")
else:
    print("No new format data found.")
