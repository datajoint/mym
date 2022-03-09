/*  mYm is a Matlab interface to MySQL server that support BLOB object
 *   
 *   
 *
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

#undef MX_COMPAT_32

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <algorithm> // for max
#include <locale.h>   
#include "mym.h"

// These typedefs are for older Matlab versions.
// If your compiler throws an error, please check why
// this definition conflicts with the one in mym.h
typedef size_t mwSize;
typedef size_t mwIndex;

#ifdef _WINDOWS
  _locale_t locUS = _create_locale(LC_NUMERIC, "english-us");
#endif

//The crazy do{}while(0) constructions circumvents unexpected results when using the macro followed by a semicolon in
//an if/else construction
#define READ_UINT(dst,src) \
    if (use32bitdims()) do{ safe_read_32uint( (dst), (_uint32*)(src), 1 );   (src) += sizeof(_uint32);  } while(0); \
    else do{ safe_read_64uint( (dst), (_uint64*)(src), 1 );   (src) += sizeof(_uint64);  } while(0)
#define READ_UINTS(dst,src,n) \
    if (use32bitdims()) do{ safe_read_32uint( (dst), (_uint32*)(src), n );   (src) += (n) * sizeof(_uint32);  } while(0); \
    else do{ safe_read_64uint( (dst), (_uint64*)(src), n );   (src) += (n) * sizeof(_uint64);  } while(0)

// Macro to write fixed size 64 bit uints
#define WRITE_UINT64(p,val) do{  *((_uint64*)(p)) = (_uint64)(val);    (p) += sizeof(_uint64);  }while(0)
#define WRITE_UINT64S(p,val,n) do{     _uint64* pTemp = (_uint64*) (p); \
                    for (size_t i=0; i < (n); ++i) { pTemp[i] = (_uint64)val[i]; } \
                    p += (n) * sizeof(_uint64);  } while(0)

/**********************************************************************
 *typestr(s):  Readable translation of MySQL field type specifier
 *       as listed in   mysql_com.h
 **********************************************************************/
static const char*typestr(enum_field_types t) {
    switch(t) {
        //  These are considered numeric by IS_NUM() macro
        case FIELD_TYPE_DECIMAL:
            return "decimal";
        case FIELD_TYPE_TINY:
            return "tiny";
        case FIELD_TYPE_SHORT:
            return "short";
        case FIELD_TYPE_LONG:
            return "long";
        case FIELD_TYPE_FLOAT:
            return "float";
        case FIELD_TYPE_DOUBLE:
            return "double";
        case FIELD_TYPE_NULL:
            return "null";
        case FIELD_TYPE_LONGLONG:
            return "longlong";
        case FIELD_TYPE_INT24:
            return "int24";
        case FIELD_TYPE_YEAR:
            return "year";
        case FIELD_TYPE_TIMESTAMP:
            return "timestamp";
            //  These are not considered numeric by IS_NUM()
        case FIELD_TYPE_DATE:
            return "date";
        case FIELD_TYPE_TIME:
            return "time";
        case FIELD_TYPE_DATETIME:
            return "datetime";
        case FIELD_TYPE_NEWDATE:
            return "newdate";   // not in manual
        case FIELD_TYPE_ENUM:
            return "enum";
        case FIELD_TYPE_SET:
            return "set";
        case FIELD_TYPE_TINY_BLOB:
            return "tiny_blob";
        case FIELD_TYPE_MEDIUM_BLOB:
            return "medium_blob";
        case FIELD_TYPE_LONG_BLOB:
            return "long_blob";
        case FIELD_TYPE_BLOB:
            return "blob";
        case FIELD_TYPE_VAR_STRING:
            return "var_string";
        case FIELD_TYPE_STRING:
            return "string";
        case FIELD_TYPE_GEOMETRY:
            return "geometry";   // not in manual 4.0
        default:
            return "unknown";
    }
}
/**********************************************************************
 *fancyprint():  Print a nice display of a query result
 *  We assume the whole output set is already stored in memory,
 *  as from mysql_store_result(), just so that we can get the
 *  number of rows in case we need to clip the printing.
 *   In any case, we make only one pass through the data.
 *  If the number of rows in the result is greater than NROWMAX,
 *  then we print only the first NHEAD and the last NTAIL.
 *  NROWMAX must be greater than NHEAD+NTAIL, normally at least
 *  2 greater to allow the the extra information
 *  lines printed when we clip (ellipses and total lines).
 *   Display null elements as empty
 **********************************************************************/
const char*contstr = "...";   //  representation of missing rows
const int contstrlen = 3;     //  length of above string
const int NROWMAX = 50;       //  max number of rows to print w/o clipping
const int NHEAD = 10;         //  number of rows to print at top if we clip
const int NTAIL = 10;         //  number of rows to print at end if we clip
static void fancyprint(MYSQL_RES*res) {
    const ulong nrow = (ulong)mysql_num_rows(res);
    const ulong nfield = mysql_num_fields(res);
    bool clip = (nrow > NROWMAX);
    MYSQL_FIELD*f = mysql_fetch_fields(res);
    /************************************************************************/
    //  Determine column widths, and format strings for printing
    //  Find the longest entry in each column header,
    //  and over the rows, using MySQL's max_length
    ulong*len = (ulong*) mxMalloc(nfield*sizeof(ulong));
    for (ulong j = 0; j<nfield; j++) {
        len[j] = (int)strlen(f[j].name);
        if (f[j].max_length > len[j])
            len[j] = f[j].max_length;
    }
    //  Compare to the continuation string length if we will clip
    if (clip) {
        for (ulong j = 0; j<nfield; j++)
            if (contstrlen > len[j])
                len[j] = contstrlen;
    }
    //  Construct the format specification for printing the strings
    char**fmt = (char**) mxMalloc(nfield*sizeof(char*));
    for (ulong j = 0; j<nfield; j++) {
        fmt[j] = (char*) mxCalloc(10, sizeof(char));
        sprintf(fmt[j], "  %%-%lus ", len[j]);
    }
    /************************************************************************/
    //  Now print the actual data
    mexPrintf("\n");
    //  Column headers
    for (ulong j = 0; j<nfield; j++)
        mexPrintf(fmt[j], f[j].name);
    mexPrintf("\n");
    //  Fancy underlines
    for (ulong j = 0; j<nfield; j++) {
        mexPrintf(" +");
        for (ulong k = 0; k<len[j]; k++)
            mexPrintf("-");
        mexPrintf("+");
    }
    mexPrintf("\n");
    //  Print the table entries
    if (nrow<=0)
        mexPrintf("(zero rows in result set)\n");
    else {
        if (!clip) {
            //  print the entire table
            mysql_data_seek(res, 0);
            for (ulong i = 0; i<nrow; i++) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (!row) {
                    mexPrintf("Printing full table data from row %d\n", i+1);
                    mexErrMsgTxt("Internal error:  Failed to get a row");
                }
                for (ulong j = 0; j<nfield; j++)
                    mexPrintf(fmt[j], (row[j] ? row[j] : ""));
                mexPrintf("\n");
            }
        }
        else {
            //  print half at beginning, half at end
            mysql_data_seek(res, 0);
            for (int i = 0; i<NHEAD; i++) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (!row) {
                    mexPrintf("Printing head table data from row %d\n", i+1);
                    mexErrMsgTxt("Internal error:  Failed to get a row");
                }
                for (ulong j = 0; j<nfield; j++)
                    mexPrintf(fmt[j], (row[j] ? row[j] : ""));
                mexPrintf("\n");
            }
            for (ulong j = 0; j<nfield; j++)
                mexPrintf(fmt[j], contstr);
            mexPrintf("\n");
            mysql_data_seek(res, nrow-NTAIL);
            for (int i = 0; i<NTAIL; i++) {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (!row) {
                    mexPrintf("Printing tail table data from row %d", nrow-NTAIL+i+1);
                    mexErrMsgTxt("Internal error:  Failed to get a row");
                }
                for (ulong j = 0; j<nfield; j++)
                    mexPrintf(fmt[j], (row[j] ? row[j] : ""));
                mexPrintf("\n");
            }
            mexPrintf("(%d rows total)\n", nrow);
        }
    }
    mexPrintf("\n");
    // These should be automatically freed when we return to Matlab,
    //  but just in case ...
    mxFree(len);
    mxFree(fmt);
}
/**********************************************************************
 *field2num():  Convert field in string format to double number
 **********************************************************************/
const double NaN = mxGetNaN();              //  Matlab NaN for null values
static bool can_convert(enum_field_types t) {
    return (IS_NUM(t) &&
        (t!=FIELD_TYPE_DATE) &&
        (t!=FIELD_TYPE_TIME) &&
        (t!=FIELD_TYPE_DATETIME) &&
        (t!=FIELD_TYPE_TIMESTAMP));
}
static double field2num(const char*s, enum_field_types t) {
    if (!s) return NaN;  // MySQL null -- nothing there
    if (IS_NUM(t)) {
        double val = NaN;
#ifdef _WINDOWS        
        int scanRet = _sscanf_s_l(s, "%lf", locUS, &val);
#else
        int scanRet = sscanf(s, "%lf", &val);
#endif
        if (scanRet < 1) {
            mexPrintf("Unreadable value \"%s\" of type %s\n", s, typestr(t));
            return NaN;
        }
        return val;
    }
    else {
        mexPrintf("Tried to convert \"%s\" of type %s to numeric\n", s, typestr(t));
        mexErrMsgTxt("Internal inconsistency");
    }
    return 0;
}

/**********************************************************************
 *field2int():  Convert field in string format to integer number
 **********************************************************************/
static void field2int(const char*s, enum_field_types t, unsigned int flags, void* val) {
    if (!s) {
        mexErrMsgTxt("Invalid value NULL in a BIGINT column.");
    }
    if (IS_NUM(t)) {
        int scanRet = 0;
        if (flags & UNSIGNED_FLAG) {
            // Read unsigned 64 bit integer
            _uint64 *val_typed = (_uint64*) val;
#ifdef _WINDOWS        
            scanRet = _sscanf_s_l(s, "%I64u", locUS, val_typed);
#else
            scanRet = sscanf(s, "%lu", (unsigned long *) val_typed);
#endif
        }
        else {
            // Read signed 64 bit integer
            _int64 *val_typed = (_int64*) val;
#ifdef _WINDOWS        
            scanRet = _sscanf_s_l(s, "%I64d", locUS, val_typed);
#else
            scanRet = sscanf(s, "%ld", (unsigned long *) val_typed);
#endif
        }
        if (scanRet < 1) {
            mexPrintf("Unreadable value \"%s\" of type %s\n", s, typestr(t));
        }
    }
    else {
        mexPrintf("Tried to convert \"%s\" of type %s to numeric\n", s, typestr(t));
        mexErrMsgTxt("Internal inconsistency");
    }
}

