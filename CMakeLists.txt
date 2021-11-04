cmake_minimum_required(VERSION 3.13.0 FATAL_ERROR)

project(OgarCppServer CXX)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "OgarCpp/src/bots/PlayerBot.h"
    "OgarCpp/src/cells/Cell.h"
    "OgarCpp/src/commands/CommandList.h"
    "OgarCpp/src/gamemodes/FFA.h"
    "OgarCpp/src/gamemodes/Gamemode.h"
    "OgarCpp/src/gamemodes/GamemodeList.h"
    "OgarCpp/src/misc/Misc.h"
    "OgarCpp/src/misc/Stopwatch.h"
    "OgarCpp/src/misc/Ticker.h"
    "OgarCpp/src/primitives/Logger.h"
    "OgarCpp/src/primitives/QuadTree.h"
    "OgarCpp/src/primitives/Reader.h"
    "OgarCpp/src/primitives/Rect.h"
    "OgarCpp/src/primitives/SimplePool.h"
    "OgarCpp/src/primitives/Writer.h"
    "OgarCpp/src/protocols/Protocol.h"
    "OgarCpp/src/protocols/Protocol6.h"
    "OgarCpp/src/protocols/ProtocolModern.h"
    "OgarCpp/src/protocols/ProtocolStore.h"
    "OgarCpp/src/protocols/ProtocolVanis.h"
    "OgarCpp/src/ServerHandle.h"
    "OgarCpp/src/Settings.h"
    "OgarCpp/src/sockets/ChatChannel.h"
    "OgarCpp/src/sockets/Connection.h"
    "OgarCpp/src/sockets/Listener.h"
    "OgarCpp/src/sockets/Router.h"
    "OgarCpp/src/web/AsyncFileReader.h"
    "OgarCpp/src/web/AsyncFileStreamer.h"
    "OgarCpp/src/web/Middleware.h"
    "OgarCpp/src/worlds/MatchMaker.h"
    "OgarCpp/src/worlds/Player.h"
    "OgarCpp/src/worlds/World.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "OgarCpp/src/bots/PlayerBot.cpp"
    "OgarCpp/src/cells/Cell.cpp"
    "OgarCpp/src/cli/Main.cpp"
    "OgarCpp/src/gamemodes/FFA.cpp"
    "OgarCpp/src/gamemodes/Gamemode.cpp"
    "OgarCpp/src/gamemodes/GamemodeList.cpp"
    "OgarCpp/src/primitives/QuadTree.cpp"
    "OgarCpp/src/primitives/SimplePool.cpp"
    "OgarCpp/src/protocols/Protocol6.cpp"
    "OgarCpp/src/protocols/ProtocolModern.cpp"
    "OgarCpp/src/protocols/ProtocolVanis.cpp"
    "OgarCpp/src/ServerHandle.cpp"
    "OgarCpp/src/sockets/ChatChannel.cpp"
    "OgarCpp/src/sockets/Connection.cpp"
    "OgarCpp/src/sockets/Listener.cpp"
    "OgarCpp/src/sockets/Router.cpp"
    "OgarCpp/src/worlds/MatchMaker.cpp"
    "OgarCpp/src/worlds/Player.cpp"
    "OgarCpp/src/worlds/World.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-O3") 

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

include_directories($ENV{VCPKG_ROOT}/installed/x64-linux/include/)
link_directories($ENV{VCPKG_ROOT}/installed/x64-linux/lib/)

set(CMAKE_CXX_STANDARD_LIBRARIES "-static-libstdc++")

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})
target_link_libraries(${PROJECT_NAME} pthread uSockets z stdc++fs uSockets ssl crypto dl)

