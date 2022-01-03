# ..:: TinyServer ::..

## Introduction
This is a webserver built with C and targeting old machines, too old for daily usage but that can still be refurbished into servers for personal as well as small/medium-sized businesses. It was designed with a low memory footprint in mind and requiring little in processing power, with no external dependencies. It nontheless achieves fast IO by minimising redundant buffer copies and excessive context switching.

It also doubles as an application server on the same machine. The server provides a C/C++ API for applications to work with, in order to process requests and responses, without needing to send the data to another server (or have a second server running on the machine and competing for resources). The specs for the API are found on the `APPLICATION.md` file.

## How to build
The server can be built by running the `build.bat` file (optionally calling it with -debug flag for a debug build), which will generate a stand-alone exe in the ~/build directory (which is created upon compilation); it then can be moved to any other directory of choice. Currently it only supports 64-bit version of Windows and MSVC compiler, with support for other compilers and platforms planned for later.

## How to run
Together with the exe a `tinyserver.conf` file is copied to the build directory, which serves to control certain aspects of the server, such as port number, size of buffers for IO, number of threads, and the location of the app library and working directory. If this file is not found upon starting up, the server uses default values for all parameters and no app.

Static files must be placed in the directory specified with `files-folder-path`, which can be relative to the exe or absolute. Likewise, any file used by the app, as well as the app library, must be placed on directory specified with `app-base-path`, which then becomes the working directory for any path specified on the request URI.

After setting up the conf file and the required files, you can run the server by simply executing the exe, and stopping it by closing the program in any way. A CLI status page appears after start up, showing the amount of resources being used, as well as the amount of IO being exchanged between clients and server. In the future I plan for it to be an interactive panel for fine-tunning the server live, but for now it's merely informative.

## Versions
Current version: **0.7** (partial support for HTTP 1.0 and 1.1, stable build with limited stress tests).
Roadmap:
- 0.8: implement LRU cache and compression
- 0.9: add support for HTTPS (TLS 1.3)
- 1.0: implement logging system and perform more extensive stress and longevity tests

Porting for other platforms and architectures, as well as full support for HTTP 1.0, 1.1, and HTTP2 will start after version 1.0 goes live. Bug fixes and small features will be implemented on rolling releases.
