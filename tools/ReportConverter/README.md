# ReportConverter

A Python XML to HTML report converter for a better visualization of drace result data.

Takes an existing drace xml report, searches for the mentioned source files on the local machine and puts everything together for a nice looking, interactive HTML document. Also works now with Valgrind/Helgrind XML files.

## How to use

### Installation
One can either directly use the python script, which is the faster option, however requires a Python3 installation, or one can use the ReportConverter.exe, which is also built, but executes pretty slowly.
Besides the script, the folder `/resources` is also needed for execution.


### Example
Windows:
```bash
    python ReportConverter.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems -s sourceDirectories]

    ReportConverter.exe -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems -s sourceDirectories]
```
Linux:
```bash
    python3 ReportConverter.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems -s sourceDirectories]
```
### Required Parameter:
```bash
-i: specifies a drace-xml file

    -i "C:/my/awesome/report.xml"
 ```
### Optional Parameters:
```bash
- -o: specifies an output directory (default is: ./draceGUI_output_<date>_<time>/)

    -o "C:/plenty/of/reports"

- -b: sourcefiles can be excluded from being loaded into the report

    -b "C:/do/not/show/in/report, C:/private" # excludes all files in the specified folders and all of their subfolders

- -w: sourcefiles can be specifically included

    -w "C:/just/show/this/files, C:/public" # exclusively includes all files in the specified folders and all their subfolders

- -s: directories (not necessarily direct parent ones) to sourcefiles can be specified

    -s "C:/dir1/possible/sourcedir, C:/dir2/possible/sourcedir" # in case of application source relocation, recursively searches through the directories for valid paths to sourcefile(s) 
```
*Info:* 
- blacklist wins over whitelist (-> whitelisted files and subfolders can be blacklisted and therefore be excluded)
- all needed files must be whitelisted if at least one element is whitelisted
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
- functools

Optional (for chart creation):
- matplotlib

### Programs

If the user has Visual Studio Code installed and code.exe is in the PATH variable (is by default), the source file can be opened at the line of interest via a click on the link in the HTML file.

If VS Code is not installed, the files will be opened with the used browser.
