# OgarCpp
[OgarII](https://github.com/Luka967/OgarII) implemented in C++ with high performance.

## Features
* Complete: Pretty much everything in OgarII besides team mode.
* Tiny: Executable only 2Mb.
* Fast: Accelerated physics calculation with multithreading.
* Fun: Bot AI is smarter.
* Example frontend included in `/public`

## Install & Build
* ### Windows
  VS2019
* ### Linux
  CMake

Project files are all in the directories. Make sure your compiler supports C++17.
There are only 2 dependencies: uwebsockets and nlohmann/json. For linker you need:
uSockets and zlib (required by uwebsockets) and pthread. 
I used vcpkg (make sure `VCPKG_ROOT` is specified in the environment) to install the dependencies but you can use whatever you want and modify CMakeLists.txt.

## Performance
There's been very little benchmark in this field and it's hard to make a standard. But the result is quite obvious when `playerMaxCells` is set to 1024 or even 2048. The original OgarII server can't keep up with all the player cell updates and cell boosting and etc, while OgarCpp uses multithreading and optimized algorithm to accelerate physics ticking. The result is quite astonishing: OgarCpp handles a  player with 1024 pieces (huge cell cluster which makes quadtree calls very expensive) with about 6% average load, while original OgarII has about 17% average load. Another very important issue is the backpressure handling in the websockets. OgarII doesn't handle backpressure which will cause the buffer to take longer and longer time to send; the client will experience lag spikes and high pings in this situation. OgarCpp handles the backpressure with a simple boolean flag telling the `Connection`\/`Player` object to skip frame if the buffer is not drained.

## Testing
You can build and run this locally for yourself, but you are too lazy (like me), you can always check out a running instance at http://ogarcpp.yuu.studio (Test Server).
This is currently hosted on a $6 high-frequency Vultr VPS (in EU), with the settings in `bench.json`. Very limited resource, yet high performance.

## TODO
* Official build release
* Memory check (pretty sure there's no leak but need more test)
* Minions
* Benchmark
* Endianness support to protocols since we are assuming the server runs on a little-endian machine
* More gamemodes (probably not going to be bothered since there's more work to frontend than the backend)
* Optimize even more!?!?

## Bugs
* Web serving crashes on Linux after awhile

## Notes
Reasons why I'm rewriting this in c++:
* Bored
* NodeJS kinda slow
* Just learned this language in some lame college courses and I want to practice it more since it's a powerful language
