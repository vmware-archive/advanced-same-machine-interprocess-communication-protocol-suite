Java
=========
Toroni is available as a Java jar.

## Build
All dependencies are in a container which can be built with:

### Linux
```sh
cd toroni
docker build -t toroni-java-buildenv --target java-dev ./java/build_env
docker run -v `pwd`:/toroni -it toroni-java-buildenv bash
```
### Windows
```sh
cd toroni
docker build -t toroni-java-buildenv --target java-dev ./java/build_env
docker run -v ${pwd}:/toroni -it toroni-java-buildenv bash
```

## Testing
```sh
# Navigate to the toroni folder in java
cd toroni/java/toroni

# Unit tests
mvn clean install -DskipTests
mvn test

# System tests
mvn clean install -DskipTests
cd src/test/java/toroni/system_tests
bash robust-writer.sh
bash robust-reader.sh
bash stress-no-expire.sh
```

### Interop testing
In order to test the compatibility of jToroni with Toroni C++, a new container should be build by
```sh
# Navigate to the toroni folder
cd toroni

# Build the container containing the java project and the C++ burst agent
docker build -t toroni-interop-test -f .\interop-tests\cpp-java.Dockerfile .
docker run --privileged --shm-size=128m -it toroni-interop-test
```

## Development
If using VS code as IDE, you can leverage the [VS Code Remote-Containers extension]( https://code.visualstudio.com/docs/remote/containers) with the Toroni Dev Container configuration. This will use the above dockerfile and install all necessary VS Code extensions in the remote container.

In order for the container to be detected you need to open the java folder within VS code and an option to reopen it in the container will appear in the bottom right corner (as long as you have [VS Code Remote-Containers extension]( https://code.visualstudio.com/docs/remote/containers) installed).

### Using JProfiler from the container
In order to use JProfiler in the container and then get the results out of the container follow these [instructions](./README.jProfiler.md).

## Directory Structure
- [java](.): jToroni
   - [toroni](./toroni): Maven project
     - [src](./toroni/src): Source code
       - [main](./toroni/src/main/java/toroni): jToroni implementation
       - [test](./toroni/src/test/java/toroni): Unit tests
     - [pom.xml](./toroni/pom.xml): Maven pom file
   - [build_env](./build_env) Container with build environment
   - [.devcontainer](./.devcontainer/) VS Code remote container settings

The detail dirs contain implementation details not intended to be used by clients. The traits dir contain default implementations of types that need to be included explicitly and are used by Toroni and which can be replaced by clients with custom implementations.