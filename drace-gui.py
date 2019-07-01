import xml.etree.ElementTree as ET
import os
import shutil
import argparse
import matplotlib
import matplotlib.pyplot as plt

#Paths
g_HTMLTEMPLATES = "templates/entries.xml"
g_CSSPATH = "templates/css"
g_JSPATH = "templates/js"
DEBUG = True

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
    __htmlTemplatesPath = g_HTMLTEMPLATES
    __htmlTemplates = (ET.parse(__htmlTemplatesPath)).getroot()
    
    def __init__(self, pathOfReport, target):
        self.sourcefileList = list()
        self.__callStackNumber = 0
        self.__errorNumber = 0
        self.__snippets = str()
        self.succesfullReportCreation = True
        self.SCM = SourceCodeManagement()

        self.__pathOfReport = pathOfReport
        if self.__inputValidation():
            self.__createReport()
            self.__countTopStackOccurences(target)
        else:
            print("input file is not valid")
            self.succesfullReportCreation = False

    def __inputValidation(self):
        try:
            self.__reportContent = ET.parse(self.__pathOfReport)
            self.__reportRoot = self.__reportContent.getroot()
        except:
            return 0

        if self.__reportRoot.find('protocolversion') != None and \
            self.__reportRoot.find('protocoltool') != None and \
            self.__reportRoot.find('preamble') != None and \
            self.__reportRoot.find('pid') != None and \
            self.__reportRoot.find('tool') != None and \
            self.__reportRoot.find('args') != None and \
            self.__reportRoot.find('status') != None and \
            self.__reportRoot.tag == 'valgrindoutput':

            return 1
        else:
            return 0

    def __getHeader(self):
        header = list()
        status = self.__reportRoot.findall('status')[1]
        strDatetime = status.find('time').text
        date = strDatetime.split('T')[0]
        time = (strDatetime.split('T')[1])[0:-1] #last digit is 'Z' -> not needed
        header.append(adjText(date))
        header.append(adjText(time))
        header.append(adjText(status.find('duration').text))
        header.append(adjText(status.find('duration').get('unit')))

        arguments = str()
        for arg in self.__reportRoot.find('args').find('vargv').findall('arg'):
            arguments += arg.text
            arguments += ' '
        header.append(adjText(arguments[0:-1])) #remove last ' '
        
        header.append(adjText(self.__reportRoot.find('args').find('argv').find('exe').text))
        header.append(adjText(self.__reportRoot.find('protocolversion').text))
        header.append(adjText(self.__reportRoot.find('protocoltool').text))

        return header

    def __createSnippetEntry(self, frame, elementNumber, tag, codeIndex):
        newSnippet = self.__htmlTemplates.find('snippet_entry').text

        newSnippet = newSnippet.replace('*SNIPPET_VAR*', ("snippet_" + str(self.__callStackNumber)))
        newSnippet = newSnippet.replace('*STACK_NUMBER*', adjText(hex(elementNumber)))
        newSnippet = newSnippet.replace('*OBJ*', adjText(frame.find('obj').text))
        newSnippet = newSnippet.replace('*FUNCTION*', adjText(frame.find('fn').text))
        newSnippet = newSnippet.replace('*INSTRUCTION_POINTER*', adjText(frame.find('ip').text))
        newSnippet = newSnippet.replace('*CODE_TAG*', tag)

        if (frame.find('file')!= None):
            newSnippet = newSnippet.replace('*FILE*', adjText(frame.find('file').text))
            newSnippet = newSnippet.replace('*DIRECTORY*', adjText(frame.find('dir').text))
            newSnippet = newSnippet.replace('*LINE_OF_CODE*', adjText(frame.find('line').text))
            if(codeIndex != -1):
                newSnippet = newSnippet.replace('*CODE_ID_VAR*', "snippet_"+str(self.__callStackNumber)+"_code")
                newSnippet = newSnippet.replace('*LANGUAGE*', self.SCM.determineLanguage(adjText(frame.find('file').text)))
                newSnippet = newSnippet.replace('*FIRST_LINE*', str(self.SCM.getFirstLineOfCodeSnippet(codeIndex)))
            else:
                newSnippet = newSnippet.replace('*CODE_ID_VAR*',  "'None'")

        else:
            newSnippet = newSnippet.replace('*FILE*', 'no filename available')
            newSnippet = newSnippet.replace('*DIRECTORY*', 'no directory available')
            newSnippet = newSnippet.replace('*LINE_OF_CODE*', 'no line of code available')

        self.__snippets += newSnippet #append referenced code snippet

    def __createCallStack(self, errorEntry, position, outputID):
        
        callStack = str()
        stackTemplate = self.__htmlTemplates.find('stack_entry').text
        stackArray = errorEntry.findall('stack')
        stack = stackArray[position]
        elementNumber = 0

        for frame in stack.findall('frame'):
            noPreview = False    
            ###make heading for the red box### 
            if elementNumber == 0:
                if len(self.__errorHeading) == 0:
                    self.__errorHeading += "<br> Obj. 1: " + (adjText(frame.find('obj').text) + ': "' + adjText(frame.find('fn').text)) + '" <br> '
                else:
                    self.__errorHeading += "Obj. 2: " + (adjText(frame.find('obj').text) + ': "' + adjText(frame.find('fn').text)) + '"'

            #general entries (always available)
            newStackElement = stackTemplate.replace('*STACK_NUMBER*', adjText(hex(elementNumber))+":")
            newStackElement = newStackElement.replace('*SNIPPET_VAR*', ("snippet_" + str(self.__callStackNumber)))
            newStackElement = newStackElement.replace('*OUTPUT_ID*', outputID+str(position))
            newStackElement = newStackElement.replace('*FUNCTION*', adjText(frame.find('fn').text))
            

            if (frame.find('file')!= None): #file is in xml report defined
                codeIndex, tag = self.SCM.handleSourceCode(frame.find('file').text, frame.find('dir').text, frame.find('line').text)
                newStackElement = newStackElement.replace('*FILE*', adjText(frame.find('file').text))

                if(codeIndex != -1):
                    newStackElement = newStackElement.replace('*CODE_VAR*', str(codeIndex))
                    newStackElement = newStackElement.replace('*CODE_ID_VAR*', "'snippet_"+str(self.__callStackNumber)+"_code'")
                    newStackElement = newStackElement.replace('*LINE_OF_CODE*', adjText(frame.find('line').text))
                    newStackElement = newStackElement.replace('*FIRST_LINE*', str(self.SCM.getFirstLineOfCodeSnippet(codeIndex)))  
                
                else: #file is not available on device or file is blacklisted or not whitelisted
                    noPreview = True
                     
                
            else: #no filepath for file in xml is given 
                codeIndex, tag = self.SCM.handleSourceCode("","", "")
                newStackElement = newStackElement.replace('*FILE*', 'no filename available')
                noPreview = True

            if noPreview:
                newStackElement = newStackElement.replace('*CODE_VAR*', "'None'")
                newStackElement = newStackElement.replace('*CODE_ID_VAR*',  "'None'")
                newStackElement = newStackElement.replace('*LINE_OF_CODE*', "'None'")
                newStackElement = newStackElement.replace('*FIRST_LINE*', "'NONE'") 
                insertPosition = newStackElement.find('btn"') 
                newStackElement = newStackElement[:insertPosition] + "grey " + newStackElement[insertPosition:] 

            
            self.__createSnippetEntry(frame, elementNumber, tag, codeIndex)
            callStack += newStackElement #append stack element
            elementNumber += 1
            self.__callStackNumber += 1 #increase global call stack number (used for reference variables)

        return callStack

    def __countTopStackOccurences(self,target):
        topStackOccurences = dict()
        errors = self.__reportRoot.findall('error')
        for error in errors:
            stacks = error.findall('stack')
            for stack in stacks:
                topFrame = stack.find('frame') #returns first element of type frame
                tmp1 = topFrame.find('file').text
                tmp2 = topFrame.find('fn').text
                identifier = tmp1 + ":\n" + tmp2

                if topStackOccurences.get(identifier) != None:
                    value = topStackOccurences.pop(identifier)
                    topStackOccurences.update({identifier: int(value)+1})
                else:
                    topStackOccurences.update({identifier: 1})
        #sort dict
        sortedOccurences = sorted(topStackOccurences.items(), key=lambda kv: kv[1])


        #make plot
        x=list()
        y=list()
        for ele in reversed(sortedOccurences[-5:]):
            x.append(ele[0])
            y.append(ele[1])


        #matplotlib.rcParams['font.size']=5
        fig = plt.figure(figsize=(20,6))    
        ax = plt.axes() 
        width = 1 # the width of the bars 
        ind = list(range(len(y)))  # the x locations for the groups
        ax.bar([1.1*ele for ele in ind], y, width, color='rgbkymc')
        ax.set_xticks([1.1*ele for ele in ind])
        ax.set_xticklabels(x, minor=False)
        
        #plt.xticks(rotation=90)
        plt.title('Top five functions by top of stack occurences')
        plt.xlabel('Function Name')
        plt.ylabel('No of top of stack occurences')      
        #plt.show()
        fig.add_axes(ax)
        plt.savefig(os.path.join(target+'/barchart.png'), dpi=300, format='png', bbox_inches='tight', orientation='landscape') # use format='svg' or 'pdf' for vectorial pictures
       

    def __createErrorList(self):
        self.__strErrors = str()
        
        errorTemplate = self.__htmlTemplates.find('error_entry').text
        errorList = self.__reportRoot.findall('error')
        self.__numberOfErrors = len(errorList)
        
        for error in errorList:
            self.__errorNumber += 1
            outputID = "output_"+str(self.__errorNumber)+"_"
            newError = errorTemplate.replace('*ERROR_ID*', adjText(error.find('unique').text))
            newError = newError.replace('*ERROR_TYPE*', adjText(error.find('kind').text))
            
            xwhat = error.findall('xwhat')
            newError = newError.replace('*XWHAT_TEXT_1*', adjText(xwhat[0].find('text').text))
            newError = newError.replace('*XWHAT_TEXT_2*', adjText(xwhat[1].find('text').text))

            self.__errorHeading = str() #reset errorHeading, will be filled filled by __createCallStack
            newError = newError.replace('*CALL_STACK_ENTRIES_1*', self.__createCallStack(error, 0, outputID))
            newError = newError.replace('*CALL_STACK_ENTRIES_2*', self.__createCallStack(error, 1, outputID))
            newError = newError.replace('*OUTPUT_ID_1*', outputID+'0')
            newError = newError.replace('*OUTPUT_ID_2*', outputID+'1')
            newError = newError.replace('*ERROR_HEADING*', self.__errorHeading)

            self.__strErrors += newError

    def __createHeader(self):
        headerInformation = self.__getHeader()
        self.htmlReport = self.__htmlTemplates.find('base_entry').text
        self.htmlReport = self.htmlReport.replace('*DATE*', headerInformation[0])
        self.htmlReport = self.htmlReport.replace('*TIME*', headerInformation[1])
        self.htmlReport = self.htmlReport.replace('*DURATION*', headerInformation[2])
        self.htmlReport = self.htmlReport.replace('*DURATION_UNIT*', headerInformation[3])
        self.htmlReport = self.htmlReport.replace('*ARGS*', headerInformation[4])
        self.htmlReport = self.htmlReport.replace('*EXE*', headerInformation[5])
        self.htmlReport = self.htmlReport.replace('*PROTOCOLVERSION*', headerInformation[6])
        self.htmlReport = self.htmlReport.replace('*PROTOCOLTOOL*', headerInformation[7])
        self.htmlReport = self.htmlReport.replace('*NUMBER_OF_ERRORS*', str(self.__numberOfErrors))
        self.htmlReport = self.htmlReport.replace('*ERROR_ENTRIES*', self.__strErrors)

    def __createReport(self):
        self.__createErrorList()
        self.__createHeader()
        self.htmlReport = self.htmlReport.replace("*SNIPPET_VARIABLES*", self.__snippets)
        self.htmlReport = self.SCM.createCodeVars(self.htmlReport)


