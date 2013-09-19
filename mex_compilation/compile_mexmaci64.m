% RUN THIS FROM THE TOP LEVEL DIRECTORY, I.E. THE ONE THAT CONTAINS
% 'mex_compilation'.

% System info:
%   Mac OS X 10.6 (Snow Leopard)
%   GCC 4.0 (installed via XCode)
%   mexopts.bat set to use /Developer/SDKs/MacOSX10.6
%   MySQL client 5.5.9 in /usr/local/mysql

% I had to delete unistd.h in the main mym directory

% I had to copy libmysql.16.0.0.dylib and libmysql.16.dylib from
% /usr/local/mysql/lib into the main mym directory

mex -g -v -largeArrayDims -I"/usr/local/mysql/include" -L"." -lz -lmysqlclient mym.cpp
