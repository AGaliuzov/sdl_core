#! /usr/bin/env bash

#!/bin/sh


function snapshot_tag() {
    date  +"SNAPSHOT_PASA%d%m%Y"
}

SNAPSHOT_TAG=`snapshot_tag`
SOURCE_DIR=""
BUILD_DIR=""
PASA_DIR=""
RELEASE_BINARIES=""
quick_mode=0

function show_help() {
	echo "h: show this help "
	echo "s: source dir (MANDATORY)"
	echo "b: build dir (MANDATORY)"
	echo "p: pasa build dir (MANDATORY)"
	echo "q: quick mode (upload only binaries on ftp) (default quick mode is disabled)"
	echo "t: snapshot tag (default is SNAPSHOT_PASA%d%m%Y)"
	exit
}

function prepare_compiler() {
  cd ${PASA_DIR}/SmartDeviceLink/
  sed -i "s/CC = \${QNX_HOST}\/usr\/bin\/qcc/CC = \${QNX_HOST}\/usr\/bin\/ntoarmv7-gcc/" cfg/qnx_armv7.mk
  sed -i "s/CPP = \${QNX_HOST}\/usr\/bin\/qcc/CPP = \${QNX_HOST}\/usr\/bin\/ntoarmv7-g++/" cfg/qnx_armv7.mk
  sed -i "s/LD = qcc/LD = \${QNX_HOST}\/usr\/bin\/ntoarmv7-g++/" cfg/qnx_armv7.mk
  sed -i "s/CC_FLAGS = \$(TARGET_FLG)/CC_FLAGS = \$(TARGET_FLG) -fPIC/" cfg/qnx_armv7.mk
  sed -i "s/LD_CPP = -lang-c++/LD_CPP = /" cfg/qnx_armv7.mk
  sed -i "s/-w9 //" cfg/qnx_armv7.mk
  sed -i "s/-Wc,//" cfg/qnx_armv7.mk
  sed -i "s/-Vgcc_ntoarmv7le_gpp//" cfg/qnx_armv7.mk
  find . -name makefile -exec sed -i "s/-Wc,//" {} \;
  find . -name makefile -exec sed -i "s/-Vgcc_ntoarmv7le_gpp//" {} \;
}


function prepare_release_binaries() {
  rm $RELEASE_BINARIES/* -r
  mkdir -p $RELEASE_BINARIES/fs/mp/apps/usr/lib/
  mkdir -p $RELEASE_BINARIES/fs/mp/etc/AppLink/
  cp $PASA/SmartDeviceLink/bin/armle-v7/release/SmartDeviceLink $RELEASE_BINARIES/fs/mp/apps/SmartDeviceLink
  cp $PASA/SmartDeviceLink/dll/armle-v7/release/*  $RELEASE_BINARIES/fs/mp/apps/usr/lib/
  cp $PASA/SmartDeviceLink/src/appMain/*.sh $RELEASE_BINARIES/fs/mp/etc/AppLink/
  cp $PASA/SmartDeviceLink/src/appMain/*.json $RELEASE_BINARIES/fs/mp/etc/AppLink/
  cp $PASA/SmartDeviceLink/src/appMain/log4cxx* $RELEASE_BINARIES/fs/mp/etc/AppLink/
  cp $PASA/SmartDeviceLink/src/appMain/*.ini $RELEASE_BINARIES/fs/mp/etc/AppLink/
  cp $PASA/SmartDeviceLink/src/appMain/*.sql $RELEASE_BINARIES/fs/mp/etc/AppLink/
  cp $PASA/SmartDeviceLink/src/appMain/*.cfg $RELEASE_BINARIES/fs/mp/etc/AppLink/
}

function build() {
  rm -r /tmp/PASA
  rm -rf $BUILD_DIR/*
  rm -rf ${PASA_DIR}/SmartDeviceLink/* 
  cd $BUILD_DIR
  cmake -DENABLE_SECURITY=OFF $SOURCE_DIR
  make pasa-tarball
  tar -xzf pasa.tar.gz -C ${PASA_DIR}/SmartDeviceLink/
  cd ${PASA_DIR}/SmartDeviceLink/
  prepare_compiler
  make -j4
  if [ $? -ne 0 ]; then
    echo "Failed to create snapshot";
    exit;
  fi;
}

function load_on_ftp() {
  lftp -u sdl_user,sdl_user ford-applink.luxoft.com -e "mirror -R $RELEASE_BINARIES snapshot/$SNAPSHOT_TAG/; exit"
  echo "Binaries are aviable: ftp://ford-applink.luxoft.com/snapshot/$SNAPSHOT_TAG/binaries"
  if [ $quick_mode -eq 0 ]; then 
    lftp -u sdl_user,sdl_user ford-applink.luxoft.com -e "put $BUILD_DIR/pasa.tar.gz -o snapshot/$SNAPSHOT_TAG/; exit"
    echo "Sources are aviable: ftp://ford-applink.luxoft.com/snapshot/$SNAPSHOT_TAG/pasa.tar.gz; exit "
  fi
}


while getopts "h?q?t:s:b:p:" opt; do
  case "$opt" in
  h|\?)
      show_help
      exit 0
      ;;
  q)  quick_mode=1
      ;;
  t)  SNAPSHOT_TAG=$OPTARG
      ;;
  s)  SOURCE_DIR=$OPTARG
      ;;
  b)  BUILD_DIR=$OPTARG
      ;;
  p)  PASA_DIR=$OPTARG
      ;;
  esac
done
shift $((OPTIND-1))


RELEASE_BINARIES=$PASA_DIR/SmartDeviceLink/binaries
echo "SNAPSHOT_TAG : "$SNAPSHOT_TAG 
echo "SOURCE_DIR : "$SOURCE_DIR 
echo "BUILD_DIR : "$BUILD_DIR 
echo "PASA_DIR : "$PASA_DIR 
echo "RELEASE_BINARIES : "$RELEASE_BINARIES 
echo "quick_mode : "$quick_mode  
if [[ $SOURCE_DIR == "" ]]; then echo "SOURCE_DIR (-s) is mandatory"; exit; fi;
if [[ $BUILD_DIR == "" ]]; then echo "BUILD_DIR (-b) is mandatory"; exit; fi;
if [[ $PASA_DIR == "" ]]; then echo "PASA_DIR (-p) is mandatory"; exit; fi;

build
prepare_release_binaries
load_on_ftp
