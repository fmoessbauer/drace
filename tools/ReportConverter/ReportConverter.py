# 
# ReportConverter: A graphical report generator for DRace
# 
# Copyright 2019 Siemens AG
# 
# Authors:
#   <Philip Harr> <philip.harr@siemens.com>
# 
# SPDX-License-Identifier: MIT
#

## \package ReportConverter
## \brief Python XML to HTML report converter for the better visualization of drace result data



import xml.etree.ElementTree as ET
import shutil
import argparse
import pathlib
import datetime
from subprocess import check_call, STDOUT, DEVNULL



try:
    import matplotlib
    import matplotlib.pyplot as plt
    noMatplotLib = False
except ImportError:
    noMatplotLib = True

#look for resources path
SCRIPTPATH = pathlib.Path(pathlib.Path(__file__).resolve().parents[0])

if pathlib.Path(SCRIPTPATH / '../resources').is_dir():
    resourcesPath = pathlib.Path(SCRIPTPATH / '../resources')

else:
    if pathlib.Path(SCRIPTPATH / 'resources').is_dir():
        resourcesPath = pathlib.Path(SCRIPTPATH / 'resources')
    else:
        print("path of resources not found")
        exit(-1)

#Paths
g_HTMLTEMPLATES = resourcesPath / "entries.xml"
g_CSSPATH = resourcesPath / "css"
g_JSPATH = resourcesPath / "js"

DEBUG = False

#info: blacklisting overrules whitelisting
SOURCEFILE_BL = list()
SOURCEFILE_WL = list()
WHITELISTING = False
NUMBEROFCODELINES = 400
if NUMBEROFCODELINES % 2:
    print('Number of maximum of displayed code lines must be even, but is:')
    print(str(NUMBEROFCODELINES))
    exit(-1)

