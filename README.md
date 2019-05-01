# simpwire

## What is this?
A small and simple asynchronous TCP library.

## Which platforms are supported?
Linux and Windows.

## Requirements to build this project
On Linux:
* CMake 3.10.2 or higher
* GCC 7.3 or higher
* Make 4.1 or higher

On Windows:
* CMake 3.10.2 or higher
* MinGW-W64 GCC-7.3.0 or higher

## How to build (and install) on Linux
When inside the top level repository folder, execute
following commands in a Bash:
```bash
mkdir -p ../simpwire_build_release #Create a build folder
cd ../simpwire_build_release #Go to build folder
cmake -DCMAKE_BUILD_TYPE=Release ../simpwire #Generate Makefiles in build folder
#=== Optional ==================================================================
#If you also want to build the demo and tests use following cmake command:
cmake -DCMAKE_BUILD_TYPE=Rlease -DBUILD_DEMO=yes -DBUILD_TEST=yes ../simpwire
#===============================================================================
make #Build project
sudo make install #Install library 
``` 

By default the headers will be installed in 
/usr/include/simpwire and the .so files will
be installed in /usr/lib/. You can change the install location
by passing an install prefix to cmake like this:
```bash
-DCMAKE_INSTALL_PREFIX=/path/to/folder
```

## How to build on Windows
The build process is almost the same as on Linux:
To build the complete project with MinGW, open a Powershell, go to the top level repository folder and exeucte (make sure that the paths to g++.exe, gcc.exe and mingw32-make.exe are in your PATH variable):
```powershell
mkdir -p ../simpwire_build_release #Create a build folder
cd ../simpwire_build_release #Go to build folder
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DEMO=yes -DBUILD_TEST=yes -DCMAKE_C_COMPILER=gc
c.exe -DCMAKE_CXX_COMPILER=g++.exe -G "MinGW Makefiles" ../simpwire #Generate Makefiles in build folder
mingw32-make  #Build project
```

## Additional info for Windows users
1. If you instruct CMake to generate a Visual Studio 2017 project, the tests won't be able      to build. However, the library itself and the demo will build. That's why it is currently    recommended to let CMake generate MinGW Makefiles for the MinGW compiler.
2. There is currently no install command available after building.

## Demo 
If you built the project using the <i>-DBUILD_DEMO=yes</i> then there should be an executable
<i>simpwire_demo</i> (<i>simpwire_demo.exe</i> on Windows) in your build directory. To run it 
on Windows, simply double click the executable. Tu run it on Linux open a Bash in the build directory
and type:
```bash
chmod +x simpwire_demo
export LD_LIBRARY_PATH=`pwd` 
./simpwire_demo
```
The demo source code is inside demo/demo.cpp.