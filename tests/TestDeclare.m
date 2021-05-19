classdef TestDeclare < Prep
    % TestDeclare tests typical connection scenarios.
    methods (Test)
        function TestDeclare_delcareTable(testCase)
            % Test table declare with comments that has \{ or \}
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            conn1 = mym(-1, 'open', testCase.CONN_INFO.host, ...
                testCase.CONN_INFO.user, testCase.CONN_INFO.password, ...
                'false');
            
            mym(conn1, ['create database `' testCase.PREFIX '_declare`']);
            mym(['create table {S}(id int, data varchar(30) default null) COMMENT "\{username\}_\{subject_nickname\}"'], ...
                sprintf('`%s_%s`.`%s`', testCase.PREFIX, 'declare', 'test_table'));
            query_string = mym(sprintf('SHOW CREATE TABLE `%s_%s`.`%s`', testCase.PREFIX, 'declare', 'test_table'));

            if ~strfind(query_string.('Create Table'){1}, 'COMMENT=''{username}_{subject_nickname}''')
                throw(MException('TestDeclare:invalidDeclareTable',...
                    'Table comments got inserted incorrectly'));
            end
            mym(conn1, ['drop database `' testCase.PREFIX '_declare`']);
            mym(conn1, 'close');
        end
    end
end