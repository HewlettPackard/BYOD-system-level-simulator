# BYOD - Build&benchmark Your Optical Device
A cycle-accurate, event-driven system-level simulator to evaluate the performance of photonic systems in computer architectures.

## Installation
Clone repository and set up all submodules
```bash
git clone git@github.hpe.com:labs/lsip-system-level-simulator.git
cd lsip-system-level-simulator
git submodule update --init --recursive 
```

### Docker (suggested)
We suggest using the provided Docker image for running and developing. You have to have Docker installed on your system (other container tools such as podman should work by adapting the `<root>/docker/dockerctl.sh` file but were not tested).

```bash
# Build the container
cd ./docker
./dockerctl.sh -b <image-name>:<tag> Containerfile <port>
cd ..
# run the container
./docker/dockerctl.sh -s <image-name>:<tag> <name> <path> <port>

# inside container
cd byod
pip install -r requirements.txt
# Setup using Autotools
./autogen.sh
./run_config.sh
# install cacti
cd tests/accelergy_cacti_plugin && make build && cd ../.. 
# compile
make && make install
# run simulation
python3 ./tests/main.py
```

It is important that you go to the correct directory for building and running the container because otherwise files will not be found (the Containerfile) or not be copied to the container (your local copy of this repository when running the container).

> We are working on providing a light-weight Alpine image...

### Local
If you want to run the project on your local machine without Docker, you first have to install the dependencies.
1. SST: [Official Installation guide](http://sst-simulator.org/SSTPages/SSTBuildAndInstall_12dot1dot0_SeriesDetailedBuildInstructions/). You need to install `sst-core` and `sst-elements`. This project has been tested with Version 14.0.0 on Ubuntu (see Container Image)
2. XTensor: We use [XTensor](https://xtensor.readthedocs.io/en/latest/) and [XTensor](https://xtensor.readthedocs.io/en/latest/) for all matrix operations in C++. Follow the installation instructions for your OS.
3. Install Accelergy according to the documentation on their [GitHub](https://github.com/Accelergy-Project/accelergy/tree/master)
4. Fully setup, compile and run the code
```bash
# Setup using Autotools
<root>/autogen.sh
<root>/run_config.sh
# install cacti
cd tests/accelergy_cacti_plugin && make build && cd ../..
# compile
make && make install
# install python deps
pip install -r <root>/requirements.txt
# run the simulation
python <root>/tests/main.py
```

## Interface
The simulator's interface is basically as with any SST project. You can configure your simulation components in `<project-root>/tests/sst_inference.py` (subject to change to `sst-config.py`). Additionally, since the data for inputs and neural network models is passed over files the State Machine component takes parameters for both files (`datapath` and `modelpath`). These paths should point to directories with the following file structures:

For the data directory:
```bash
<data-dir>
|-- data.npy
```

For the neural network model directory:
```bash
<model-dir>
|-- bias.npy
|-- voltages_S.npy
|-- voltages_U.npy
|-- voltages_V.npy
|-- weights.npy
```
Where `bias.npy` is the one dimensional bias vector for the single layer we are currently implementing. `voltages_*.npy` are the voltages to be applied to the MZIs for each matrix from the singular value decomposition. `weights.npy` are the actual weights and are used internally for functional modeling to remove the need for reverting the decomposition algorithm.

When you use our setup and pre-processing script `<project-root>/tests/main.py` it will write the data in such format to directories in your `<project-root>`. However, you can pass any directories that adhere to the provided structure.

## Demo
We have a demo notebook which can be run from the Docker container. 
> If you run the notebook locally you have to have the pip modules installed globally

```bash
# start the jupyter server
jupyter notebook --ip=0.0.0.0 --port=<forwarded-port>
```
Then, open the Jupyter server in the browser and open to `tests/demo.ipynb`.

## Steps forward
Also, see open issues
- add latency breakdown by adding statistic for active runtime for each component
- add automatic post-processing script that can be run to generate figures
- add exception if passed directory for data or model in state machine does not match required format
- allow more input data at once (by fixing memory operations in state machine)
- include drivers for DACs
- allow multi-layer inference
- wavelength division multiplexing
- configurable optical core
    - make subfunctionalities to subcomponents (E-O, O-E, MZI mesh/MRR xbar array, ...)
- more user-friendly interface
    - simulation configuration using YAML (or sth) to hide required pre-processing for energy modeling which makes `sst_inference.py` less readable
    - wrap pre-processing for energy modeling to provide a unified interface
    - make data generation/setup and pre-processing more usable
- state machine based on commands instead of hard-coded long list of states (-> remove need for individual state machines by introducing programmability)

# Authors
- Morten Kapusta, [morten-kapusta](github.hpe.com/morten-kapusta), morten.kapusta@hpe.com
- Fabian Böhm, [fabian-bohm](github.hpe.com/fabian-bohm), fabian.bohm@hpe.com

# Credits
Autotools and Docker related content is based on the implementations of the [XTIME-SST Simulator](https://github.com/HewlettPackard/X-TIME/tree/main/cycle_accurate) by John Moon (jmoon@hpe.com)
