#! /usr/bin/env bash
SNAPSHOT_TAG=$1
SOURCE_DIR=$2
BUILD_DIR=$3
PASA_DIR=$4
RELEASE_BINARIES=$PASA_DIR/SmartDeviceLink/binaries

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

function ommit_license() {
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



rm -r /tmp/PASA
rm -rf $BUILD_DIR/*
rm -rf ${PASA_DIR}/SmartDeviceLink/* 

cd $BUILD_DIR
cmake -DENABLE_SECURITY=OFF $SOURCE_DIR
make pasa-tarball
tar -xzf pasa.tar.gz -C ${PASA_DIR}/SmartDeviceLink/
cd ${PASA_DIR}/SmartDeviceLink/
ommit_license
make -j4

if [ $? -ne 0 ]; then
  echo "Failed to create snapshot";
  exit;
fi;

prepare_release_binaries
lftp -u sdl_user,sdl_user ford-applink.luxoft.com -e "mirror -R $RELEASE_BINARIES snapshot/$SNAPSHOT_TAG/; exit"
lftp -u sdl_user,sdl_user ford-applink.luxoft.com -e "mirror -R /tmp/PASA  snapshot/$SNAPSHOT_TAG/; exit"


