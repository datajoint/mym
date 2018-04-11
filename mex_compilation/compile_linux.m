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


% check if mysql_config is availalble, if so, copy the .so files from location where package is installed
[status,result]=system('mysql_config --libs') ;
if (status==0)
    idx = strfind(result, '-L') 
    if ~isempty(idx)
        idx1 = strfind(result, ' ') 
        if ~isempty(idx1)
            idx1 = idx1(1) ;
            src_dir = result(idx+2:idx1-1) ;
            cbuf = sprintf('cp %s/libmysqlclient.so* %s',src_dir,mysql_lib) 
            [s1,~]=system(cbuf);
            if (s1 ~= 0)
                disp('Unable to copy libmysqlclient.so.* to mym mysql-connector folder') ;
                return ;
            end
        else
            disp('Source folder for libmysqlclient.so.* cannot be found') ;
            return ;
        end
    else
        disp('mysql_config returned an empty string, error may have occurred during mysqlclient-dev package installation') ;
        return ;
    end
else
    disp('Install mysqlclient-dev package before running this function') ;
    return ;
end

lib = fullfile(mym_base, 'lib', mexext());

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
    sprintf('-L"%s"', lib), ...
	'-lmysqlclient', ...
 	'-lz', ...
    fullfile(mym_src, 'mym.cpp'));

% Pack mex with all dependencies into distribution directory
copyfile(['mym.' mexext()], distrib_out);
copyfile(fullfile(lib, '*'), distrib_out);
copyfile(fullfile(mysql_lib, 'libmysqlclient.so*'), distrib_out);
copyfile([mym_base '/readme_linux.txt'],distrib_out) ;