/**********************************************************************
 *getstring():   Extract string from a Matlab array
 *   (Space allocated by mxCalloc() should be freed by Matlab
 *  when control returns out of the MEX-function.)
 *  This is based on an original by Kimmo Uutela
 **********************************************************************/
static char* getstring(const mxArray*a) {
    int llen = (int)mxGetM(a) * (int)mxGetN(a) * sizeof(mxChar) + 1;
    char*c = (char*) mxCalloc(llen, sizeof(char));
    if (mxGetString(a, c, llen))
        mexErrMsgTxt("Can\'t copy string in getstring()");
    return c;
}
/**********************************************************************
 *updateplugindir():   Update Client Plugin Directory
 *  Set LIBMYSQL_PLUGIN_DIR environment variable
 *  Utilize mym root directory
 *  Future client plugin libraries to be packaged here
 **********************************************************************/
static void updateplugindir() {
    mxArray *mym_string[1];
    mxArray *mym_path[1];
    mxArray *mym_fileparts[3];
    char*mym_directory = NULL;

    mym_string[0] = mxCreateString("mym");
    mexCallMATLAB(1, mym_path, 1, mym_string, "which");
    mexCallMATLAB(3, mym_fileparts, 1, mym_path, "fileparts");
    mym_directory = getstring(mym_fileparts[0]);

    char environment_string[1000];
    strcpy(environment_string,"LIBMYSQL_PLUGIN_DIR=");
    strcat(environment_string,mym_directory);
    #ifdef _WINDOWS        
        _putenv(environment_string);
    #else
        putenv(environment_string);
    #endif

    // //Confirm Path
    // printf("Path:  %s\n", mym_directory); 
    // printf("LIBMYSQL_PLUGIN_DIR[getenv]:  %s\n", getenv("LIBMYSQL_PLUGIN_DIR"));

    mxDestroyArray(mym_string[0]);
    mxDestroyArray(mym_path[0]);
    mxDestroyArray(mym_fileparts[0]);
    mxDestroyArray(mym_fileparts[1]);
    mxDestroyArray(mym_fileparts[2]);
}
/**********************************************************************
 *use32bitdims():   Check if dimensions should be read as 32-bit
 *  Get MYM_USE_32BIT_DIMS environment variable
 *  Return true/false based on MYM_USE_32BIT_DIMS flag
 **********************************************************************/
static bool use32bitdims() {
    return getenv("MYM_USE_32BIT_DIMS") && strcasecmp(getenv("MYM_USE_32BIT_DIMS"), "true") == 0;
}
/**********************************************************************
 * mysql():  Execute the actual action
 * Which action we perform is based on the first input argument,
 * which must be present and must be a character string:
 *   'open', 'close', 'use', 'status', or a legitimate MySQL query.
 *  This version does not permit binary characters in query string,
 * since the query is converted to a C null-terminated string.
 *  If no output argument is given, then information is displayed.
 * If an output argument is given, then we operate silently, and
 * return status information.
 **********************************************************************/
