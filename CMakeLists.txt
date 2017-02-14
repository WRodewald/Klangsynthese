cmake_minimum_required (VERSION 3.6.0)
project (Klangsynthese)



############# Port Audio #############

add_subdirectory (Libs/portaudio)
include_directories(Libs/portaudio/include)


############# TINY XML 2 #############

add_subdirectory (Libs/tinyxml2)
include_directories(Libs/tinyxml2)


############# RtMidi #############

include_directories(Libs/rtmidi-2.1.1) # source is directly included to build


############# Klangsynthese #############

include_directories(Source)

SET(SourceFiles
	Source/IProcessor.h
	Source/AudioIO.h
	Source/AudioIO.cpp
	Source/MidiIO.h
	Source/MidiIO.cpp
	
	Source/Util/RingBuffer.h
	Source/Util/StackBuffer.h
	
	Source/Entry.cpp
	Source/ConfigManager.h
	Source/ConfigManager.cpp
	Source/CQTAdditiveTable.h
	Source/CQTAdditiveTable.cpp
	Source/CQTTablePlayer.h
	Source/CQTTablePlayer.cpp
	
	Libs/rtmidi-2.1.1/RtMidi.h
	Libs/rtmidi-2.1.1/RtMidi.cpp
)

add_executable(Klangsynthese ${SourceFiles})


############# RtMidi #############

if (MSVC OR WIN32)
	
	target_compile_definitions(Klangsynthese PUBLIC __WINDOWS_MM__)	

elseif (UNIX)
	
	set(RT_MIDI_UNIX_API "ALSA" CACHE STRING "ALSA or JACK")
	string( TOLOWER ${RT_MIDI_UNIX_API} RtMidiLinuxAPILower )
	if(${RtMidiLinuxAPILower} STREQUAL "alsa")
	target_compile_definitions(Klangsynthese PUBLIC __LINUX_ALSA__)	

	elseif(${RtMidiLinuxAPILower}  STREQUAL "jack")
	target_compile_definitions(Klangsynthese PUBLIC __UNIX_JACK__)	
	endif()
	
elseif (APPLE)

	target_compile_definitions(Klangsynthese PUBLIC __MACOSX_CORE__)	

endif()


############# TINY XML 2 #############

if(TARGET tinyxml2_static)
	target_link_libraries(Klangsynthese tinyxml2_static)	
elseif(TARGET tinyxml2)
	target_link_libraries(Klangsynthese tinyxml2)	
endif()


############# Port Audio #############

target_link_libraries(Klangsynthese portaudio_static)


############# CONFIG #############

set_property(TARGET Klangsynthese PROPERTY CXX_STANDARD 11)

