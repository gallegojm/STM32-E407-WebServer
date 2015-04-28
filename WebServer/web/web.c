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

//  Stack areas for the http web threads.
WORKING_AREA( wa_http_web[ WEB_NBR_CLIENTS ], WEB_THREAD_STACK_SIZE );

//  array of parameters for each http web thread
struct server_stru ss[ WEB_NBR_CLIENTS ];

// structure for connection listening for clients
struct netconn *srvconn;

static const char h1[] = "HTTP/1.1 200 OK\r\nContent-type: ";
static const char h2i[] = "image/x-icon";
static const char h2t[] = "text\r\n";
static const char h3[] = "/html\r\n";
static const char h4[] = "Content-Length: ";
static const char h5[] = "\r\n";
static const char h6[] = "Connection: close\r\n\r\n";

static const char h10[] = "HTTP/1.1 200 OK\r\nContent-type: text\r\n\r\n";
static const char h11[] = "<html><h2>Server status</h2>";
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

// Very incomplete http server routine

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

  // Read the data from the port, waiting 2 s if nothing yet there.
  // We assume the request (the part we care about) is in one netbuf
  netconn_set_recvtimeout( newconn, 2000 );
  err = netconn_recv( newconn, &inbuf );

  if( err == ERR_OK )
  {
    ss[ tnum ].fase = 100;
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
        if( c == ' ' )
          break;
        fname[ i ++ ] = c;
      }
      fname[ i ] = 0;
      // Discard the rest of received data
      netbuf_delete( inbuf );
      // Is it special command 'status' ?
      if( ! strcmp( fname, "status" ) )
      {
        // Send a status of all http web threads
        netconn_write( newconn, h10, sizeof( h10 ) - 1, NETCONN_NOCOPY );
        netconn_write( newconn, h11, sizeof( h11 ) - 1, NETCONN_NOCOPY );
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
          // name of last file opened by the thread
          netconn_write( newconn, ss[ i ].fname, strlen( ss[ i ].fname ), NETCONN_COPY );
          netconn_write( newconn, h14, sizeof( h14 ) - 1, NETCONN_NOCOPY );
        }
        netconn_write( newconn, h15, sizeof( h15 ) - 1, NETCONN_NOCOPY );
      }
      // If we can open the file...
      else if( f_open( & file, fname, FA_READ ) == FR_OK )
      {
        strcpy( ss[ tnum ].fname, fname );
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
          ss[ tnum ].fase ++;
        }

        f_close( & file );
        ss[ tnum ].fase = 10;
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


//  HTTP web thread.

msg_t http_web( void * p )
{
  struct server_stru * ss = p;
  struct netconn *webconn;
  char tn[] = "http_web_n";

  // names thread as http_web_1, http_web_2, ...
  tn[ strlen( tn ) - 1 ] = ss->num + '1';
  chRegSetThreadName( tn );

  while( TRUE )
  {
    if( netconn_accept( srvconn, & webconn ) == ERR_OK ) // checks for new client and create a connexion to serve it...
    {
      ss->fase = 1;
      http_server_serve( webconn, ss->num );   // ... call the http function...
      ss->fase = 2;
      netconn_delete( webconn );      // ... delete the connexion.
      ss->fase = 0;
    }
  }
  return RDY_OK;
}

//  HTTP server thread.

bool http_server( void )
{
  uint8_t i;

  // Create a TCP connection handle
  srvconn = netconn_new( NETCONN_TCP );
  LWIP_ERROR( "http_server: invalid srvconn", (srvconn != NULL), return false; );

  // Bind to port 80 (HTTP) with default IP address
  netconn_bind( srvconn, NULL, WEB_THREAD_PORT );

  // Put the connection into LISTEN state
  netconn_listen( srvconn );

  //  Initialize web thread' parameters for each thread
  for( i = 0; i < WEB_NBR_CLIENTS; i ++ )
  {
    ss[ i ].num = i;
    ss[ i ].fase = 0;
    ss[ i ].fname[ 0 ] = 0;
  }

  //  Creates the HTTP Web threads
  for( i = 0; i < WEB_NBR_CLIENTS; i ++ )
    chThdCreateStatic( wa_http_web[ i ], sizeof( wa_http_web[ i ] ),
                       WEB_THREAD_PRIORITY, http_web, & ss[ i ] );
  return true;
}
