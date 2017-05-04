# Klangsynthese

## CMake:
- set BUILD_STATIC_LIBS to true (SHARED to false) 
- configure RtMIDI and PortAudio to use ALSA 

Build should run through smoothly when all dependencies (basically only ALSA / Jack) are installed. 

## Executable 

run 

```
./Klangsynthese <filename> [-c] [-d] [-mt] [-a] [-v <voices>] [-l <limit]
```

- ```-c```  config mode to set audio out, sample rate and internal frame size
- ```-d```  debug mode, additional output (CPU load etc)
- ```-mt```  uses multi threading when importing include files 
- ```-a```  auto  mode, plays an arpeggio in case MIDI input isn't working 
- ```-l```  limit of bins being processed. Automatically filters out the quietest bins in every table
- ```-v```  specifies the number of voices being processed
- ```-file``` imports table files or cache (```.table```) files
