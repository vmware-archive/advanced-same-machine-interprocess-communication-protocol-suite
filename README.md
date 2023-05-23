[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![codecov](https://codecov.io/gh/vmware-labs/advanced-same-machine-interprocess-communication-protocol-suite/branch/main/graph/badge.svg?token=MM49G1ZUL0)](https://codecov.io/gh/vmware-labs/advanced-same-machine-interprocess-communication-protocol-suite)

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
Toroni is available as [C++ header-only library](./cpp/README.md).
## Contributing
The Advanced Same Machine Interprocess Communication Protocol Suite project team welcomes contributions from the community. Before you start working with this project please read and sign our Contributor License Agreement (https://cla.vmware.com/cla/1/preview). If you wish to contribute code and you have not signed our Contributor Licence Agreement (CLA), our bot will prompt you to do so when you open a Pull Request. For any questions about the CLA process, please refer to our [FAQ](https://cla.vmware.com/faq).

## License
Advanced Same Machine Interprocess Communication Protocol Suite is available under the [Apache 2 license](LICENSE).