void mexFunction(int nlhs, mxArray*plhs[], int nrhs, const mxArray*prhs[]) {
    int cid = 0;   // ID number of the current connection
    int jarg = 0;  // Number of first string arg: becomes 1 if id is specified
    /*********************************************************************/
    // show GPL license
    /*if (!runonce) {
        runonce = true;
        mexPrintf("Matlab<->MySQL connector v1.36\n");
    }*/

    // Set numeric locale to English (US), such that '.' is used as decimal point
    //char* lcOldNumeric = setlocale(LC_NUMERIC, "english-us");
    // ***********

    // Parse the first argument to see if it is a specific id number
    if ((nrhs!=0) && mxIsNumeric(prhs[0]))  {
        if ((mxGetM(prhs[0])!=1) || (mxGetN(prhs[0])!=1)) {
            mexPrintf("Usage:  %s([id], command, [ host, user, password ])\n", mexFunctionName());
            mexPrintf("First argument is array %d x %d\n", mxGetM(prhs[0]), mxGetN(prhs[0]));
            mexErrMsgTxt("Invalid connection ID");
        }
        double xid = *mxGetPr(prhs[0]);
        cid = int(xid);
        if ((double(cid)!=xid) || (cid<-1) || (cid>=MAXCONN)) {
            mexPrintf("Usage:  %s([id], command, [ host, user, password ])\n", mexFunctionName());
            mexPrintf("id = %g -- Must be integer between 0 and %d\n", xid, MAXCONN-1);
            mexErrMsgTxt("Invalid connection ID");
        }
        jarg++;
    }
    if (debug)
        mexPrintf("cid = %d  jarg = %d\n", cid, jarg);
    /*********************************************************************/
    //  Parse the result based on the first argument
    enum querytype { OPEN, CLOSE, CLOSE_ALL, USE, STATUS, CMD, SERIALIZE, DESERIALIZE, VERSION } q;
    char* query = NULL;
    if (nrhs<=jarg)
        q = STATUS;
    else {
        if (!mxIsChar(prhs[jarg]))
            mexErrMsgTxt("The command string is missing!");
        query = getstring(prhs[jarg]);
        if (!strcasecmp(query, "open"))
            q = OPEN;
        else if (!strcasecmp(query, "close"))
            q = CLOSE;
        else if (!strcasecmp(query, "closeall"))
            q = CLOSE_ALL;
        else if (!strcasecmp(query, "use") || !strncasecmp(query, "use ", 4))
            q = USE;
        else if (!strcasecmp(query, "status"))
            q = STATUS;
        else if (!strcasecmp(query, "version"))
            q = VERSION;
        else if (strcasestr(query, "deserialize") != NULL)
            q = DESERIALIZE;
        else if (strcasestr(query, "serialize") != NULL)
            q = SERIALIZE;
        else
            q = CMD;
    }
    //  Check that the arguments are all character strings
    if ((q!=CMD) & (q!=SERIALIZE) & (q!=DESERIALIZE))
        for (int j = jarg; j<nrhs; j++) {
            if (!mxIsChar(prhs[j])) {
                mexPrintf("Usage:  %s([id], command, [ host, user, password ])\n", mexFunctionName());
                mexErrMsgTxt("All args must be strings, except id");
            }
        }
    if (debug){
        if (q==OPEN)
            mexPrintf("q = OPEN\n");
        if (q==CLOSE)
            mexPrintf("q = CLOSE\n");
        if (q==USE)
            mexPrintf("q = USE\n");
        if (q==STATUS)
            mexPrintf("q = STATUS\n");
        if (q==VERSION)
            mexPrintf("q = VERSION\n");
        if (q==SERIALIZE)
            mexPrintf("q = SERIALIZE\n");
        if (q==DESERIALIZE)
            mexPrintf("q = DESERIALIZE\n");
        if (q==CMD)
            mexPrintf("q = CMD\n");
    }
    // found the correct cid, if automatic scan is requested
    if (cid==-1) {
        if (q!=OPEN)
            mexErrMsgTxt("automatic free connection scan, only when issuing a mYm open command!");
        cid = 0;
        while ((cid<MAXCONN) && c[cid].isopen)
            ++cid;
        if (cid==MAXCONN)
            mexErrMsgTxt("no free connection ID");
    }
    // Shorthand notation now that we know which id we have
    typedef MYSQL* mp;
    mp &conn = c[cid].conn;
    bool &isopen = c[cid].isopen;
    if (q==OPEN) {
        // OPEN A NEW CONNECTION
        ////////////////////////
        //  Close connection if it is open
        if (isopen) {
            mysql_close(conn);
            isopen = false;
            conn = NULL;
        }
        //  Extract information from input arguments
        char*host = NULL;
        if (nrhs>=(jarg+2))
            host = getstring(prhs[jarg+1]);
        char*user = NULL;
        if (nrhs>=(jarg+3))
            user = getstring(prhs[jarg+2]);
        char*pass = NULL;
        if (nrhs>=(jarg+4))
            pass = getstring(prhs[jarg+3]);
        int port = hostport(host);  // returns zero if there is no port
        //  Establish and test the connection
        //  If this fails, then conn is still set, but isopen stays false
        if (!(conn = mysql_init(conn)))
            mexErrMsgTxt("Couldn\'t initialize MySQL connection object");

        const char*ssl_input = "none";
        int mode_option = SSL_MODE_PREFERRED;

        if (nrhs>=(jarg+5))
            ssl_input = getstring(prhs[jarg+4]);

        if (!strcasecmp(ssl_input, "true")) {
            ssl_input = "true";
            mode_option = SSL_MODE_REQUIRED;
        }
        else if (!strcasecmp(ssl_input, "false")) {
            ssl_input = "false";
            mode_option = SSL_MODE_DISABLED;
        }
        else if (strstr (ssl_input,"{")) {
            mode_option = SSL_MODE_REQUIRED;
            mexErrMsgIdAndTxt("mYm:TLS:InvalidStruct",
                "Custom TLS struct definition not supported yet.");
        }

        mysql_options(conn, MYSQL_OPT_SSL_MODE, &mode_option);

        if (nlhs<1) {
            mexPrintf("Connecting to  host = %s", (host) ? host : "localhost");
            if (port)
                mexPrintf("  port = %d", port);
            if (user)
                mexPrintf("  user = %s", user);
            if (pass)
                mexPrintf("  password = %s", pass);
            mexPrintf("  ssl = %s", ssl_input);
            mexPrintf("\n");
        }

        updateplugindir();
        //my_bool  my_true = true;
        //mysql_options(conn, MYSQL_OPT_RECONNECT, &my_true);
        if (!mysql_real_connect(conn, host, user, pass, NULL, port, NULL, CLIENT_MULTI_STATEMENTS))
            mexErrMsgIdAndTxt("MySQL:Error",
                mysql_error(conn));
        const char*c = mysql_stat(conn);
        if (c) {
            if (nlhs<1)
                mexPrintf("%s\n", c);
        }
        else
            mexErrMsgTxt(mysql_error(conn));
        isopen = true;
        
        //  Now we are OK -- if he wants output, give him the cid
        if (nlhs>=1) {
            if (!(plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL)))
                mexErrMsgTxt("Unable to create matrix for output");
            *mxGetPr(plhs[0]) = cid;
        }
    }
    else if (q==CLOSE) {
        // CLOSE CONNECTION
        ////////////////////////
        if (isopen) {
            // only if open
            mysql_close(conn);
            isopen = false;
            conn = NULL;
        }
    }
    else if (q==CLOSE_ALL) {
        // CLOSE ALL CONNECTIONS
        ////////////////////////
        for (int i = 0; i<MAXCONN; i++)
            if (c[i].isopen) {
                mysql_close(c[i].conn);
                c[i].conn = NULL;
                c[i].isopen = false;
            }
    }
    else if (q==USE) {
        // SPECIFY DATABASE
        ////////////////////////
        if (!isopen)
            mexErrMsgTxt("Not connected");
        if (mysql_ping(conn)) {
            isopen = false;
            mexErrMsgTxt(mysql_error(conn));
        }
        char*db = NULL;
        if (!strcasecmp(query, "use")) {
            if (nrhs>=2)
                db = getstring(prhs[jarg+1]);
            else
                mexErrMsgTxt("Must specify a database to use");
        }
        else if (!strncasecmp(query, "use ", 4)) {
            db = query+4;
            while (*db==' ' ||*db=='\t')
                db++;
        }
        else
            mexErrMsgTxt("How did we get here?  Internal logic error!");
        if (mysql_select_db(conn, db))
            mexErrMsgTxt(mysql_error(conn));
        if (nlhs<1)
            mexPrintf("Current database is \"%s\"\n", db);
        else {
            // he wants some output, give him a 1
            if (!(plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL)))
                mexErrMsgTxt("Unable to create matrix for output");
            *mxGetPr(plhs[0]) = 1.;
        }
    }
    else if (q==STATUS) {
        // GET CONNECTION STATUS
        ////////////////////////
        const char*ssl_status;
        if (nlhs<1) {
            //  He just wants a report
            if (jarg>0) {
                //  mysql(cid, 'status')  Give status of the specified id
                if (!isopen) {
                    mexPrintf("%2d: Not connected\n", cid);
                    return;
                }
                if (mysql_ping(conn)) {
                    isopen = false;
                    mexErrMsgTxt(mysql_error(conn));
                }
                if (mysql_get_ssl_cipher(conn)) {ssl_status = "(encrypted)";} else ssl_status = "";
                mexPrintf("%2d:  %-30s   Server version %s %s\n", cid, mysql_get_host_info(conn), 
                    mysql_get_server_info(conn), ssl_status);
            }
            else {
                //  mysql('status') with no specified connection
                int nconn = 0;
                for (int j = 0; j<MAXCONN; j++)
                    if (c[j].isopen)
                        nconn++;
                if (debug)
                    mexPrintf("%d connections open\n", nconn);
                if (nconn==0) {
                    mexPrintf("No connections open\n");
                    return;
                }
                if ((nconn==1) && (c[0].isopen)) {
                    // Only connection number zero is open
                    // Give simple report with no connection id #
                    if (mysql_ping(conn)) {
                        isopen = false;
                        mexErrMsgTxt(mysql_error(conn));
                    }
                    if (mysql_get_ssl_cipher(conn)) {ssl_status = "(encrypted)";} else ssl_status = "";
                    mexPrintf("Connected to %s Server version %s Client %s %s\n", mysql_get_host_info(conn), 
                        mysql_get_server_info(conn), mysql_get_client_info(), ssl_status);
                }
                else {
                    // More than one connection is open
                    // Give a detailed report of all open connections
                    for (int j = 0; j<MAXCONN; j++) {
                        if (c[j].isopen) {
                            if (mysql_ping(c[j].conn)) {
                                c[j].isopen = false;
                                mexPrintf("%2d:  %s\n", mysql_error(c[j].conn));
                                continue;
                            }
                            if (mysql_get_ssl_cipher(c[j].conn)) {ssl_status = "(encrypted)";} else ssl_status = "";
                            mexPrintf("%2d: %-30s Server version %s %s\n", j, mysql_get_host_info(c[j].conn), 
                                mysql_get_server_info(c[j].conn), ssl_status);
                        }
                    }
                }
            }
        }
        else {
            //  He wants a return value for this connection
            if (!(plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL)))
                mexErrMsgTxt("Unable to create matrix for output");
            double *pr = mxGetPr(plhs[0]);
            *pr = 0.;
            if (!isopen)
                *pr = 1.;
            //return;
            else if (mysql_ping(conn)) {
                isopen = false;
                *pr = 2.;
                //return;
            }
            else if (!mysql_stat(conn)) {
                isopen = false;
                *pr = 3.;
                //return;
            }
        }
    }
    else if (q==CMD) {
        // ISSUE A MYSQL COMMAND
        ////////////////////////
        //  Check that we have a valid connection
        if (!isopen)
            mexErrMsgTxt("Not connected");
        if (mysql_ping(conn)) {
            isopen = false;
            mexErrMsgTxt(mysql_error(conn));
        }
        //******************PLACEHOLDER PROCESSING******************
        // global placeholders variables and constant
        const unsigned expectedNumberOfPlaceholders = nrhs-jarg-1;   // expected number of placeholders
        unsigned query_flags = 0;
        unsigned nb_flags = 0;
        unsigned nac = 0;                                   // actual number
        size_t lengthOfQuery = strlen(query);  // Length of the query which is needed for mysql_real_query later
        // Check for presence of query flags. This needs to be expanded once we have additional flags
        for (int i=nrhs-1; i > jarg; --i) {
            if (mxIsChar(prhs[i]) && (strcasecmp(getstring(prhs[i]), ML_FLAG_BIGINT_TO_DOUBLE)==0)) {
                ++nb_flags;
            }
            else {
                break;
            }
        }

        // If there is placeholders, parse them accordingly
        if (expectedNumberOfPlaceholders) {
            // local placeholders variables and constant
            char** po = 0;                              // pointer to placeholders openning symbols
            char** pc = 0;                              // pointer to placeholders closing symbols
            bool*  pec = 0;               // pointer to 'enable compression field'
            char** pa = 0;                              // pointer to placeholder in-string argument
            unsigned* ps = 0;                           // pointer to placeholders size (in bytes)
            pfserial* pf = 0;                           // pointer to serialization function
            // LOOK FOR THE PLACEHOLDERS
            po = (char**)mxCalloc(expectedNumberOfPlaceholders+1, sizeof(char*));
            pc = (char**)mxCalloc(expectedNumberOfPlaceholders+1, sizeof(char*));
            if ((po[nac++] = strstr(query, PH_OPEN)))
                while (po[nac-1]&&nac<=expectedNumberOfPlaceholders) {
                    pc[nac-1] = strstr(po[nac-1]+1, PH_CLOSE);
                    if (pc[nac-1]==0)
                        mexErrMsgTxt("Placeholders are not correctly closed!");
                    po[nac] = strstr(pc[nac-1]+1, PH_OPEN);
                    nac++;
                }
            nac--;
            // Adjust placeholders based on the number of flags present
            if ((nac < expectedNumberOfPlaceholders) && (nac >= (expectedNumberOfPlaceholders-nb_flags))) {
                nb_flags = expectedNumberOfPlaceholders-nac;
            }
            else if (nac != expectedNumberOfPlaceholders) {
                mexErrMsgTxt("The number of placeholders differs from that of additional arguments!");
            }
            // now we have the correct number of placeholders
            // read the types and in-string arguments
            ps = (unsigned*)mxCalloc(nac, sizeof(unsigned));
            pa = (char**)mxCalloc(nac, sizeof(char*));
            pec = (bool*)mxCalloc(nac, sizeof(bool));
            pf = (pfserial*)mxCalloc(nac, sizeof(pfserial));
            for (unsigned i = 0; i<nac; i++) {
                // placeholder function+control ph type+control arg type
                getSerialFct(po[i]+strlen(PH_OPEN), prhs[jarg+i+1], pf[i], pec[i]);
                // placeholder size in bytes
                ps[i] = (unsigned)(pc[i]-po[i]+strlen(PH_OPEN));
                // placeholder in-string argument
                pa[i] = po[i]+strlen(PH_OPEN)+1;
                *(pc[i]) = 0;
                // we can add a termination character in the query,
                // it will be processed anyway and we already have the original query length
            }
            // BUILD THE NEW QUERY
            // serialize
            char** pd = (char**)mxCalloc(nac, sizeof(char*));
            size_t* plen = (size_t*)mxCalloc(nac, sizeof(size_t));
            uLongf cmp_buf_len = 10000;
            Bytef* pcmp = (Bytef*)mxCalloc(cmp_buf_len, sizeof(Bytef));
            size_t nb = 0;
            for (unsigned i = 0; i<nac; i++) {
                // serialize individual field
                pd[i] = pf[i](plen[i], prhs[jarg+i+1], pa[i], true);
                if (pec[i] && (plen[i]>MIN_LEN_ZLIB)) {
                    // compress field
                    const uLongf max_len = compressBound(plen[i]);
                    if (max_len>cmp_buf_len){
                        pcmp = (Bytef*)mxRealloc(pcmp, max_len);
                        cmp_buf_len = max_len;
                    }
                    uLongf len = cmp_buf_len;
                    if (compress(pcmp, &len, (Bytef*)pd[i], plen[i])!=Z_OK)
                        mexErrMsgTxt("Compression failed");
                    const float cmp_rate = plen[i]/(LEN_ZLIB_ID+1.f+sizeof(_uint64)+len);
                    if (cmp_rate>MIN_CMP_RATE_ZLIB) {
                        const size_t len_old = plen[i];
                        plen[i] = LEN_ZLIB_ID+1+sizeof(_uint64)+len;
                        pd[i] = (char*)mxRealloc(pd[i], plen[i]);
                        memcpy(pd[i], (const char*)ZLIB_ID, LEN_ZLIB_ID);
                        *(pd[i]+LEN_ZLIB_ID) = 0;

                        *((_uint64*)(pd[i]+LEN_ZLIB_ID+1)) = (_uint64) len_old;
                        memcpy(pd[i]+LEN_ZLIB_ID+1+sizeof(_uint64), pcmp, len);
                        //BUG: *((_uint64*)(pd[i]+LEN_ZLIB_ID+1+sizeof(_uint64))) = (_uint64) len;
                    }
                }
                nb += plen[i];
            }
            // create new query
            char* nq = (char*)mxCalloc(2*nb+lengthOfQuery+1, sizeof(char));  // new query
            char* pnq = nq; // running pointer to new query
            const char* poq = query; // running pointer to old query
            for (unsigned i = 0; i<nac; i++) {
                memcpy(pnq, poq, po[i]-poq);
                pnq += po[i]-poq;
                poq = po[i]+ps[i];
                pnq += mysql_real_escape_string(conn, pnq, pd[i], plen[i]);
                pd[i]=NULL;
                mxFree(pd[i]);
            }
            memcpy(pnq, poq, lengthOfQuery-(poq-query)+1);
            // replace the old query by the new one
            pnq += lengthOfQuery-(poq-query)+1;
            mxFree(query);
            query = nq;
            lengthOfQuery = (ulong)(pnq-nq);
            mxFree(po);
            mxFree(pc);
            mxFree(pec);
            mxFree(ps);
            mxFree(pa);
            mxFree(pd);
            mxFree(plen);
            if (pcmp!=NULL)
                mxFree(pcmp);
        }
        
        // Remove any white spaces at the beginning (NOTE for some reason mxFree crashes if this runs before it gets to the CMD block)
        removeWhiteSpaceAtTheBeginning(query);
        lengthOfQuery = strlen(query);

        // Check if it is Create or Alter, if so then do curely bracket parsing
        if (isSubstringFountAtTheBeginningCaseInsenstive(query, "CREATE") || isSubstringFountAtTheBeginningCaseInsenstive(query, "ALTER"))
        {
             // Check that no placeholders are present in the query except for \{ and \}
            char* parsedQueryString = (char*)mxCalloc(strlen(query), sizeof(char)); // Query string with escape character removed
            size_t parsedQueryStringCurrentIndex = 0;

            // Loop through the query string character by character
            for (size_t i = 0; query[i] != '\0'; i++) {
                
                if ((query[i] == '{' || query[i] == '}') && i != 0) {
                    if (query[i - 1] != '\\') {
                        // Curley bracket doesn't seem to be escaped thus throw and error
                        mxFree(parsedQueryString);
                        mexErrMsgTxt("The query contains placeholders, but no additional arguments!");
                    }
                    else {
                        // Valid curley bracket thus add it to the parsedQueryString with the \ removed
                        parsedQueryStringCurrentIndex--;
                        parsedQueryString[parsedQueryStringCurrentIndex] = query[i];
                    }
                }
                else {
                    // Non curley bracket thus add it to the parsedQueryString
                    parsedQueryString[parsedQueryStringCurrentIndex] = query[i];
                }

                // Increment the currentIndex counter
                parsedQueryStringCurrentIndex++;
            }
            // Add ending character
            parsedQueryString[parsedQueryStringCurrentIndex] = '\0';

            // Update the query string
            mxFree(query); // Free up the old string allocation
            query = parsedQueryString;
            lengthOfQuery = strlen(query); // Update the length of the query
            
        }
       
        // Process flags
        for (int i=nrhs-nb_flags; i < nrhs; ++i) {
            if  (strcasecmp(getstring(prhs[i]), ML_FLAG_BIGINT_TO_DOUBLE)==0) {
                query_flags |= FLAG_BIGINT_TO_DOUBLE;
            }
        }

        //  Execute the query (data stays on server)
        if (nac!=0) {
            if (mysql_real_query(conn, query, lengthOfQuery))
                mexErrMsgTxt(mysql_error(conn));
        }
        else if (mysql_query(conn, query))
            mexErrMsgTxt(mysql_error(conn));
        //  Download the data from server into our memory
        //  We need to be careful to deallocate res before returning.
        //  Matlab's allocation routines return instantly if there is not
        //  enough free space, without giving us time to dealloc res.
        //  This is a potential memory leak but I don't see how to fix it.
        MYSQL_RES*res = mysql_store_result(conn);
        //  As recommended in Paul DuBois' MySQL book (New Riders, 1999):
        //  A NULL result set after the query can indicate either
        //  (1) the query was an INSERT, DELETE, REPLACE, or UPDATE, that
        //    affect rows in the table but do not return a result set; or
        //  (2) an error, if the query was a SELECT, SHOW, or EXPLAIN
        //    that should return a result set but didn't.
        //  Distinguish between the two by checking mysql_field_count()
        //  We return in either case, either correctly or with an error
        if (!res) {
            if (!mysql_field_count(conn)) {
                ulong nrows = (ulong)mysql_affected_rows(conn);
                if (nlhs<1)
                    return;
                else {
                    plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
                    if (plhs[0]==0)
                        mexErrMsgTxt("Unable to create numeric matrix for output");
                    *mxGetPr(plhs[0]) = (double) nrows;
                    return;
                }
            }
            else
                mexErrMsgTxt(mysql_error(conn));
        }
        ulong nrow = (ulong)mysql_num_rows(res), nfield = mysql_num_fields(res);
        //  If he didn't ask for any output (nlhs = 0),
        //     then display the output and return
        if (nlhs<1) {
            fancyprint(res);
            mysql_free_result(res);
            return;
        }
        //  If we are here, he wants output
        //  He must give exactly the right number of output arguments
        if (nlhs!=1) {
            mysql_free_result(res);
            mexPrintf("You specified %d output arguments, and got %d columns of data\n", nlhs, nfield);
            mexErrMsgTxt("Must give one output argument");
        }
        //  Fix the column types to fix MySQL C API sloppiness
        MYSQL_FIELD*f = mysql_fetch_fields(res);
        //fix_types(f, res);
        //  Create field list
        const char **fieldnames=(const char **)mxMalloc(nfield*sizeof(char*));
        //
        for (ulong j=0; j<nfield;j++)
        {
            fieldnames[j]=f[j].name;
        }
        //  Create the Matlab structure for output
        
        if (!(plhs[0]=mxCreateStructMatrix(1,1,nfield,fieldnames)))
        {
            mysql_free_result(res);
            mexErrMsgTxt("Unable to create structure for output");
        }
        mxFree(fieldnames);

        // Create temporary mxArray pointer array
        mxArray* tmpPr;

        //  Create the Matlab arrays for output        
        double **pr = (double**) mxMalloc(nfield*sizeof(double*));
        _int64 **i_pr = (_int64**) mxMalloc(nfield*sizeof(_int64*));

        //  Create the individual output field arrays
        for (ulong j = 0; j<nfield; j++) {
            if ((f[j].type == FIELD_TYPE_LONGLONG) && !(query_flags & FLAG_BIGINT_TO_DOUBLE)){
                const mwSize ndim = 2;
                const mwSize dims[2] = {nrow, 1};
                mxClassID integer_class = (f[j].flags & UNSIGNED_FLAG) ? mxUINT64_CLASS : mxINT64_CLASS;
                if( !(tmpPr = mxCreateNumericArray(ndim, dims, integer_class, mxREAL)) ) {
                    mysql_free_result(res);
                    mexErrMsgTxt("Unable to create numeric array for output");
                }
                i_pr[j] = (_int64*) mxGetData(tmpPr);
            } else if (can_convert(f[j].type)) {
                if (!(tmpPr = mxCreateDoubleMatrix(nrow, 1, mxREAL))) {
                    mysql_free_result(res);
                    mexErrMsgTxt("Unable to create numeric matrix for output");
                }
                pr[j] = mxGetPr(tmpPr);
            }
            else {
                // This case also handles BLOBs
                if (!(tmpPr = mxCreateCellMatrix(nrow, 1))) {
                    mysql_free_result(res);
                    mexErrMsgTxt("Unable to create cell matrix for output");
                }
                pr[j]=NULL;
            }
            mxSetField(plhs[0],0,f[j].name,tmpPr);
        }
        //  Load the data into the cells
        mysql_data_seek(res, 0);
        for (ulong i = 0; i<nrow; i++) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (!row) {
                mexPrintf("Scanning row %d for data extraction\n", i+1);
                mexErrMsgTxt("Internal error:  Failed to get a row");
            }
            unsigned long *p_lengths = mysql_fetch_lengths(res);
            for (ulong j = 0; j<nfield; j++) {
                if ((f[j].type == FIELD_TYPE_LONGLONG) && !(query_flags & FLAG_BIGINT_TO_DOUBLE)) {
                    field2int(row[j], f[j].type, f[j].flags, &i_pr[j][i]);
                } else if (can_convert(f[j].type)) {
                    pr[j][i] = field2num(row[j], f[j].type);
                } else if ((f[j].type==FIELD_TYPE_BLOB) && (f[j].charsetnr==63)) {
                    tmpPr=mxGetField(plhs[0],0,f[j].name);
                    mxSetCell(tmpPr, i, deserialize(row[j], p_lengths[j]));
                } else if ((f[j].type==FIELD_TYPE_STRING) && (f[j].flags & BINARY_FLAG)) {
                    tmpPr=mxGetField(plhs[0],0,f[j].name);
                    mxArray *c;
                    const uint8_t *p = (uint8_t *)row[j];
                    c = mxCreateNumericMatrix (1, p_lengths[j], mxUINT8_CLASS, mxREAL);
                    uint8_t *vro = (uint8_t*)mxGetData(c);
                    for (int k=0; k < p_lengths[j]; k++) {
                        vro[k] = p[k];
                    }
                    mxSetCell(tmpPr, i, c);
                } else {
                    tmpPr=mxGetField(plhs[0],0,f[j].name);
                    mxArray *c = mxCreateString(hex2char(row[j], p_lengths[j]));
                    mxSetCell(tmpPr, i, c);
                }
            }
        }
        mxFree(pr);
        mxFree(i_pr);
        mysql_free_result(res);
    }
    else if (q==SERIALIZE) {
        if (debug)
            mexPrintf("In Serialize...\n");
        // serialize the matlab variable, it can be an array, cell, struct etc
        //******************PLACEHOLDER PROCESSING******************
        // global placeholders variables and constant
        const unsigned expectedNumberOfPlaceholders = nrhs-jarg-1;   // expected number of placeholders
        unsigned query_flags = 0;
        unsigned nb_flags = 0;
        unsigned nac = 0;                                   // actual number
        if (debug)
            mexPrintf("Num of args = %d\n", (int) expectedNumberOfPlaceholders) ;

        if (expectedNumberOfPlaceholders) {
            // local placeholders variables and constant
            char** po = 0;                      // pointer to placeholders openning symbols
            char** pc = 0;                      // pointer to placeholders closing symbols
            bool*  pec = 0;                     // pointer to 'enable compression field'
            char** pa = 0;                      // pointer to placeholder in-string argument
            unsigned* ps = 0;                   // pointer to placeholders size (in bytes)
            pfserial* pf = 0;                   // pointer to serialization function
            // LOOK FOR THE PLACEHOLDERS
            po = (char**)mxCalloc(expectedNumberOfPlaceholders+1, sizeof(char*));
            pc = (char**)mxCalloc(expectedNumberOfPlaceholders+1, sizeof(char*));
            if ((po[nac++] = strstr(query, PH_OPEN)))
            while (po[nac-1]&&nac<=expectedNumberOfPlaceholders) {
                pc[nac-1] = strstr(po[nac-1]+1, PH_CLOSE);
                if (pc[nac-1]==0)
                    mexErrMsgIdAndTxt("mYm:Serialization:Placeholders",
                        "Placeholders are not correctly closed!");
                po[nac] = strstr(pc[nac-1]+1, PH_OPEN);
                nac++;
            }
            nac--; // utilized to find {}s
            if (nac != expectedNumberOfPlaceholders) {
                mexErrMsgIdAndTxt("mYm:Serialization:Placeholders",
                    "The number of placeholders differs from that of additional arguments!");
            }

            // output of serialized variable, for each placeholder
            char** pd = (char**)mxCalloc(nac, sizeof(char*));
            // length of bytes in the serialized variable, for each place holder
            size_t* plen = (size_t*)mxCalloc(nac, sizeof(size_t));

            // now we have the correct number of placeholders
            // read the types and in-string arguments
            ps = (unsigned*)mxCalloc(nac, sizeof(unsigned));
            pa = (char**)mxCalloc(nac, sizeof(char*));
            pec = (bool*)mxCalloc(nac, sizeof(bool));
            pf = (pfserial*)mxCalloc(nac, sizeof(pfserial));
            for (unsigned i = 0; i<nac; i++) {
                // placeholder function+control ph type+control arg type
                // first parameter points to the first character of placeholder
                getSerialFct(po[i]+strlen(PH_OPEN), prhs[jarg+i+1], pf[i], pec[i]);
                // placeholder size in bytes
                ps[i] = (unsigned)(pc[i]-po[i]+strlen(PH_OPEN));
                // placeholder in-string argument
                pa[i] = po[i]+strlen(PH_OPEN)+1; // e.g. i in {Si}
                *(pc[i]) = 0;
                // we can add a termination character in the query,
                // it will be processed anyway and we already have the original query length
            }
            if (debug)
                mexPrintf("Num of vars = %d\n", (int) nac);

            // serialize
            uLongf cmp_buf_len = 10000;
            Bytef* pcmp = (Bytef*)mxCalloc(cmp_buf_len, sizeof(Bytef));
            size_t nb = 0;
            for (unsigned i = 0; i<nac; i++) {
                // serialize individual field
                pd[i] = pf[i](plen[i], prhs[jarg+i+1], pa[i], true);
                if (debug)
                    mexPrintf("Length of serial string = %d\n", plen[i]) ;
                if (pec[i] && (plen[i]>MIN_LEN_ZLIB)) { // compress if needed
                    // compress field
                    const uLongf max_len = compressBound(plen[i]);
                    if (max_len>cmp_buf_len){
                        pcmp = (Bytef*)mxRealloc(pcmp, max_len);
                        cmp_buf_len = max_len;
                    }
                    uLongf len = cmp_buf_len;
                    if (compress(pcmp, &len, (Bytef*)pd[i], plen[i])!=Z_OK)
                        mexErrMsgIdAndTxt("mYm:Serialization:Compression",
                            "Compression failed");
                    const float cmp_rate = plen[i]/(LEN_ZLIB_ID+1.f+sizeof(_uint64)+len);
                    if (cmp_rate>MIN_CMP_RATE_ZLIB) {
                        const size_t len_old = plen[i];
                        plen[i] = LEN_ZLIB_ID+1+sizeof(_uint64)+len;
                        pd[i] = (char*)mxRealloc(pd[i], plen[i]);
                        memcpy(pd[i], (const char*)ZLIB_ID, LEN_ZLIB_ID);
                        *(pd[i]+LEN_ZLIB_ID) = 0;

                        *((_uint64*)(pd[i]+LEN_ZLIB_ID+1)) = (_uint64) len_old;
                        memcpy(pd[i]+LEN_ZLIB_ID+1+sizeof(_uint64), pcmp, len);
                        //BUG: *((_uint64*)(pd[i]+LEN_ZLIB_ID+1+sizeof(_uint64))) = (
                        //          _uint64) len;
                    }
                }
                nb += plen[i];
            }
            mxFree(query);
            mxFree(po);
            mxFree(pc);
            mxFree(pec);
            mxFree(ps);
            mxFree(pa);
            if (pcmp!=NULL)
                mxFree(pcmp);

    //      return the serialized items as cell array of unsigned byte vectors
            if (nlhs > 0) {
                mxArray *cell_array_ptr ;
                mxArray *vec ;
                cell_array_ptr = mxCreateCellMatrix(nac,1);
                if (cell_array_ptr != NULL) {
                    for (unsigned i=0; i<nac; i++){
                        vec = mxCreateNumericMatrix(plen[i],1,mxUINT8_CLASS,mxREAL) ;
                        if (vec != NULL) {
                            memcpy(mxGetPr(vec), pd[i], plen[i]) ;
                            mxFree(pd[i]) ;
                            mxSetCell(cell_array_ptr,i,vec);
                        }
                        else {
                            mexErrMsgIdAndTxt("mYm:Serialization:MemoryAllocation",
                                "Unable to allocate memory for serialized output of a "
                                "variable\n");
                        }
                    }    
                    mxFree(pd);
                    mxFree(plen);
                    plhs[0] = cell_array_ptr; 
                }
                else {
                    mexErrMsgIdAndTxt("mYm:Serialization:CellAllocation",
                        "Unable to allocate cell matrix for output variables\n");
                }
            }
        }
    }
    else if (q==DESERIALIZE) {
        if ((nlhs == 1) && (nrhs == 2)) {
            // the 2nd input argument is a pointer to the matlab variable containing 
            // serialized data
            const mwSize* input_dims = mxGetDimensions(prhs[1]);
            plhs[0] = deserialize((const char *) mxGetPr(prhs[1]),
                (const size_t) std::max<size_t>(input_dims[0], input_dims[1]));
        }
        else {
            mexErrMsgIdAndTxt("mYm:Deserialization:Mismatch",
                "There must be only one input and one output variable\n");
        }
    }
    else if (q==VERSION) {
        if (nrhs > (jarg+1))
            mexErrMsgTxt("Version command does not take additional inputs");
        if (nlhs == 0) {
            mexPrintf("mYm version %d.%d.%d\n", MYM_VERSION_MAJOR, MYM_VERSION_MINOR,
                MYM_VERSION_BUGFIX);
        } else if (nlhs == 1) {
            const char* versionFields[] = {"major", "minor", "bugfix"};
            plhs[0] = mxCreateStructMatrix(1, 1, sizeof(versionFields) / sizeof(*versionFields),
                versionFields);
            mxSetFieldByNumber(plhs[0], 0, 0, mxCreateDoubleScalar(MYM_VERSION_MAJOR));
            mxSetFieldByNumber(plhs[0], 0, 1, mxCreateDoubleScalar(MYM_VERSION_MINOR));
            mxSetFieldByNumber(plhs[0], 0, 2, mxCreateDoubleScalar(MYM_VERSION_BUGFIX));
        } else {
            mexErrMsgTxt("Zero or one output arguments allowed for version command");
        }
    }
    else {
        mexPrintf("Unknown query type q = %d\n", q);
        mexErrMsgTxt("Internal code error");
    }
}

