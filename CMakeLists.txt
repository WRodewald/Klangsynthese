cmake_minimum_required (VERSION 3.6.0)
project (Klangsynthese)



############# Port Audio #############

add_subdirectory (Libs/portaudio)
include_directories(Libs/portaudio/include)


############# TINY XML 2 #############

add_subdirectory (Libs/tinyxml2)
include_directories(Libs/tinyxml2)

############# Klangsynthese #############

include_directories(Source)

SET(SourceFiles
	Source/IProcessor.h
	Source/AudioIO.h
	Source/AudioIO.cpp
	
	Source/Util/RingBuffer.h
	Source/Util/StackBuffer.h
	
	Source/Entry.cpp
	Source/ConfigManager.h
	Source/ConfigManager.cpp
	Source/CQTAdditiveTable.h
	Source/CQTAdditiveTable.cpp
	Source/CQTTablePlayer.h
	Source/CQTTablePlayer.cpp
)

add_executable(Klangsynthese ${SourceFiles})


if(TARGET tinyxml2_static)
	target_link_libraries(Klangsynthese tinyxml2_static)	
elseif(TARGET tinyxml2)
	target_link_libraries(Klangsynthese tinyxml2)	
endif()

target_link_libraries(Klangsynthese portaudio_static)


############# CONFIG #############

set_property(TARGET Klangsynthese PROPERTY CXX_STANDARD 11)

