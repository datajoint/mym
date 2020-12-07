classdef TestInsertFetch < Prep
    % TestExternal tests inserting and fetching the same result.
    methods (Test)
        function TestInsertFetch_testInsertFetch(testCase)
            % insert/fetch test.
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(curr_conn, ['create database `' testCase.PREFIX  '_insert`;']);

            % test random string
            testCase.TestInsertFetch_check(testCase, curr_conn, 'varchar(7)','','S','raphael');
            % test 8-byte ASCII string
            testCase.TestInsertFetch_check(testCase, curr_conn, 'varchar(32)','','S', ...
                'lteachen');
            testCase.TestInsertFetch_check(testCase, curr_conn, 'longblob','','M', ...
                int64([1;2]));
            testCase.TestInsertFetch_check(testCase, curr_conn, 'varchar(4)','','S','ýýýý');
            testCase.TestInsertFetch_check(testCase, curr_conn, 'datetime','','S', ...
                '2018-01-24 14:34:16');

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx)';
            testCase.TestInsertFetch_check(testCase, curr_conn, 'binary(16)','uuid','B',v);

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx);
            v_char = char(v)';
            testCase.TestInsertFetch_check(testCase, curr_conn, 'varchar(16)','','S',v_char);

            mym(curr_conn, 'close');
        end
        function TestInsertFetch_testNullableBlob(testCase)
            % https://github.com/datajoint/datajoint-matlab/issues/195
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(curr_conn, ['create database `' testCase.PREFIX '_nullable`']);
            mym(['create table `' testCase.PREFIX '_nullable`.`blob_field` ' ...
                '(id int, data longblob default null)']);
            mym(['insert into `' testCase.PREFIX '_nullable`.`blob_field` (`id`) values (0)']);
            res = mym(curr_conn, ['select * from `' testCase.PREFIX '_nullable`.`blob_field`']);
            mym(curr_conn, 'close');
        end
        function TestInsertFetch_testString(testCase)
            % https://github.com/datajoint/mym/issues/47
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(curr_conn, ['create database `' testCase.PREFIX '_string`']);
            mym(['create table `' testCase.PREFIX '_string`.`various` ' ...
                '(id int, data varchar(30) default null)']);
            mym(['insert into `' testCase.PREFIX '_string`.`various` (`id`, `data`) ' ...
                'values (0, "{S}")'], '');
            res = mym(curr_conn, ['select length(data) as len from `' testCase.PREFIX ...
                '_string`.`various`']);
            testCase.verifyEqual(double(res.len), 0);
            mym(curr_conn, 'close');
        end
        function TestInsertFetch_testdj0Error(testCase)
            % https://github.com/datajoint/mym/issues/43
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(curr_conn, ['create database `' testCase.PREFIX '_dj0`']);
            mym(['create table `' testCase.PREFIX '_dj0`.`blob` ' ...
                '(id int, data longblob)']);
            % normal dj0
            % python: pack([1,2,3]).hex().upper()
            mym(['insert into `' testCase.PREFIX '_dj0`.`blob` (`id`, `data`) values (0, ' ...
                'X''646A300002030000000000000004000000000000000A01000104000000000000000A0' ...
                '1000204000000000000000A010003'')']);
            try
                res = mym(curr_conn, ...
                    ['select * from `' testCase.PREFIX '_dj0`.`blob` where id=0']);
                assert(false);
            catch ME
                mym(curr_conn, 'close');
                if ~strcmp(ME.identifier,'mYm:CrossPlatform:Compatibility')
                    rethrow(ME);
                end
            end
            % compressed dj0
            % python: pack([1,2,3]*28).hex().upper()
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(['insert into `' testCase.PREFIX '_dj0`.`blob` (`id`, `data`) values (0, ' ...
                'X''5A4C31323300FD03000000000000789C4BC93260600A6180001628CDC5C8C088C4664' ...
                '262338FAA195533AA6678A80100444706E9'')']);
            try
                res = mym(curr_conn, ...
                    ['select * from `' testCase.PREFIX '_dj0`.`blob` where id=0']);
                assert(false);
            catch ME
                mym(curr_conn, 'close');
                if ~strcmp(ME.identifier,'mYm:CrossPlatform:Compatibility')
                    rethrow(ME);
                end
            end
        end
    end
    methods (Static)
        function TestInsertFetch_check(testCase, conn_id, mysql_datatype, dj_datatype, ...
                flag, data)
            table_name = ['test_' strrep(mysql_datatype, '(', '')];
            table_name = strrep(table_name, ')', '');
            if dj_datatype
                dj_type_comment = [' comment ":' dj_datatype ':"'];
            else
                dj_type_comment = [];
            end
            mym(conn_id, ['create table `' testCase.PREFIX '_insert`.`' table_name ...
                '` (id int, name ' mysql_datatype dj_type_comment ');']);
            mym(conn_id, ['INSERT INTO `' testCase.PREFIX '_insert`.`' table_name ...
                '` (`id`,`name`) VALUES (0,"{' flag '}") '],data);
            res = mym(conn_id, ['select * from `' testCase.PREFIX '_insert`.`' table_name '`']);
            % Verify that multiple fetch will return same data.
            res = mym(conn_id,['select * from `' testCase.PREFIX '_insert`.`' table_name '`']);
            ret = res.name{1};
            testCase.verifyTrue(all(ret == data));
        end
    end
end