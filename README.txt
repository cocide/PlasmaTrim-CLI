PlasmaTrim-CLI
==============

Mac and Linux each have a folder in the repo, cd into the approperate folder for your system and make to compile
./ptrim help will give you more information about the options.


Things that Need to Eventually Happen
=====================================

On the fly control of multiple units
  The basic idea is that you give a sequence, or set of sequences, to the devices and they stay synced.
  This probably won't upload the sequence to the device and therefore would not be saved after a power cycle.
    Because we are not loading the device you won't have a limit for amounts of 'slots'.
    This could also have a start loop value so you can run an intro before the loop begins.
  This might involve making a new .ptSeq format which needs to work with the current format and applications.

Better fade timing.
  Currently this is rather rough, it seems that usleep is not all that accurate. Anyone got any ideas?


Log of Change
=============

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