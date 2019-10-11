# ReportConverter

A Python XML to HTML report converter for the better visualization of drace result data.

Takes an existing drace xml report searches for the mentioned source files on the local machine and puts everthing together for a nice looking, interactive HTML document.

## How to use

### Installation
One can either use directly the python script, which is the faster option, but requires a Python3 installation or one can use the ReportConverter.exe, which is also built, but exexcutes pretty slow.
Beside the script the folder '/resources is also needed for execution.


### Example
Windows:
```bash
    python ReportConverter.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]

    ReportConverter.exe -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]
```
Linux:
```bash
    python3 ReportConverter.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]
```
### needed parameter:
```bash
-i: specifies a drace-xml file

    -i "C:/my/awesome/report.xml"
 ```
### optional parameters:
```bash
- -o: specifies an output directory (default is: ./draceGUI_output_<date>_<time>/)

    -o "C:/plenty/of/reports"

- -b: sourcefiles can be excluded from being loaded in the report

    -b "C:/do/not/show/in/report, C:/private" excludes all files in the specified folders and all of their subfolders

- -w: sourcefile can specifically included;

    -w "C:/just/show/this/files, C:/public" exclusively includes all files in the specified folders and all their subfolders
```
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
