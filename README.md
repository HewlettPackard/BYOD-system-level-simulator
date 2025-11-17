# BYOD - <u>B</u>uild & benchmark <u>Y</u>our <u>O</u>ptical <u>D</u>evice

## Introduction

BYOD is an event-driven system-level simulation framework for the design and evaluation of large-scale photonic information processing systems. It consists of a set of modular simulation tools for opto-electronics components, such as lasers, optical modulators, photodiodes, optical meshes and analog-digital converters, that perform time-discrete functional simulations of signal flows. These modular simulators can be flexibly combined to build scalable simulations of opto-electronic systems, potentially containing thousands of components. BYOD is built on top of the structurcal simulation toolkit (SST), a widely used simulation framework for high-performance-computing (HPC) architectures. BYOD's opto-electronic simualtion tools can be interfaced with various computer architecture simulation tools, allowing co-simulation of photonic components with, e.g., memory, network controllers and CPUs. This enables BYOD to be used for design and cycle-accurate evaluation of photonic systems inside of computer architectures, e.g., for photonic interconnects [^1] or photonic accelerators [^2,^3].

Possible use cases for BYOD include:
+ Functional verification, profiling and optimization of pipelined data flows in photonic systems
+ Cycle-accurate evaluation of througput, latency and energy consumption
+ Optimization of hardware parameters to maximize a system's performance
+ Co-design and benchmarking of photonic systems and computer architectures
+ Development and testing of software stacks (e,g, compilers) for opto-electronic computer architectures

BYOD has been used, a.o., for the design of photonic hardware accelerators for linear algebra operations and AI workloads [^2,^3]. 

## Installing BYOD

BYOD requires installation of the Structural Simulation Toolkit [(SST)](www.sst-simulator.org) developed by Sandia National Laboratories. SST can be installed on various Unix, Linux, MaxOS and Windows (using the [Windows Subsystem for Linux](https://learn.microsoft.com/en-us/windows/wsl/install)) systems. A full list of officially supported operating systems is provided [here](https://sst-simulator.org/SSTPages/SSTElementReleaseMatrix/). To simplify the installation process, BYOD can be run as a Docker container. A build script for the container image with all dependencies is provided here: [Docker image builder](./docker/) 

Detailed installation instructions for BYOD can be found here:
[Installation instructions](./sst-elements/src_cpp)

## Using BYOD

BYOD provides a set of modular simulators for various opto-electronic components that can compiled as SST components. Components include ports for receiving and sending data to other components. Each component will perform operations on an event-driven basis, e.g., when receiving data or during a simulated clock cycle. The operations within elements and the communiation with other elements incorporate processing delays to model the propagation of the data flow in time. Simulations of photonic systems in BYOD can be created and executed like any other [SST simulation](https://sst-simulator.org/sst-docs/docs/guides/runningSST) by using a Python configuration file. The [configuration file](https://sst-simulator.org/sst-docs/docs/config) defines and configures simulated components (e.g., lasers, modulators, CPUs) and links them together to enable data flow between simulators:

```text
component = sst.Component("<component name>", "byod.<BYOD component>") # define simulator component
component.addParams({ # configure component parameters
    "<name of parameter>" : <value>,
})
link = sst.Link("<link name>") # create component link
link.connect( (component, "<port>", "<delay>"), (<component>, "<port>", "<delay>") ) # link ports of different components with a propagation delay
```

Simulations are executed using the command:

```text
$ sst <name of config file>.py
```

SST can also natively exploit the message parsing interface (MPI) to run simulations in parallel and speed up computation. Parallel simulations are executed using the command:

```text
$ mpirun -np <#parallel processes> sst <name of config file>.py
```

To learn the usage of BYOD, we have provided a set of tutorial examples that explain how to build and run simulations. A list of all tutorials can be found here:
[Tutorials](./tutorials/) 

An overview of simulator elements and features included in BYOD can be found here:
[Documentation](./documentation/) 

## Citing BYOD

Please cite the following:

> F. Böhm, S. d'Herbais de Thun, M. Kapusta, M. Hejda, B. Tossoun, R. Beausoleil, T. Van Vaerenbergh, "BYOD: A System-Level Simulator Framework for End-To-End Evaluation and Architecture Design of Photonic AI Accelerators," 2025 Conference on Lasers and Electro-Optics Europe & European Quantum Electronics Conference (CLEO/Europe-EQEC), Munich, Germany, 2025 [doi:10.1109/CLEO/Europe-EQEC65582.2025.11109777](https://doi.org/10.1109/CLEO/Europe-EQEC65582.2025.11109777)


## Contributing to BYOD
Thank you for your interest in contributing to BYOD! BYOD is an open-source project and we are continiously looking into extending its functionality and its use cases. If you have found a bug or
want to ask a question or discuss a  new feature (such as adding new simulation models or architectures), please open an
[issue](https://github.com/HewlettPackard/BYOD-system-level-simulator/issues).   Once  a new  feature
has  been implemented  and tested, or a bug has been fixed, please submit a
[pull](https://github.com/HewlettPackard/BYOD-system-level-simulator/pulls) request.

In order for us to accept your pull request, you will need to `sign-off` your commit.
This [page](https://wiki.linuxfoundation.org/dco) contains more information about it.
In short, please add the following line at the end of your commit message:
```text
Signed-off-by: First Second <email>
```

## License
BYOD is licensed under the [MIT](https://github.com/HewlettPackard/X-TIME/blob/master/LICENSE) license.


[^1]: D. Liang et al., "An Energy-Efficient and Bandwidth-Scalable DWDM Heterogeneous Silicon Photonics Integration Platform," in IEEE Journal of Selected Topics in Quantum Electronics, vol. 28, no. 6: High Density Integr. Multipurpose Photon. Circ., pp. 1-19, Nov.-Dec. 2022, Art no. 6100819, [doi:10.1109/JSTQE.2022.3181939](https://ieeexplore.ieee.org/document/9794616)
[^2]: B. Tossoun et al., "Large-Scale Integrated Photonic Device Platform for Energy-Efficient AI/ML Accelerators," in IEEE Journal of Selected Topics in Quantum Electronics, vol. 31, no. 3: AI/ML Integrated Opto-electronics, 2025, Art no. 8200326, [doi:10.1109/JSTQE.2025.3527904](https://ieeexplore.ieee.org/abstract/document/10835188).
[^3]: F. Böhm et al., "BYOD: A System-Level Simulator Framework for End-To-End Evaluation and Architecture Design of Photonic AI Accelerators," 2025 Conference on Lasers and Electro-Optics Europe & European Quantum Electronics Conference (CLEO/Europe-EQEC), Munich, Germany, 2025 [doi:10.1109/CLEO/Europe-EQEC65582.2025.11109777](https://doi.org/10.1109/CLEO/Europe-EQEC65582.2025.11109777)