void removeWhiteSpaceAtTheBeginning(char* string) 
{
    // Do a quick check at the start to see if any work needs to be done
    if (!strlen(string) || !isspace(string[0])) 
    {
        // The first character is not an empty space thus just return the orignal string
        return;
    }

    // There are some white spaces that needs to be removed
    size_t currentStringIndex = 1; // Offset of 1 due to the check at the beginning

    // Loop through the string until a none whitespace character is found
    for (; string[currentStringIndex] != '\0'; currentStringIndex++) 
    {
        if (string[currentStringIndex] != ' ') 
        {
            break;
        }
    }

    char* sanitizeString = (char*)mxCalloc(strlen(string), sizeof(char));
    size_t sanitizeStringIndex = 0;
    // Copy the rest of the string over
    for (; string[currentStringIndex] != '\0'; currentStringIndex++) 
    {
        sanitizeString[sanitizeStringIndex] = string[currentStringIndex];
        sanitizeStringIndex++;
    }

    // Replace the string
    mxFree(string); // Free up the old string allocation
    string = sanitizeString;
}

/**
 * Helper function for checking if string subString is found at the start of string sourceString
 * @param sourceString Source string to find subString at the start
 * @param subString String target to look at the beginning of sourceString
 * @returns Boolean answer if subString was found at the start of sourceString
 */