class ReportCreator:
    _htmlTemplatesPath = str(g_HTMLTEMPLATES)
    _topStackGraphFileName = 'topStackBarchart.png'
    _errorTimesPlot        = 'errorTimes.png'


    try:
        if check_call(['code', '--version'], stdout=DEVNULL, stderr=STDOUT, shell=True) == 0: #check if vscode is installed, for sourcefile linking
            _vscodeFlag = True
        else:
            _vscodeFlag = False
    except:
        _vscodeFlag = False


    def __init__(self, pathOfReport, target):
        self.sourcefileList = list()
        self._callStackNumber = 0
        self._errorNumber = 0
        self._snippets = str()
        self.succesfullReportCreation = True
        self.SCM = SourceCodeManagement()

        try:
            self._htmlTemplates = (ET.parse(self._htmlTemplatesPath)).getroot()
        except FileNotFoundError:
            print("template file is missing")
            self.succesfullReportCreation = False
            return

        self._pathOfReport = pathOfReport
        if self._inputValidation():
            self._createReport()
            if not noMatplotLib:
                self._makeHistogramm(target)
                self._countTopStackOccurences(target)
        else:
            print("input file is not valid")
            self.succesfullReportCreation = False

    def _inputValidation(self):

        
        try:
            self._reportContent = ET.parse(self._pathOfReport)
        except ET.ParseError:
            return 0

        self._reportRoot = self._reportContent.getroot()
        
        if self._reportRoot.find('protocolversion') != None and \
            self._reportRoot.find('protocoltool') != None and \
            self._reportRoot.find('preamble') != None and \
            self._reportRoot.find('pid') != None and \
            self._reportRoot.find('tool') != None and \
            self._reportRoot.find('args') != None and \
            self._reportRoot.find('status') != None and \
            self._reportRoot.tag == 'valgrindoutput':

            return 1
        else:
            return 0

    def _getHeader(self):
        header = list()
        status = self._reportRoot.findall('status')[1]
        strDatetime = status.find('time').text
        date = strDatetime.split('T')[0]
        time = (strDatetime.split('T')[1])[0:-1] #last digit is 'Z' -> not needed
        header.append(adjText(date))
        header.append(adjText(time))
        header.append(adjText(status.find('duration').text))
        header.append(adjText(status.find('duration').get('unit')))

        arguments = str()
        for arg in self._reportRoot.find('args').find('vargv').findall('arg'):
            arguments += arg.text
            arguments += ' '
        header.append(adjText(arguments[0:-1])) #remove last ' '
        
        header.append(adjText(self._reportRoot.find('args').find('argv').find('exe').text))
        header.append(adjText(self._reportRoot.find('protocolversion').text))
        header.append(adjText(self._reportRoot.find('protocoltool').text))

        return header

    def _makeFileEntry(self, frame):
        strDir = adjText(frame.find('dir').text)
        strFile = adjText(frame.find('file').text)
        strLine = adjText(frame.find('line').text)
        offset = adjText(frame.find('offset').text)

        if self._vscodeFlag:
            entry  = "<a href='vscode://file/" + strDir + "/" + strFile + ":" + strLine + ":" + offset +"'>"+ strFile +":" + strLine + ":" + offset + "</a>"
        else:
            entry  = "<a href='file://"+ strDir + "/" + strFile + ":" + strLine + "'>" + strFile + ":" + strLine + "</a>"
        
        return entry

    def _createSnippetEntry(self, frame, elementNumber, tag, codeIndex, buttonID):
        newSnippet = self._htmlTemplates.find('snippet_entry').text

        newSnippet = newSnippet.replace('*SNIPPET_VAR*', ("snippet_" + str(self._callStackNumber)))
        newSnippet = newSnippet.replace('*STACK_NUMBER*', adjText(hex(elementNumber)))
        newSnippet = newSnippet.replace('*OBJ*', adjText(frame.find('obj').text))
        newSnippet = newSnippet.replace('*FUNCTION*', adjText(frame.find('fn').text))
        newSnippet = newSnippet.replace('*INSTRUCTION_POINTER*', adjText(frame.find('ip').text))
        newSnippet = newSnippet.replace('*CODE_TAG*', tag)
        newSnippet = newSnippet.replace('*SNIPPET_BUTTON_ID*', buttonID)

        if (frame.find('file')!= None):
            newSnippet = newSnippet.replace('*FILE_NAME_ENTRY*', self._makeFileEntry(frame))
            newSnippet = newSnippet.replace('*DIRECTORY*', adjText(frame.find('dir').text))
            newSnippet = newSnippet.replace('*SHORT_DIR*', adjText(self._makeShortDir(frame.find('dir').text)))
            newSnippet = newSnippet.replace('*LINE_OF_CODE*', adjText(frame.find('line').text))


            if(codeIndex != -1):
                newSnippet = newSnippet.replace('*CODE_ID_VAR*', "snippet_"+str(self._callStackNumber)+"_code")
                newSnippet = newSnippet.replace('*LANGUAGE*', self.SCM.determineLanguage(adjText(frame.find('file').text)))
                newSnippet = newSnippet.replace('*FIRST_LINE*', str(self.SCM.getFirstLineOfCodeSnippet(codeIndex)))
            else:
                newSnippet = newSnippet.replace('*CODE_ID_VAR*',  "'None'")

        else:
            newSnippet = newSnippet.replace('*FILE_NAME_ENTRY*', 'no filename avail.')
            newSnippet = newSnippet.replace('*DIRECTORY*', 'no directory avail.')
            newSnippet = newSnippet.replace('*SHORT_DIR*', 'no directory avail.')

        self._snippets += newSnippet #append referenced code snippet

    def _makeShortDir(self, strDir):
        elements = strDir.split("\\")
        return elements[0] + "/" + elements[1] + "/.../" + elements[-1]

    def _createCallStack(self, errorEntry, position, outputID):
        
        callStack = str()
        stackTemplate = self._htmlTemplates.find('stack_entry').text
        stackArray = errorEntry.findall('stack')
        stack = stackArray[position]
        elementNumber = 0


        for frame in stack.findall('frame'):
            noPreview = False 
            buttonID = "button_" + str(self._errorNumber) + "_" + str(position) + "_" + str(elementNumber)
            strOutputID = outputID+str(position)

            if elementNumber == 0:
                ###make heading for the red box### 
                if len(self._errorHeading) == 0:
                    self._errorHeading += "<br> Obj. 1: " + (adjText(frame.find('obj').text) + ': "' + adjText(frame.find('fn').text)) + '" <br> '
                else:
                    self._errorHeading += "Obj. 2: " + (adjText(frame.find('obj').text) + ': "' + adjText(frame.find('fn').text)) + '"'
           
            #general entries (always available)
            newStackElement = stackTemplate.replace('*STACK_NUMBER*', adjText(hex(elementNumber))+":")
            newStackElement = newStackElement.replace('*SNIPPET_VAR*', ("snippet_" + str(self._callStackNumber)))
            newStackElement = newStackElement.replace('*OUTPUT_ID*', strOutputID)
            newStackElement = newStackElement.replace('*FUNCTION*', adjText(frame.find('fn').text))
            newStackElement = newStackElement.replace('*BUTTON_ID*', buttonID)
            

            if (frame.find('file')!= None): #file is in xml report defined
                codeIndex, tag = self.SCM.handleSourceCode(frame.find('file').text, frame.find('dir').text, frame.find('line').text)
                newStackElement = newStackElement.replace('*FILE*', adjText(frame.find('file').text))

                if(codeIndex != -1):
                    newStackElement = newStackElement.replace('*CODE_VAR*', str(codeIndex))
                    newStackElement = newStackElement.replace('*CODE_ID_VAR*', "'snippet_"+str(self._callStackNumber)+"_code'")
                    newStackElement = newStackElement.replace('*LINE_OF_CODE*', adjText(frame.find('line').text))
                    newStackElement = newStackElement.replace('*FIRST_LINE*', str(self.SCM.getFirstLineOfCodeSnippet(codeIndex)))  
                
                else: #file is not available on device or file is blacklisted or not whitelisted
                    noPreview = True
                     
                
            else: #no filepath for file in xml is given 
                codeIndex, tag = self.SCM.handleSourceCode("","", "")
                newStackElement = newStackElement.replace('*FILE*', 'no filename avail.')
                noPreview = True

            if noPreview:
                newStackElement = newStackElement.replace('*CODE_VAR*', "'None'")
                newStackElement = newStackElement.replace('*CODE_ID_VAR*',  "'None'")
                newStackElement = newStackElement.replace('*LINE_OF_CODE*', "'None'")
                newStackElement = newStackElement.replace('*FIRST_LINE*', "'NONE'") 
                searchStr = 'class="'
                insertPosition = newStackElement.find(searchStr)+len(searchStr) #to add the ".grey" class the position before after class
                #insertPosition += newStackElement[insertPosition:].find('"')
                newStackElement = newStackElement[:insertPosition] + "grey-button " + newStackElement[insertPosition:] 

            
            self._createSnippetEntry(frame, elementNumber, tag, codeIndex, buttonID)
            callStack += newStackElement #append stack element
            elementNumber += 1
            self._callStackNumber += 1 #increase global call stack number (used for reference variables)

        return callStack

    def _makeHistogramm(self, target):
        errorTimes = dict()
        statusNode = self._reportRoot.findall('status')[1]
        totalDuration =  int(statusNode.find('duration').text)
        errors = self._reportRoot.findall('error')
        
        for error in errors:
            timePoint = (round(float(100 * int(error.find('timestamp').text) /totalDuration))) #get occurance in %
            if errorTimes.get(timePoint) != None:
                value = errorTimes.pop(timePoint)
                errorTimes.update({timePoint: int(value)+1})
            else:
                    errorTimes.update({timePoint: 1})
            
        x = list(errorTimes.keys())
        y = list(errorTimes.values())
        #make plot       
        fig = plt.figure(figsize=(10,4))    
        ax = plt.axes() 
        ax.scatter(x, y, color='#009999', edgecolor='black')

        xRangeEnd = max(y)+1
        if xRangeEnd < 3:    #0, 1, 2 shall be always visible, even if max(y) is only 1
            xRangeEnd = 3
        ax.set_yticks([i for i in range(0, xRangeEnd)])
        ax.set_xticks([i for i in range(0, 110, 10)])
      
        plt.title('Error occurrences by time',fontfamily="monospace", fontweight='bold')
        plt.ylabel('Occurrences', fontfamily="monospace",fontweight='bold')
        plt.xlabel('Execution of program in %. \n Total execution time = ' + str(totalDuration) + 'ms', fontfamily="monospace",fontweight='bold')   

        
        fig.add_axes(ax)
        #plt.show()
        figPath = pathlib.Path(target+'/'+self._errorTimesPlot)
        plt.savefig(str(figPath), dpi=300, format='png', bbox_inches='tight', orientation='landscape') # use format='svg' or 'pdf' for vectorial pictures

    def _countTopStackOccurences(self, target):
        topStackOccurences = dict()
        errors = self._reportRoot.findall('error')
        
        for error in errors:
            stacks = error.findall('stack')
            for stack in stacks:
                topFrame = stack.find('frame') #returns first element of with frame tag
                
                if(topFrame != None):
                    tmp1 = topFrame.find('file').text
                    tmp2 = topFrame.find('fn').text

                    if(tmp1 != None and tmp2 != None):
                        if(len(tmp2) > 20): #split function name in half if it is too long
                            tmp2 = tmp2[:len(tmp2)//2] + '\n' + tmp2[len(tmp2)//2:]
                        identifier = tmp1 + ":\n" + tmp2

                        if topStackOccurences.get(identifier) != None:
                            value = topStackOccurences.pop(identifier)
                            topStackOccurences.update({identifier: int(value)+1})
                        else:
                            topStackOccurences.update({identifier: 1})

        
        #sort dict
        sortedOccurences = sorted(topStackOccurences.items(), key=lambda kv: kv[1])
        x=list()
        y=list()
        for ele in reversed(sortedOccurences[-5:]): #append the 5 largest values in decending order
            x.append(ele[0]) #x values (basically the function names)
            y.append(ele[1]) #y values occurrences (bar height)

        #make plot       
        fig = plt.figure(figsize=(10,4))    
        ax = plt.axes() 
        barWidth = 0.9 # the width of the bars 
        xLoc = list(range(len(y)))  # the x locations for the groups
        ax.bar([loc for loc in xLoc], y, barWidth, color='#009999')
        ax.set_xticks([loc for loc in xLoc])
        ax.set_xticklabels(x, minor=False)
        
        plt.title('Top five functions by top of stack occurrences',fontfamily="monospace", fontweight='bold')
        plt.xlabel('Function Name', fontfamily="monospace",fontweight='bold')
        plt.ylabel('No. of top of stack occurrences', fontfamily="monospace",fontweight='bold')   

        for i,v in enumerate(y):
            ax.text(i,  v, str(v), ha='center',color='black', fontweight='bold')
        
        
        fig.add_axes(ax)
        #plt.show()
        figPath = pathlib.Path(target+'/'+self._topStackGraphFileName)
        plt.savefig(str(figPath), dpi=300, format='png', bbox_inches='tight', orientation='landscape') # use format='svg' or 'pdf' for vectorial pictures
       
    def _createErrorList(self):
        self._strErrors = str()
        
        errorTemplate = self._htmlTemplates.find('error_entry').text
        errorList = self._reportRoot.findall('error')
        self._numberOfErrors = len(errorList)
        
        for error in errorList:
            
            outputID = "output_"+str(self._errorNumber)+"_"
            newError = errorTemplate.replace('*ERROR_ID*', adjText(error.find('unique').text))
            newError = newError.replace('*ERROR_TYPE*', adjText(error.find('kind').text))
            
            xwhat = error.findall('xwhat')
            newError = newError.replace('*XWHAT_TEXT_1*', adjText(xwhat[0].find('text').text))
            newError = newError.replace('*XWHAT_TEXT_2*', adjText(xwhat[1].find('text').text))

            self._errorHeading = str() #reset errorHeading, will be filled filled by _createCallStack
            newError = newError.replace('*CALL_STACK_ENTRIES_1*', self._createCallStack(error, 0, outputID))
            newError = newError.replace('*CALL_STACK_ENTRIES_2*', self._createCallStack(error, 1, outputID))
            newError = newError.replace('*OUTPUT_ID_1*', outputID+'0')
            newError = newError.replace('*OUTPUT_ID_2*', outputID+'1')
            newError = newError.replace('*ERROR_HEADING*', self._errorHeading)

            self._errorNumber += 1
            self._strErrors += newError

    def _createHeader(self):
        headerInformation = self._getHeader()
        self.htmlReport = self._htmlTemplates.find('base_entry').text
        self.htmlReport = self.htmlReport.replace('*DATE*', headerInformation[0])
        self.htmlReport = self.htmlReport.replace('*TIME*', headerInformation[1])
        self.htmlReport = self.htmlReport.replace('*DURATION*', headerInformation[2])
        self.htmlReport = self.htmlReport.replace('*DURATION_UNIT*', headerInformation[3])
        self.htmlReport = self.htmlReport.replace('*ARGS*', headerInformation[4])
        self.htmlReport = self.htmlReport.replace('*EXE*', headerInformation[5])
        self.htmlReport = self.htmlReport.replace('*PROTOCOLVERSION*', headerInformation[6])
        self.htmlReport = self.htmlReport.replace('*PROTOCOLTOOL*', headerInformation[7])
        self.htmlReport = self.htmlReport.replace('*NUMBER_OF_ERRORS*', str(self._numberOfErrors))
        self.htmlReport = self.htmlReport.replace('*ERROR_ENTRIES*', self._strErrors)
        if not noMatplotLib:
            self.htmlReport = self.htmlReport.replace('*TOP_OF_STACK_GRAPH*', self._topStackGraphFileName)
            self.htmlReport = self.htmlReport.replace('*ERROR_TIMES_PLOT*', self._errorTimesPlot)
        else:
            self.htmlReport = self.htmlReport.replace('*TOP_OF_STACK_GRAPH*', '')

    def _createReport(self):
        self._createErrorList()
        self._createHeader()
        self.htmlReport = self.htmlReport.replace("*SNIPPET_VARIABLES*", self._snippets)
        self.htmlReport = self.SCM.createCodeVars(self.htmlReport)


class SourceCodeManagement:
    _htmlTemplatesPath = str(g_HTMLTEMPLATES)
    _htmlTemplates = (ET.parse(_htmlTemplatesPath)).getroot()

    def __init__(self):
        self._sourcefilelist = list()

    def _createSourcefileEntry(self, path, line):
        #one entry consists of the full file path the line number of interest 
        sourceFile = open(path, mode='r')
        sourceLineList = sourceFile.readlines()
        if len(sourceLineList) > NUMBEROFCODELINES:
            newElement = [path, int(line), False]
        else:
            newElement = [path, int(line), True]
        
        self._sourcefilelist.append(newElement)
        return self._sourcefilelist.index(newElement)

        
    def _returnCode(self, fullPath, justExistance, line = 0):
        returnSrc = False
        try: #may throw an an exception in earlier version (until 3.6), therefore try-catch 
            fp = pathlib.Path(fullPath).resolve() #returns absolute path
        except FileNotFoundError:
            return -1

        if fp.is_file():
            for element in SOURCEFILE_BL: #blacklisting routine
                if str(element) in str(fp): #both are absoulte paths, so comparing is valid
                    return -1
                
            if WHITELISTING:
                for element in SOURCEFILE_WL:
                    if str(element) in str(fullPath):
                        returnSrc = True
                        break
                if not returnSrc:
                    return -1
            if justExistance:
                return 0
        else:
            return -1
        
        #if we are we want to return the source code
        return adjText(self._getLines(fullPath, line))


    def _getLines(self, path, line):
        sourceFile = open(path, mode='r')
        sourceLineList = sourceFile.readlines()

        if len(sourceLineList) > NUMBEROFCODELINES:
            if line <= NUMBEROFCODELINES//2:
                begin = 0
                end = NUMBEROFCODELINES
            else:
                begin = (line - NUMBEROFCODELINES//2) - 1 #-1 because array starts with 0
                end = begin + NUMBEROFCODELINES

            sourceLineList = sourceLineList[begin:end]
            
        sourceCode = str()
        for sourceLine in sourceLineList:
            sourceCode += sourceLine

        sourceFile.close()
        return sourceCode


    def handleSourceCode(self, filename, directory, line):
        fullPath = pathlib.Path(directory +'/'+ filename)
        
        src = self._returnCode(fullPath, justExistance=1)
        if src == -1:
            return -1, self._htmlTemplates.find('no_code_entry').text

        index = -1
        #check if source file is already in the list
        for item in self._sourcefilelist:
            if item[0] == fullPath:
                if item[2] or (int(line) - NUMBEROFCODELINES//10) <= item[1] <= (int(line) + NUMBEROFCODELINES//10): 
                    index = self._sourcefilelist.index(item)
                    #entry = item

        if index == -1:
            index = self._createSourcefileEntry(fullPath, line)
            
        strIndex = 'code_' + str(index) 
        return strIndex, (self._htmlTemplates.find('code_entry').text)

    def createCodeVars(self, report):
        codeString = str()

        for sourceObject in self._sourcefilelist:
            src = self._returnCode(sourceObject[0], justExistance=0, line = sourceObject[1])
            tmpCode = "code_" + str(self._sourcefilelist.index(sourceObject)) + ' = `' + src + '`;\n'
            codeString += tmpCode

        report = report.replace("*CODE_VARIABLES*", codeString)
        return report

    def determineLanguage(self, filename):
        fileParts = filename.split('.')
        if len(fileParts) == 1:
            return 'cpp' #files without file endigs are treated as cpp files
        else:
            ending = fileParts[-1]
            if ending == 'c':
                return 'c'
            elif ending == 'cpp':
                return 'cpp'
            elif ending == 'h':
                return 'cpp'
            elif ending == 'cs':
                return 'csharp'
            elif ending == 'css':
                return 'css'
            elif ending == 'js':
                return 'javascript'
            elif ending == 'html':
                return 'markup'
            else:
                return 'cpp'     

    def getFirstLineOfCodeSnippet(self, index):
        codeSnippet = int(index.split("_")[-1]) #index is e.g. code_3
        srcObject = self._sourcefilelist[codeSnippet]

        if srcObject[2]:
            return 1
        else:
            firstLine = srcObject[1] - NUMBEROFCODELINES//2
            return  firstLine #srcObject[1] is line of interest of snippet


def adjText(text): #change html symbols e.g. & -> &amp;
    text = text.replace('&', '&amp;')
    text = text.replace('<', '&lt;')
    text = text.replace('>', '&gt;')
    text = text.replace('"', '&quot;')
    text = text.replace('\\', '/')
    return text

def parseArgumentString(fileList, strEntries):
    strEntries = strEntries.replace("\\","/")
    listEntries = strEntries.split(',')
    for entry in listEntries: 
        #remove potential leading and trailing whitespaces
        while entry[0] == ' ':
            entry = entry[1:]
        while entry[-1] == ' ':
            entry = entry[:-1]

        newObject = pathlib.Path(entry)
        newObject = newObject.resolve()
        fileList.append(newObject)   

    return      

def returnDateString():
    date = datetime.datetime.now()
    return date.strftime('%Y%m%d_%H%M')

def main():
    global SOURCEFILE_BL, SOURCEFILE_WL, WHITELISTING, DEBUG
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--inputFile", help='define <input_file>', type=str)
    parser.add_argument("-o", "--outputDirectory", help='define <output_directory>', type=str)
    parser.add_argument("-b", "--blacklist", help='add blacklist entries <entry1,entry2 ...>', type=str)
    parser.add_argument("-w", "--whitelist", help='add whitelist entries entries <entry1,entry2 ...>', type=str)
    parser.add_argument("--Debug", help='Debug Mode', action="store_true")
    args = parser.parse_args()
    
    ###args handling
    if args.Debug:
        print("Debug Mode is on")
        inFile = pathlib.Path(SCRIPTPATH / 'test_files/test.xml')    
        targetDirectory = pathlib.Path(SCRIPTPATH / 'test_files/output')    

    else:
        if args.inputFile != None:
            inFile = pathlib.Path(args.inputFile)
        else:
            print("You must specify an input file")
            print()
            parser.print_help()
            exit(-1)

        if not inFile.is_file():
            print("Your input file does not exist")
            parser.print_help()
            exit(-1)

    strDate = returnDateString()
    if args.outputDirectory != None:
        targetDirectory = pathlib.Path(args.outputDirectory+'/draceGUI_output_'+strDate)
    else:
        targetDirectory = pathlib.Path('./draceGUI_output_'+strDate)

    if args.blacklist != None:
        parseArgumentString(SOURCEFILE_BL, args.blacklist)
        
    if args.whitelist != None:
        parseArgumentString(SOURCEFILE_WL, args.whitelist)
        WHITELISTING = True
    #end of args handling

    
    if not targetDirectory.is_dir():
        targetDirectory.mkdir()

    #report gets generated here
    report = ReportCreator(str(inFile), str(targetDirectory))

    if report.succesfullReportCreation:

        #write report to destination
        output = open(str(targetDirectory)+'/index.html', mode='w')
        output.write(report.htmlReport)
        output.close()

        #copy needed files to destination
        cssPath = pathlib.Path(str(targetDirectory)+"/css")
        jsPath  = pathlib.Path(str(targetDirectory)+"/js")

        if cssPath.is_dir():
            shutil.rmtree(str(cssPath))
            
        if jsPath.is_dir():
            shutil.rmtree(str(jsPath))

        shutil.copytree(str(g_CSSPATH.resolve()), str(targetDirectory / "css"))
        shutil.copytree(str(g_JSPATH.resolve()), str(targetDirectory / "js"))
        shutil.copy(str((resourcesPath / 'legend.png').resolve()), str(targetDirectory))
        print("Report creation successfull")
        print("Report is at:")
        print(targetDirectory)
        return 0

    else:
        print("Report creation was NOT successfull")
        targetDirectory.rmdir()
        return -1


if __name__ == "__main__":
    main()
