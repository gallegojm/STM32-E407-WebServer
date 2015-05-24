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

#ifndef _WEB_H_
#define _WEB_H_

#define WEB_VERSION               "2015-05-22"

#define WEB_THREAD_PORT           80

// number of clients we want to serve simultaneously
#define WEB_NBR_CLIENTS           8

// size of file buffer for reading a file
#define SD_BUF_SIZE               512

// max length of a file name
#define FNAME_LEN                 32

#define SERVER_THREAD_STACK_SIZE  256
#define WEB_THREAD_STACK_SIZE     ( 896 + SD_BUF_SIZE + FNAME_LEN )

#define WEB_THREAD_PRIORITY       (LOWPRIO + 2)

// define a structure of parameters for an http web thread
struct server_stru
{
  uint8_t num;
  uint32_t fase;
  uint32_t count;
  uint32_t count0;
  struct netconn *webconn;
  binary_semaphore_t semrequest;
  char fname[ FNAME_LEN ];
};

extern THD_WORKING_AREA( wa_http_server, SERVER_THREAD_STACK_SIZE );

#ifdef __cplusplus
extern "C" {
#endif
  THD_FUNCTION( http_server, p );
#ifdef __cplusplus
}
#endif

#endif // _WEB_H_
