function mymSetup()
% mymSetup()
% Adds mym directories to your path and verifies that mym is
% working correctly.

basePath = fileparts(mfilename('fullpath'));
mexPath = fullfile(basePath, 'distribution', mexext());

assert(logical(exist(mexPath, 'dir')), ...
    ['This distribution of mym does not include binaries for your ' ...
    'platform. Please obtain the ' ...
    '<a href="https://github.com/datajoint/mym">source code for mym</a> ' ...
    'and compile the MEX file for your platform.'])
addpath(mexPath);

% Change into distribution directory to avoid calling
% mym.m which only contains documentation.
oldp = cd(fileparts(mexPath));
oldpRestore = onCleanup(@() cd(oldp));

% Try calling mym
try
    mym()
    disp 'mym is now ready for use.'
catch mym_err
    disp 'mym was added to your path but did not execute correctly.'
    rethrow(mym_err);
end