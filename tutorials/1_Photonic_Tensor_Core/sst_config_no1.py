
import os
FILE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(FILE_DIR,'../../utils'))
from clements_utils import *
sys.path.append(FILE_DIR)
import sys
import sst
import numpy as np

# --- Set the simulation parameters ---

clock = 1.0 #clock frequency in GHz
clock = str(clock) +f"Ghz"
size = 8 #size of the data vector
resolution = 8 #bit resolution of the data
data_size = 10 #number of data points

DEBUG_LEVEL = 0 #select the level of debuggin output, 0 disables component outputs, 1 shows detailed timestampts
STATISTICSPATH = os.path.abspath(os.path.join(FILE_DIR, "output/sim_output.csv")) #output files for the energy simulations
STATISTICSPATH_DRAM = os.path.abspath(os.path.join(FILE_DIR, "output")) #output folder for the DRAM energy simulations
DRAM_CONFIG = os.path.abspath(os.path.join(FILE_DIR,'../../utils/DRAM_configs/LPDDR4_8Gb_x16_2400.ini')) #config file for a LPDDR4 memory

# --- Generate the test data to be read from memory ---

test_data = np.random.uniform(0, pow(2,resolution), size * data_size).astype(int)
print('Test data vectors: \n', test_data.reshape(data_size, size))
data_bytes = voltages_to_bytes(test_data, resolution) #Convert data into bytes to create the memory image

# --- Set up the CPU ---

cpu = sst.Component("cpu", "byod.StreamingCPU")
cpu.addParams({
    "memory": np.array(data_bytes).flatten().tolist(),
    "size": size,
    "resolution": resolution,
    "frequency": clock,
    "vector_count": len(test_data) / size,
    "verbose": DEBUG_LEVEL
})

iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
iface.enableAllStatistics()

# --- Set up the memory ---

#Define a memory controller running at half the clock frequency of the memory
memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
        "debug": 0,
        "debug_level": 0,
        "clock": f"1.2GHz",
        "addr_range_end": 1024 * 1024 * 1024 - 1,
})

#Connect the memory controller with the DRAMSim3 simulator for a DDR4 memory
memory = memctrl.setSubComponent("backend", "memHierarchy.dramsim3")
memory.enableAllStatistics()
memory.addParams({
    "mem_size": "1GiB",
    "config_ini" : DRAM_CONFIG,
    "output_dir": STATISTICSPATH_DRAM
})

# --- Connect the components with links ---

#link CPU and memory
link_cpu_memory_link = sst.Link("link_cpu_cache_link")
link_cpu_memory_link.connect( (iface, "lowlink", "1ps"), (memctrl, "highlink", "1ps") )

#link CPU output and input for the callback loop
link_cpu_callback = sst.Link("link_cpu_callback")
link_cpu_callback.connect( (cpu, "outputData", "1ps"), (cpu, "input", "1ps") )

# --- Configure the simulation output file ---

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath": STATISTICSPATH, "separator": ","})
