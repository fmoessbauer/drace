#  /*
#   * DRace-GUI: A graphical report generator for DRace
#   *
#   * Copyright 2019 Siemens AG
#   *
#   * Authors:
#   *   <Philip Harr> <philip.harr@siemens.com>
#   *
#   * SPDX-License-Identifier: MIT
# */
import unittest
import pathlib
import draceGUI as gui

STRREPORTPATH = './test_files/output/index.html'

class TestMethods(unittest.TestCase):
    def testFilesAvaliable(self):
        paths = list()
        paths.append(pathlib.Path(STRREPORTPATH))
        paths.append(pathlib.Path('./test_files/output/legend.png'))
        paths.append(pathlib.Path('./test_files/output/js/jquery.min.js'))
        paths.append(pathlib.Path('./test_files/output/js/prism.js'))
        paths.append(pathlib.Path('./test_files/output/js/materialize.min.js'))
        paths.append(pathlib.Path('./test_files/output/css/style.css'))
        paths.append(pathlib.Path('./test_files/output/css/prism.css'))
        paths.append(pathlib.Path('./test_files/output/css/materialize.min.css'))
        
        try:
            import matplotlib
            paths.append(pathlib.Path('./test_files/output/topStackBarchart.png'))
            paths.append(pathlib.Path('./test_files/output/errorTimes.png'))
        except ImportError:
            pass

        flag = 0
        for element in paths:
            if not element.is_file():
                print(str(element) + ' is not available')
                flag = 1

        self.assertEqual(flag, 0)

#checj if determine language works
    def testDetermineLanguage(self):
        languageMap = {
            'test.h': 'cpp',
            'test.cpp':'cpp',
            'test.c':'c',
            'test.cs':'csharp',
            'test.css':'css',
            'test.html':'markup',
            'test.js':'javascript',
            'test':'cpp'
        }
        testObject = gui.SourceCodeManagement()
        for key, value in languageMap.items():
            self.assertEqual(value, testObject.determineLanguage(key))


# check if all placeholders were removed
    def testPlaceholdersGone(self):
        
        with open('test_files/placeholders.txt', 'r') as placeholderFile:
            allLines = placeholderFile.readlines()

        #retrieve all placeholders
        lines = list()
        start = False
        for line in allLines:
            line = line.replace('\n','')
            line = line.replace(' ', '')
            if start and line != '':
                if line[0]=='*':
                    lines.append(line)
            if line == 'Placeholders:':
                start = True

        with open(STRREPORTPATH, 'r') as html:
            strReport = html.read()
        
        for tag in lines:
            if tag in strReport:
                self.fail()

        self.assertEqual(1,1)



if __name__ == '__main__':
    unittest.main()
