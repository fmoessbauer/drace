# DRaceGUI

A graphical user interface for a more beginner friendly usage of the DRace data race detector tool.

With the windows application all the needed paths (e.g. to DynamoRIO, the drace-client, the executable under test) can be selected. Furthermore some options can be selected and some validation is done by the tool.

Once, everything is set up, the command can be copied manually (Copy to Clipboard) into a shell or the RUN button can be hit which will directly open up a powershell window and execute the command.


## How to use

### Installation
If the install target of DRace is built, the ```drace-gui.exe``` can be found in the ```/bin``` folder and can be launched from there. Qt5 and Boost is needed to build the executable.


### Hints
- Debug Check-Box: sets the ```-debug``` flag of DynamoRio.
- Report Check-Box: if set, a HTML report will be created directly after the DRace execution.
- MSR Check-Box: sets the ```--extctrl``` of the ```drace-client.dll``` and will start the ```MSR.exe```, when command is launched via the RUN button.
- If custom DRace flags are set with the text input, potentially needed quotes must be set by the user.
- The RUN button only works, if the command meets certain validation criteria (this does not mean that a command is valid, when RUN works).
- The report settings can be adjusted in *Report->Configure Report*.
- Further settings can be found under *Tools->Options*, for instance setting the configuration file with the default extension to be able to open with the DRaceGUI by default.




## Dependencies
- Qt5
- Boost
