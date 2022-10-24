JProfiler in the container
=============

## Download JProfiler
Download JProfiler from [here](https://www.ej-technologies.com/download/jprofiler/files).

## Modify the image and rebuild the container
JProfiler should be added to the image
```docker
FROM maven:3.8-jdk-11 AS java-dev
RUN wget https://download.ej-technologies.com/jprofiler/jprofiler_linux-x64_13_0_3.tar.gz -P /tmp/ &&\
 tar -xzf /tmp/jprofiler_linux-x64_13_0_3.tar.gz -C /usr/local &&\
 rm /tmp/jprofiler_linux-x64_13_0_3.tar.gz
EXPOSE 8849
...
```
If port 8849 is in use you can choose another one. After that build the image and run the container with
```sh
docker build -t toroni-java-buildenv --target java-dev ./java/build_env
docker run -v ${pwd}:/toroni -p 8849:8849 -it toroni-java-buildenv bash
```

## Run Java code
Run the Java code you want to run adding `-agentpath:/usr/local/jprofiler13.0.3/bin/linux-x64/libjprofilerti.so=port=8849` option. Once you see `JProfiler> Waiting for a connection from the JProfiler GUI ...` on the terminal you are done with the work in the container.

## Attach the container to the JProfiler GUI
1. Open the JProfiler (outside the container) and open a new session (press `Ctrl+N` or click `"New Session"` in the Session menu).
2. Select the `"Attach to remote JVM"` option and enter the IP address (in our case `127.0.0.1`) and port (in our case `8849`) in Profiled JVM Settings section. Then click `OK`.
3. Click `Instrumentation` in the list of possibilities that will appear.
4. Change `Initial recording profile` to `CPU recording`, change `JVM exit action` to `Keep VM alive for profiling` and click `OK`.

Now you will be able to see the results of your program running inside the container.