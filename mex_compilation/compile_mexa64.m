% RUN THIS FROM THE TOP LEVEL DIRECTORY, I.E. THE ONE THAT CONTAINS
% 'mex_compilation'.
%
% Build script for MYM using VC++ 2008.
% Refer to the mexopts.bat in SVN for setup of paths

% This script assumes
% MySQL in "c:\Program Files\MySQL\MySQL Server 5.0"


mex -g -v -largeArrayDims -I. -I"/usr/include/mysql"  -L"/usr/lib64/mysql" -L. -lmysqlclient libz.so.2  mym.cpp -v
