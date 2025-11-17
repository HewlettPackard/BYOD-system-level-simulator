
import os
import sys
FILE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(FILE_DIR,'../../utils'))
from clements_utils import *
from byod_components import StreamingCPU, ADC, DAC, Modulator
sys.path.append(FILE_DIR)
import sst
import numpy as np

# --- Set the simulation parameters ---

clock = 1.0 #clock frequency in GHz
clock = str(clock) +f"Ghz"
size = 8 #size of the input data vector
resolution = 8 #bit resolution of the data
data_size = 10 #number of data points
P_pi = 1E-3 #power for a pi phase shift
R = 1000 #resistance of the heater in Ohms
vmax = np.sqrt(P_pi * 2 * R) #voltage to achieve a 2pi phase shift

DEBUG_LEVEL = 0
STATISTICSPATH = os.path.abspath(os.path.join(FILE_DIR, "output/sim_output.csv"))
STATISTICSPATH_DRAM = os.path.abspath(os.path.join(FILE_DIR, "output"))
DRAM_CONFIG = os.path.abspath(os.path.join(FILE_DIR,'../../utils/DRAM_configs/LPDDR4_8Gb_x16_2400.ini'))

# --- Set up helper functions for each component (energy models, data pre-processing, ...) ---

cpu_helper = StreamingCPU(resolution = resolution)
adc_helper = ADC(resolution = resolution)
dac_helper = DAC(resolution = resolution, maxVin = vmax)
mod_helper = Modulator(R = R, pPi = P_pi)

# --- Generate the test data to be read from memory ---

test_data = np.random.uniform(0, 1, size * data_size)
voltages_data = mod_helper.getVoltagesFromAmplitudes(np.sqrt(test_data))
adlevels_data = dac_helper.getLevelsFromVoltages(voltages_data)
data_bytes = cpu_helper.getBytesFromLevels(adlevels_data)

print('Data vector: \n', voltages_to_AD_levels(test_data, resolution, vmax).reshape((data_size, size)))

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

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
        "debug": 0,
        "debug_level": 0,
        "clock": f"1.2GHz",
        "addr_range_end": 1024 * 1024 * 1024 - 1,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.dramsim3")
memory.enableAllStatistics()
memory.addParams({
    "mem_size": "1GiB",
    "config_ini" : DRAM_CONFIG,
    "output_dir": STATISTICSPATH_DRAM
})

# --- Set up the DAC array ---

dac_data = sst.Component("dac_data", "byod.DAC")
dac_data.enableAllStatistics()
dac_data.addParams({
    "size": size,
    "maxVout": vmax,
	"resolution": resolution,
	"frequency": clock,
    "dacType": "R2R",
    "verbose": DEBUG_LEVEL,
})

# --- Set up the optical modulator array ---

mod = sst.Component("mod", "byod.AmplitudeModulator")
mod.enableAllStatistics()
mod.addParams({
    "size": size,
    "laserPower": 0.005,
    "laserWpe": 0.5,
    "verbose": DEBUG_LEVEL,
})

# --- Select and set up thermo-optic modulators for the modulator array ---

ampMod = mod.setSubComponent("modulator", "byod.thermoOpticModulator")
ampMod.addParams({
    "resistance": R,
    "p_pi": P_pi
})

# --- Set up the photodetector array ---

pd = sst.Component("pd", "byod.Photodetector")
pd.enableAllStatistics()
pd.addParams({
    "size": size,
    "tiaGain": 1/(0.005),
    "verbose": DEBUG_LEVEL,
})

# --- Set up the ADC array ---

adc_data = sst.Component("adc_data", "byod.ADC")
adc_data.enableAllStatistics()
adc_data.addParams({
    "size": size,
    "maxVin": vmax,
	"resolution": resolution,
	"frequency": clock,
    "conversionEnergy": "2.66",
    "latency": 100,
    "verbose": DEBUG_LEVEL
})

# --- Connect the components with links ---

link_cpu_memory_link = sst.Link("cpu_memory")
link_cpu_memory_link.connect( (iface, "lowlink", "1ps"), (memctrl, "highlink", "1ps") )

link_cpu_dac = sst.Link("cpu_dac")
link_cpu_dac.connect( (cpu, "outputData", "1ps"), (dac_data, "input", "1ps"))

link_dac_mod = sst.Link("dac_mod")
link_dac_mod.connect( (dac_data, "output", "1ps"), (mod, "input", "1ps"))

link_mod_pd = sst.Link("mod_pd")
link_mod_pd.connect( (mod, "output", "10ps"), (pd, "input", "10ps"))

link_pd_adc = sst.Link("pd_adc")
link_pd_adc.connect( (pd, "output", "1ps"), (adc_data, "input", "1ps"))

link_adc_cpu = sst.Link("adc_cpu")
link_adc_cpu.connect( (adc_data, "output", "1ps"), (cpu, "input", "1ps"))

# --- Configure the simulation output file ---

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath": STATISTICSPATH, "separator": ","})
