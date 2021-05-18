classdef TestDeclare < Prep
    % TestDeclare tests typical connection scenarios.
    methods (Test)
        function TestDeclare_delcareTable(testCase)
            % force new connections test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            conn1 = mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');
            
            mym(conn1, ['create database `' testCase.PREFIX '_declare`']);
            mym(['create table `' testCase.PREFIX '_declare`.`test_table` ' ...
                '(id int, data varchar(30) default null) COMMENT "\{username\}_\{subject_nickname\}"']);
            mym(conn1, ['drop database `' testCase.PREFIX '_declare`']);
            mym(conn1, 'close');
        end
    end
end