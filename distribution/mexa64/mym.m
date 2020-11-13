% MYM - Interact with a MySQL database server 
%       Copyright 2005, EPFL (Yannick Maret)
%
% Copyright notice: this code is a heavily modified version of the original
% work of Robert Almgren from University of Toronto.
%
% If no output arguments are given, then display results. Otherwise returns
% requested data silently.
%
% INSTALLATION
% ------------
% (Recommended) Using GHToolbox (FileExchange Community Toolbox):
%   1. Install *GHToolbox* using using an appropriate method in
%        https://github.com/datajoint/GHToolbox
%   2. run: `ghtb.install('datajoint/mym')`
%
% Greater than R2016b:
%   1. Utilize MATLAB built-in GUI i.e. *Top Ribbon -> Add-Ons -> Get Add-Ons*
%   2. Search and Select `mym`
%   3. Select *Add from GitHub*
%
% Less than R2016b:
%   1. Utilize MATLAB built-in GUI i.e. *Top Ribbon -> Add-Ons -> Get Add-Ons*
%   2. Search and Select `mym`
%   3. Select *Download from GitHub*
%   4. Save `mym.mltbx` locally
%   5. Navigate in MATLAB tree browser to saved toolbox file
%   6. Right-Click and Select *Install*
%   7. Select *Install*
%
% From Source:
%   1. Download `mym.mltbx` locally
%   2. Navigate in MATLAB tree browser to saved toolbox file
%   3. Right-Click and Select *Install*
%   4. Select *Install*
%
% mym() or mym
% ------------
%   shows status of all open connections (returns nothing).
% mym('open', host, user, password)
% ---------------------------------
%   Open a connection with specified parameters, or defaults if none
%     host:     default is local host. Use colon for port number
%     user:     default is Unix login name.
%     password: default says connect without password.
%     use_tls:  (optional) TLS encryption type. Values are as follows:
%                - 'true':           TLS encryption is required.
%                - 'false':          TLS encryption is disabled.
%                - 'nan'(default):   TLS encryption utilized if available.
%   Examples: mym('open','arkiv')      %  connect on default port
%             mym('open','arkiv:2215') %  TLS Preferred
%             mym('open','arkiv','root','simple','true') %  TLS Required  
%   If successful, open returns 0 if successful, and throw an error
%   otherwise. The program can maintain up to 20 independent connections.
%   Any command may be preceded by a connection handle -- an integer from 0
%   to 10 -- to apply the command to that connection.
%   Examples: mym(5,'open','host2')  %  connection 5 to host 2
%             mym                    %  status of all connections
%   When no connection handle is given, mym use 0 by default. If the
%   corresponding connection is open, it is closed and opened again.
%   It is possible to ask mym to look for an available connection
%   handle by using -1. The used connection handle is then returned.
%   Example:  cid = mym(-1, 'open', 'host2')  %  cid contains the used
%                                                connection handle
% mym('close')
% ------------
%   Close the current connection. Use mym('closeall') to closes all open 
%   connections.
% mym('use',db)  or   mym('use db')
% ---------------------------------
%   Set the current database to db   
%   Example:  mym('use cme')
% mym('status')
% -------------
%   Display information about the connection and the server. Connections
%   utilizing TLS encryption will be displayed as '(encrypted)'.
%   Return  0  if connection is open and functioning
%           1  if connection is closed
%           2  if should be open but we cannot ping the server
% mym(query)
% ----------
%   Send the given query or command to the MySQL server. If arguments are
%   given on the left, then each argument is set to the column of the
%   returned query. Dates and times in result are converted to Matlab 
%   format: dates are serial day number, and times are fraction of day. 
%   String variables are returned as cell arrays.
%   Example: p = mym('select price from contract where date="1997-04-30"');
%            % Returns price for contract that occured on April 30, 1997.
%   Note: All string comparisons are case-insensitive
%   Placeholders: in a query the following placeholders can be used: {S}, 
%   {Si}, {M}, {F}, and {B}.
%   Example: i = 1000;
%            B = [1 2; 3 4];
%            mym('INSERT INTO test(id,value) VALUES("{Si}","{M}")',i,B);
%            A = mym('SELECT value FROM test WHERE id ="{Si}")', 1000);
%            % Insert the array B into table test with id=1000. Then the 
%            % value is retrieved and put into A.
%   {S}  is remplaced by a string given by the corresponding argument arg. 
%        Arg can be a matlab string or a scalar. The format of the string
%        for double scalars is [sign]d.ddddddEddd; for integers the format 
%        is [sign]dddddd.
%   {Sn} is the same as {S} but for double scalar only. The format of the
%        string is [sign]d.ddddEddd, where the number of decimal after the 
%        dot is given by n.
%   {Si} is the same as {S} but for double scalar only. The corresponding 
%        double is first converted to an integer (using floor).
%   {M}  is replaced by the binary representation of the corresponding
%        argument (it can be a scalar, cell, numeric or cell array, or a 
%        structure).
%   {B}  is replaced by the binary representation of the uint8 vector
%        given in the corresponding argument.
%   {F}  is the same as {B} but for the file whose name is given in the
%        corresponding argument.
%   Note: 1) {M}, {B} and {F} need to be put in fields of type BLOB
%         2) {M}, {B} and {F} binary representations are compressed -- only 
%            if the space gain is larger than 10% --. We use zlib v1.2.3!
%            The compression can be switched off by using  {uM}, {uB} and
%            {uF}, instead.
%         3) {M} does not work if the endian of the client used to store
%            the BLOB is different than that used to fetch it.
%         4) time fields are returned as string dump
%         5) this kind of insert does not work properly:
%             mym(INSERT INTO tbl(id,txt) VALUES(1000,"abc{dfg}h")');
%            as the "abc{dfg}h" is mistaken for a mYm command. A possible
%            solution is to use the following command:
%             mym(INSERT INTO tbl(id,txt) VALUES(1000,"{S}")','abc{dfg}h');
%
% mym('serialize {placeholder1}, ...',matlab_variable1, ...)
% -------------
%   Return : cell array of vectors of type ubyte, a vector per matlab variable 
%
% mym('deserialize', serialized_input)
% -------------
%   Return : matlab variable of the appropriate type 
%
% mym('version')
% --------------
% Displays the version of mym when no output arguments are given.
% With a single output argument, a struct with the fields 'major',
% 'minor' and 'bugfix' is returned. These fields contain numeric
% scalars corresponding to the version major.minor.bugfix
%
% SOURCE
% ------
% https://github.com/datajoint/mym
%
% HISTORY
% -------
% v1.36   - fixed bug for Linux64 related to cross-platform compatibility. Blobs written on windows can be read under 
% 		Linux 32/64 and vice-versa.
% v1.35   - fixed bug with incorrect pointer increment under Linux
% v1.3    - fixed bug with saving corrupted cells
% v1.2    - fixed bug with memory allocation, result now is returned as a structure 
% v1.1    - fixed bug with port number being ignored when specified
% 
% v1.0.9	- a space is now used when the variable corresponding to a string placeholder is empty
%         - we now use strtod instead of sscanf(s, "%lf")
% 	- add support for stored procedure
% v1.0.8	- corrected a problem occurring with MySQL commands that do not return results
%         - the M$ Windows binary now use the correct runtime DLL (MSVC80.DLL insteaLd of MSVC80D.DL)
% v1.0.7  - logical values are now correctly considered as numerical value when using placeholder {Si}
%         - corrected a bug occuring when closing a connection that was not openned
%         - added the possibility to get the next free connection ID when oppening a connection
% v1.0.6  - corrected a bug where mym('use', 'a_schema') worked fine while mym(conn, 'use', 'a_schema') did not work
%         - corrected a segmentation violation that happened when issuing a MySQL command when not connected
%         - corrected the mex command (this file)
%         - corrected a bug where it was impossible to open a connection silently
%         - use std::max<int>(a, b) instead of max(a, b)
% v1.0.5  - added the preamble 'u', permitting to save binary fields without using compression
%         - corrected a bug in mym('closeall')
%         - corrected various mistakes in the help file (thanks to Jörg Buchholz)
% v1.0.4  corrected the behaviour of mYm with time fields, now return a string dump of the field
% v1.0.3 	minor corrections
% v1.0.2  put mYm under GPL license, official release
% v1.0.1  corrected a bug where non-matlab binary objects were not returned
% v1.0.0  initial release


