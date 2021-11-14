# OgarCpp
This project is a naive c++ rewrite of [OgarII](https://github.com/Luka967/OgarII)

## Project Discontinued & DO NOT USE FOR PRODUCTION 
This project is c++ port without in-depth optimizations that utilize low-level design patterns. I was learning c++ in 2019 and had little experience with multithreading so the code is very awkwardly parallelized (lots of inefficient locking). There could be some undiscovered bugs since I didn't test it extensively or had any production use.

## Features
* Complete: All OgarII features besides team mode and minions.
* Tiny: Executable only 2Mb.
* Fast: Accelerated physics calculation with multithreading.
* Fun: Bot "AI" is smarter.

## Install & Build
* Windows - VS2019
* Linux - CMake

Project files are all in the directories. Make sure your compiler supports C++17.
Packages required: uwebsockets and nlohmann/json.
Use vcpkg (make sure `VCPKG_ROOT` is specified in the environment) to install the dependencies but you can modify CMakeLists.txt.

## Performance
Improvement can be seen when `playerMaxCells` is set to 1024 or even 2048. The original OgarII server can't keep up with all the player cell updates and cell boosting and etc, while OgarCpp uses paralleled algorithm to accelerate physics ticking. OgarCpp handles a player with 1024 pieces (huge cell cluster makes quadtree calls very expensive) with about 6% average load, while original OgarII has about 17% average load. Another very important issue is the backpressure handling in the websockets. OgarII doesn't handle backpressure which will cause the buffer to build up causing lag spikes for clients. OgarCpp handles the backpressure with a simple flag telling the `Connection`\/`Player` object to skip the tick function if the buffer is not drained and runs the websocket server in a different thread so it does not block any physics simulation.

## TODO
* Official build release
* Memory check
* Benchmark
* Protocols Endianness support -- current code assume a little-endian machine
