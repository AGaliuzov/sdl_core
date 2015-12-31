#! /bin/bash
# predefined variables:
#	WORKSPACE should contain sdl by path build/bin and ATF by path ATFE
EXECUTE_DIR=${EXECUTE_DIR:-`pwd`} 
SDL_BIN_DIR=${SDL_BIN_DIR:-`readlink -f $EXECUTE_DIR/../bin`}
if [ -z "$1" ];
    then echo "Please pass path to atf build dir as first parameter";
    exit 1;
fi
ATF_BIN_DIR=$1
REPORTS_DIR=${REPORTS_DIR:-$EXECUTE_DIR/reports}
REPORTS_WEB=${REPORTS_WEB:-$REPORTS_DIR} #   http://zln-ford-01.luxoft.com:8080/job/develop_pasa_atf/ws/reports
echo EXECUTE_DIR=$EXECUTE_DIR
echo SDL_BIN_DIR=$SDL_BIN_DIR
echo ATF_BIN_DIR=$ATF_BIN_DIR
echo REPORTS_DIR=$REPORTS_DIR
echo REPORTS_WEB=$REPORTS_WEB

is_sdl_alive(){
    ps aux | grep smartDeviceLinkCore
    RES=$(ps aux | grep smartDeviceLinkCore | wc -l)
    if [ $RES = "2" ]; then return 0; else return 1; fi
}

kill_sdl() {
    if is_sdl_alive ; then
	ps aux | grep smartDeviceLinkCore | grep -v grep |  awk '{print $2}'| xargs kill -9
    fi
}

read_core(){
	cd $SDL_BIN_DIR
	if [ -f $SDL_BIN_DIR/core ]; then
	  RESULT=1
	  echo "bt" |  gdb ./smartDeviceLinkCore core;
	  echo "thread all apply bt" |  gdb ./smartDeviceLinkCore core;
	fi
}

#Run SDL
clear_sdl() {
	kill_sdl
	cp $EXECUTE_DIR/sdl_preloaded_pt.json $SDL_BIN_DIR/
	cp $EXECUTE_DIR/smartDeviceLink.ini $SDL_BIN_DIR/smartDeviceLink.ini
	rm -f $SDL_BIN_DIR/app_info.dat
	rm -rf $SDL_BIN_DIR/storage
    rm -rf $SDL_BIN_DIR/*.log
	rm -f $EXECUTE_DIR/sdl.pid
    ls -la $SDL_BIN_DIR
}

# Prepare enviroment
cp $ATF_BIN_DIR/interp $EXECUTE_DIR/
cp $ATF_BIN_DIR/start.sh $EXECUTE_DIR/
cp $ATF_BIN_DIR/StopSDL.sh $EXECUTE_DIR/
cp $ATF_BIN_DIR/StartSDL.sh $EXECUTE_DIR/
cp $ATF_BIN_DIR/WaitClosingSocket.sh $EXECUTE_DIR/
cp -r $ATF_BIN_DIR/modules $EXECUTE_DIR/
cp -r $ATF_BIN_DIR/data $EXECUTE_DIR/
cp -r $EXECUTE_DIR/config.lua $EXECUTE_DIR/modules/config.lua


#Run ATF
cd $EXECUTE_DIR
echo "" > console_output
rm -rf $REPORTS_DIR
mkdir $REPORTS_DIR
RESULT=0
kill_sdl
for test_file in ./tests/*.lua; do
    echo "Execute Test file : $test_file" | tee -a console_output
    SCRIPT_REPORTS_DIR=$REPORTS_DIR/$(basename -s .lua $test_file)_reports
    mkdir $SCRIPT_REPORTS_DIR
    clear_sdl
    rm -rf TestingReports
    ./start.sh --storeFullSDLLogs --sdl_core=$(readlink -f $SDL_BIN_DIR)/ $test_file| tee -a console_output
    if is_sdl_alive ; then
        kill_sdl
    fi
    cp -r ./TestingReports/* $SCRIPT_REPORTS_DIR/
done

FAIL_COUNT=$(cat console_output | grep '\[FAIL\]' | wc -l)
if [ $FAIL_COUNT != "0" ]; then
  RESULT=1
fi

cp console_output $REPORTS_DIR/console_output.txt

touch $EXECUTE_DIR/TestingReports.html
cd $REPORTS_DIR/
for i in *_reports; do
  echo $i
  echo '<li><a href="'$REPORTS_WEB/$i'" > '$i' </a>' >> $REPORTS_DIR/TestingReports.html
done

echo $RESULT
exit $RESULT

