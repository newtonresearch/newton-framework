#Newton.framework for macOS
The Newton OS in a framework.
Well, enough of it to enable the Newton Connection and Newton Toolkit apps.


##NOTES ON THE NEWTON SOURCE
Let's make no bones about it, the source is derived from close inspection of the symbols included in the Newton ROM v2.2,
the header files distributed with the Newton C++ Tools and Lantern DDKs, and the Newton Programmer's Guide, etc.
It's mostly faithful to the original, even down to things that really have no place on another platform.
The most obvious change is in renaming classes having a T prefix (Apple's Pascal heritage) to a C prefix (because we are now in a C world).
Comments are (I hope) sprinkled adequately, but there is no overall design document, hence these notes.


##BUILDING
Open the Newton Xcode project. It builds for macOS Sierra, 64-bit.
There are three targets within the project:

* **Framework** builds the Newton.framework used by NCX (Newton Connection).
* **NTKFramework** builds the NTK.framework used by NTX (Newton Toolkit).
* **MesagePad** builds a standalone application that mimics a MP2000. This is very much a work still in progress.

The source is mostly C++, but there are also some NewtonScript Ref objects built with assembler.


##POINTS OF INTEREST

###ROM Data
NewtonScript ROM data is extracted from a ROM image file and turned into assembler source by the ResourceMaker/ResMaker.xcodeproj Xcode project.
This means we have access to all NewtonScript symbols, frames, functions, magic pointers, etc from the ROM (good) but they can not be changed (bad).
The goal is to recreate the NewtonScript sources to those assets so they can be built into a package by NTK and extracted by ResMaker into assembler source for linkage into the C world.

###Native functions
The Newton.exp file (Toolkit.exp for the NTK framework) defines native functions in the same way as the Newton C++ Tools for MPW.

###Virtual Memory
Some MMU functions have been implemented as an aid to understanding Newton memory management,
but there is of course no actual page faulting which leads to some memory management issues (stacks don’t grow, VBOs aren’t loaded when needed, etc).

###Memory Allocation
The original OS mixes NewPtr, malloc and free (and sometimes calls the wrong function to free that memory).
Here, all allocation (well, there are some exceptions) is done with a call to NewPtr/FreePtr (which replaces DisposPtr).
Memory is actually allocated by malloc/free (see FakePointers.cc) or new/delete.

###Stores
A flash store has been implemented.
This is used for VBOs as well as soup storage. It uses a mmap’d file so is persistent.
It would be interesting to know whether the implementation is faithful enough to read a real Newton store image.

###Recognisers
Most high-level recogniser classes have been implemented, and there is a tap recogniser, but it needs more of the view system to be working to do anything useful.

###View System
Transitioning away from QuickDraw to Quartz is still in a state of flux. Some bold design decisions need to be made here.

###NewtonScript Compiler & Interpreter
Upgrading these components is a very interesting project in its own right.
The original NewtonScript compiler was built using yacc and flex, defined in NewtonScript.y and NewtonScript.l.
The source here is transitioning to LLVM so that both bytecode and native code can be produced.

###Multitasking
Task switching is performed without using host platform tasks.
Each Newton task has its own registers, stack and data area which are switched by pseudo-SWI using assembler.
Only the MessagePad target uses task switching.

###Protocols
Protocol classes are built using inline assembler and hand-mangled function names.
Back in the day this process was automated, of course, but I never had the time or inclination to write the tools to do it.

###Communications
Some classes have been implemented but there is no integration with the host platform.

###MPW Tools
The REx tool builds a valid ROM extension. Other MPW tools are work in progress.
