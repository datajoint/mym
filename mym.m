% setupMYM (invoked only first time to set path and removes itself)
function varargout = mym(varargin)
    warning('mYm:Setup:FirstInvokation', 'Setting mym to path as first invokation.');
    % determine state
    origDir = pwd;
    ext = dbstack;
    ext = ext.file;
    [~,~,ext] = fileparts(ext);
    filename = [mfilename('fullpath') ext];
    % add to mex-path as appropriate (persist)
    toolboxName = 'mym';
    s = settings;
    if verLessThan('matlab', '9.2')
        toolboxRoot = [strrep(s.matlab.addons.InstallationFolder.ActiveValue, '\', '/') ...
                       '/Toolboxes/' toolboxName '/code'];
    else
        toolboxRoot = [strrep(s.matlab.addons.InstallationFolder.ActiveValue, '\', '/') ...
                       '/Toolboxes/' toolboxName];
    end
    mymPath = [toolboxRoot '/distribution/' mexext];
    addpath(mymPath);
    pathfile = fullfile(userpath, 'startup.m');
    if exist(pathfile, 'file') == 2
        fid = fopen(pathfile, 'r');
        f = fread(fid, '*char')';
        fclose(fid);
        if ~contains(f, ['addpath(''' mymPath ''');'])
            fid = fopen(pathfile, 'a+');
            fprintf(fid, '\n%s\n',['addpath(''' mymPath ''');']);
            fclose(fid);
        end
    elseif exist(pathfile, 'file') == 0
        fid = fopen(pathfile, 'a+');
        fprintf(fid, '\n%s\n',['addpath(''' mymPath ''');']);
        fclose(fid);
    end
    % determine proper mym response and restore state
    cd(mymPath);
    if nargout
        [varargout{1:nargout}] = mym(varargin{:});
    else
        mym(varargin{:});
    end
    cd(origDir);
    % remove file to prevent collisions
    delete(filename);
end