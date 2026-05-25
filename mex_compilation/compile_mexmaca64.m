function compile_mexmaca64()
% Build script for MyM (64-bit ARM Mac OS)
%
% Notes:
%
%   1. we're relying on the system zlib.

mym_base = fileparts(fileparts(mfilename('fullpath')));
mym_src = fullfile(mym_base, 'src');
build_out = fullfile(mym_base, 'build', mexext());
distrib_out = fullfile(mym_base, 'distribution', mexext());

% Set up input and output directories
mysql_base = fullfile(mym_base, 'mysqlclient');
mysql_include = fullfile(mysql_base, 'include');
% mysql_platform_include = fullfile(mysql_base, ['include_' mexext()]);
mysql_lib = fullfile(mysql_base, ['lib_' mexext()]);
% mariadb_lib = fullfile(mym_base, ['maria-plugin/','lib_',mexext()]);

mkdir(build_out);
mkdir(distrib_out);
oldp = cd(build_out);
pwd_reset = onCleanup(@() cd(oldp));

mex( ...
    '-v', ...
    '-largeArrayDims', ...
    'LDFLAGS=$LDFLAGS -ld_classic', ...
    'MACOSX_DEPLOYMENT_TARGET=13.0', ...
    sprintf('-I"%s"', mysql_include), ... % sprintf('-I"%s"', mysql_platform_include), ...
    sprintf('-L"%s"', mysql_lib), ... % sprintf('-L"%s"', mariadb_lib), ...
    '-lmysqlclient', ...
    '-lz', ...
    fullfile(mym_src, 'mym.cpp'));


% Find old libmysqlclient path
[~,old_link] = system(['otool -L ' ...
    fullfile(build_out, ['mym.' mexext()]) ...
    ' | grep libmysqlclient.24.dylib | tail -1 |awk ''{print $1}''']);

% Change libmysqlclient reference to mym mex directory
system(['install_name_tool -change "' strip(old_link) '" "' ...
    fullfile('@loader_path','lib', 'libmysqlclient.24.dylib') '" "' ...
    fullfile(build_out, ['mym.' mexext()]) '"']);

% Pack mex with all dependencies into distribution directory
copyfile(['mym.' mexext()], distrib_out, 'f');
copyfile(fullfile(mym_src, 'mym.m'), distrib_out, 'f');
copyfile(fullfile(mysql_lib, 'lib*'), fullfile(distrib_out,'lib'), 'f');
% copyfile(fullfile(mariadb_lib, 'dialog.so'), distrib_out, 'f');
