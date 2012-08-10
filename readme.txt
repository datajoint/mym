mYm v1.36 readme.txt

All feedback appreciated to heartofmars@yahoo.com, yannick.maret@epfl.ch

GPL
---
mYm is a Matlab interface to MySQL server that support BLOB object

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

author:                   yannick MARET
e-mail:                   yannick.maret@a3.epfl.ch
old fashioned paper mail: EPFL-STI-ITS-LTS1 
                          Yannick MARET
                          ELD241
                          Station 11
                          CH-1015 Lausanne    

Copyright notice: some parts of this code (server connection, fancy print) is based on an original code by Robert Almgren (http://www.mmf.utoronto.ca/resrchres/mysql/). The present code is under GPL license with express agreement of Mr. Almgren.
 
WHAT IS MYM?
------------
mYm is a Matlab interface to MySQL server. It is based on the original 'MySQL and Matlab' by Robert Almgren and adds the support for Binary Large Object (BLOB). That is, it can insert matlab objects (e.g. array, structure, cell) into BLOB fields, as well retrieve from them. To save space, the matlab objects is first compressed (using zlib) before storing it into a BLOB field. Like Almgren's original, mYm supports multiple connections to MySQL server.

INSTALLATION
------------
-Windows add the path to the files extracted for archive to Matlab path variable. Use "mym" from Matlab.

-Windows[compilation] The source should be compiled with combination of "mex" and microsoft vcc compiler.
	-install visual studio 2003/2005
        -run "mex -setup" from Matlab command line and choose the compiler installed (do not choose LCC).
	-use command
            mex -I[mysql_include_dir] -I[zlib_include_dir] -L[mysql_lib_dir] -L[zlib_lib_dir] -llibmysql -lzlib -lmysqlclient -lzlibwapi mym.cpp 
	to compiler mex-function.

-Linux A64 zlib.so.2 and zlib.so.2.2 into /usr/lib64 so that they can be dynamically linked to compiles mexa64. These libraries
are actually recompiled zlib 1.2.3 according to 
	http://groups.google.ca/group/comp.soft-sys.matlab/browse_thread/thread/e6625c4699f9f116/1867b981fac2977b?lnk=raot
	in order to force Matlab to link mex function agains custom zlib library instead of zlib.1.1.4 shipped with Matlab
	
-xNix[compilation]  the source can be compiled using the following command (thanks to Jeffrey Elrich)
         mex -I[mysql_include_dir] -I[zlib_include_dir] -L[mysql_lib_dir] -L[zlib_lib_dir] -lz -lmysqlclient mym.cpp
	 (on Mac OS X you might also need the -lSystemStubs switch to avoid namespace clashes)
         Note: to compile, the zlib library should be installed on the system (including the headers).
               for more information, cf. http://www.zlib.net/

	See also http://groups.google.ca/group/comp.soft-sys.matlab/browse_thread/thread/e6625c4699f9f116/1867b981fac2977b?lnk=raot

HOW TO USE IT
-------------
see mym.m

HISTORY
-------

v1.36   - fixed bug for Linux64 related to cross-platform compatibility. Blobs written on windows can be read under 
		Linux 32/64 and vice-versa.
v1.35   - fixed bug with incorrect pointer increment under Linux
v1.3    - fixed bug with saving corrupted cells
v1.2    - fixed bug with memory allocation, result now is returned as a structure 
v1.1    - fixed bug with port number being ignored when specified

v1.0.9	- a space is now used when the variable corresponding to a string placeholder is empty
        - we now use strtod instead of sscanf(s, "%lf")
	- add support for stored procedure
v1.0.8	- corrected a problem occurring with MySQL commands that do not return results
        - the M$ Windows binary now use the correct runtime DLL (MSVC80.DLL insteaLd of MSVC80D.DL)
v1.0.7  - logical values are now correctly considered as numerical value when using placeholder {Si}
        - corrected a bug occuring when closing a connection that was not openned
        - added the possibility to get the next free connection ID when oppening a connection
v1.0.6  - corrected a bug where mym('use', 'a_schema') worked fine while mym(conn, 'use', 'a_schema') did not work
        - corrected a segmentation violation that happened when issuing a MySQL command when not connected
        - corrected the mex command (this file)
        - corrected a bug where it was impossible to open a connection silently
        - use std::max<int>(a, b) instead of max(a, b)
v1.0.5  - added the preamble 'u', permitting to save binary fields without using compression
        - corrected a bug in mym('closeall')
        - corrected various mistakes in the help file (thanks to Jorg Buchholz)
v1.0.4  corrected the behaviour of mYm with time fields, now return a string dump of the field
v1.0.3 	minor corrections
v1.0.2  put mYm under GPL license, official release
v1.0.1  corrected a bug where non-matlab binary objects were not returned
v1.0.0  initial release