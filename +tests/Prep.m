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
    end

    methods (TestClassSetup)
        function init(testCase)
            disp('---------------INIT---------------');
            clear functions;
            testCase.addTeardown(@testCase.dispose);
            addpath('./distribution/mexa64');

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
       
    methods (Static)
        function dispose()
            disp('---------------DISP---------------');
            warning('off','MATLAB:RMDIR:RemovedFromPath');
            
            curr_conn = mym(-1, 'open', tests.Prep.CONN_INFO_ROOT.host, ...
                tests.Prep.CONN_INFO_ROOT.user, tests.Prep.CONN_INFO_ROOT.password, ...
                'false');

            cmd = {...
            'DROP USER ''datajoint''@''%%'';'
            'DROP USER ''djview''@''%%'';'
            'DROP USER ''djssl''@''%%'';'
            };
            mym(curr_conn, sprintf('%s',cmd{:}));
            mym('closeall');

            warning('on','MATLAB:RMDIR:RemovedFromPath');
            rmpath('./distribution/mexa64')
        end
    end
end