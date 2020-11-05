[![View mym on File Exchange](https://www.mathworks.com/matlabcentral/images/matlab-file-exchange.svg)](https://www.mathworks.com/matlabcentral/fileexchange/81208-mym)

## mym

MySQL API for MATLAB with support for BLOB objects

MYM - Interact with a MySQL database server 
       Copyright 2005, EPFL (Yannick Maret)

Copyright notice: this code is a heavily modified version of the original
work of Robert Almgren from University of Toronto [sourceforge project](http://sourceforge.net/projects/mym/)

See mym.m for further documentation.

## Running Tests Locally

* Create an `.env` with desired development environment values e.g.
``` sh
MATLAB_USER=raphael
MATLAB_LICENSE="#\ BEGIN----...---------END" # For image usage instructions see https://github.com/guzman-raphael/matlab, https://hub.docker.com/r/raphaelguzman/matlab
MATLAB_VERSION=R2018b
MATLAB_HOSTID=XX:XX:XX:XX:XX:XX
MATLAB_UID=1000
MATLAB_GID=1000
MYSQL_TAG=5.7
```
* `cp local-docker-compose.yml docker-compose.yml`
* `docker-compose up` (Note configured `JUPYTER_PASSWORD`)
* Select a means of running MATLAB e.g. Jupyter Notebook, GUI, or Terminal (see bottom)
* Run desired tests. Some examples are as follows:

| Use Case                     | MATLAB Code                                                                    |
| ---------------------------- | ------------------------------------------------------------------------------ |
| Run all tests                | `run(Main)`                                                              |
| Run one class of tests       | `run(TestTls)`                                                           |
| Run one specific test        | `runtests('TestTls/TestTls_testInsecureConn')`                                   |
| Run tests based on test name | `import matlab.unittest.TestSuite;`<br>`import matlab.unittest.selectors.HasName;`<br>`import matlab.unittest.constraints.ContainsSubstring;`<br>`suite = TestSuite.fromClass(?Main, ... `<br><code>&nbsp;&nbsp;&nbsp;&nbsp;</code>`HasName(ContainsSubstring('Conn')));`<br>`run(suite)`|


### Launch Jupyter Notebook
* Navigate to `localhost:8888`
* Input Jupyter password
* Launch a notebook i.e. `New > MATLAB`


### Launch MATLAB GUI (supports remote interactive debugger)
* Shell into `mym_app_1` i.e. `docker exec -it mym_app_1 bash`
* Launch Matlab by runnning command `matlab`


### Launch MATLAB Terminal
* Shell into `mym_app_1` i.e. `docker exec -it mym_app_1 bash`
* Launch Matlab with no GUI by runnning command `matlab -nodisplay`


## Installation

### (Recommended) Using GHToolbox (FileExchange Community Toolbox)

1. Install *GHToolbox* using using an appropriate method in https://github.com/datajoint/GHToolbox
2. run: `ghtb.install('datajoint/mym')`

### Greater than R2016b

1. Utilize MATLAB built-in GUI i.e. *Top Ribbon -> Add-Ons -> Get Add-Ons*
2. Search and Select `mym`
3. Select *Add from GitHub*

### Less than R2016b

1. Utilize MATLAB built-in GUI i.e. *Top Ribbon -> Add-Ons -> Get Add-Ons*
2. Search and Select `mym`
3. Select *Download from GitHub*
4. Save `mym.mltbx` locally
5. Navigate in MATLAB tree browser to saved toolbox file
6. Right-Click and Select *Install*
7. Select *Install*

### From Source

1. Download `mym.mltbx` locally
2. Navigate in MATLAB tree browser to saved toolbox file
3. Right-Click and Select *Install*
4. Select *Install*
