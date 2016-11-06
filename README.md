Simple developer tools for developing with Qt
---------------------------------------------


#### Logger

Simple, convinient, thread safe and flexible logger for developing with Qt

Features:
* Stream input (used QDebug)
* Many destinations 
* Log levels, messages types, messages tags
* Very small overhead for disabled debug (with use macro) 
* Catch Qt messages
* Custom output format
* Custom messages types
* Filter by type

 
[Example](https://github.com/igorkorsukov/qzebradev/blob/master/tests/loggertests.cpp#L10)


To use Logger within your software project include the Logger source into your project

Source:
* qzebradev/logger.h - logger and base stuff
* qzebradev/logger.cpp - logger and base stuff
* qzebradev/logdefdest.h - default destinations for console and file 
* qzebradev/logdefdest.cpp - default destinations for console and file 
* qzebradev/log.h - macro for simple use logger

Change log.h as you see fit, remove unnecessary

Or use all QZebraDev suite 


#### Profiler
Simple, embedded profiler with very small overhead for developing with Qt

Features:
* Steps duration measure 
* Function duration measure 
* Detecting long function during functions execution (It helps determine the hovering function)
* Enabled / disabled on compile time and run time
* Custom data printer

[Example](https://github.com/igorkorsukov/qzebradev/blob/master/tests/profilertests.cpp#L13)


To use Profiler within your software project include the Profiler source into your project

Source:
* qzebradev/profiler.h - profiler and macro to use
* qzebradev/profiler.cpp - profiler and macro to use


Or use all QZebraDev suite, including log.h

#### Concurrent monitor
under developing
