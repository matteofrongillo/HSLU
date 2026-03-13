## Wind energy
import pandas as pd
import numpy as np
from scipy.interpolate import interp1d
import matplotlib.pyplot as plt


#inputs
electrical_efficiency = 0.96
mechanical_efficiency = 0.95
auxiliary_efficiency = 0.98
overall_efficiency = electrical_efficiency * mechanical_efficiency * auxiliary_efficiency

#data windtubine
rated_power_kW = 900
v_cut_in_m_per_s = 3.0  
v_rated_m_per_s = 16.5
v_cut_out_m_per_s = 34
swept_area_m2 = 1521
density_air_kg_per_m3 = 1.225  # kg/m^3 (at sea level and 15°C)



# Path to your Excel file
file_path = "Desktop/wind_energy_data.xlsx"  # Update this path to your actual file location

# Read Excel file
df = pd.read_excel(file_path, engine="openpyxl")

# Select only the needed columns
df = df[["reference_timestamp", "10minutes_mean_wind_andermatt_km/h", "10minutes_mean_wind_oberäger_km/h"]]

#Change from km/h to m/s & rename columns
df["10minutes_mean_wind_andermatt_km/h"] = df["10minutes_mean_wind_andermatt_km/h"] / 3.6
df["10minutes_mean_wind_oberäger_km/h"] = df["10minutes_mean_wind_oberäger_km/h"] / 3.6
 
df = df.rename(columns={
    "reference_timestamp": "time",
    "10minutes_mean_wind_andermatt_km/h": "10minutes_mean_wind_andermatt_m/s", 
    "10minutes_mean_wind_oberäger_km/h": "10minutes_mean_wind_oberäger_m/s"
})

plt.figure(figsize=(10, 5))
plt.plot(df["time"], df["10minutes_mean_wind_andermatt_m/s"], label="Andermatt Wind Speed (m/s)")
plt.plot(df["time"], df["10minutes_mean_wind_oberäger_m/s"], label="Oberäger Wind Speed (m/s)")
plt.xlabel("Time")
plt.ylabel("Wind Speed (m/s)")
plt.title("10-Minute Mean Wind Speed Over Time")
plt.legend()
plt.grid()
plt.show()

#Cut in and out Wind speed
df.loc[(df["10minutes_mean_wind_andermatt_m/s"] < v_cut_in_m_per_s) |
       (df["10minutes_mean_wind_andermatt_m/s"] > v_cut_out_m_per_s),
       "power_kW"] = 0
df.loc[(df["10minutes_mean_wind_oberäger_m/s"] < v_cut_in_m_per_s) |
       (df["10minutes_mean_wind_oberäger_m/s"] > v_cut_out_m_per_s),
       "power_kW"] = 0

#CP as a functin of wind speed (m/s)
v_points = np.array([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25])
cp_points = np.array([0.00,0.00,0.10,0.32,0.43,0.48,0.49,0.50,0.50,0.50,
                      0.48,0.44,0.40,0.34,0.29,0.25,0.21,0.18,0.15,
                      0.13,0.12,0.10,0.09,0.08,0.07])
cp_function = interp1d(v_points, cp_points,
                       bounds_error=False,
                       fill_value=0.0)
plt.figure(figsize=(10, 5))
plt.plot(v_points, cp_points, "o-", label="Cp vs Wind Speed")
plt.xlabel("Wind Speed (m/s)")  
plt.ylabel("Power Coefficient (Cp)")
plt.title("Power Coefficient (Cp) as a Function of Wind Speed")
plt.show()
df["Cp_andermatt"] = cp_function(df["10minutes_mean_wind_andermatt_m/s"])
df["Cp_oberäger"] = cp_function(df["10minutes_mean_wind_oberäger_m/s"])



# Convert timestamp to datetime (important!)
df["time"] = pd.to_datetime(df["time"])

# Set time as index (recommended for time series)
df = df.set_index("time")

# Calculate power in Watts
df["power_andermatt_kW"] = (
    overall_efficiency
    * df["Cp_andermatt"]
    * 0.5
    * density_air_kg_per_m3
    * swept_area_m2
    * df["10minutes_mean_wind_andermatt_m/s"]**3
    / 1000  # Convert to kW
)
df["power_oberäger_kW"] = (
    overall_efficiency
    * df["Cp_oberäger"]
    * 0.5
    * density_air_kg_per_m3
    * swept_area_m2
    * df["10minutes_mean_wind_oberäger_m/s"]**3
    / 1000  # Convert to kW
)

# Since data is hourly: energy per hour = power_kW * 1h
df["energy_andermatt_kWh"] = df["power_andermatt_kW"] * 1
df["energy_oberäger_kWh"] = df["power_oberäger_kW"] * 1

#cummulative energy produciton
df["cumulative_energy_andermatt_kWh"] = df["energy_andermatt_kWh"].cumsum()
df["cumulative_energy_oberäger_kWh"] = df["energy_oberäger_kWh"].cumsum()
#total energy
Total_energy_andermatt_kWH = df["energy_andermatt_kWh"].sum()
Total_energy_oberäger_kWH = df["energy_oberäger_kWh"].sum()

print(f"Total energy produced in Andermatt: {Total_energy_andermatt_kWH:.2f} kWh")
print(f"Total energy produced in Oberäger: {Total_energy_oberäger_kWH:.2f} kWh")

#plot energy and power over time
plt.figure(figsize=(12, 6))
plt.plot(df.index, df["power_andermatt_kW"], label="Power Andermatt (kW)")
plt.plot(df.index, df["power_oberäger_kW"], label="Power Oberäger (kW)")
plt.xlabel("Time")
plt.ylabel("Power (kW)")    

plt.plot(df.index, df["cumulative_energy_andermatt_kWh"], label="Cumulative Energy Andermatt (kWh)")
plt.plot(df.index, df["cumulative_energy_oberäger_kWh"], label="Cumulative Energy Oberäger (kWh)")
plt.title("Wind Power and Energy Production Over Time")
plt.legend()
plt.grid()
plt.show()

print(df.head())