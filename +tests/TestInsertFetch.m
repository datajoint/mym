classdef TestInsertFetch < tests.Prep
    % TestExternal tests inserting and fetching the same result.
    methods (Test)
        function TestInsertFetch_testInsertFetch(testCase)
            % insert/fetch test.
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(curr_conn, 'create database `djtest_insert`;');

            testCase.TestInsertFetch_check(curr_conn, 'varchar(7)','','S','raphael');
            testCase.TestInsertFetch_check(curr_conn, 'longblob','','M',int64([1;2]));
            testCase.TestInsertFetch_check(curr_conn, 'varchar(4)','','S','ýýýý');
            testCase.TestInsertFetch_check(curr_conn, 'datetime','','S','2018-01-24 14:34:16');

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx)';
            testCase.TestInsertFetch_check(curr_conn, 'binary(16)','uuid','B',v);

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx);
            v_char = char(v)';
            testCase.TestInsertFetch_check(curr_conn, 'varchar(16)','','S',v_char);

            mym(curr_conn, 'close');
        end
        function TestInsertFetch_testNullableBlob(testCase)
            % https://github.com/datajoint/datajoint-matlab/issues/195
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(curr_conn, 'create database `djtest_nullable`');
            mym(['create table `djtest_nullable`.`blob_field` ' ...
                '(id int, data longblob default null)']);
            mym('insert into `djtest_nullable`.`blob_field` (`id`) values (0)');
            res = mym(curr_conn, 'select * from `djtest_nullable`.`blob_field`');
            mym(curr_conn, 'close');
        end
    end
    methods (Static)
        function TestInsertFetch_check(conn_id, mysql_datatype, dj_datatype, flag, data)
            table_name = ['test_' strrep(mysql_datatype, '(', '')];
            table_name = strrep(table_name, ')', '');
            if dj_datatype
                dj_type_comment = [' comment ":' dj_datatype ':"'];
            else
                dj_type_comment = [];
            end
            mym(conn_id, ['create table `djtest_insert`.`' table_name ...
                '` (id int, name ' mysql_datatype dj_type_comment ');']);
            mym(conn_id, ['INSERT INTO `djtest_insert`.`' table_name ...
                '` (`id`,`name`) VALUES (0,"{' flag '}") '],data);
            res = mym(conn_id, ['select * from `djtest_insert`.`' table_name '`']);
            % Verify that multiple fetch will return same data.
            res = mym(conn_id, ['select * from `djtest_insert`.`' table_name '`']);
            ret = res.name{1};
            assert(all(ret == data));
        end
    end
end