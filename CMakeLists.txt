cmake_minimum_required (VERSION 3.6.0)
project (Klangsynthese)



############# Port Audio #############

add_subdirectory (Libs/portaudio)
include_directories(Libs/portaudio/include)


############# TINY XML 2 #############

add_subdirectory (Libs/tinyxml2)
include_directories(Libs/tinyxml2)

unset (BUILD_STATIC_LIBS CACHE)
unset (BUILD_SHARED_LIBS CACHE)

set (BUILD_STATIC_LIBS ON)
set (BUILD_SHARED_LIBS OFF)


############# Klangsynthese #############

include_directories(Source)

SET(SourceFiles
	Source/IProcessor.h
	Source/AudioIO.h
	Source/AudioIO.cpp
	
	Source/Util/RingBuffer.h
	Source/Util/StackBuffer.h
	
	Source/Entry.h
	Source/Entry.cpp
	Source/ConfigManager.h
	Source/ConfigManager.cpp
)


add_executable(Klangsynthese ${SourceFiles})

target_link_libraries(Klangsynthese portaudio_static)
target_link_libraries(Klangsynthese tinyxml2) #_static


############# CONFIG #############

set_property(TARGET Klangsynthese PROPERTY CXX_STANDARD 11)

