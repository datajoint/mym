classdef TestConnection < tests.Prep
    % TestConnection tests typical connection scenarios.
    methods (Test)
        function testNewConnection(testCase)
            % force new connections test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            connections = evalc("mym('status')");
            connections = splitlines(connections);
            connections(end)=[];

            testCase.verifyEqual(length(connections), 2);
            mym('closeall');
        end
        function testReuseConnection(testCase)
            % reuse existing connections test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            conn1 = mym('open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            conn2 = mym('open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            connections = evalc("mym('status')");
            connections = splitlines(connections);
            connections(end)=[];

            testCase.verifyEqual(conn1, conn2);
            testCase.verifyEqual(length(connections), 1);
            mym('closeall');
        end
    end
end