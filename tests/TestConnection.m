classdef TestConnection < Prep
    % TestConnection tests typical connection scenarios.
    methods (Test)
        function TestConnection_testNewConnection(testCase)
            % force new connections test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            conn1 = mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            conn2 = mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            connections = evalc('mym(''status'')');
            connections = splitlines(connections);
            connections(end)=[];

            testCase.verifyEqual(length(connections), 2);
            mym(conn1, 'close');
            mym(conn2, 'close');
        end
        function TestConnection_testReuseConnection(testCase)
            % reuse existing connections test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            conn1 = mym('open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            conn2 = mym('open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');

            connections = evalc('mym(''status'')');
            connections = splitlines(connections);
            connections(end)=[];

            testCase.verifyEqual(conn1, conn2);
            testCase.verifyEqual(length(connections), 1);
            mym(conn1, 'close');
        end
    end
end