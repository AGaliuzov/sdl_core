# Short developers automated smoke with ATF

## HOW-TO :

### 1. Clone ATF and build it
https://github.com/CustomSDL/sdl_atf

### 2. Build and install SDL
```
$ cd $SDL_BUILD_DIR
$ cmake $SDL_SOURCE_DIR
$ make install
```
*For more complicated build please use [Build and run steps](https://github.com/CustomSDL/sdl_panasonic/blob/APPLINK-20481_ATF_ShortSmokeTests/build_and_run_steps.txt)*

### 3. Run ATF short smoke tests
```
$ cd $SDL_BUILD_DIR/atf
$ bash -e ./build_server.sh $ATF_BUILD_DIR
```

#### Used variables description:
* SDL_BUILD_DIR - Path to SDL build directory
* ATF_BUILD_DIR - Path to ATF build directory

#### Additional parameters
You can setup enviromat variables to modify options:
* EXECUTE_DIR - root directory to run ATF scripts (default is current directory)
* SDL_BIN_DIR - path to directory with SmartDeviceLinkCore
* REPORTS_DIR - path for ATF reports
* REPORTS_WEB - aces to reports by web ( will be inserved in html report)
