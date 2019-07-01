#DRace-GUI

A Python XML to HTML report converter for the better visualization of drace result data.

Takes an existing drace xml report searches for the mentioned source files on the local machine and puts everthing together for a nice looking, interactive HTML document.

###call with:
    python drace-gui.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]

###needed parameter:
-i: specifies a drace-xml file
    -i "C:/my/awesome/report.xml"
 
###optional parameters:
-o: specifies an output directory (default is: ./drace-gui_output/)
    -o "C:/plenty/of/reports"
-b: sourcefiles can be excluded from being loaded in the report
    -b "C:/do/not/show/in/report, C:/private" excludes all files in the specified folders and all of their subfolders
-w: sourcefile can specifically included;
    -w "C:/just/show/this/files, C:/public" exclusively includes all files in the specified folders and all their subfolders
info: 
- blacklist wins over whitelist (-> whitelisted files and subfolders can be blacklisted and therefore be excluded)
- all needed files must be whitelisted if at least on element is whitelisted