Lab 3 - Hello Triangle and Installing prerequisite software
========================================

Lab machines
------------

The required development applications should already be installed. For the
machines in 20-127, you can optionally use the Intel C++ compiler by running

	> source /opt/intel/bin/compilervars.sh intel64

You will then have access to `icc` (Intel C Compiler) and `icpc` (Intel C++
Compiler).

Ubuntu Linux
------------

You'll need the following if you don't have them already.

	> sudo apt-get update
	> sudo apt-get install g++
	> sudo apt-get install cmake
	> sudo apt-get install freeglut3-dev
	> sudo apt-get install libxrandr-dev
	> sudo apt-get install libxinerama-dev
	> sudo apt-get install libxcursor-dev

Although Eigen, GLFW, and GLEW are also available as packages, we'll download
them separately. Feel free to get these packages, but we won't be using them
for building our labs and assignments.

Mac OS X
--------

You can use homebrew/macports or install these manually.

- Xcode developer tools. You'll need to log in with your Apple ID.
- CMake (<https://cmake.org/download/>)

Make sure the commands `g++` and `cmake` work from the command prompt.

Windows
-------

You'll need to download these manually.

- Visual Studio. Any version should work. I've tested Visual Studio 2015
  (aka version 14) on Windows 8.
- CMake (<https://cmake.org/download/>). Make sure to add CMake to the system
  path when asked to do so.

Make sure the command `cmake` works from the command prompt.

**Install Eigen, GLFW, and GLEW**
=================================

Lab machines
------------

For compatibility issues with the old version of g++ installed on the lab
machines, we will be using these files on my home directory. Download these
and extract them somewhere (e.g., `~/lib/`).

- Eigen: `/home/sueda/lib/eigen-3.2.6.zip`
- GLFW: `/home/sueda/lib/glfw-3.1.2.zip`
- GLEW: `/home/sueda/lib/glew-1.13.0.tgz`

All platforms except lab machines
---------------------------------

For all other platforms, you can download the latest versions from the web.
For each of these libraries, download and extract them somewhere (e.g.,
`~/lib`).

- Eigen: get the source from <http://eigen.tuxfamily.org/index.php?title=Main_Page>.
- GLFW: get the source from <http://www.glfw.org/download.html>.
- Glew
  - OSX & Linux
    - Get the source from <http://glew.sourceforge.net/>.
    - Compile the source by typing `make`. (This could take a while.)
  - Windows
    - Get the Windows binaries from <http://glew.sourceforge.net/>.

**Set the environment variables for Eigen, GLFW, and GLEW**
===========================================================

OSX & Linux
-----------

In `~/.bash_profile` (or `~/.bashrc` if `.bash_profile` doesn't exist), add the
following lines.

	export EIGEN3_INCLUDE_DIR=ABS_PATH_TO_EIGEN
	export GLFW_DIR=ABS_PATH_TO_GLFW
	export GLEW_DIR=ABS_PATH_TO_GLEW

Set these variables to point to the directories that you extracted Eigen,
GLFW, and GLEW to.

Windows
-------

Control Panel -> System -> Advanced -> Environment Variables -> User Variables

- Set `EIGEN3_INCLUDE_DIR` to `ABS_PATH_TO_EIGEN`
- Set `GLFW_DIR` to `ABS_PATH_TO_GLFW`
- Set `GLEW_DIR` to `ABS_PATH_TO_GLEW`

Important Note on Including Eigen and GLEW
------------------------------------------

- If you're having trouble linking with GLEW, make sure you `#define GLEW_STATIC` before you `#include <GL/glew.h>`.
- If you're having trouble running your application with Eigen, try `#define EIGEN_DONT_ALIGN_STATICALLY` before you `#include <Eigen/Dense>`.

**Building and Running the Lab/Assignment**
===========================================

All platforms
-------------

Download and extract the lab file [here](/L00.zip):
(<http://users.csc.calpoly.edu/~ssueda/teaching/CSC474/2016W/labs/L00/L00.zip>).

We'll perform an "out-of- source" build, which means that the binary files
will not be in the same directory as the source files. In the folder that
contains CMakeLists.txt, run the following.

	> mkdir build
	> cd build

Then run one of the following, depending on your choice of platform and IDE.

OSX & Linux Makefile
--------------------

	> cmake ..

This will generate a Makefile that you can use to compile your code. To
compile the code, run the generated Makefile.

	> make -j4

The `-j` argument speeds up the compilation by multithreading the compiler.
This will generate an executable, which you can run by typing

	> ./lab3

!Note this assume a resources directory

To build in release mode, use `ccmake ..` and change `CMAKE_BUILD_TYPE` to
`Release`. Press 'c' to configure then 'g' to generate. Now `make -j4` will
build in release mode.

To change the compiler, read [this
page](http://cmake.org/Wiki/CMake_FAQ#How_do_I_use_a_different_compiler.3F).
The best way is to use environment variables before calling cmake. For
example, to use the Intel C++ compiler:

	> which icpc # copy the path
	> CXX=/path/to/icpc cmake ..

OSX Xcode
---------

	> cmake -G Xcode ..

This will generate `lab3.xcodeproj` project that you can open with Xcode.

- To run, change the target to `lab3` by going to Product -> Scheme -> lab3.
  Then click on the play button or press Command+R to run the application.
- Edit the scheme to add command-line arguments (`../../resources`) or to run
  in release mode.

Windows Visual Studio 2015
--------------------------

	> cmake -G "Visual Studio 14 2015" ..

This will generate `lab3.sln` file that you can open with Visual Studio.
Other versions of Visual Studio are listed on the CMake page
(<https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html>).

- To build and run the project, right-click on `lab3` in the project explorer
  and then click on "Set as Startup Project." Then press F7 (Build Solution)
  and then F5 (Start Debugging).
- To add a commandline argument (`../resources`), right-click on `lab3` in
  the project explorer and then click on "Properties" and then click to
  "Debugging."