bool isSubstringFountAtTheBeginningCaseInsenstive(const char* sourceString, const char* subString)
{
    for (size_t i = 0; subString[i] != '\0'; i++) 
    {
        if (tolower(sourceString[i]) != tolower(subString[i]))
        {
            return false;
        }
    }
    return true;
}

/*************************************SERIALIZE******************************************/
// Serialize a structure array
char* serializeStruct(size_t &rnBytes, const mxArray *rpArray, const char *rpArg, const bool rhead) 
{
    // number of dimensions
    mwSize ndims = mxGetNumberOfDimensions(rpArray);
    rnBytes = 1+sizeof(_uint64)+ndims*sizeof(_uint64);
    if (rhead)
        rnBytes += (ulong)LEN_ID_MATLAB+1;

    // get number of fields and their names
    const int nfields = mxGetNumberOfFields(rpArray);
    rnBytes += sizeof(int);
    const char** pname = (const char**)mxCalloc(nfields, sizeof(char*));
    int* fnl = (int*)mxCalloc(nfields, sizeof(int));
    for (int i = 0; i<nfields; i++) {
        pname[i] = mxGetFieldNameByNumber(rpArray, i);
        fnl[i] = (int)strlen(pname[i]);
        rnBytes += fnl[i]+1;
    }

    // loop through each element of the array and serialize it
    const mwSize nelts = mxGetNumberOfElements(rpArray);
    char** pser = (char**)mxCalloc(nelts*nfields, sizeof(char*));
    size_t* plen = (size_t*)mxCalloc(nelts*nfields, sizeof(size_t));
    const mxArray* pf;
    unsigned li = 0;
    for (int i = 0; i<nelts; i++) {
        for (int j = 0; j<nfields; j++) {
            bool isWeCreatedAnEmptyMatrix=false;
            mxArray* fakeEmptyMatrix;
            pf = mxGetFieldByNumber(rpArray, i, j);
            if (pf==NULL)
            {
                // sometimes, matlab returns null, for good and empty
                // elements. the reasons for this are unknown
                // so we create an empty double matrix, to overcome it
                fakeEmptyMatrix=mxCreateDoubleMatrix((mwSize)0,(mwSize)0,mxREAL);
                pf=fakeEmptyMatrix;
                isWeCreatedAnEmptyMatrix=true;
            }
            if (mxIsSparse(pf)) {
                pser[li] = serializeSparse(plen[li], pf, rpArg, false);
            } else if (mxIsNumeric(pf) || mxIsLogical(pf) || mxIsChar(pf)) {
                pser[li] = serializeArray(plen[li], pf, rpArg, false);
            } else if (mxIsStruct(pf)) {
                pser[li] = serializeStruct(plen[li], pf, rpArg, false);
            } else if (mxIsCell(pf)) {
                pser[li] = serializeCell(plen[li], pf, rpArg, false);
            } else {
                mexErrMsgTxt("unsupported type");
            }
            rnBytes += plen[li]+sizeof(_uint64);
            li++;
            if (isWeCreatedAnEmptyMatrix)
                mxDestroyArray(fakeEmptyMatrix);
        }
    }
    
    // put together the partial serializations
    char* p_serial = (char*)mxCalloc(rnBytes, sizeof(char));
    char* p_work = p_serial;
    if (rhead) {
        memcpy(p_work, ID_MATLAB, sizeof(char)*LEN_ID_MATLAB);
        p_work += LEN_ID_MATLAB;
        *(p_work++) = 0;
    }
    *(p_work++) = ID_STRUCT;

    // number of dimensions
    WRITE_UINT64(p_work, ndims);

    // length of each dimension
    const mwSize* pdims = mxGetDimensions(rpArray);
    WRITE_UINT64S(p_work, pdims, ndims);

    // number of fields
    memcpy(p_work, (const char*)&nfields, sizeof(const int));
    p_work += sizeof(const int);
    
    // field names
    for (int i = 0; i<nfields; i++) {
        memcpy(p_work, (const char*)pname[i], fnl[i]);
        p_work += fnl[i];
        *(p_work++) = 0;
    }

    // partial serialization
    size_t plenScalar;
    for (int i = 0; i<nfields*nelts; i++) {
        plenScalar= plen[i];
        WRITE_UINT64(p_work, plenScalar);
        memcpy(p_work, (const char*)pser[i], plenScalar);
        p_work += plen[i];
    }
    if (p_work-p_serial!=rnBytes) {
        // should never happen
        mexPrintf("serializeStruct -> memory stats: %d [rnBytes = %d]\n", p_work - p_serial, rnBytes);
        mexErrMsgTxt("Oops, I haven't allocated enough memory, BEWARE");
    }

    // free unused memory
    mxFree(pname);
    mxFree(fnl);
    for (int i = 1; i<nelts; i++)
        mxFree(pser[i]);
    mxFree(plen);
    return p_serial;
}

