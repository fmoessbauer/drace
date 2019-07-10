import unittest
import pathlib


class TestMethods(unittest.TestCase):
    def test_files_avaliable(self):
        paths = list()
        paths.append(pathlib.Path('./test_files/output/index.html'))
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


if __name__ == '__main__':
    unittest.main()
