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

#define WEB_THREAD_PORT           80

// number of clients we want to serve simultaneously
#define WEB_NBR_CLIENTS           8
#if WEB_NBR_CLIENTS > _FS_SHARE
  #error "_FS_SHARE defined in ffconf.h must be >= WEB_NBR_CLIENTS"
#endif
#if WEB_NBR_CLIENTS + 1 > MEMP_NUM_NETCONN
  #error "MEMP_NUM_NETCONN defined in lwipopts.h must be >= WEB_NBR_CLIENTS + 1"
#endif

// size of file buffer for reading a file
#define SD_BUF_SIZE               512

// max length of a file name
#define FNAME_LEN                 32

#define WEB_THREAD_STACK_SIZE     (1024 + SD_BUF_SIZE)

#define WEB_THREAD_PRIORITY       (LOWPRIO + 2)

// define a structure of parameters for an http web thread
struct server_stru
{
  uint8_t num;
  uint32_t fase;
  char fname[ FNAME_LEN ];
};

extern WORKING_AREA( wa_http_web[ WEB_NBR_CLIENTS ], WEB_THREAD_STACK_SIZE );
extern struct server_stru ss[ WEB_NBR_CLIENTS ];

#ifdef __cplusplus
extern "C" {
#endif
  msg_t http_web( void *p );
  bool http_server( void );
#ifdef __cplusplus
}
#endif

#endif // _WEB_H_
