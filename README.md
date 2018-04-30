# simpwire

## What is this?
A small and simple asynchronous TCP library.

## Which platforms are supported?
Currently this runs only on Linux.

## How to build and install
When inside the top level repository folder, execute
following commands:
```bash
cd build
make
sudo make install
sudo ldconfig
``` 
By default the headers will be installed in 
/usr/include/simpwire and the .so files will
be installed in /usr/lib/.
To customize the headers install path
change the variable $(HEADER_INSTALL_LOCATION).
To customize the .so files install path
change the variable $(LIBRARY_INSTALL_LOCATION).

## Demo
In the demo folder there is a file demo.cpp 
which shows how simpwire can be used. To build
and execute the demo do the following:
```bash
cd demo_build
make 
export LD_LIBRARY_PATH=../build
./simpwire_demo
```


