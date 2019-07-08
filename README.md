# DRace-GUI

A Python XML to HTML report converter for the better visualization of drace result data.

Takes an existing drace xml report searches for the mentioned source files on the local machine and puts everthing together for a nice looking, interactive HTML document.

## How to use

### Installation
The tool itself does not need any installation, just a Python3 installation is needed.
Beside the script the folder '/templates' is also needed for execution.

### Example
Windows:
    python drace-gui.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]

Linux:
    python3 drace-gui.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]

### needed parameter:

-i: specifies a drace-xml file

    -i "C:/my/awesome/report.xml"
 
### optional parameters:

- -o: specifies an output directory (default is: ./drace-gui_output/)

    -o "C:/plenty/of/reports"

- -b: sourcefiles can be excluded from being loaded in the report

    -b "C:/do/not/show/in/report, C:/private" excludes all files in the specified folders and all of their subfolders

- -w: sourcefile can specifically included;

    -w "C:/just/show/this/files, C:/public" exclusively includes all files in the specified folders and all their subfolders

*Info:* 
- blacklist wins over whitelist (-> whitelisted files and subfolders can be blacklisted and therefore be excluded)
- all needed files must be whitelisted if at least on element is whitelisted
- if a path is specified, all subelements of the path are treated the same

## Dependencies

Python3

### Libraries

Mandatory (only standard python libs):
- xml.etree.ElementTree
- os
- shutil
- pathlib
- argparse

Optional (for chart creation)
- matplotlib

### Programs

If the user has installed Visual Studio Code and code.exe is in the PATH variable (is by default), then source file can be opened at the line of interest via a click on the link in the HTML file.

If VS Code is not installed the files will be opened with the used browser.