// Serialize a cell
char* serializeCell(size_t &rnBytes, const mxArray *rpArray, const char *rpArg, const bool rhead)
{
    // number of dimensions
    const mwSize ndims = mxGetNumberOfDimensions(rpArray);
    rnBytes = 1+(1+ndims)*sizeof(_uint64);
    if (rhead)
        rnBytes += LEN_ID_MATLAB+1;

    // loop through each element of the cell array and serialize it
    const mwSize nelts = mxGetNumberOfElements(rpArray);
    char** pser = (char**)mxCalloc(nelts, sizeof(char*));
    size_t* plen = (size_t*)mxCalloc(nelts, sizeof(size_t));
    const mxArray* pf;
    for (int i = 0; i < nelts; i++) {
        bool isWeCreatedAnEmptyMatrix=false;
        mxArray* fakeEmptyMatrix;
        pf = mxGetCell(rpArray, i);
        if (pf == NULL) 
        {   // sometimes pf null on empty elements,
            // we create a fake empty double element
            fakeEmptyMatrix=mxCreateDoubleMatrix((mwSize)0,(mwSize)0,mxREAL);
            pf=fakeEmptyMatrix;
            isWeCreatedAnEmptyMatrix=true;
        }
        if (mxIsSparse(pf)) {
            pser[i] = serializeSparse(plen[i], pf, rpArg, false);
        } else if (mxIsNumeric(pf) || mxIsLogical(pf) || mxIsChar(pf)) {
            pser[i] = serializeArray(plen[i], pf, rpArg, false);
        } else if (mxIsStruct(pf)) {
            pser[i] = serializeStruct(plen[i], pf, rpArg, false);
        } else if (mxIsCell(pf)) {
            pser[i] = serializeCell(plen[i], pf, rpArg, false);
        } else {
            mexErrMsgTxt("unsupported type");
        }
        rnBytes += plen[i]+sizeof(_uint64);
        if (isWeCreatedAnEmptyMatrix)
        {
            mxDestroyArray(fakeEmptyMatrix);
        }
    }

    // put together the partial serializations
    char* p_serial = (char*)mxCalloc(rnBytes, sizeof(char));
    char* p_work = p_serial;
    if (rhead) {
        memcpy(p_work, ID_MATLAB, sizeof(char)*LEN_ID_MATLAB);
        p_work += LEN_ID_MATLAB;
        *(p_work++) = 0;
    }
    *(p_work++) = ID_CELL;

    // number of dimensions
    WRITE_UINT64(p_work, ndims);

    // length of each dimension
    const mwSize* pdims = mxGetDimensions(rpArray);
    WRITE_UINT64S(p_work, pdims, ndims);

    // partial serialization
    size_t plenScalar;
    for (int i = 0; i<nelts; i++) {
        plenScalar=plen[i];
        WRITE_UINT64(p_work, plenScalar);
        memcpy(p_work, (const char*)pser[i], plenScalar);
        p_work  +=  plen[i];
    }

    if (p_work-p_serial != rnBytes) {
        // should never arrives here
        mexPrintf("serializeCell -> memory stats: %d [rnBytes = %d]\n", p_work - p_serial, rnBytes);
        mexErrMsgTxt("Oops, I haven't allocated enough memory, BEWARE");
    }

    // free unused memory
    for (int i = 1; i<nelts; i++)
        mxFree(pser[i]);
    mxFree(plen);
    return p_serial;
}


// Serialize a numeric or a character array
char* serializeArray(size_t &rnBytes, const mxArray *rpArray, const char *rpArg, const bool rhead) 
{
    // compute number of bytes and allocate memory
    mwSize ndims = mxGetNumberOfDimensions(rpArray);
    mxComplexity complexity = mxIsComplex(rpArray) ? mxCOMPLEX : mxREAL;
    size_t n_bytes_data;
    if (complexity != mxCOMPLEX)
        n_bytes_data = mxGetElementSize(rpArray) * mxGetNumberOfElements(rpArray);
    else
        n_bytes_data = mxGetElementSize(rpArray) * mxGetNumberOfElements(rpArray) * 2;

    size_t nb = 1 + sizeof(_uint64) + ndims*sizeof(_uint64) + sizeof(mxClassID) 
        + sizeof(mxComplexity) + n_bytes_data;

    if (rhead)
        nb +=  LEN_ID_MATLAB + 1;

    char *p_serial = (char*) mxCalloc(nb, sizeof(char));
    char *p_work = p_serial;
    if (!p_serial)
        mexErrMsgTxt("Not enough memory");

    /***SERIALIZE***/
    // the first byte is set to 'A' for Array
    if (rhead) {
        memcpy(p_work, ID_MATLAB, sizeof(char) * LEN_ID_MATLAB);
        p_work += LEN_ID_MATLAB;
        *(p_work++) = 0;
    }
    *(p_work++) = ID_ARRAY;

    // number of dimensions
    WRITE_UINT64(p_work, ndims);

    // length of each dimension
    const mwSize* pdims = mxGetDimensions(rpArray);
    WRITE_UINT64S(p_work, pdims, ndims);

    // classID
    mxClassID class_id = mxGetClassID(rpArray);
    memcpy(p_work, (const char*) &class_id, sizeof(mxClassID));
    p_work += sizeof(mxClassID);

    // complexity
    memcpy(p_work, (const char*) &complexity, sizeof(mxComplexity));
    p_work += sizeof(mxComplexity);

    // data
    if (complexity == mxCOMPLEX) {
        // real part data
        const char *pdata = (const char*) mxGetData(rpArray);
        memcpy(p_work, pdata, n_bytes_data / 2);
        p_work += n_bytes_data / 2;

        // imaginary part data (if exists)
        pdata = (const char*) mxGetImagData(rpArray);
        memcpy(p_work, pdata, n_bytes_data / 2);
        p_work += n_bytes_data / 2;
    }
    else {
        // real part data
        const char *pdata = (const char*) mxGetData(rpArray);
        memcpy(p_work, pdata, n_bytes_data);
        p_work += n_bytes_data;
    }

    // sanity check --> should nver happen
    rnBytes = (ulong)(p_work - p_serial);
    if (rnBytes > nb) {
        mexPrintf("serializeArray -> memory stats: %d [nb = %d]\n", p_work - p_serial, nb);
        mexErrMsgTxt("Oops, I haven't allocated enough memory, BEWARE");
    }

    return p_serial;
}



// Serialize a sparse matrix
char* serializeSparse(size_t &rnBytes, const mxArray *rpArray, 
                        const char *rpArg, const bool rhead) 
{
    // compute number of bytes and allocate memory
    mwSize columns = mxGetN(rpArray);
    mwSize nzmax = mxGetNzmax(rpArray);
    
    // We always use 64bit integer indices for sparse matrices
    // size of JC and IR arrays (in the file!)
    size_t n_bytes_jc = sizeof(_uint64) * (columns + 1);
    size_t n_bytes_ir = sizeof(_uint64) * nzmax;
    
    // size of data array
    size_t n_bytes_data = mxGetElementSize(rpArray) * nzmax;
    size_t n_bytes_imag = 0;
    mxComplexity complexity = mxIsComplex(rpArray) ? mxCOMPLEX : mxREAL;
    if (complexity == mxCOMPLEX) {
        n_bytes_imag = n_bytes_data;
    }
    
    // total bytes needed
    //       = ('P') + (rows, cols, nzmax) + (classID) + (complexity) + 
    //             (jc array) + (ir array) + real data    + imaginary data
    size_t nb = 1 + 3 * sizeof(_uint64) + sizeof(mxClassID) + sizeof(mxComplexity) 
        + n_bytes_jc + n_bytes_ir + n_bytes_data + n_bytes_imag;

    if (rhead)
        nb += LEN_ID_MATLAB + 1;

    // allocate memory
    char *p_serial = (char*) mxCalloc(nb, sizeof(char));
    char *p_work = p_serial;
    if (!p_serial)
        mexErrMsgTxt("Not enough memory");

    /* SERIALIZE */
    // the first byte is set to 'P' for sparse matrix
    if (rhead) {
        memcpy(p_work, ID_MATLAB, sizeof(char) * LEN_ID_MATLAB);
        p_work += LEN_ID_MATLAB;
        *(p_work++) = 0;
    }
    *(p_work++) = ID_SPARSE;

    // number of rows and columns and number of nonzero entries
    mwSize rows = mxGetM(rpArray);
    WRITE_UINT64(p_work, rows);
    WRITE_UINT64(p_work, columns);
    WRITE_UINT64(p_work, nzmax);

    // classID
    mxClassID class_id = mxGetClassID(rpArray);
    memcpy(p_work, (const char*) &class_id, sizeof(mxClassID));
    p_work += sizeof(mxClassID);

    // complexity
    memcpy(p_work, (const char*) &complexity, sizeof(mxComplexity));
    p_work += sizeof(mxComplexity);
    
    // Store index data for sparse matrices as 64 bit uints. This tries
    // to alleviate compatibility issues between 32 and 64 bit platforms in conjunction
    // with Matlab 2006b or later.
    // ********************************************************************************

    // JC and IR data
    const mwIndex* pJcData = mxGetJc(rpArray);
    WRITE_UINT64S(p_work, pJcData, columns+1);
    
    const mwIndex* pIrData = mxGetIr(rpArray);
    WRITE_UINT64S(p_work, pIrData, nzmax);
    
    // real part data
    const char* pdata = (const char*) mxGetData(rpArray);
    memcpy(p_work, pdata, n_bytes_data);
    p_work += n_bytes_data;

    // imaginary part data (if exists)
    if (complexity == mxCOMPLEX) {
        pdata = (const char*) mxGetImagData(rpArray);
        memcpy(p_work, pdata, n_bytes_imag);
        p_work += n_bytes_imag;
    }

    // sanity check --> should never happen
    rnBytes = (p_work - p_serial);
    if (rnBytes > nb) {
        mexPrintf("serializeSparse -> memory stats: %d [nb = %d]\n", p_work - p_serial, nb);
        mexErrMsgTxt("Oops, I haven't allocated enough memory, BEWARE");
    }

    return p_serial;
}


