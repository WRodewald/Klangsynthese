cmake_minimum_required (VERSION 3.6.0)
project (Klangsynthese)

add_subdirectory (Libs/portaudio)

include_directories(Source)

include_directories(Libs/portaudio/include)


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
