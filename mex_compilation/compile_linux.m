function compile_linux()
% Build script for MyM (32-bit and 64-bit Linux)

mym_base = fileparts(fileparts(mfilename('fullpath')));
mym_src = fullfile(mym_base, 'src');
build_out = fullfile(mym_base, 'build', mexext());
distrib_out = fullfile(mym_base, 'distribution', mexext());

% Set up input and output directories
mysql_base = fullfile(mym_base, 'mysql-connector');
mysql_include = fullfile(mysql_base, 'include');
mysql_platform_include = fullfile(mysql_base, ['include_' mexext()]);
mysql_lib = fullfile(mysql_base, ['lib_' mexext()]);

lib = fullfile(mym_base, 'lib', mexext());

mkdir(build_out);
mkdir(distrib_out);
oldp = cd(build_out);
pwd_reset = onCleanup(@() cd(oldp));

mex( ...
    '-v', ...
    '-g', ...
    '-largeArrayDims', ...
    sprintf('-I"%s"', mysql_include), ...
    sprintf('-I"%s"', mysql_platform_include), ...
    sprintf('-L"%s"', mysql_lib), ...
    sprintf('-L"%s"', lib), ...
	'-lmysqlclient', ...
    'LINKLIBS="$LINKLIBS -Wl,-rpath,\$ORIGIN -Wl,-z,origin"', ...
	'-lz', ...
    fullfile(mym_src, 'mym.cpp'));

% Pack mex with all dependencies into distribution directory
copyfile(['mym.' mexext()], distrib_out);
copyfile(fullfile(lib, '*'), distrib_out);
copyfile(fullfile(mysql_lib, 'libmysqlclient.so*'), distrib_out);