// Serialize a matlab string
char* serializeString(size_t &rnBytes, const mxArray*rpArray, const char*rpArg, const bool) {
    const mwSize* pdims = mxGetDimensions(rpArray);
    const size_t length = std::max<size_t>(pdims[0], pdims[1]);
    const mwSize n_dims = mxGetNumberOfDimensions(rpArray);
    char* p_buf = NULL;
    if (mxIsEmpty(rpArray)) {
        p_buf = (char*) '\0';
        rnBytes = 0;
    }
    else if (mxIsChar(rpArray)) {
        if ((n_dims!=2) || !((pdims[0]==1) || (pdims[1]==1)))
            mexErrMsgTxt("String placeholders only accept CHAR 1-by-M arrays or M-by-1!");
        // matlab string
        p_buf = (char*)mxCalloc(length+1, sizeof(char));
        p_buf = mxArrayToString(rpArray);
        p_buf = char2hex(p_buf, strlen(p_buf), length + 1);
        rnBytes = length;
    }
    else if (mxIsNumeric(rpArray)||mxIsLogical(rpArray)) {
        // matlab scalar
        p_buf = (char*)mxCalloc(512, sizeof(char));
        if (length!=1)
            mexErrMsgTxt("Only scalars are accepted!");
        if (mxIsComplex(rpArray))
            mexErrMsgTxt("Complex scalars are not supported");
        double value;
        bool conv_to_int = true;
        if (mxIsDouble(rpArray)) {
            value = *((double*)mxGetData(rpArray));
            conv_to_int = false;
        }
        else if (mxIsSingle(rpArray)) {
            value = *((float*)mxGetData(rpArray));
            conv_to_int = false;
        }
        else if (mxIsInt8(rpArray))
            value = *((char*)mxGetData(rpArray));
        else if (mxIsInt32(rpArray))
            value = *((int*)mxGetData(rpArray));
        else if (mxIsUint8(rpArray))
            value = *((unsigned char*)mxGetData(rpArray));
        else if (mxIsUint32(rpArray))
            value = *((unsigned*)mxGetData(rpArray));
        else if (mxIsLogical(rpArray))
            value = *(char*)mxGetData(rpArray)!=0;
        else
            mexErrMsgTxt("Unsupported type, the supported types are (U)INT 8 and 32, FLOAT and DOUBLE");
        if (*rpArg=='i') {
            conv_to_int = true;
            value = floor(value);
        }
        if (conv_to_int)
#ifdef _WINDOWS            
            rnBytes = _sprintf_l(p_buf, "%d", locUS, (int)value);
#else
            rnBytes = sprintf(p_buf, "%d", (int)value);
#endif
        else {
#ifdef _WINDOWS                        
            int n = _atoi_l(rpArg, locUS);
#else
            int n = atoi(rpArg);
#endif
            if (n==0)
#ifdef _WINDOWS                                        
                rnBytes = _sprintf_l(p_buf, "%.6f", locUS, value);
#else            
                rnBytes = sprintf(p_buf, "%.6f", value);
#endif
            else
#ifdef _WINDOWS                                                        
                rnBytes = _sprintf_l(p_buf, "%.*f", locUS, n, value);
#else
                rnBytes = sprintf(p_buf, "%.*f", n, value);
#endif
        }
    }
    else
        mexErrMsgTxt("Only char string and non-complex scalar are supported");
    return p_buf;
}
// serialize binary data, given as a UINT8 vector
char* serializeBinary(size_t &rnBytes, const mxArray*rpArray, const char*rpArg, const bool) {
    rnBytes = mxGetNumberOfElements(rpArray);
    char*p_buf = (char*)mxCalloc(rnBytes, sizeof(char));
    memcpy(p_buf, (const char*)mxGetData(rpArray), rnBytes);
    return p_buf;
}
// Serialize a file, the filename is given
char* serializeFile(size_t &rnBytes, const mxArray*rpArray, const char*rpArg, const bool) {
    const mwSize* pdims = mxGetDimensions(rpArray);
    const size_t length = std::max<size_t>(pdims[0], pdims[1]);
    // filename provided
    char*p_name = (char*)mxCalloc(length+1, sizeof(char));
    mxGetString(rpArray, p_name, length+1);
    FILE*pf = fopen(p_name, "rb");
    mxFree(p_name);
    if (!pf)
        mexErrMsgTxt("Could not open file");
    rnBytes = file_length(pf);
    char*p_buf = (char*)mxCalloc(rnBytes, sizeof(char));
    fread(p_buf, sizeof(char), rnBytes, pf);
    fclose(pf);
    return p_buf;
}


/*************************************DESERIALIZE******************************************/


// deserialize a numeric or char Array
mxArray* deserializeArray(const char *rpSerial, const size_t rlength) 
{
    const char *p_serial = rpSerial;

    // number of dimensions
    mwSize ndims;
    READ_UINT(&ndims, p_serial);
    
    // length of each dimension
    mwSize *pdims = (mwSize*) mxCalloc(ndims, sizeof(mwSize));
    READ_UINTS(pdims, p_serial, ndims);
    
    // classID
    mxClassID class_id;
    memcpy((char*) &class_id, p_serial, sizeof(mxClassID));
    p_serial += sizeof(mxClassID);
    
    // complexity
    mxComplexity complexity;
    memcpy((char*) &complexity, p_serial, sizeof(mxComplexity));
    p_serial += sizeof(mxComplexity);
    
    // create the numeric array
    mxArray *p_array = mxCreateNumericArray(ndims, pdims, class_id, complexity);
    mxFree(pdims);
    
    // real part data
    char *pdata = (char*) mxGetData(p_array);
    size_t nbytes_data_data = mxGetElementSize(p_array) * mxGetNumberOfElements(p_array);
    memcpy((char*) pdata, p_serial, nbytes_data_data);
    p_serial += nbytes_data_data;

    // imaginary part data (if exists)
    if (complexity == mxCOMPLEX) {
        pdata = (char*) mxGetImagData(p_array);
        memcpy((char*) pdata, p_serial, nbytes_data_data);
    }
    
    return p_array;
}


// deserialize a sparse matrix
mxArray* deserializeSparse(const char *rpSerial, const size_t rlength) 
{
    const char *p_serial = rpSerial;

    // number of rows and columns
    mwSize rows, columns, nzmax;
    READ_UINT(&rows, p_serial);
    READ_UINT(&columns, p_serial);
    READ_UINT(&nzmax, p_serial);

    // classID
    mxClassID class_id;
    memcpy((char*) &class_id, p_serial, sizeof(mxClassID));
    p_serial += sizeof(mxClassID);

    // complexity
    mxComplexity complexity;
    memcpy((char*) &complexity, p_serial, sizeof(mxComplexity));
    p_serial += sizeof(mxComplexity);

//  mexPrintf("rows=%d, cols=%d, nzmax=%d, compl=%d\n",rows,columns,nzmax,complexity);

    // create the sparse matrix
    mxArray *p_array;
    if (class_id == mxLOGICAL_CLASS) {
        p_array = mxCreateSparseLogicalMatrix(rows, columns, nzmax);
    } else {
        p_array = mxCreateSparse(rows, columns, nzmax, complexity);
    }

    // JC data
    mwSize* pJcData = mxGetJc(p_array);
    READ_UINTS(pJcData, p_serial, columns+1);
    
    // size of IR array
    mwSize* pIrData = mxGetIr(p_array);
    READ_UINTS(pIrData, p_serial, nzmax);
    
    // real part data
    void* pdata = mxGetData(p_array);
    size_t n_bytes_data = mxGetElementSize(p_array) * nzmax;
    memcpy(pdata, p_serial, n_bytes_data);
    p_serial += n_bytes_data;

    // imaginary part data (if exists)
    if (complexity == mxCOMPLEX) {
        pdata = mxGetImagData(p_array);
        memcpy(pdata, p_serial, n_bytes_data);
    }
    
    return p_array;
}


// deserialize struct Array
mxArray* deserializeStruct(const char* rpSerial, const size_t rlength){
    const char*p_serial = rpSerial;
    // number of dimensions
    mwSize ndims;
    READ_UINT(&ndims, p_serial);
    
    // length of each dimension
    mwSize *pdims = (mwSize*) mxCalloc(ndims, sizeof(mwSize));
    READ_UINTS(pdims, p_serial, ndims);
    
    mwSize nelts = 1;
    for (int i = 0; i<ndims; i++)
        nelts *= pdims[i];
    // number of field
    int nfields;
    memcpy(&nfields, (const char*)p_serial, sizeof(const int));
    p_serial += sizeof(const int);
    // field names
    const char** field_names = (const char**)mxCalloc(nfields, sizeof(char*));
    for (int i = 0; i<nfields; i++) {
        field_names[i] = p_serial;
        p_serial += strlen(p_serial)+1;
    }
    // create structure array
    mxArray* ps = mxCreateStructArray(ndims, pdims, nfields, field_names);
    // partial serialization
    for (int i = 0; i<nelts; i++)
        for (int j = 0; j<nfields; j++) {
            mwSize nbytes1;
            size_t nbytes;
            READ_UINT(&nbytes1, p_serial);
            nbytes = nbytes1;
            mxArray* pf;
            if (*p_serial==ID_ARRAY)
                pf = deserializeArray(p_serial+1, nbytes);
            else if (*p_serial==ID_SPARSE)
                pf = deserializeSparse(p_serial+1, nbytes);
            else if (*p_serial==ID_STRUCT)
                pf = deserializeStruct(p_serial+1, nbytes);
            else if (*p_serial==ID_CELL)
                pf = deserializeCell(p_serial+1, nbytes);
            else
                mexErrMsgTxt("Unknow type");
            mxSetFieldByNumber(ps, i, j, pf);
            p_serial += nbytes;
        }
    // free unused memory
    mxFree(field_names);
    return ps;
}
// deserialize cell array
mxArray* deserializeCell(const char* rpSerial, const size_t rlength){
    const char* p_serial = rpSerial;
    // number of dimensions
    mwSize ndims;
    READ_UINT(&ndims, p_serial);
    
    // length of each dimension
    mwSize *pdims = (mwSize*) mxCalloc(ndims, sizeof(mwSize));
    READ_UINTS(pdims, p_serial, ndims);
    
    mwSize nelts = 1;
    for (int i = 0; i<ndims; i++)
        nelts *= pdims[i];
    // create structure array
    mxArray* ps = mxCreateCellArray(ndims, pdims);
    // partial serialization
    for (int i = 0; i<nelts; i++){
        size_t nbytes;
        mwSize nbytes1;
        READ_UINT(&nbytes1, p_serial);
        nbytes=nbytes1;
        mxArray* pf;
        if (*p_serial==ID_ARRAY)
            pf = deserializeArray(p_serial+1, nbytes);
        else if (*p_serial==ID_SPARSE)
            pf = deserializeSparse(p_serial+1, nbytes);
        else if (*p_serial==ID_STRUCT)
            pf = deserializeStruct(p_serial+1, nbytes);
        else if (*p_serial==ID_CELL)
            pf = deserializeCell(p_serial+1, nbytes);
        else
            mexErrMsgTxt("Unknow type");
        mxSetCell(ps, i, pf);
        p_serial += nbytes;
    }
    return ps;
}
/*
 *   returns the length of a file in bytes, and replace the file pointer to its state before calling
 */