class SourceCodeManagement:
    __htmlTemplatesPath = g_HTMLTEMPLATES
    __htmlTemplates = (ET.parse(__htmlTemplatesPath)).getroot()

    def __init__(self):
        self.__sourcefilelist = list()

    def __createSourcefileEntry(self, path, line):
        #one entry consists of the full file path the line number of interest and wether the complete file is copied in the html report or not
        src = open(path, mode='r')
        sourceCode = src.readlines()
        src.close()
        if len(sourceCode) <= NUMBEROFCODELINES:
            newElement = [path, int(line), True]
        else:
            newElement = [path, int(line), False]

        self.__sourcefilelist.append(newElement)
        return self.__sourcefilelist.index(newElement)
        
    def __returnCode(self, fullPath, justExistance, line = 0, completeFileFlag = False):
        returnSrc = False
        
        if os.path.isfile(fullPath):
            for element in SOURCEFILE_BL: #blacklisting routine
                if element in fullPath:
                    return -1
                
            if WHITELISTING:
                for element in SOURCEFILE_WL:
                    if element in fullPath:
                        returnSrc = True
                        break
                if not returnSrc:
                    return -1
            if justExistance:
                return 0
        else:
            return -1

        #return sourcecode
        sourceFile = open(fullPath, mode='r')
        if completeFileFlag:
            sourceCode = sourceFile.read()
            sourceCode = sourceCode

        else:
            sourceLineList = sourceFile.readlines()
            if line <= NUMBEROFCODELINES//2:
                begin = 0
                end = NUMBEROFCODELINES
            else:
                begin = (line - NUMBEROFCODELINES//2) - 1
                end = begin + NUMBEROFCODELINES

            listOfInterest = sourceLineList[begin:end]
            sourceCode = str()
            for sourceLine in listOfInterest:
                sourceCode += sourceLine

        sourceFile.close()
        return adjText(sourceCode)

    def handleSourceCode(self, filename, directory, line):
        fullPath = directory +'/'+ filename
        fullPath = fullPath.replace('\\', '/')
        
        src = self.__returnCode(fullPath, justExistance=1)
        if src == -1:
            return -1, self.__htmlTemplates.find('no_code_entry').text

        index = -1
        #entry = list()

        for item in self.__sourcefilelist:
            if item[0] == fullPath:
                if item[2] or (int(line) - NUMBEROFCODELINES//10) <= item[1] <= (int(line) + NUMBEROFCODELINES//10): 
                    index = self.__sourcefilelist.index(item)
                    #entry = item

        if index == -1:
            index = self.__createSourcefileEntry(fullPath, line)
            
        strIndex = 'code_' + str(index) 
        return strIndex, (self.__htmlTemplates.find('code_entry').text)

    def createCodeVars(self, report):
        codeString = str()

        for sourceObject in self.__sourcefilelist:
            src = self.__returnCode(sourceObject[0], justExistance=0, line = sourceObject[1], completeFileFlag = sourceObject[2])
            tmpCode = "code_" + str(self.__sourcefilelist.index(sourceObject)) + ' = `' + src + '`;\n'
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
        srcObject = self.__sourcefilelist[codeSnippet]
        if srcObject[2]:
            return 1
        else:
            return srcObject[1] - NUMBEROFCODELINES//2


def adjText(text): #change html symbols e.g. & -> &amp;
    text = text.replace('&', '&amp;')
    text = text.replace('<', '&lt;')
    text = text.replace('>', '&gt;')
    text = text.replace('"', '&quot;')
    text = text.replace('\\', '/')
    return text

def addListEntries(fileList, strEntries):
    strEntries = strEntries.replace("\\","/")
    listEntries = strEntries.split(',')
    for entry in listEntries: 
        #remove potential leading and trailing whitespaces
        while entry[0] == ' ':
            entry = entry[1:]
        while entry[-1] == ' ':
            entry = entry[:-1]
        
        fileList.append(entry)



def main():
    global SOURCEFILE_BL, SOURCEFILE_WL, WHITELISTING
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--inputFile", help='input file', type=str)
    parser.add_argument("-o", "--outputDirectory", help='output directory', type=str)
    parser.add_argument("-b", "--blacklist", help='add blacklist entries <entry1,entry2 ...>', type=str)
    parser.add_argument("-w", "--whitelist", help='add whitelist entries entries <entry1,entry2 ...>', type=str)
    args = parser.parse_args()
    
    ###args handling
    if args.inputFile != None:
        inFile = args.inputFile
    else:
        inFile = None

    if args.outputDirectory != None:
        targetDirectory = args.outputDirectory
    else:
        targetDirectory = './drace-gui_output'

    if args.blacklist != None:
        addListEntries(SOURCEFILE_BL, args.blacklist)
        
    if args.whitelist != None:
        addListEntries(SOURCEFILE_WL, args.whitelist)
        WHITELISTING = True
    #end of args handling


    if DEBUG:
        if inFile == None:
            inFile = 'test_files/test.xml'    
       
        targetDirectory = 'test_files/output'
        

    try:
        if not os.path.isfile(inFile):
            print("Your input file does not exist")
            exit(-1)
    except TypeError:
        print("You must specify an input file")
        exit(-1)
    

    report = ReportCreator(inFile, targetDirectory)

    if report.succesfullReportCreation:

        if not os.path.isdir(targetDirectory):
            os.mkdir(targetDirectory)

        #write report to destination
        output = open(targetDirectory+'/index.html', mode='w')
        output.write(report.htmlReport)
        output.close()

        #copy needed files to destination
        if  os.path.isdir(targetDirectory+"/css"):
            shutil.rmtree(targetDirectory+"/css")
        if  os.path.isdir(targetDirectory+"/js"):
            shutil.rmtree(targetDirectory+"/js")
        shutil.copytree(g_CSSPATH, targetDirectory+"/css")
        shutil.copytree(g_JSPATH, targetDirectory+"/js")
        return 0

    else:
        return -1


if __name__ == "__main__":
    main()