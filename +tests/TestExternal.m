classdef TestExternal < tests.Prep
    % TestExternal tests external storage serialization/deserialization.
    methods (Test)
        function testArraySerialization(testCase)
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

            % user cellfun
            for i = 1 : length(array_tests)
                % disp(array_tests{i});
                % bool = testCase.repack(array_tests{i}) == array_tests{i};
                % testCase.verifyTrue(sum(bool,'all') == numel(bool));
                unpacked = testCase.repack(array_tests{i});
                % testCase.verifyTrue(isequal(array_tests{i},unpacked));
                testCase.verifyTrue(tests.lib.celleq(array_tests(i),{unpacked}));

            end
        end 
        function testStructSerialization(testCase)
            % struct serialization test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);

            % import matlab.unittest.constraints.IsEqualTo;
            % import matlab.unittest.constraints.StructComparator;
            % import matlab.unittest.constraints.NumericComparator;

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
                struct('lvl1',struct('lvl2',{[8,5,3;4,6,7;4,8,2], {'now',{[4,2,8;5,5,8;3,3,8]}}, {multi}})),
                % ,struct("sample","bye")
            };

            % user cellfun
            for i = 1 : length(array_tests)
                % disp(array_tests{i});
                unpacked = testCase.repack(array_tests{i});
                % disp(unpacked);

                % testCase.verifyThat(array_tests{i}, IsEqualTo(unpacked, 'Using', ...
                %     StructComparator(NumericComparator)))

                testCase.verifyTrue(tests.lib.celleq(array_tests(i),{unpacked}));
            end
        end
        function testCellSerialization(testCase)
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
                {[8,5,3;4,6,7;4,8,2], {struct('lvl1',struct('lvl2',multi)),{[4,2,8;5,5,8;3,3,8],'yes'}}, {multi}}
            };

            % user cellfun
            for i = 1 : length(array_tests)
                % disp(array_tests{i});
                unpacked = testCase.repack(array_tests{i});
                % disp(unpacked);

                testCase.verifyTrue(tests.lib.celleq(array_tests(i),{unpacked}));
            end

            % % curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, ...
            % %     testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
            % %     'false');

            % orig = {[8,5,3;4,6,7;4,8,2], [4,2,8;5,5,8;3,3,8]};

            % packed_cell = mym('serialize {M}', orig);
            % packed = packed_cell{1};
            % unpacked = mym('deserialize',packed);

            % testCase.verifyTrue(sum(norm(orig{1}-unpacked{1},2)+norm(orig{2}-unpacked{2},2)) == 0);
            % % mym('closeall');
        end
    end
    methods (Static)
        function unpacked = repack(data)
            % serialization test

            packed_cell = mym('serialize {M}', data);
            packed = packed_cell{1};
            unpacked = mym('deserialize',packed);
        end
    end
end