int file_length(FILE*f) {
    int pos;
    int end;
    pos = ftell(f);
    fseek(f, 0, SEEK_END);
    end = ftell(f);
    fseek(f, pos, SEEK_SET);
    return end;
}

#ifdef __GNUC__
    static const _uint64 castMask = (sizeof(_uint64) > sizeof(mwSize)) ? (_uint64)(((mwSize)0)-1) : ((_uint64)0xFFFFFFFFFFFFFFFFULL);
#else
    static const _uint64 castMask = (sizeof(_uint64) > sizeof(mwSize)) ? (_uint64)(((mwSize)0)-1) : ((_uint64)0xFFFFFFFFFFFFFFFF);
#endif

static const _uint64 checkMask= ~castMask;

void safe_read_64uint(mwSize* dst, _uint64* src, size_t n) {
    for (size_t i=0; i < n; ++i) {
        const _uint64& val = src[i];

        if (val & checkMask)
            mexErrMsgTxt("This dataset exceeds the size limitations of the current platform (e.g. reading huge matrices on 32 bit machines)");
        else
            dst[i] = (mwSize)(val & castMask);
    }
}

void safe_read_32uint(mwSize* dst, _uint32* src, size_t n) {
    for (size_t i=0; i < n; ++i) {
            dst[i] = mwSize(src[i]);
    }
}

static void getSerialFct(const char* rpt, const mxArray* rparg, pfserial& rpf, bool& rpec) {
    const mwSize n_dims = mxGetNumberOfDimensions(rparg);
    const mwSize* p_dim = mxGetDimensions(rparg);
    bool no_compression = false;
    const char* pt = rpt;
    // first check for preamble
    if (*pt==PRE_NO_COMPRESSION) {
        no_compression = true;
        pt++;
    }
    if (*pt==PH_MATLAB) {
        // this placeholder results in a serialized version of array, cell or structure 
        rpec = true;
        if (mxIsNumeric(rparg) || mxIsChar(rparg) || mxIsLogical(rparg))
            rpf = &serializeArray;
        else if (mxIsCell(rparg))
            rpf = &serializeCell;
        else if (mxIsStruct(rparg))
            rpf = &serializeStruct;
        else
            mexErrMsgTxt("Matlab placeholder only support array, structure, or cell");
    }
    else if ((*pt==PH_BINARY) || (*pt==PH_FILE)) {
        // this placeholder results in a binary dump of the corresponding data
        rpec = true;
        if (*pt==PH_BINARY)
            if ((n_dims!=2) || !((p_dim[0]==1) || (p_dim[1]==1)) || !mxIsUint8(rparg))
                mexErrMsgTxt("Binary placeholders only accept UINT8 1-by-M or M-by-1 arrays!");
            else
                rpf = &serializeBinary;
        else
            if ((n_dims!=2) || !((p_dim[0]==1) || (p_dim[1]==1)) || !mxIsChar(rparg))
                mexErrMsgTxt("String placeholders only accept CHAR 1-by-M or M-by-1 arrays!");
            else
                rpf = &serializeFile;
    }
    else if (*pt==PH_STRING) {
        // this placeholder results in a text version of the data
        rpec = false;
        rpf = &serializeString;
    }
    else
        mexErrMsgTxt("Unknow placeholders!");
    if (no_compression)
        rpec = false;
}


// entry point
mxArray* deserialize(const char* rpSerial, const size_t rlength) {
    mxArray* p_res = NULL;
    bool could_not_deserialize = true;
    bool used_compression = false;
    char* p_cmp = NULL;
    const char* p_serial = rpSerial;
    size_t length = rlength;
    if (p_serial==0) {
        // the row is empty: return an empty array
        p_res = mxCreateNumericArray(0, 0, mxCHAR_CLASS, mxREAL);
        return p_res;
    }
    if (strcmp(p_serial, ZLIB_ID)==0) {
        p_serial = p_serial+LEN_ZLIB_ID+1;
        // read the length in bytes
        mwSize len;
        size_t lenLong;
        READ_UINT(&len, p_serial);
        char* p_cmp = (char*)mxCalloc(len, sizeof(char));
        lenLong=len;
        try {
            uLongf lenLong32 = (lenLong & 0xFFFFFFFF);
            int res = uncompress((Bytef*)p_cmp, &lenLong32, (const Bytef*)p_serial, length);
            if (res==Z_OK) {
                used_compression = true;
                p_serial = p_cmp;
                length = len;
            }
            else
                p_serial = rpSerial;
        }
        catch(...) {
            p_serial = rpSerial;
        }
    }
    if (p_serial != 0 && !strcasecmp(p_serial, "dj0"))
        mexErrMsgIdAndTxt("mYm:CrossPlatform:Compatibility",
            "Blob data ingested utilizing DataJoint-Python version >=0.12 not yet supported.");
    if (strcmp(p_serial, ID_MATLAB)==0) {
        p_serial = p_serial+LEN_ID_MATLAB+1;
        try {
          could_not_deserialize = false;
          if (*p_serial==ID_ARRAY)
            // the blob contains an array
            p_res = deserializeArray(p_serial+1, length);
          else if (*p_serial==ID_SPARSE)
            // the blob contains a sparse matrix
            p_res = deserializeSparse(p_serial+1, length);
          else if (*p_serial==ID_STRUCT)
            // the blob contains a struct
            p_res = deserializeStruct(p_serial+1, length);
          else if (*p_serial==ID_CELL)
            // the blob contains a struct
            p_res = deserializeCell(p_serial+1, length);
          else
            // the blob contains an unknow object
            could_not_deserialize = true;
        }
        catch(...) {
          could_not_deserialize = true;
          p_serial = rpSerial;
        }
    }
    if (could_not_deserialize) {
        // we don't know what to do with the stuff in the database
        // we return a memory dump in UINT8 format
        mwSize p_dim[2] = {length, 1};
        p_res = mxCreateNumericArray(2, p_dim, mxUINT8_CLASS, mxREAL);
        memcpy((char*)mxGetData(p_res), p_serial, length);
    }
    if (used_compression)
        mxFree(p_cmp);
    return p_res;
}
char *hex2char(char *original_val, const size_t char_length) {
    const uint8_t *pnt = (uint8_t *)original_val;
    uint8_t *result_pnt = new uint8_t[char_length*4];
    unsigned int offset = 0;
    for( unsigned int a = 0; a < char_length; ++a )
    {
        if     (pnt[a]<=0x7F) {
            result_pnt[a+offset] = pnt[a];
        }
        else if(pnt[a]<=0x7FF) {
            result_pnt[a+offset] = ((pnt[a]>>6) + 0xC0);
            result_pnt[a+offset+1] = ((pnt[a] & 0x3F) + 0x80);
            offset += 1;
        }
        else if(0xD800<=pnt[a] && pnt[a]<=0xDFFF) {
            mexErrMsgIdAndTxt("mYm:Deserialization:UTF8",
                "Invalid block of UTF8 detected.");
        } //invalid block of utf8
        else if(pnt[a]<=0xFFFF) {
            result_pnt[a+offset] = ((pnt[a]>>12) + 0xE0);
            result_pnt[a+offset+1] = (((pnt[a]>>6) & 0x3F) + 0x80);
            result_pnt[a+offset+2] = ((pnt[a] & 0x3F) + 0x80);
            offset += 2;
        }
        else if(pnt[a]<=0x10FFFF) {
            result_pnt[a+offset] = ((pnt[a]>>18) + 0xF0);
            result_pnt[a+offset+1] = (((pnt[a]>>12) & 0x3F) + 0x80);
            result_pnt[a+offset+2] = (((pnt[a]>>6) & 0x3F) + 0x80);
            result_pnt[a+offset+3] = ((pnt[a] & 0x3F) + 0x80);
            offset += 3;
        }
    }
    result_pnt[char_length+offset]= 0x00;
    return (char *)result_pnt;
}
char *char2hex(char *original_val, const size_t vlength, const size_t char_length) {
    unsigned int idx = 0;
    unsigned int curr_length = 0;
    unsigned char u0,u1,u2,u3;
    uint8_t *result_pnt = new uint8_t[char_length];
    for(unsigned int a = 0; a < vlength;)
    {
        u0 = original_val[a];
        curr_length = 1;
        if (((u0 & 0xF8) == 0xF0) && ((a + curr_length + 3) <= vlength)) {
            curr_length += 3;
            u1 = original_val[a+1];
            u2 = original_val[a+2];
            u3 = original_val[a+3];
            result_pnt[idx] = (((u0-0xF0)<<18) + ((u1-0x80)<<12) + ((u2-0x80)<<6) + (u3-0x80));
        }
        else if (((u0 & 0xF0) == 0xE0) && ((a + curr_length + 2) <= vlength)) {
            curr_length += 2;
            u1 = original_val[a+1];
            u2 = original_val[a+2];
            if (u0 == 0xED && (u1 & 0xA0) == 0xA0) {
                mexErrMsgIdAndTxt("mYm:Serialization:UTF8",
                    "Invalid block of UTF8 detected.");
            }
            result_pnt[idx] = (((u0-0xE0)<<12) + ((u1-0x80)<<6) + (u2-0x80));
        }
        else if (((u0 & 0xE0) == 0xC0) && ((a + curr_length + 1) <= vlength)) {
            curr_length++;
            u1 = original_val[a+1];
            result_pnt[idx] = (((u0-0xC0)<<6) + (u1-0x80));
        }
        else {
            result_pnt[idx] = u0;
        }
        idx++;
        a += curr_length;
    }
    result_pnt[idx]= 0x00;
    return (char*)result_pnt;
}