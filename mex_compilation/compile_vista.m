% RUN THIS FROM THE TOP LEVEL DIRECTORY, I.E. THE ONE THAT CONTAINS
% 'mex_compilation'.
% Build script for MYM using VC++ 2008.
% Refer to the mexopts.bat in SVN for setup of paths

% This script assumes
% MySQL in "c:\Program Files\MySQL\MySQL Server 5.0"

p = mfilename('fullpath');
ndx = strfind(p,filesep);
optsFile = [p(1:ndx(end)) 'mexopts.bat']; % Compiler options file
compileDir = p(1:ndx(end-1));
cd(compileDir);   % Compilation has to be called from main dir
eval(['mex -v -I. -I"C:\Program Files\MySQL\MySQL Server 5.0\include" -I".\zlib_windows\include" -L".\zlib_windows\lib" -L"C:\Program Files\MySQL\MySQL Server 5.0\lib\opt" -L.   -llibmysql -lzlib -lmysqlclient -lzlibwapi mym.cpp -v -f ',optsFile])
