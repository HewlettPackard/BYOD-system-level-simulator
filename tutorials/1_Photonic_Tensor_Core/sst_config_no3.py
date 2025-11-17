
import os
FILE_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(FILE_DIR,'../../utils'))
from byod_components import StreamingCPU, ADC, DAC, Modulator, ClementsMesh
sys.path.append(FILE_DIR)
import sys
import sst
import numpy as np
from argparse import ArgumentParser

# --- Set the simulation parameters ---

parser = ArgumentParser()
parser.add_argument("-clock", type=float,
                    help="set the clock frequency in GHz", default = 1.0) #clock frequency can be set as command line argument when running the SST simulation
args = parser.parse_args()
clock = str(args.clock) +f"Ghz"
size = 8 #size of the data vector
resolution = 8 #bit resolution of the data
data_size = 50 #number of data points
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
clementsMesh = ClementsMesh(size = size, pPi = P_pi, R = R)
mod_helper = Modulator(R = R, pPi = P_pi)

# --- Generate the test data and test matrix. Decompose the matrix into unitary matrices (U,S,V) using SVD decomposition ---

test_matrix = np.random.uniform(0, 1, (size,size))
test_data = np.random.uniform(0, 1, size * data_size)
U, S, V = np.linalg.svd(test_matrix)
scale_s = np.max(S)
S = S / scale_s
U = U
V = V

# --- Compute voltages to program the Clements meshes ---

clementsMesh.decompose(U)
voltages_u = clementsMesh.voltages
clementsMesh.decompose(V)
voltages_v = clementsMesh.voltages
voltages_s = mod_helper.getVoltagesFromAmplitudes(S)
voltages_data = mod_helper.getVoltagesFromAmplitudes(test_data)

# --- Compute corresponding DAC levels ---

adlevels_u = dac_helper.getLevelsFromVoltages(voltages_u)
adlevels_s = dac_helper.getLevelsFromVoltages(voltages_s)
adlevels_v = dac_helper.getLevelsFromVoltages(voltages_v)
adlevels_data = dac_helper.getLevelsFromVoltages(voltages_data)

# --- Generate memory image ---

weight_bytes = np.zeros(0)
weight_bytes = np.append(weight_bytes, cpu_helper.getBytesFromLevels(adlevels_u))
weight_bytes = np.append(weight_bytes, cpu_helper.getBytesFromLevels(adlevels_s))
weight_bytes = np.append(weight_bytes, cpu_helper.getBytesFromLevels(adlevels_v))

data_bytes = cpu_helper.getBytesFromLevels(adlevels_data)

# --- Set up the CPU ---

cpu = sst.Component("test", "byod.StreamingCPU")
cpu.addParams({
    "memory": (np.append(weight_bytes, data_bytes)).tolist(),
    "weight_address": (np.array([0, len(weight_bytes), len(weight_bytes) / np.ceil(resolution/8), resolution]).flatten()).tolist(),
    "vector_base_addr": len(weight_bytes),
    "size": size,
    "resolution": resolution,
    "frequency": clock,
    "vector_count": len(test_data) / size,
    "verbose": DEBUG_LEVEL,
})

iface = cpu.setSubComponent("memory", "memHierarchy.standardInterface")
iface.enableAllStatistics()

# --- Set up the memory ---

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams(
    {
        "debug": 0,
        "debug_level": 0,
        "clock": f"1.2GHz",
        "addr_range_end": 1024 * 1024 * 1024 - 1,
    }
)

memory = memctrl.setSubComponent("backend", "memHierarchy.dramsim3")
memory.enableAllStatistics()
memory.addParams({
    "mem_size": "1GiB",
    "config_ini" : DRAM_CONFIG,
    "output_dir": STATISTICSPATH_DRAM,
})

# --- Set up the DAC connecting the CPU to the optical modulator for feeding in data ---

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

# --- Set up the DAC connecting the CPU to the mesh modulators for programming the weight matrix ---

dac_weight = sst.Component("dac_weight", "byod.DAC")
dac_weight.enableAllStatistics()
dac_weight.addParams({
    "size": (2 * size * size + size),
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

# --- Set up the optical mesh for performing a matrix-vector multiplication on the input data ---

mesh = sst.Component("mesh", "byod.ClementsSVD")
mesh.enableAllStatistics()
mesh.addParams({
    "size": size,
    "opticalLoss": 0.0,
    "verbose": DEBUG_LEVEL,
})

mesh_mod = mesh.setSubComponent("modulator", "byod.thermoOpticModulator")
mesh_mod.addParams({
    "resistance": R,
    "p_pi": P_pi,
})
mesh_mod.enableAllStatistics()

# --- Set up the photodetector detecting the output of the optical mesh ---

pd = sst.Component("pd", "byod.Photodetector")
pd.enableAllStatistics()
pd.addParams({
    "size": size,
    "tiaGain": 100,
    "verbose": DEBUG_LEVEL,
})

# --- Setup the ADC connecting the photodetector the CPU input ---

adc_data = sst.Component("adc_data", "byod.ADC")
adc_data.enableAllStatistics()
adc_data.addParams({
    "size": size,
    "maxVin": vmax,
	"resolution": resolution,
	"frequency": clock,
    "conversionEnergy": "2.66",
    "latency": 100,
    "verbose": DEBUG_LEVEL,
})

# --- Connect the components with links ---

link_cpu_memory_link = sst.Link("link_cpu_memory_link")
link_cpu_memory_link.connect( (iface, "lowlink", "10ps"), (memctrl, "highlink", "10ps") )

link_cpu_dac = sst.Link("cpu_dac")
link_cpu_dac.connect( (cpu, "outputData", "10ps"), (dac_data, "input", "10ps"))

link_cpu_dacweight = sst.Link("cpu_dacweight")
link_cpu_dacweight.connect( (cpu, "outputWeight0", "10ps"), (dac_weight, "input", "10ps"))

link_dac_mod = sst.Link("dac_mod")
link_dac_mod.connect( (dac_data, "output", "10ps"), (mod, "input", "10ps"))

link_dacweight_mesh = sst.Link("dacweight_mesh")
link_dacweight_mesh.connect( (dac_weight, "output", "10ps"), (mesh, "inputWeight", "10ps"))

link_mod_mesh = sst.Link("mod_mesh")
link_mod_mesh.connect( (mod, "output", "10ps"), (mesh, "inputData", "10ps"))

link_mesh_pd = sst.Link("mesh_pd")
link_mesh_pd.connect( (mesh, "output", "10ps"), (pd, "input", "10ps"))

link_pd_adc = sst.Link("pd_adc")
link_pd_adc.connect( (pd, "output", "10ps"), (adc_data, "input", "10ps"))

link_adc_cpu = sst.Link("adc_cpu")
link_adc_cpu.connect( (adc_data, "output", "10ps"), (cpu, "input", "10ps"))

# --- Set the output for the energy statistics ---

sst.setStatisticLoadLevel(7)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath": STATISTICSPATH, "separator": ","})
