# OgarCpp
Luka's OgarII implemented in C++ with high performance.

## Features
* Pretty much everything in OgarII besides team mode.
* Improved performance
* Improved bot AI

## Install
* ### Windows
  Use VS2019
* ### Linux
  Use Codelite
  
## Build
Project files are all in the directories. Make sure your compiler supports C++17.
There are only 2 dependencies: uwebsockets and nlohmann/json. For linker you need:
uSockets and zlib (required by uwebsockets) and pthread.
I used vcpkg to install the dependencies but you can use whatever you want.

## Performance
There's been very little benchmark in this field and it's hard to make a standard. But the result is quite obvious when `playerMaxCells` is set to 1024 or even 2048. The original OgarII server can't keep up with all the player cell updates and cell boosting and etc, while OgarCpp uses multithreading and optimized algorithm to accelerate physics ticking. The result is quite astonishing: OgarCpp handles a  player with 1024 pieces (huge cell cluster which makes quadtree calls very expensive) with about 6% average load, while original OgarII has about 17% average load. Another very important issue is the backpressure handling in the websockets. OgarII doesn't handle backpressure which will cause the buffer to take longer and longer time to send; the client will experience lag spikes and high pings in this situation. OgarCpp handles the backpressure with a simple boolean flag telling the `Connection`\/`Player` object to skip frame if the buffer is not drained.

## Testing
You can build and run this locally for yourself, but you are too lazy (like me), you can always check out a running instance at http://ogarcpp.yuu.studio (Test Server).
It is currently hosted on a $6 high-frequency Vultr VPS (in EU), with the settings in `bench.json`. Very limited resource, yet high performance.

## TODO
* More gamemodes (probably not going to be bothered since there's more work to frontend than the backend)
* Endianness support to protocols since we are assuming the server runs on a little-endian machine
* Improve physics calculation (fix solotricks)
* Optimize even more!?!?

## Notes
Reasons why I rewrite this in c++:
* Bored
* Just learned this language  through lame college courses and I want to practice it more since it's a powerful language
* The original implementation is in NodeJS. It has some serious performance issues when there're lots of cells (causing heavy quadtree queries and array operations). So I tried to optimize most of the logic in OgarII and made it multithreaded (since NodeJS only uses one thread).
