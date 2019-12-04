classdef TestInsertFetch < tests.Prep
    % TestExternal tests external storage serialization/deserialization.
    methods (Test)
        function testInsertFetch(testCase)
            % array serialization test
            st = dbstack;
            disp(['---------------' st(1).name '---------------']);
            curr_conn = mym(-1, 'open', testCase.CONN_INFO.host, testCase.CONN_INFO.user, ...
                testCase.CONN_INFO.password, 'false');

            mym(curr_conn, 'drop database if exists `djtest_insert`;');
            mym(curr_conn, 'create database `djtest_insert`;');
            
            
            data = 'raphael';
            ret = testCase.check(curr_conn, 'varchar(7)','S',data);
            % disp(ret);
            % disp(data);
            assert(all(ret == data));

            data = int64([1;2]);
            ret = testCase.check(curr_conn, 'longblob','M',data);
            % disp(ret);
            % disp(data);
            assert(all(ret == data));


            data = 'ýýýý';
            ret = testCase.check(curr_conn, 'varchar(8)','S',data);
            % disp(ret);
            % disp(data);
            assert(all(ret == data));


            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx)';

            ret = testCase.check(curr_conn, 'binary(16)','B',v);
            % disp(ret);
            % disp(v);
            assert(all(ret == v));


            data = '1d751e2e-1e74-faf8-4ab4-85fde8ef72be';
            data = strrep(data, '-', '');
            v = data;
            hexstring = v';
            reshapedString = reshape(hexstring,2,16);
            hexMtx = reshapedString.';
            decMtx = hex2dec(hexMtx);
            v = uint8(decMtx);
            v_char = char(v)';

            ret = testCase.check(curr_conn, 'varchar(24)','S',v_char);
            % disp(ret);
            % disp(v_char);
            assert(all(ret == v_char));
            


            mym('closeall');
        end 
    end
    methods (Static)
        function ret = check(conn_id, datatype, flag, data)
            mym(conn_id, ['create table `djtest_insert`.`test_' datatype '` (id int, name ' datatype ' comment ":' datatype ':");']);


            % mym(curr_conn, 'insert into `djtest_insert`.`test` (id, name) values (0, "raphael");');
            mym(conn_id, ['INSERT INTO `djtest_insert`.`test_' datatype '` (`id`,`name`) VALUES (0,"{' flag '}") '],data);
            
            
            res = mym(conn_id, ['select * from `djtest_insert`.`test_' datatype '`']);
            ret = res.name{1};
        end
    end
end