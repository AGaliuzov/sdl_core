#!/usr/bin/env bash
sed -i "s/CC_cfg_3rd_party = \${QNX_HOST}\/usr\/bin\/qcc/CC_cfg_3rd_party = \${QNX_HOST}\/usr\/bin\/ntoarmv7-gcc/" cfg/qnx_armv7.mk
sed -i "s/CPP_cfg_3rd_party = \${QNX_HOST}\/usr\/bin\/qcc/CPP_cfg_3rd_party = \${QNX_HOST}\/usr\/bin\/ntoarmv7-g++/" cfg/qnx_armv7.mk
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