# OgarCpp
This project is a naive rewrite of [OgarII](https://github.com/Luka967/OgarII) in C++ with multithreading. I stopped working on this since it's just a blind rewrite without in-depth optimizations that utilize low-level design patterns.

## Features
* Complete: Pretty much everything in OgarII besides team mode.
* Tiny: Executable only 2Mb.
* Fast: Accelerated physics calculation with multithreading.
* Fun: Bot AI is smarter.
* Example frontend included in `/public` (vanis client)

## Install & Build
* ### Windows
  VS2019
* ### Linux
  CMake

Project files are all in the directories. Make sure your compiler supports C++17.
There are only 2 dependencies: uwebsockets and nlohmann/json. For linker you need:
uSockets, zlib (required by uwebsockets) and pthread. 
Use vcpkg (make sure `VCPKG_ROOT` is specified in the environment) to install the dependencies but you can use whatever you want and modify CMakeLists.txt.

## Performance
There's been very little benchmark in this field and it's hard to make a standard, but the improvement can be seen when `playerMaxCells` is set to 1024 or even 2048. The original OgarII server can't keep up with all the player cell updates and cell boosting and etc, while OgarCpp uses multithreading and optimized algorithm to accelerate physics ticking. OgarCpp handles a player with 1024 pieces (huge cell cluster which makes quadtree calls very expensive) with about 6% average load, while original OgarII has about 17% average load. Another very important issue is the backpressure handling in the websockets. OgarII doesn't handle backpressure which will cause the buffer to take longer and longer time to send; the client will experience lag spikes and high pings in this situation. OgarCpp handles the backpressure with a simple boolean flag telling the `Connection`\/`Player` object to skip the tick function if the buffer is not drained.

## TODO
* Official build release
* Memory check (pretty sure there's no leak but need more test)
* Minions
* Benchmark
* Endianness support to protocols since we are assuming the server runs on a little-endian machine
* More gamemodes (probably not going to be bothered since there's more work to frontend than the backend)
* Optimize even more!?!?

## Bugs
* Web serving crashes on Linux after sometime

## Notes
Reasons why I rewrote this in c++:
* Bored
* NodeJS is slow with only 1 thread available, multithreading is too much pain in JS.
* Just learned C++ in some college courses and I wanted to practice it more since it's a powerful language
