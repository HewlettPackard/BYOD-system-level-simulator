# BYOD - Build&benchmark Your Optical Device
A cycle-accurate, event-driven system-level simulator to evaluate the performance of photonic systems in computer architectures.

## Installation
Clone repository and set up all submodules
```bash
git clone stuff
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
The simulator's interface is basically as with any SST project. 



# Authors
- Fabian Böhm
- Sébastien d'Herbais de Thun
- Morten Kapusta, [morten-kapusta](github.hpe.com/morten-kapusta)


# Credits

