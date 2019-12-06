classdef TestInsertFetch < tests.Prep
    % TestExternal tests inserting and fetching the same result.
    methods (Test)
        function testInsertFetch(testCase)
            % insert/fetch test.
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');
            mym(curr_conn, 'create database `djtest_insert`;');

            testCase.check(curr_conn, 'varchar(7)','','S','raphael');
            testCase.check(curr_conn, 'longblob','','M',int64([1;2]));
            testCase.check(curr_conn, 'varchar(4)','','S','ýýýý');

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx)';
            testCase.check(curr_conn, 'binary(16)','uuid','B',v);

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx);
            v_char = char(v)';
            testCase.check(curr_conn, 'varchar(16)','','S',v_char);

            mym('closeall');
        end
    end
    methods (Static)
        function check(conn_id, mysql_datatype, dj_datatype, flag, data)
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
            ret = res.name{1};
            disp(ret);
            disp(data);
            assert(all(ret == data));
        end
    end
end