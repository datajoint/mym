function compile_mexmaci64()
% Build script for MyM (64-bit Mac OS X)
% For Mac, we can rely on the system zlib. It has been at version >= 1.2.3
% since OS X 10.4

mym_base = fileparts(fileparts(mfilename('fullpath')));
mym_src = fullfile(mym_base, 'src');
build_out = fullfile(mym_base, 'build', mexext());
distrib_out = fullfile(mym_base, 'distribution', mexext());

% Set up input and output directories
mysql_base = fullfile(mym_base, 'mysql-connector');
mysql_include = fullfile(mysql_base, 'include');
mysql_platform_include = fullfile(mysql_base, ['include_' mexext()]);
mysql_lib = fullfile(mysql_base, ['lib_' mexext()]);

mkdir(build_out);
mkdir(distrib_out);
oldp = cd(build_out);
pwd_reset = onCleanup(@() cd(oldp));

mex( ...
    '-v', ...
    '-largeArrayDims', ...
    sprintf('-I"%s"', mysql_include), ...
    sprintf('-I"%s"', mysql_platform_include), ...
    sprintf('-L"%s"', mysql_lib), ...
	'-lmysqlclient', ...
	'-lz', ...
    fullfile(mym_src, 'mym.cpp'));

% find old libmysql path
[~,old_link] = system(['otool -D ' ...
    fullfile(build_out, ['mym.' mexext()]) ...
    ' | grep libmysqlclient.18.dylib | tail -1']);

% Change libmysql reference to mym mex directory
system(['install_name_tool -change "' old_link '" ' ...
    '"@loader_path/libmysqlclient.18.dylib" "' ...
    fullfile(build_out, ['mym.' mexext()]) '"']);

% Pack mex with all dependencies into distribution directory
copyfile(['mym.' mexext()], distrib_out);
copyfile(fullfile(mysql_lib, 'libmysqlclient*'), distrib_out);