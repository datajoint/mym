/*  mYm is a Matlab interface to MySQL server that support BLOB object
*   Copyright (C) 2005 Swiss Federal Institute of technology (EPFL), Lausanne, CH
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*
*   Author:                     Yannick Maret
*   e-mail:                     yannick.maret@a3.epfl.ch
*   old fashioned paper mail:   EPFL-STI-ITS-LTS1
*                               Yannick MARET
*                               ELD241
*                               Station 11
*                               CH-1015 Lausanne
*
*   Notice: some parts of this code (server connection, fancy print) is based  
*   on an original code by Robert Almgren (http://www.mmf.utoronto.ca/resrchres/mysql/).
*   The present code is under GPL license with the express agreement of Mr. Almgren.
*/

#ifndef MY_MAT_H
#define MY_MAT H

// mym version information
#define MYM_VERSION_MAJOR 2
#define MYM_VERSION_MINOR 7
#define MYM_VERSION_BUGFIX 1


// some local defintion
#ifndef ulong
	typedef unsigned long ulong;
#endif
#ifndef uchar
	typedef unsigned char uchar;
#endif

#include <mex.h>  //  Definitions for Matlab API
#include <zlib.h>
#include <math.h>
#include "matrix.h"

// We need a platform- and compiler-independent (rofl) fixed size 64 bit integer
#include <stdint.h>
#ifdef __GNUC__
	#include <sys/types.h>
	#define _uint64 uint64_t
	#define _int64  int64_t
#elif _MSC_VER
	#define _uint64 unsigned __int64
	#define _int64 __int64
	#define _WINDOWS 1
#else
	#error "I don't know how to declare a 64 bit uint for this compiler. Please fix!"
#endif


const bool debug = false;  //  turn on information messages

#if (defined(_WIN32)||defined(_WIN64))&&!defined(__WIN__)
	#include <windows.h>
	#include <winsock.h> // to overcome a bug in mysql.h
	/* We use case-insensitive string comparison functions strcasecmp(), strncasecmp().
	   These are a BSD addition and are also defined on Linux, but not on every OS, 
	   in particular Windows. The two "inline" declarations below fix this problem. If
	   you get errors on other platforms, move the declarations outside the WIN32 block */
	inline int strcasecmp(const char *s1, const char *s2) { return strcmp(s1, s2); }
	inline int strncasecmp(const char *s1, const char *s2, size_t n) { return strncmp(s1, s2, n); }
#endif
#include <mysql.h>  //  Definitions for MySQL client API

// DLL entry point
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
/*************** serializing functions ***************/
// matlab objects
char* serializeArray(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
char* serializeSparse(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
char* serializeStruct(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
char* serializeCell(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
char* serializeScalar(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
char* serializeString(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
// generic object
char* serializeBinary(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
char* serializeFile(size_t &rnBytes, const mxArray* rpArray, const char* rpArg, const bool);
// deserializing functions
mxArray* deserialize(const char* rpSerial, const size_t rlength);
mxArray* deserializeArray(const char* rpSerial, const size_t rlength);
mxArray* deserializeSparse(const char* rpSerial, const size_t rlength);
mxArray* deserializeStruct(const char* rpSerial, const size_t rlength);
mxArray* deserializeCell(const char* rpSerial, const size_t rlength);
// utility functioms
int file_length(FILE *f); // get the size of a file in byte
unsigned long min_mysql_escape(char* rpout, const char* rpin, const unsigned long nin);
void safe_read_64uint(mwSize* dst, _uint64* src, size_t n);
//void safe_read_64uint(mwSize* dst, unsigned __int64* src, size_t n);

/**********************************************************************
 *
 * hostport(s):  Given a host name s, possibly containing a port number
 *  separated by the port separation character (normally ':').
 * Modify the input string by putting a null at the end of
 * the host string, and return the integer of the port number.
 * Return zero in most special cases and error cases.
 * Modified string will not contain the port separation character.
 * Examples:  s = "myhost:2515" modifies s to "myhost" and returns 2515.
 *      s = "myhost"    leaves s unchanged and returns 0.
 *
 **********************************************************************/
const char portsep = ':';   //  separates host name from port number
static int hostport(char *s) {
	//   Look for first portsep in s; return 0 if null or can't find
	if (!s || !(s = strchr(s, portsep))) 
		return 0;
	//  If s points to portsep, then truncate and convert tail
	*s = 0;
	s+=1;
	return atoi(s);   // Returns zero in most special cases
}
/*********************************************************************/
//  Static variables that contain connection state
/*
 *  isopen gets set to true when we execute an "open"
 *  isopen gets set to false when either we execute a "close"
 *            or when a ping or status fails
 *   We do not set it to false when a normal query fails;
 *   this might be due to the server having died, but is much
 *   more likely to be caused by an incorrect query.
 */
class conninfo {
public:
	MYSQL *conn;   //  MySQL connection information structure
	bool isopen;   //  whether we believe that connection is open
	conninfo() { 
		conn = NULL; 
		isopen = false; 
  	}
};
const int MAXCONN = 20;
static conninfo c[MAXCONN];   //  preserve state for MAXCONN different connections
// for GPL license
static bool runonce = false;
/*********************************************************************/
const char ID_MATLAB[] = "mYm";
const size_t LEN_ID_MATLAB = strlen(ID_MATLAB);
const char ID_ARRAY   = 'A';
const char ID_SPARSE  = 'P';
const char ID_CELL    = 'C';
const char ID_STRUCT  = 'S';
// Placeholder related constants
const char PH_OPEN[]   = "{";  // placeholder openning symbols						
const char PH_CLOSE[]  = "}";  // placeholder closing symbols					
const char PH_BINARY   = 'B';
const char PH_FILE     = 'F';
const char PH_MATLAB   = 'M';
const char PH_STRING   = 'S';
// Preamble argument
const char PRE_NO_COMPRESSION = 'u';
// Compression
const ulong MIN_LEN_ZLIB = 1000;        // minimum number of byte for trying compressiom
const float MIN_CMP_RATE_ZLIB = 1.1f;   // minimum compression ratio
const char ZLIB_ID[] = "ZL123";
const size_t LEN_ZLIB_ID = strlen(ZLIB_ID);
typedef char* (*pfserial)(size_t &, const mxArray*, const char*, const bool);

// Query flags passed from Matlab to mym
const char ML_FLAG_BIGINT_TO_DOUBLE[] = "bigint_to_double";
enum CMD_FLAGS {
	FLAG_BIGINT_TO_DOUBLE = 1
};


static void getSerialFct(const char* rpt, const mxArray* rparg, pfserial& rpf, bool& rpec);
mxArray* deserialize(const char* rpSerial, const size_t rlength);

#endif // MY_MAT_H
