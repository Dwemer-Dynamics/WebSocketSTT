cmake_minimum_required(VERSION 3.21)

project(WebSocketSTT VERSION 0.0.1)


find_package(SkyrimScripting.CMake CONFIG REQUIRED)
find_package(SkyrimScripting.Plugin CONFIG REQUIRED)

add_skse_plugin(${PROJECT_NAME} 
DATA_FOLDER "C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition/Data"
SOURCES Plugin.cpp Papyrus.cpp VoiceRecord.cpp)

# Find PortAudio
find_package(portaudio CONFIG REQUIRED)
find_package(ixwebsocket CONFIG REQUIRED)


target_link_libraries(${PROJECT_NAME} PRIVATE SkyrimScriptingPlugin::SkyrimScripting.Plugin portaudio_static ixwebsocket::ixwebsocket)
