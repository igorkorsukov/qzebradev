Simple developer tools for developing with Qt
---------------------------------------------


#### Logger

Simple, convinient, thread safe logger and flexible logger for Qt

Features:
* Stream input (use QDebug)
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
* logger.h - logger and base stuff
* logger.cpp - logger and base stuff
* logdefdest.h - default destinations for console and file 
* logdefdest.cpp - default destinations for console and file 
* log.h - macro for simple use logger

Change log.h change log as you see fit, remove unnecessary

Or use all QZebraDev suite 


#### Profiler
under developing

#### Concurrent monitor
under developing
