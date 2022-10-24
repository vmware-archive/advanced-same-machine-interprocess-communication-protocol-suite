C++
=========
Toroni is available as C++ header-only library.

## Build
All dependencies are in a container which can be built with:

### Linux
```sh
cd toroni
docker build -t toroni-cpp-buildenv --target dev-env ./cpp/build_env
docker run -v `pwd`:/toroni -it toroni-cpp-buildenv bash
```
### Windows
```sh
cd toroni
docker build -t toroni-cpp-buildenv --target dev-env ./cpp/build_env
docker run -v ${pwd}:/toroni -it toroni-cpp-buildenv bash
```

## Testing
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

## Doxygen
cmake -S. -Bbuild -Dp_build_doc=ON && cmake --build build --target docs_doxygen
```

### Burst Agent Image
Build an image with the system test burst agent built in release.
```sh
cd toroni/cpp
docker build -t toroni-cpp-burst-agent --target system-test-burst -f ./build_env/Dockerfile .
```

## Development
If using VS code as IDE, you can leverage the [VS Code Remote-Containers extension]( https://code.visualstudio.com/docs/remote/containers) with the Toroni Dev Container configuration. This will use the above dockerfile and install all necessary VS Code extensions in the remote container.

In order for the container to be detected you need to open the cpp folder within VS code and an option to reopen it in the container will appear in the bottom right corner (as long as you have [VS Code Remote-Containers extension]( https://code.visualstudio.com/docs/remote/containers) installed).

## Directory Structure
- [cpp](./cpp): Toroni c++ implementation
   - [src](./cpp/src): header-only implementatoin
   - [unit_tests](./cpp/unit_tests): unit tests
   - [system_tests](./cpp/system_tests): system tests
   - [benchmark_tests](./cpp/system_tests): system tests
   - [build_env](./cpp/build_env) container with build environment
   - [.devcontainer](./cpp/.devcontainer/) VS Code remote container settings
The detail dirs contain implementation details not indended to be used by clients. The traits dir contain default implementations of types that need to be included explicitly and are used by Toroni and which can be replaced by clients with custom implementations.