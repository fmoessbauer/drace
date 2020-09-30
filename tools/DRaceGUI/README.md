# DRaceGUI

A graphical user interface for a more beginner friendly usage of the DRace data race detector tool.

With the windows application, all the needed paths (e.g. to DynamoRIO, the drace-client, the executable under test) can be selected. Further options can also be selected and some validation is done by the tool.

Once all data and options are entered, the command could either be copied manually (Copy to Clipboard) into a shell and executed or the RUN button could be clicked, which would directly open up a powershell window and execute the command.


## How to use
For a more detailed usage of the GUI, please refer to the [HowTo](https://code.siemens.com/multicore/drace/-/tree/master/HowTo).

### Dependencies
- Qt5
- Boost
- [nlohmann/json](https://github.com/nlohmann/json)


### Installation
If the install target of DRace is built, the ```drace-gui.exe``` can be found in the ```/bin``` folder and can be launched from there. Qt5 and Boost are needed to build this executable.


### Hints
- *Debug* Check-Box: sets the ```-debug``` flag of DynamoRio.
- *Report* Check-Box: if set, an HTML report will be created directly after the DRace execution.
- *MSR* Check-Box: sets the ```--extctrl``` of the ```drace-client.dll``` and will start the ```MSR.exe``` when command is launched via the *RUN* button.
- If custom DRace flags are set with the text input, potentially needed quotes must be set by the user.
- The *RUN* button only works, if the command meets certain validation criteria (this does not mean that a command is valid, when RUN works).
- By default, the output is shown in the output console. To run in a separate window, click the respective checkbox. Otherwise, the command can be copied and executed in an arbitrary shell. Please note that the embedded console does not serve as a terminal emulator, but rather as simple input and output fields for communication with the running process.
- The report settings can be adjusted in *Report->Configure Report*.
- Further settings can be found under *Tools->Options*, for instance setting the configuration file with the default extension to be able to open with the DRaceGUI by default *[please note that moving, deleting or updating the DRaceGUI after setting this option requires reverting and setting it again]*.