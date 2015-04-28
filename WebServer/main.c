/*
 *
 *  Multi client Web Server on STM32-E407 with ChibiOs
 *
 *  Copyright (c) 2015 by Jean-Michel Gallego
 *
 *  Please read file ReadMe.txt for instructions
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ch.h"
#include "hal.h"

#include "lwipthread.h"
#include "ff.h"
#include "web/web.h"

//===========================================================================
//   FatFs related
//===========================================================================

//  FS object.
static FATFS SDC_FS;


//===========================================================================
//   Green LED blinker thread
//===========================================================================

static WORKING_AREA( waThread1, 128 );

static msg_t Thread1( void *arg )
{
  (void )arg;
  chRegSetThreadName( "blinker" );
  while( TRUE )
  {
    palTogglePad( GPIOC, GPIOC_LED );
    chThdSleepMilliseconds( 500 );
  }
  return RDY_OK;
}


//===========================================================================
//   Main  code
//===========================================================================

int main( void )
{
  uint8_t i;

  halInit();
  chSysInit();

  //  Mount the SD card
  sdcStart( &SDCD1, NULL );
  sdcConnect( &SDCD1 );
  f_mount( 0, & SDC_FS );

  //  Creates the blinker thread.
  chThdCreateStatic( waThread1, sizeof( waThread1 ), NORMALPRIO, Thread1, NULL );

  //  Creates the LWIP threads (it changes priority internally).
  chThdCreateStatic( wa_lwip_thread, LWIP_THREAD_STACK_SIZE, NORMALPRIO + 3,
                     lwip_thread, NULL );

  // Initialize web server
  http_server();

   //  Normal main() thread activity
  while( TRUE )
  {
    chThdSleepMilliseconds( 1000 );
  }
}
