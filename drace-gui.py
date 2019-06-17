import xml.etree.ElementTree as ET
import os
import shutil

copy_file = True
if not copy_file:
    copy_text = True
else:
    copy_text = False

class ReportCreator:
    __errorTag  = 'error'
    __htmlTemplatesPath = "templates/entries.xml"
    __htmlTemplates = (ET.parse(__htmlTemplatesPath)).getroot()
    sourcefileList = list()


    def __init__(self, pathOfReport):
        self.__pathOfReport = pathOfReport
        self.__reportContent = ET.parse(self.__pathOfReport)
        self.__reportRoot = self.__reportContent.getroot()
        self.__createReport()

    def __getHeader(self):
        header = list()
        status = self.__reportRoot.findall('status')[1]
        strDatetime = status.find('time').text
        date = strDatetime.split('T')[0]
        time = (strDatetime.split('T')[1])[0:-1] #last digit is 'Z' -> not needed
        header.append(self.adjText(date))
        header.append(self.adjText(time))
        header.append(self.adjText(status.find('duration').text))

        arguments = str()
        for arg in self.__reportRoot.find('args').find('vargv').findall('arg'):
            arguments += arg.text
            arguments += ' '
        header.append(self.adjText(arguments[0:-1])) #remove last ' '
        
        header.append(self.adjText(self.__reportRoot.find('args').find('argv').find('exe').text))
        header.append(self.adjText(self.__reportRoot.find('protocolversion').text))
        header.append(self.adjText(self.__reportRoot.find('protocoltool').text))

        return header

    def __returnCode(self, filename, directory):
        fullPath = directory +'/'+ filename
        fullPath = fullPath.replace('\\', '/')
        
        if os.path.isfile(fullPath):
            if copy_file:
                sourceCode = (self.__htmlTemplates.find('code_entry_reference').text).replace('*SOURCEFILE*', filename)
                self.sourcefileList.append(fullPath)
                return sourceCode

            
            if copy_text:
                sourceFile = open(fullPath, mode='r')
                sourceCode = sourceFile.read()
                sourceFile.close()
                sourceCode = (self.__htmlTemplates.find('code_entry').text).replace('*CODE*', self.adjText(sourceCode))
                return sourceCode

        else:
            return self.__htmlTemplates.find('no_code_entry').text

    def __createCallStack(self, errorEntry, position):
        callStack = str()
        stackTemplate = self.__htmlTemplates.find('stack_entry').text
        stackArray = errorEntry.findall('stack')
        stack = stackArray[position]

        elementNumber = 0
        for frame in stack.findall('frame'):
            newStackElement = stackTemplate.replace('*STACK_NUMBER*', self.adjText(hex(elementNumber)))
            newStackElement = newStackElement.replace('*OBJ*', self.adjText(frame.find('obj').text))
            newStackElement = newStackElement.replace('*FUNCTION*', self.adjText(frame.find('fn').text))
            newStackElement = newStackElement.replace('*INSTRUCTION_POINTER*', self.adjText(frame.find('ip').text))

            if (frame.find('file')!= None):
                newStackElement = newStackElement.replace('*FILE*', self.adjText(frame.find('file').text))
                newStackElement = newStackElement.replace('*DIRECTORY*', self.adjText(frame.find('dir').text))
                newStackElement = newStackElement.replace('*CODE_TAG*', self.__returnCode(frame.find('file').text, frame.find('dir').text))
                newStackElement = newStackElement.replace('*LINE_OF_CODE*', self.adjText(frame.find('line').text))
            else:
                newStackElement = newStackElement.replace('*FILE*', 'no filename available')
                newStackElement = newStackElement.replace('*DIRECTORY*', 'no directory available')
                newStackElement = newStackElement.replace('*CODE_TAG*', self.__returnCode('',''))
                newStackElement = newStackElement.replace('*LINE_OF_CODE*', 'no line of code available')
            
            

            callStack += newStackElement
            elementNumber += 1

        return callStack



    def __createErrorList(self):
        self.__strErrors = str()
        errorTemplate = self.__htmlTemplates.find('error_entry').text
        errorList = self.__reportRoot.findall(self.__errorTag)
        self.__numberOfErrors = len(errorList)
        
        for error in errorList:
            newError = errorTemplate.replace('*ERROR_ID*', self.adjText(error.find('unique').text))
            newError = newError.replace('*ERROR_TYPE*', self.adjText(error.find('kind').text))
            
            xwhat = error.findall('xwhat')
            newError = newError.replace('*XWHAT_TEXT_1*', self.adjText(xwhat[0].find('text').text))
            newError = newError.replace('*XWHAT_TEXT_2*', self.adjText(xwhat[1].find('text').text))

            newError = newError.replace('*CALL_STACK_ENTRIES_1*', self.__createCallStack(error, 0))
            newError = newError.replace('*CALL_STACK_ENTRIES_2*', self.__createCallStack(error, 1))
            
            self.__strErrors += newError

    def __createHeader(self):
        headerInformation = self.__getHeader()
        self.htmlReport = self.__htmlTemplates.find('base_entry').text
        self.htmlReport = self.htmlReport.replace('*DATE*', headerInformation[0])
        self.htmlReport = self.htmlReport.replace('*TIME*', headerInformation[1])
        self.htmlReport = self.htmlReport.replace('*DURATION*', headerInformation[2])
        self.htmlReport = self.htmlReport.replace('*ARGS*', headerInformation[3])
        self.htmlReport = self.htmlReport.replace('*EXE*', headerInformation[4])
        self.htmlReport = self.htmlReport.replace('*PROTOCOLVERSION*', headerInformation[5])
        self.htmlReport = self.htmlReport.replace('*PROTOCOLTOOL*', headerInformation[6])
        self.htmlReport = self.htmlReport.replace('*NUMBER_OF_ERRORS*', str(self.__numberOfErrors))
        self.htmlReport = self.htmlReport.replace('*ERROR_ENTRIES*', self.__strErrors)

    def __createReport(self):
        self.__createErrorList()
        self.__createHeader()


    def adjText(self, text):
        #text = text.replace('<', '&lt;')
        #text = text.replace('>', '&gr;')
        #text = text.replace('&', '&amp;')
        return text
        

def main():
    targetDirectory = 'test_files/output'

    report = ReportCreator('test_files/test.xml')

    if not os.path.isdir(targetDirectory):
        os.mkdir(targetDirectory)
    
    output = open(targetDirectory+'/output.html', mode='w')
    output.write(report.htmlReport)
    output.close()

    for path in report.sourcefileList:
        shutil.copy(path, targetDirectory)
    
    #shutil.copy("templates/css", targetDirectory)
    #shutil.copy("templates/js", targetDirectory)

    return 0

if __name__ == "__main__":
    main()