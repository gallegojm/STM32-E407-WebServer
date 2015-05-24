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

#include <string.h>

#include "ch.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#include "ff.h"

#include "web.h"

//  Stack area for the http server thread.
THD_WORKING_AREA( wa_http_server, SERVER_THREAD_STACK_SIZE );

//  Stack areas for the http web threads.
THD_WORKING_AREA( wa_http_web[ WEB_NBR_CLIENTS ], WEB_THREAD_STACK_SIZE );

//  array of parameters for each http web thread
struct server_stru ss[ WEB_NBR_CLIENTS ];

static const char h1[] = "HTTP/1.1 200 OK\r\nContent-type: ";
static const char h2i[] = "image/x-icon";
static const char h2t[] = "text\r\n";
static const char h3[] = "/html\r\n";
static const char h4[] = "Content-Length: ";
static const char h5[] = "\r\n";
static const char h6[] = "Connection: close\r\n\r\n";

static const char h10[] = "HTTP/1.1 200 OK\r\nContent-type: text\r\n\r\n";
static const char h11a[] = "<html><h3>Web Server version ";
static const char h11b[] = "</h3>";
static const char h12[] = "<meta http-equiv=\"refresh\" content=\"1\">";
static const char h13[] = " ";
static const char h14[] = "<br>";
static const char h15[] = "</html>\r\n\n";

