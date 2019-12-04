classdef TestInsertFetch < tests.Prep
    % TestExternal tests external storage serialization/deserialization.
    methods (Test)
        function testInsertFetch(testCase)
            % array serialization test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');     
            mym(curr_conn, 'create database `djtest_insert`;');
            
            testCase.check(curr_conn, 'varchar(7)','S','raphael');
            testCase.check(curr_conn, 'longblob','M',int64([1;2]));
            testCase.check(curr_conn, 'varchar(8)','S','ýýýý');

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx)';
            testCase.check(curr_conn, 'binary(16)','B',v);

            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx);
            v_char = char(v)';
            testCase.check(curr_conn, 'varchar(24)','S',v_char);

            mym('closeall');
        end 
    end
    methods (Static)
        function check(conn_id, datatype, flag, data)
            mym(conn_id, ['create table `djtest_insert`.`test_' datatype '` (id int, name ' datatype ' comment ":' datatype ':");']);
            mym(conn_id, ['INSERT INTO `djtest_insert`.`test_' datatype '` (`id`,`name`) VALUES (0,"{' flag '}") '],data);
            res = mym(conn_id, ['select * from `djtest_insert`.`test_' datatype '`']);
            ret = res.name{1};
            assert(all(ret == data));
        end
    end
end