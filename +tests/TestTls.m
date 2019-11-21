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
            mym('closeall');
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
            mym('closeall');
        end
        function testPreferredConn(testCase)
            % preferred connection test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            check(mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password))
            
            check(mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'none'))
            
            check(mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'anything'))

            function check(conn_id)
                connections = evalc("mym('status')");
                connections = splitlines(connections);
                connections(end)=[];
                testCase.verifyTrue(contains(connections{conn_id+1},'encrypted'));

                res = mym(conn_id, 'SHOW STATUS LIKE ''Ssl_cipher'';');
                testCase.verifyTrue(length(res.Value{1}) > 0);
            end
            mym('closeall');
        end
        function testRejectException(testCase)
            % test exception on require TLS
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            try
                curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, ...
                    'djssl', 'djssl', 'false');
                testCase.verifyTrue(false);
            catch
                e = lasterror;
                testCase.verifyEqual(e.identifier, 'MySQL:Error');
                testCase.verifyTrue(contains(e.message,...
                    ["requires secure connection","Access denied"])); %MySQL8,MySQL5
            end          
            mym('closeall');
        end
    end
end