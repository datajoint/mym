classdef TestExternal < Prep
    % TestExternal tests external storage serialization/deserialization.
    methods (Test)
        function TestExternal_testArraySerialization(testCase)
            % array serialization test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            multi(:,:,1) = [4,3;2,7];
            multi(:,:,2) = [0,5;3,8];
            array_tests = {
                [5,10],
                [2,3,4;4,5,3;5,9,7],
                [5],
                1,
                [],
                [2.3,2.56,2.45],
                [4.5342;123.3145;345.2133],
                multi,
                'hello',
                ''
            };
            testCase.TestExternal_verify(testCase, array_tests);
        end 
        function TestExternal_testStructSerialization(testCase)
            % struct serialization test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            multi(:,:,1) = [4,3;2,7];
            multi(:,:,2) = [0,5;3,8];
            array_tests = {
                struct(...
                    'x',[8,5,3;4,6,7;4,8,2],...
                    'y',[4,2,8;5,5,8;3,3,8]...
                ),
                struct('sample',multi),
                struct('lvl1',struct('lvl2',multi)),
                struct('sample','hi'),
                struct('lvl1',struct('lvl2',{[8,5,3;4,6,7;4,8,2], ...
                    {'now',{[4,2,8;5,5,8;3,3,8]}}, {multi}})),
                % ,struct("sample","bye")
            };
            testCase.TestExternal_verify(testCase, array_tests);
        end
        function TestExternal_testCellSerialization(testCase)
            % cell serialization test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            multi(:,:,1) = [4,3;2,7];
            multi(:,:,2) = [0,5;3,8];
            array_tests = {
                {[8,5,3;4,6,7;4,8,2], [4,2,8;5,5,8;3,3,8]},
                {[8,5,3;4,6,7;4,8,2], [4,2,8;5,5,8;3,3,8], multi},
                {[8,5,3;4,6,7;4,8,2], {0,{[4,2,8;5,5,8;3,3,8]}}, {multi}},
                {'bye', 'now'},
                {[8,5,3;4,6,7;4,8,2], {struct('lvl1',struct('lvl2',multi)), ...
                    {[4,2,8;5,5,8;3,3,8],'yes'}}, {multi}}
            };
            testCase.TestExternal_verify(testCase, array_tests);
        end
        function TestExternal_testdj0Error(testCase)
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            % https://github.com/datajoint/mym/issues/43
            % normal dj0
            % python: pack([1,2,3]).hex().upper()
            value = ['646A300002030000000000000004000000000000000A010001040000000000000' ...
                '00A01000204000000000000000A010003'];
            hexstring = value';
            reshapedString = reshape(hexstring,2,length(value)/2);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            packed = uint8(decMtx);
            try
                unpacked = mym('deserialize', packed);
            catch ME
                testCase.verifyEqual(ME.identifier, 'mYm:CrossPlatform:Compatibility');
            end
            % compressed dj0
            % python: pack([1,2,3]*28).hex().upper()
            value = ['5A4C31323300FD03000000000000789C4BC93260600A6180001628CDC5C8C088C4' ...
                '664262338FAA195533AA6678A80100444706E9'];
            hexstring = value';
            reshapedString = reshape(hexstring,2,length(value)/2);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            packed = uint8(decMtx);
            try
                unpacked = mym('deserialize', packed);
            catch ME
                testCase.verifyEqual(ME.identifier, 'mYm:CrossPlatform:Compatibility');
            end
        end
        function TestExternal_test32BitRead(testCase)
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            % https://github.com/datajoint/mym/issues/86
            value = ['6D596D00530200000001000000010000000400000068697473007369646573007461736B73007374616765004D000000410200000001000000070000000600000000000000000000000000F8FF000000000000F03F000000000000F03F0000000000000000000000000000F03F0000000000000000000000000000F8FF230000004102000000010000000700000004000000000000006C006C006C006C00720072006C002300000041020000000100000007000000040000000000000064006400640064006400640064002500000041020000000100000008000000040000000000000053007400610067006500200031003000'];
            hexstring = value';
            reshapedString = reshape(hexstring,2,length(value)/2);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            packed = uint8(decMtx);
            try
                setenv('USE_32BIT_DIMS', 'true');
                unpacked = mym('deserialize', packed);
                setenv('USE_32BIT_DIMS', 'false');
            catch ME
                testCase.verifyEqual(ME.identifier, 'mYm:CrossPlatform:Compatibility');
            end
        end
    end
    methods (Static)
        function TestExternal_verify(testCase, array_tests)
            for i = 1 : length(array_tests)
                % single serialization test
                packed_cell = mym('serialize {M}', array_tests{i});
                packed = packed_cell{1};
                unpacked = mym('deserialize', packed);
                testCase.verifyTrue(lib.celleq(array_tests(i),{unpacked}));
            end
            % Check multiple at once (extras are ignored)
            packed_cell = mym('serialize {M}, {M}, {M}, {M}, {M}, {M}, {M}, {M}, {M}, {M}', ...
                array_tests{:,1});
            unpacked = cellfun(@(x) mym('deserialize', x), packed_cell,'UniformOutput',false)';
            testCase.verifyTrue(lib.celleq(array_tests',unpacked));
        end
    end
end