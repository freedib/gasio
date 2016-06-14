# Basic setup

The default build environment is for Mac OS X (Darwin) with eclipse

In the Configuration directory, we can find setup files for Darwin, Linux and Windows, either for eclipse or standalone makefiles.

To work with eclipse
- close eclipse
- replace the defaults .project and .cprojet with one from the Configuration/{program}-{system}-eclipse directory
- restart eclipse

To work with standalone makefiles
- copy the Debug and Release directories from the Configuration directory in root directory (same level as Sources)
- go either in Debug or Release and just type make

Required libraries (if new project from scratch)
- on Windows, required libraries are **ws2_32** and **winsock**

Binaries
- gasio is a shared library. no installation script is provided. to install, just copy it in a standard library directory (/usr/local/lib or \windows)
- echo is linked over libgasio
- echotest is standalone