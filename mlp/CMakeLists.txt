cmake_minimum_required(VERSION 3.28)
project(mlp)

set(CMAKE_CXX_STANDARD 17)

set(INCLUDE_DIRS_LOCAL src src/cli src/mlp extern extern/oscpack)
set(INCLUDE_DIRS_SYSTEM /opt/local/include /usr/local/include)
set(LINK_DIRS /opt/local/lib /usr/local/lib)

IF(WIN32)
    set(IpSystemTypePath ip/win32)
    set(LIBS ${LIBS} Ws2_32 winmm)
ELSE(WIN32)
    set(IpSystemTypePath ip/posix)
ENDIF(WIN32)

ADD_LIBRARY(oscpack
        extern/oscpack/ip/IpEndpointName.h
        extern/oscpack/ip/IpEndpointName.cpp
        extern/oscpack/ip/NetworkingUtils.h
        extern/oscpack/${IpSystemTypePath}/NetworkingUtils.cpp
        extern/oscpack/ip/UdpSocket.h
        extern/oscpack/${IpSystemTypePath}/UdpSocket.cpp
        extern/oscpack/ip/PacketListener.h
        extern/oscpack/ip/TimerListener.h
        extern/oscpack/osc/OscTypes.h
        extern/oscpack/osc/OscTypes.cpp
        extern/oscpack/osc/OscHostEndianness.h
        extern/oscpack/osc/OscException.h
        extern/oscpack/osc/OscPacketListener.h
        extern/oscpack/osc/MessageMappingOscPacketListener.h
        extern/oscpack/osc/OscReceivedElements.h
        extern/oscpack/osc/OscReceivedElements.cpp
        extern/oscpack/osc/OscPrintReceivedElements.h
        extern/oscpack/osc/OscPrintReceivedElements.cpp
        extern/oscpack/osc/OscOutboundPacketStream.h
        extern/oscpack/osc/OscOutboundPacketStream.cpp
)
target_include_directories(oscpack PUBLIC extern/oscpack)

add_executable(mlp-cli src/cli/main.cpp)

target_include_directories(mlp-cli PUBLIC ${INCLUDE_DIRS_LOCAL})
target_include_directories(mlp-cli PUBLIC ${INCLUDE_DIRS_SYSTEM})
target_link_directories(mlp-cli PUBLIC ${LINK_DIRS})
target_link_libraries(mlp-cli PUBLIC oscpack rtaudio)
