PlasmaTrim-CLI
==============

Simply run make from the root directory to compile.
./ptrim help will give you more information about the options.

including the ptrim-lib.h and hidapi.h will allow you to use these functions with your software, however you will need to handle all the hidapi connections.


Things that Need to Eventually Happen
=====================================

Network support:
  Eventually I will have a daemon that can just be ran and allow PlasmaTrims to be on the network over TCP.
  It will automagically notice device addition/removals and allow multiple clients to control different units at the same time.
  I really want it to support some form of mDNS autodiscovery, the idea being that any application that supports this protocol will just 'find' the PlasmaTrims automatically

On the fly control of multiple units:
  Not sure how well this is working currently, time for others to test.

Better fade timing:
  Currently this is rather rough, it seems that usleep is not all that accurate. Anyone got any ideas?

Windows:
  Eventually this needs to happen


Log of Change
=============

Now with less fail
Oct 25 2012 - v0.2.1
  Commented a lot of code
  Split the meat and potatos bit into its own file so it can be included by others
  Made the stream command keep better sync of multiple devices
  changed the folder structure up a lot
  one Makefile in the root directory to be easier


And then there were two
Oct 24 2012 - v0.2.0
  Added multithread support for Linux/Mac (still no Windows support at all)
  Multiple devices can now be controlled at the same time, the result is the ability to use a sequence on several devices and they do not drift
  Added the ability to call multiple devices by wrapping a set of space seperated paths or serials in quotes
  Added the ability to call all devices by using 'all' instead of a path or serial


It's better than before, just no one will notice
Oct 24 2012 - v0.1.1
  Added a version string to the help output
  Added \r to the output for eventual Windows support
  Moved error output to standard error
  Shortened the code by moving common things into functions
  Running with only a serial or path now prints the devices info
  Let fade actually control the brightness based on user input or the saved brightness (oops, my bad, though I already did that)
  Shrank some variable sizes by meaningless amounts to make the program run unnoticeably lighter


The beginning
Oct 23 2012 - v0.1.0
  Moved the project from bash. It supports all HID commands and fading to new colors.