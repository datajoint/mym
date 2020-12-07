function [result, why] = celleq(cell1, cell2, funh2string, ignorenan)
    % CELLEQ performs an equality comparison between two cell arrays by
    % recursively comparing the elements of the cell array, their values and
    % sub-values
    %
    % USAGE:
    %
    % celleq(cell1, cell2)
    %       Performs a comparison and returns true if all the elements
    %       of the cell arrays are identical. It will fail if
    %       elements include function handles or other objects which don't
    %       have a defined eq method. 
    %
    % [iseq, info] = celleq(cell1, cell2)
    %       This syntax returns a logical iseq and a second output info which
    %       is a structure that contains a field "Reason" which gives you a
    %       text stack of why the difference occurred as well as a field
    %       "Where" which contains the indices of the element and subelement
    %       where the comparison failed. If iseq is true, info contains empty
    %       strings in its fields.
    %
    % [...] = celleq(cell1, cell2, funh2string, ignorenan)
    %       Illustrates an alternate syntax for the function with an additional
    %       input arguments. funh2string, if true, instructs function handle
    %       comparisons to return true if the string representations of the
    %       function handles are the same. ignorenan, if true, will return true
    %       for nan == nan. By default both properties are set to false
    %
    % METHOD:
    % 1. Compare sizes of cell arrays
    % 2. Recursively compare the elements of the cell array and keep track of
    % the recursion path (to populate the info variable if comparison fails)
    %
    % EXAMPLE:
    %
    % c1 = {1:5, 'blah', {'hello', @disp, {[7 6 NaN 3], false}}, 16};
    % c2 = {1:5, 'blah', {'hello', @disp, {[7 6 NaN 3], true }}, 16};
    % celleq(c1, c2)
    % [iseq, info] = celleq(c1, c2)
    % [iseq, info] = celleq(c1, c2, true)
    % [iseq, info] = celleq(c1, c2, true, true)
    %
    % LICENSE
    % Copyright (c) 2016, The MathWorks, Inc.
    % All rights reserved.
    %
    % Redistribution and use in source and binary forms, with or without
    % modification, are permitted provided that the following conditions are met:
    %
    % * Redistributions of source code must retain the above copyright notice, this
    %   list of conditions and the following disclaimer.
    %
    % * Redistributions in binary form must reproduce the above copyright notice,
    %   this list of conditions and the following disclaimer in the documentation
    %   and/or other materials provided with the distribution.
    % * In all cases, the software is, and all modifications and derivatives of the
    %   software shall be, licensed to you solely for use in conjunction with
    %   MathWorks products and service offerings.
    %
    % THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    % AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    % IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    % DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
    % FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    % DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    % SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    % CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    % OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    % OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    if nargin < 3
        funh2string = false;
    end
    if nargin < 4
        ignorenan = false;
    end
    result = true; % Prove me wrong!
    why = struct('Reason','','Where','');
    if any(size(cell1) ~= size(cell2))
        result = false;
        why = struct('Reason','Sizes are different','Where','');
        return
    end
    for i = 1:numel(cell1)
        why = struct('Reason','','Where',sprintf('{%d}',i));
        if any(size(cell1{i}) ~= size(cell2{i}))
            result = false;
            why.Reason = 'Sizes are different';
            return
        end
        if ~strcmp(class(cell1{i}),class(cell2{i}))
            result = false;
            why.Reason = 'Different Classes/Types';
            return
        end
        % At this point we know they have the same size and class
        try
            whysub = struct('Reason',['Unequal ' class(cell1{i}) 's'],...
                            'Where','');
    
            switch class(cell1{i})
                case 'cell'
                    [result, whysub] = lib.celleq(cell1{i},cell2{i},funh2string, ...
                        ignorenan);
                case 'struct'
                    [result, whysub] = lib.structeq(cell1{i},cell2{i},funh2string, ...
                        ignorenan);
                case 'function_handle'
                    if funh2string
                        result = strcmp(func2str(cell1{i}), func2str(cell1{i}));
                    else
                        result = false;
                    end
                case {'double', 'single'}
                    if ignorenan
                        cell1{i}(isnan(cell1{i})) = 0;
                        cell2{i}(isnan(cell2{i})) = 0;
                    elseif any(isnan(cell1{i}(:)))
                        whysub.Reason = [whysub.Reason ' that contain NaNs'];
                    end
                    result = eq(cell1{i},cell2{i});
                otherwise
                    result = eq(cell1{i},cell2{i});
            end
            % result could be a vector
            result = all(result(:));
            if ~result
                why.Reason = sprintf('Unequal Subcell <- %s',whysub.Reason);
                why.Where = [why.Where whysub.Where];
                return;
            end
        catch ME
            result = false;
            why.Reason = ['Subcell comparison failed: ' ME.message];
            return
        end
    end
end