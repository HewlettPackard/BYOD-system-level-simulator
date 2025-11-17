
import os
import sys
FILE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(FILE_DIR)
import subprocess
import sys
import numpy as np
import json

# --- Set up the sweep ---

frequencies = np.arange(0.05, 2.5, 0.1) #array containing the frequencies to be simulated from 50 MHz to 2.5 GHz
runtime_array = np.zeros(len(frequencies))
energy_array = np.zeros(len(frequencies))
i=0

# --- Main loop for running the simulations ---

for freq in frequencies:
    
    subprocess.run(["sst", "sst_config_no3.py", "--", "-c", str(freq)]) #run the simulation with command line argument "$sst sst_config_no3.py -- -c <clock freq>"

    # --- Collect simulation results ---

    with open('output/dramsim3.json', 'r') as file:
        data = json.load(file)

    mem_energy = float(data['0']['total_energy'])

    data = np.loadtxt('output/sim_output.csv', delimiter = ',', dtype = 'str')
    runtime = float(data[1][4]) / 1000
    dac_data_energy = float(data[1][6])
    dac_weight_energy = float(data[2][6])
    modulator_energy = float(data[3][6])
    mesh_energy = float(data[4][6])
    pd_energy = float(data[5][6])
    adc_energy = float(data[6][6])
    total_energy = np.sum(np.array([mem_energy, dac_data_energy, dac_weight_energy, modulator_energy, mesh_energy, pd_energy, adc_energy]))
    energy_array[i] = total_energy
    runtime_array[i] = runtime
    i+=1
print('Sweep finished')
np.savetxt("output/sweep_out.txt", [frequencies, runtime_array, energy_array])
