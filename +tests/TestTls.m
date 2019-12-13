classdef TestTls < tests.Prep
    % TestTls tests TLS connection scenarios.
    methods (Test)
        function testSecureConn(testCase)
            % secure connection test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'true');

            connections = evalc("mym('status')");
            connections = splitlines(connections);
            connections(end)=[];
            testCase.verifyTrue(contains(connections{curr_conn+1},'encrypted'));

            res = mym(curr_conn, 'SHOW STATUS LIKE ''Ssl_cipher'';');
            testCase.verifyTrue(length(res.Value{1}) > 0);
            mym(curr_conn, 'close');
        end
        function testInsecureConn(testCase)
            % insecure connection test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');

            connections = evalc("mym('status')");
            connections = splitlines(connections);
            connections(end)=[];
            testCase.verifyTrue(~contains(connections{curr_conn+1},'encrypted'));

            res = mym(curr_conn, 'SHOW STATUS LIKE ''Ssl_cipher'';');
            testCase.verifyEqual( ...
                res.Value{1}, ...
                '');
            mym(curr_conn, 'close');
        end
        function testPreferredConn(testCase)
            % preferred connection test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            conn = [];
            conn(1) = check(mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password));

            conn(2) = check(mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, 'none'));

            conn(3) = check(mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, 'anything'));

            for idx = 1:length(conn)
                mym(conn(idx), 'close');
            end

            function conn_id = check(conn_id)
                connections = evalc("mym('status')");
                connections = splitlines(connections);
                connections(end)=[];
                testCase.verifyTrue(contains(connections{conn_id+1},'encrypted'));

                res = mym(conn_id, 'SHOW STATUS LIKE ''Ssl_cipher'';');
                testCase.verifyTrue(length(res.Value{1}) > 0);
            end
        end
        function testRejectException(testCase)
            % test exception on require TLS
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            try
                curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, ...
                    'djssl', 'djssl', 'false');
                testCase.verifyTrue(false);
                mym(curr_conn, 'close');
            catch
                e = lasterror;
                testCase.verifyEqual(e.identifier, 'MySQL:Error');
                testCase.verifyTrue(contains(e.message,...
                    ["requires secure connection","Access denied"])); %MySQL8,MySQL5
            end
        end
    end
end