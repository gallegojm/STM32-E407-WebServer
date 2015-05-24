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
 
  Runs on an Olimex STM32-E407 board.
  Compiled with ChibiStudio and chibios 3.0.0p5
  Build from chibios3\demos\STM32\RT-STM32F407-OLIMEX_E407-LWIP-FATFS-USB
  All files unmodified except:
   Makefile :
    USE_SMART_BUILD = no
    CHIBIOS = C:/ChibiStudio/chibios3
   ffconf.h :
    #define _FS_REENTRANT   1       /* 0:Disable or 1:Enable */
   lwipopts.h :
    #define MEM_SIZE          6400
    #define MEMP_NUM_TCP_PCB  8     /must be >= WEB_NBR_CLIENTS
    #define MEMP_NUM_NETBUF   8     /must be >= WEB_NBR_CLIENTS
    #define MEMP_NUM_NETCONN  9     /must be >= (WEB_NBR_CLIENTS + 1)
    #define LWIP_SO_RCVTIMEO  1
   and of course web.h and web.c

  To test, save a few pictures on the memory and then create html files like:
<html>
<h2>Thank you for testing!</h2>
<img src="image1.jpg" alt="image1.jpg" width="320" />
<img src="image2.jpg" alt="image2.jpg" width="320" />
</html>

  Calling http://192.168.1.20/status refresh every seconds a small status
page showing the state of each web thread:
  - number of thread,
  - 'fase' of thread,
  - number of blocks sended 
  - aproximative speed in kB/s of transmition
  - name of file transmitted
  The estimation of speed depends of the acuracy of the rate of refresh.
So it is not very precise but must be sufficient to have an idea of the
impact of values of different parameters
  Ultimately, the sum of the speeds is displayed when greater than zero.
  
 