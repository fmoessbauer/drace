# How to use DRace and its tools

DRace is a data-race detector for windows applications which uses DynamoRIO to dynamically instrument a binary at runtime. This tutorial shall provide an overview on how to get and use DRace and the belonging tools.

This `HowTo.md` provides explanations about all parts of the usage of DRace. At the end, a [step by step tutorial](##Step-by-step-Tutorial) is provided, in which you can train your skills in using DRace.

## Get the tools

For the usage of DRace the following components are needed:

- DRace + Tools
- DynamoRio

### DRace + Tools

There are two ways to get DRace. You can download the prebuilt packages from
**[here](https://github.com/siemens/drace/releases)**. The zip-archive must be extracted and you are done.

Alternatively, **[this Git-repository](https://github.com/siemens/drace)**  can be cloned and DRace can be built by yourself.
Further information about how to build DRace by yourself are given in the global [README](https://github.com/siemens/drace/blob/master/README.md) of the repository.

### DynamoRio

DynamoRio is a dynamic instrumentation framework, which is used by DRace. A prebuilt zip-archive can be downloaded **[here](https://github.com/DynamoRIO/dynamorio/releases)**.

It is recommended to use the latest cron build. Once the download is finished, you must extract the zip-archive into a directory. To make the usage of DRace more convenient, it is recommended to put the path of the **drrun.exe**  into the Windows-PATH environment variable: drrun.exe is in ```./DynamoRIO-Windows-<version>/bin64```

## Usage

### GUI-Usage

For new users the most convenient way to use DRace is to use the DRaceGUI. The ```drace-gui.exe``` is contained in the ```./drace/bin``` folder. With the gui, the quiet long and unhandy DRace command can be build in an easier fashion.

Furthermore, a working configuration can be saved in a text file and restored at a later time.
Additionally, some plausability checks are exectued one the inputs. Correct and incorrect inputs are marked with green and red. The fields must be filled like described in the following.
Once all mandatory fields are filled correctly, one can directly execute the command in a powershell-instance by pressing **RUN**. Alternatively, the created command can be copied to the clipboard and pasted in an abitrary shell.

The following fields are mandatory:

- DynamoRIO - Path: Path to ```drrun.exe```
- DRace - Path: Path to the ```drace-client.dll```
- Select Detector: Select one of the available detectors (more information about detectors are [here](##Detectors))
- Configuration File: Path to ```drace.ini```
- Executable: Path to the application under test

The following fields are optional:

- Debug Mode: This will start DynamoRIO in the debug mode.
- Report: This option will create an HTML-Report after the analysis has finished. To set the option the report settings must be set correctly ([here](####Report-Settings))
- MSR: This option starts the managed code resolver, if one wants to analyse applications with .NET code.
- DRace Flags: here additional DRace Flags can be set. Be carefull, the string you type in is just copied to the command and not sanitized. Furthermore, use single quotes, when you need quotes. Availble DRace flags are [here](###Shell-Usage)

![1](./Images/dracegui_empty.png "Empty DRaceGUI")
![2](./Images/dracegui_filled.png "Filled DRaceGUI")

#### Report-Settings

![report_pic](./Images/report_settings.png)

A nice looking HTML-Report can be created by using the **[ReportConverter](##ReportConverter)**. The GUI extends the command such that the ReportConverter is used. Therefore, the path to the Python script `ReportConverter.py` or the `ReportConverter.exe` must be specified in "Report > Configure Report". Also a name for the xml report which is created by DRace must be specified (or just use the default value).

Note: Python 3 must be installed and in the Windows-PATH environment variable when using the python script.

More information about the ReportConverter is [here](##ReportConverter)

### Shell-Usage

**Run the detector as follows**

```bash
drrun.exe -c drace-client.dll <detector parameter> -- application.exe <app parameter>
```

**Command Line Options**

All available command line options can be found [here](https://github.com/siemens/drace/blob/master/README.md).

## Detectors

DRace is shipped with three different detector backends. The detector backend is evaluating the program trace which comes from DRace and reports the actual data race.

The following detectors are available:

- tsan (default)
- fasttrack (experimental)
- dummy

Tsan is the default and fastest detector of the three and basically the one to use at the moment. Fasttrack is less optimized than tsan and has only experimental support at the moment. But unlike Tsan, Fasttrack is fully open source. The dummy detector does not detect any races. It is there to evaluate the overhead of the other detectors vs the instrumentation overhead.

## ReportConverter

DRace has the option to create xml reports which contain the detected data races. This xml report can be converted to a nice looking and interactive html document which can help ease the debugging after the usage of DRace. It can be used as follows or with the GUI ([more infos to the usage](https://github.com/siemens/drace/blob/master/tools/ReportConverter/README.md)):

```
Windows:
    python ReportConverter.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]
    ReportConverter.exe -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]

Linux:
    python3 ReportConverter.py -i inputFile [-o outputDirectory -b blacklistItems -w whitelistItems]

```

### Report
The generated Report looks like the following example pictures. The first picture shows an overview, whereas the second one shows a single error entry. The labels  pretty much explain all the elements of the report.

Note: If one wants to open a source file by clicking on its name, it will be opened with **VSCode**, if it is installed and the exe of it is in Windows-PATH (must be installed before the report was created). Otherwise, just a new browser tab with the file will be opened.

**Report Overview:**
![5](./Images/rep_ov.png "Overview of Report")

**Error Entry:**
![6](./Images/err_entry.png "Single Error Entry")

## Step-by-step Tutorial

Now a step by step example on how to use DRace with an example application will be provided.

1. Install DRace and DynamoRIO (if not already installed)

You can find an explanation **[here](##Get-the-tools)**.

2. Configure a DRace-Command using the GUI ([more Details](##GUI-Usage))

    - Find the pathes to drrun, drace-client.dll and drace.ini
    - The specified executable must be the delivered sample application `./drace/sample/ShoppingRush.exe`
    - Configure the ReportConverter
    - The report box must be ticked
    - The MSR and Debug Mode box must be unticked
    - Use TSAN as detector

![7](./Images/gui_example.png)

3. Execute DRace

After setting everything up in the GUI, it's time to hit the run button and execute DRace for the first time. A powershell window will appear and after a short while and everything went well, you will see something like this. Please note that during runtime, a MSVC "Debug Assertion Failed" might fire, which states "erase iterator out of range". Please just click cancel and everything should be fine.

![8](./Images/powershell_out.png)

There, you can see DRace found either one or two data races and it shows in which folder the report was created. Navigate to the folder and open the `index.html` with a browser of your choice (it is recommended to use Chrome or Firefox).

4. Examine the report

After opening the report, you can see where exactly DRace found potential data races (More information about how to read the report is [here](###Report)). This is a good starting point to figure out why the code produces data races.

Now, you can start to fix the application.

5. Fix the application and rerun DRace

Your job is now to fix the racy parts in the source file. It is located in `./drace/sample/ShoppingRush.cpp`.
If you think you did the trick you can recompile the example application and rerun with DRace. You should compile the application in "debug" mode as otherwise the race might not be exactly detected at the spot of the occurence. As already described, the debug assertion might fire. If this is the case just click "Cancel" and everything should be fine.

You're done when the application doesn't produce any races anymore, when it is analysed with DRace and still produces the correct output.

6. Compare your solution with ours

If you want, you can now compare your solution with the example solution provided in `./drace/sample/ShoppingRushSolution.cpp`.

A few words to the actual problem:

The pointer invalidation in line 101 (`shop.erase(elit);`) was recognized by the developer. With the locking of the mutex in line 96 the developer tried to prevent concurrent invalidations of the pointer. The remaining problem which leads to the data races is that this pointer is also used in lines 85, 89, 90 and 91. There it won't be invalidated, but an already invalidated pointer may be used. This leads to an undefined behaviour of the application. If the locking of the mutex is moved above line 85 (`int size = shop.size();`) everything is safe and should work fine.

🎉🎉🎉Congrats you're done with the tutorial. 🎉🎉🎉
