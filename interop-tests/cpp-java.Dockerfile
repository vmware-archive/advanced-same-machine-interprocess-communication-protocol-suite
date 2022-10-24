FROM rikorose/gcc-cmake:gcc-11 AS cpp-builder-release
WORKDIR /toroni/cpp
COPY cpp .
RUN cmake -S . -B ./build-release -DCMAKE_BUILD_TYPE=Release
RUN cmake --build ./build-release --config Release

FROM ubuntu:22.04 AS java-builder
RUN apt update
RUN apt-get -y install maven vim net-tools cmake
WORKDIR /toroni/java
COPY java .
WORKDIR /toroni/java/toroni
RUN mvn install -DskipTests

FROM java-builder AS interop-env
WORKDIR /toroni/cpp
COPY cpp .
WORKDIR /toroni/interop-tests
COPY interop-tests .
COPY --from=cpp-builder-release /toroni/cpp/build-release/system_tests/burst /cpp-burst

ENTRYPOINT (cmake -S . -B ./build && cmake --build ./build && cd build && ctest --timeout 180)