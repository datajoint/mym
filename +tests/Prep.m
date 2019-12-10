classdef Prep < matlab.unittest.TestCase
    % Setup and teardown for tests.
    properties (Constant)
        CONN_INFO_ROOT = struct(...
            'host', getenv('DJ_HOST'), ...
            'user', getenv('DJ_USER'), ...
            'password', getenv('DJ_PASS'));
        CONN_INFO = struct(...
            'host', getenv('DJ_TEST_HOST'), ...
            'user', getenv('DJ_TEST_USER'), ...
            'password', getenv('DJ_TEST_PASSWORD'));
        PREFIX = 'djtest';
    end

    methods (TestClassSetup)
        function init(testCase)
            disp('---------------INIT---------------');
            clear functions;
            addpath(['./distribution/' mexext()]);

            curr_conn = mym(-1, 'open', testCase.CONN_INFO_ROOT.host, ...
                testCase.CONN_INFO_ROOT.user, testCase.CONN_INFO_ROOT.password, ...
                'false');
            
            res = mym(curr_conn, 'select @@version as version');
            if tests.lib.compareVersions(res.version,'5.8')
                cmd = {...
                'CREATE USER IF NOT EXISTS ''datajoint''@''%%'' '
                'IDENTIFIED BY ''datajoint'';'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));

                cmd = {...
                'GRANT ALL PRIVILEGES ON `djtest%%`.* TO ''datajoint''@''%%'';'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));

                cmd = {...
                'CREATE USER IF NOT EXISTS ''djview''@''%%'' '
                'IDENTIFIED BY ''djview'';'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));

                cmd = {...
                'GRANT SELECT ON `djtest%%`.* TO ''djview''@''%%'';'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));

                cmd = {...
                'CREATE USER IF NOT EXISTS ''djssl''@''%%'' '
                'IDENTIFIED BY ''djssl'' '
                'REQUIRE SSL;'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));

                cmd = {...
                'GRANT SELECT ON `djtest%%`.* TO ''djssl''@''%%'';'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));
            else
                cmd = {...
                'GRANT ALL PRIVILEGES ON `djtest%%`.* TO ''datajoint''@''%%'' '
                'IDENTIFIED BY ''datajoint'';'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));

                cmd = {...
                'GRANT SELECT ON `djtest%%`.* TO ''djview''@''%%'' '
                'IDENTIFIED BY ''djview'';'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));

                cmd = {...
                'GRANT SELECT ON `djtest%%`.* TO ''djssl''@''%%'' '
                'IDENTIFIED BY ''djssl'' '
                'REQUIRE SSL;'
                };
                mym(curr_conn, sprintf('%s',cmd{:}));
            end
            mym('closeall');
        end
    end
    methods (TestClassTeardown)
        function dispose(testCase)
            disp('---------------DISP---------------');
            warning('off','MATLAB:RMDIR:RemovedFromPath');
            
            curr_conn = mym(-1, 'open', testCase.CONN_INFO_ROOT.host, ...
                testCase.CONN_INFO_ROOT.user, testCase.CONN_INFO_ROOT.password, ...
                'false');

            mym(curr_conn, 'SET FOREIGN_KEY_CHECKS=0;');
            res = mym(curr_conn, ...
                ['SELECT CAST(schema_name AS char(50)) as db ' ...
                'FROM information_schema.schemata ' ...
                'where schema_name like "' testCase.PREFIX '_%";']);
            for i = 1:length(res.db)
                mym(curr_conn, ...
                    ['DROP DATABASE ' res.db{i} ';']);
            end
            mym(curr_conn, 'SET FOREIGN_KEY_CHECKS=1;');

            cmd = {...
            'DROP USER ''datajoint''@''%%'';'
            'DROP USER ''djview''@''%%'';'
            'DROP USER ''djssl''@''%%'';'
            };
            mym(curr_conn, sprintf('%s',cmd{:}));
            mym('closeall');

            warning('on','MATLAB:RMDIR:RemovedFromPath');
            rmpath(['./distribution/' mexext()])
        end
    end
end