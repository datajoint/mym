function [result, why] = structeq(struct1, struct2, funh2string, ignorenan)
    % STRUCTEQ performs an equality comparison between two structures by
    % recursively comparing the elements of the struct array, their fields and
    % subfields. This function requires companion function CELLEQ to compare
    % two cell arrays.
    %
    % USAGE:
    %
    % structeq(struct1, struct2)
    %       Performs a comparison and returns true if all the subfields and
    %       properties of the structures are identical. It will fail if
    %       subfields include function handles or other objects which don't
    %       have a defined eq method. 
    %
    % [iseq, info] = structeq(struct1, struct2)
    %       This syntax returns a logical iseq and a second output info which
    %       is a structure that contains a field "Reason" which gives you a
    %       text stack of why the difference occurred as well as a field
    %       "Where" which contains the indices and subfields of the structure
    %       where the comparison failed. If iseq is true, info contains empty
    %       strings in its fields.
    %
    % [...] = structeq(struct1, struct2, funh2string, ignorenan)
    %       Illustrates an alternate syntax for the function with additional
    %       input arguments. See the help for CELLEQ for more information on the
    %       meaning of the arguments
    %
    % METHOD:
    % 1. Compare sizes of struct arrays
    % 2. Compare numbers of fields
    % 3. Compare field names of the arrays
    % 4. For every element of the struct arrays, convert the field values into
    % a cell array and do a cell array comparison recursively (this can result
    % in multiple recursive calls to CELLEQ and STRUCTEQ)
    %
    % EXAMPLE:
    % % Compare two handle graphics hierarchies
    % figure;
    % g = surf(peaks(50));
    % rotate3d
    % hg1 = handle2struct(gcf);
    % set(g,'XDataMode', 'manual');
    % hg2 = handle2struct(gcf);
    % 
    % structeq(hg1, hg2)
    % [iseq, info] = structeq(hg1, hg2)
    % [iseq, info] = structeq(hg1, hg2, true)
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
    why = struct('Reason','','Where','');
    if any(size(struct1) ~= size(struct2))
        result = false;
        why = struct('Reason','Sizes are different',Where,'');
        return
    end
    fields1 = fieldnames(struct1);
    fields2 = fieldnames(struct2);
    % Check field lengths
    if length(fields1) ~= length(fields2)
        result = false;
        why = struct('Reason','Number of fields are different','Where','');
        return
    end
    % Check field names
    result = tests.lib.celleq(fields1,fields2);
    result = all(result);
    if ~result
        why = struct('Reason','Field names are different','Where','');
        return
    end
    for i = 1:numel(struct1)
        props1 = struct2cell(struct1(i));
        props2 = struct2cell(struct2(i));
        [result, subwhy] = tests.lib.celleq(props1,props2,funh2string,ignorenan);
        result = all(result);
        if ~result
    
            [fieldidx, subwhy.Where] = strtok(subwhy.Where, '}');
            fieldidx = str2double(fieldidx(2:end));
            %str2double(regexp(subwhy.Where,'{([0-9]+)}','tokens','once'));
            where = sprintf('(%d).%s%s',i,fields1{fieldidx},subwhy.Where(2:end));
            why = struct('Reason',sprintf('Properties are different <- %s', ...
                subwhy.Reason),'Where',where);
            return
        end
    end
end