static const char h20[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
static const char h21[] = "<html><h2><center>Error: 404 Not Found: ";
static const char h22[] = "</center></h2></html>\r\n\n";

// Convert an integer to string (must be <= 9999999999)
//
// parameters:
//   s: string where the conversion is made (must be large enough
//   i: integer t convert
//   z: size of returned string (leading space filled with '0'
//      if z=0, no leading space
//
// return pointer to string

char * i2strZ( char * s, uint32_t i, uint8_t z )
{
  char * psi = s + ( z > 0 ? z : sizeof( s ));

  * -- psi = 0;
  if( i == 0 )
    * -- psi = '0';
  for( ; i; i /= 10 )
    * -- psi = '0' + i% 10;
  if( z > 0 )
    while( psi > s )
      * -- psi = '0';
  return psi;
}


/*******************************************************************************
**                                                                            **
**                   Very incomplete http server routine                      **
**                                                                            **
*******************************************************************************/

void http_server_serve( struct netconn * newconn, uint8_t tnum )
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  u16_t i;
  char  c;
  FIL   file;
  u32_t filesize;
  u8_t  rbuf[ SD_BUF_SIZE ];           // data buffer for reading the file
  u16_t nb;
  char  fname[ FNAME_LEN ];
  char  str[ 15 ];
  char * pstr;
  char * pext;
  u32_t rate, trate;

  // Read the data from the port, waiting 2 s if nothing yet there.
  // We assume the request (the part we care about) is in one netbuf
  netconn_set_recvtimeout( newconn, 2000 );
  err = netconn_recv( newconn, &inbuf );

  if( err == ERR_OK )
  {
    ss[ tnum ].fase = 5;
    // Get a pointer on the data and his length
    netbuf_data( inbuf, (void **) & buf, & buflen );

    // Check for HTTP GET command
    if( ! strncmp( buf, "GET /", 5 ))
    {
      // read next word
      i = 0;
      while( i < FNAME_LEN )
      {
        c = buf[ i + 5 ];
        if( c == ' ' || c == 0 )
          break;
        fname[ i ++ ] = c;
      }
      fname[ i ] = 0;
      if( strlen( fname ) == 0 )
        strcpy( fname, "index.html" );
      strcpy( ss[ tnum ].fname, fname );
      // Discard the rest of received data
      netbuf_delete( inbuf );

      // Is it special command 'status' ?
      if( ! strcmp( fname, "status" ))
      {
        ss[ tnum ].fase = 6;
        trate = 0;
        // Send a status of all http web threads
        netconn_write( newconn, h10, sizeof( h10 ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h11a, sizeof( h11a ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, WEB_VERSION, sizeof( WEB_VERSION ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h11b, sizeof( h11b ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h12, sizeof( h12 ) - 1, NETCONN_NOCOPY );
        for( i = 0; i < WEB_NBR_CLIENTS; i ++ )
        {
          // number of the thread
          pstr = i2strZ( str, i, 0 );
          netconn_write( newconn, pstr, strlen( pstr ), NETCONN_COPY );
          netconn_write( newconn, h13, sizeof( h13 ) - 1, NETCONN_NOCOPY );
          // phase of the thread
          pstr = i2strZ( str, ss[ i ].fase, 0 );
          netconn_write( newconn, pstr, strlen( pstr ), NETCONN_COPY );
          netconn_write( newconn, h13, sizeof( h13 ) - 1, NETCONN_NOCOPY );
          // if( i != tnum && ss[ i ].fname[ 0 ] != 0 )
          if(ss[ i ].fase > 1 )
          {
            // number of records sent
            pstr = i2strZ( str, ss[ i ].count, 0 );
            netconn_write( newconn, pstr, strlen( pstr ), NETCONN_COPY );
            netconn_write( newconn, h13, sizeof( h13 ) - 1, NETCONN_NOCOPY );
            // rate on last second in kBytes per seconds
            rate = ( ss[ i ].count - ss[ i ].count0 ) * 2;
            trate += rate;
            ss[ i ].count0 = ss[ i ].count;
            pstr = i2strZ( str, rate, 0 );
            netconn_write( newconn, pstr, strlen( pstr ), NETCONN_COPY );
            netconn_write( newconn, h13, sizeof( h13 ) - 1, NETCONN_NOCOPY );
            // name of file opened by the thread
            netconn_write( newconn, ss[ i ].fname, strlen( ss[ i ].fname ), NETCONN_COPY );
          }
          else
          {
            ss[ i ].fname[ 0 ] = 0;
            ss[ i ].count = 0;
            ss[ i ].count0 = 0;
          }
          netconn_write( newconn, h14, sizeof( h14 ) - 1, NETCONN_NOCOPY );
        }
        // sum of all rates
        if( trate > 0 )
        {
          pstr = i2strZ( str, trate, 0 );
          netconn_write( newconn, pstr, strlen( pstr ), NETCONN_COPY );
          netconn_write( newconn, h14, sizeof( h14 ) - 1, NETCONN_NOCOPY );
        }
        netconn_write( newconn, h15, sizeof( h15 ) - 1, NETCONN_NOCOPY );
        ss[ tnum ].fname[ 0 ] = 0;
      }

      // If we can open the file...
      else if( f_open( & file, fname, FA_READ ) == FR_OK )
      {
        ss[ tnum ].fase = 7;
        ss[ tnum ].count = 0;
        ss[ tnum ].count0 = 0;
        filesize = f_size( & file );
        pstr = i2strZ( str, filesize, 0 );
        pext = strrchr( fname, '.' );

        netconn_write( newconn, h1, sizeof( h1 ) - 1, NETCONN_NOCOPY );
        if( ! strcmp( pext, ".jpg" ) || ! strcmp( pext, ".ico" ) )
          netconn_write( newconn, h2i, sizeof( h2i ) - 1, NETCONN_NOCOPY );
        else
          netconn_write( newconn, h2t, sizeof( h2t ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h3, sizeof( h3 ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h4, sizeof( h4 ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, pstr, strlen( pstr ), NETCONN_COPY );
        netconn_write( newconn, h5, sizeof( h5 ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h6, sizeof( h6 ) - 1, NETCONN_NOCOPY );

        while( f_read( & file, rbuf, SD_BUF_SIZE, (UINT *) & nb ) == FR_OK && nb > 0 )
        {
          netconn_write( newconn, rbuf, nb, NETCONN_COPY );
          ss[ tnum ].count ++;
        }

        f_close( & file );
        ss[ tnum ].fase = 8;
      }

      // file does not exists
      else
      {
        netconn_write( newconn, h20, sizeof( h20 ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h21, sizeof( h21 ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, fname, strlen( fname ), NETCONN_COPY );
        netconn_write( newconn, h22, sizeof( h22 ) - 1, NETCONN_NOCOPY );
      }
    }
    else
      netbuf_delete( inbuf );
  }
  // Give the web browser time to receive the data
  chThdSleepMilliseconds( 100 );
  // Close the connection
  netconn_close( newconn );
}


/*******************************************************************************
**                                                                            **
**                               HTTP web thread                              **
**                                                                            **
*******************************************************************************/

THD_FUNCTION( http_web, p )
{
  struct server_stru * pss = p;
  char tn[] = "http_web_n";

  // names thread as http_web_1, http_web_2, ...
  tn[ strlen( tn ) - 1 ] = pss->num + '1';
  chRegSetThreadName( tn );

  while( true )
  {
    // wait to listen to new client
    if( chBSemWait( & pss->semrequest ) == MSG_OK )
    {
      pss->fase = 4;
      http_server_serve( pss->webconn, pss->num );   // ... call the http function...
      pss->fase = 9;
      netconn_delete( pss->webconn );      // ... delete the connexion.
      pss->webconn = NULL;
      pss->fase = 0;
    }
  }
}


/*******************************************************************************
**                                                                            **
**                             HTTP server thread                             **
**                                                                            **
*******************************************************************************/

THD_FUNCTION( http_server, p )
{
  struct netconn *srvconn;
  uint8_t i;

  (void ) p;

  //  Initialize web thread' parameters for each thread
  for( i = 0; i < WEB_NBR_CLIENTS; i ++ )
  {
    ss[ i ].num = i;
    ss[ i ].fase = 0;
    ss[ i ].count = 0;
    ss[ i ].count0 = 0;
    ss[ i ].fname[ 0 ] = 0;
    ss[ i ].webconn = NULL;
    chBSemObjectInit( & ss[ i ].semrequest, true );
  }

  //  Creates the HTTP Web threads
  for( i = 0; i < WEB_NBR_CLIENTS; i ++ )
    chThdCreateStatic( wa_http_web[ i ], sizeof( wa_http_web[ i ] ),
                       WEB_THREAD_PRIORITY, http_web, & ss[ i ] );

  // Create a TCP connection handle
  srvconn = netconn_new( NETCONN_TCP );
  LWIP_ERROR( "http_server: invalid srvconn", (srvconn != NULL), return; );

  // Bind to port 80 (HTTP) with default IP address
  netconn_bind( srvconn, NULL, WEB_THREAD_PORT );

  // Put the connection into LISTEN state
  netconn_listen( srvconn );

  //  Goes to the final priority after initialization
  chThdSetPriority( WEB_THREAD_PRIORITY );

  // Listen for a new client request, cyclically on each connection
  while( true )
  {
    // Look for the first connection not used
    for( i = 0; i < WEB_NBR_CLIENTS; i ++ )
      if( ss[ i ].webconn == NULL )
        break;
    if( i == WEB_NBR_CLIENTS )
      // All connections occupied
      chThdSleepMilliseconds( 300 );
    else
    {
      ss[ i ].fase = 1;
      // Wait for a new client request
      if( netconn_accept( srvconn, & ss[ i ].webconn ) == ERR_OK )
      {
        ss[ i ].fase = 2;
        // Wake up the corresponding thread
        chBSemSignal( & ss[ i ].semrequest );
        ss[ i ].fase = 3;
      }
      else
        ss[ i ].fase = 0;
    }
  }
}
