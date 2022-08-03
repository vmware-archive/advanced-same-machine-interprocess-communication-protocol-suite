[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# Advanced Same Machine Interprocess Communication Protocol Suite
## Overview
Advanced Same Machine Interprocess Communication Protocol Suite, or Toroni for short, is a protocol suite for advanced interprocess communication on the same machine. Available as library. Support for more languages and protocols is on the way.

A major design goal for Toroni is to be brokerless. There is no process whose crash may affect the remaining communicating processes.

Protocol Overview:
- RMP - a brokerless reliable totally ordered many-to-many message broadcast
- TP - a publish/subscribe protocol running on top of RMP

## RMP
The Reliable Message Protocol (RMP) is
- brokerless, meaning no dedicated server process is needed to run the protocol
- many-to-many
- totally ordered, meaning all readers see messages from all writers in the same order
- reliable, meaning a reader can detect if it has missed a message
- termination safe, meaning crash of any communicating process is not harmful to the rest
- does not use timeouts to detect process crash
- each message is received exactly once
- messages of any size; capped by the shared memory size

RMP uses shared memory for the data plane and UDP multicast for notification. RMP achieves termination safety by using a robust futex to guard writing the shared memory together with an appropriate data structure. RMP does not use timeouts to detect the death of a process. Reader's access to the shared memory is wait-free.

## TP
The Topic Protocol (TP)
- is publish/subscribe protocol running on top of RMP
- uses client-side filtering, meaning there is no per-topic membership notion
- supports dynamic at runtime sets of reader and writer processes, meaning the only coupling between readers and writers is the topic
- hierarchical topic space, able to post to all descendants, able to receive for all descendants
- readers receive only messages created after the reader was created

## Efficiency
Toroni is very efficient:
- No use, no cost - 0 threads will be used if there are no running readers and writers
- CPU friendly - 0 byte UDP message instead of busy waiting for new messages or other idle strategies
- Throghput of millions messages per second
- Latency in microsends

## Pluggable
Toroni is easy to plug into the threading model your project. It  does not involve thread and synchronization calls per-se.

Toroni itself is also pluggable. UDP notification is opt-in and can be replaced with other notification mechanisms. Custom backpressure strategies can be used.

## C++
Toroni is available as C++ header-only library.

### Build
All dependencies are in a container which can be built with:

#### Linux
```sh
cd toroni
docker build -t toroni-cpp-buildenv ./cpp/build_env
docker run -v `pwd`:/toroni -it toroni-cpp-buildenv bash
```
#### Windows
```sh
cd toroni
docker build -t toroni-cpp-buildenv ./cpp/build_env
docker run -v ${pwd}:/toroni -it toroni-cpp-buildenv bash
```

### Testing
```sh
# Navigate to the cpp folder
cd toroni/cpp

# Unit tests
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug
cmake --build ./build --config Debug
(cd ./build/unit_tests && make test)

# Unit tests with code coverage
cmake -S. -Bbuild -DENABLE_COVERAGE=ON && cmake --build build --target unit_tests unit_test-genhtml
(cd ./build/unit_tests && make test unit_test-gcov)

# System Test
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
(cd ./build/system_tests && make test)

# Documentation
cmake -S. -Bbuild -Dp_build_doc=ON && cmake --build build --target docs_doxygen
```

### Development
If using VS code as IDE, you can leverage the [VS Code Remote-Containers extension]( https://code.visualstudio.com/docs/remote/containers) with the Toroni Dev Container configuration. This will use the above dockerfile and install all necessary VS Code extensions in the remote container.

In order for the container to be detected you need to open the cpp folder within VS code and an option to reopen it in the container will appear in the bottom right corner (as long as you have [VS Code Remote-Containers extension]( https://code.visualstudio.com/docs/remote/containers) installed).

### Directory Structure
- [cpp](./cpp): Toroni c++ implementation
   - [src](./cpp/src): header-only implementatoin
   - [unit_tests](./cpp/unit_tests): unit tests
   - [system_tests](./cpp/system_tests): system tests
   - [benchmark_tests](./cpp/system_tests): system tests
   - [build_env](./cpp/build_env) container with build environment
   - [.devcontainer](./cpp/.devcontainer/) VS Code remote container settings
The detail dirs contain implementation details not indended to be used by clients. The traits dir contain default implementations of types that need to be included explicitly and are used by Toroni and which can be replaced by clients with custom implementations.

## Contributing
The advanced-same-machine-interprocess-communication-protocol-suite project team welcomes contributions from the community. Before you start working with advanced-same-machine-interprocess-communication-protocol-suite, please
read our [Developer Certificate of Origin](https://cla.vmware.com/dco). All contributions to this repository must be
signed as described on that page. Your signature certifies that you wrote the patch or have the right to pass it on
as an open-source patch. For more detailed information, refer to [CONTRIBUTING.md](CONTRIBUTING.md).

## License
advanced-same-machine-interprocess-communication-protocol-suite is available under the [Apache 2 license](LICENSE).
