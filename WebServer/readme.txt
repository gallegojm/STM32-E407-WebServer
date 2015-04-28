*****************************************************************************
**           Multi client Web server on STM32-E407 with ChibiOs            **
*****************************************************************************

  My goal is to explore the possibilities of a server running on an Olimex
STM32-E407 board with a SD card able to serve several clients at a time.
  The function responsible for analyzing html queries is very simplified
and recognizes only the phrase GET /<filename> where filename is a text file
html) or a jpg or ico. Images are important as they take a lot of time for
transmitting and give me time to see how the program reacts.
  A special case is "GET /status" that display a short page with the status
of each http web threads: number, phase and name of last file.
  My main difficulty is to assemble threads, priorities, buffers connection
and good configuration of lwip and fatfs.
  You have to modify _FS_SHARE defined in ffconf.h and MEMP_NUM_NETCONN
defined in lwipopts.h (see web/web.h for explanation)
 
  Runs on an Olimex STM32-E407 board.
  Build with ChibiStudio (chibios 2.6.7)

  To test, save a few pictures on the memory and then create html files like:
<html>
<h2>Thank you for testing!</h2>
<img src="image1.jpg" alt="image1.jpg" width="320" />
<img src="image2.jpg" alt="image2.jpg" width="320" />
</html>
