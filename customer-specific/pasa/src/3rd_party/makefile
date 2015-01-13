#
# Standard Module Makefile version 2.0
#
 
# Name of the componet (team/domain prefix '_' component name)
COMPONENT_NAME = 3rd_party
 
ifndef PRJ_ROOT
export PRJ_ROOT = $(CURDIR)/../
endif
include $(PRJ_ROOT)cfg/depends.mk
 
# Standard set of derived targets
library: expat apr apr-util log4cxx
	@echo "***** `date` Done building library: $(COMPONENT_NAME) ******"

expat:
	cd expat-2.1.0 && CC="${CC_cfg_3rd_party}" CXX="${CPP_cfg_3rd_party}" ./configure --host=arm-nto-qnx && cd ..
	$(MAKE) -C expat-2.1.0 OPTIMIZATION=NONE TARGET_FLG=
	cp expat-2.1.0/.libs/libexpat.so.7 $(SLIB_PATH)libexpat$(DEBUG_EXT).so.7
	ln -sf $(SLIB_PATH)libexpat$(DEBUG_EXT).so.7 $(SLIB_PATH)libexpat$(DEBUG_EXT).so

apr: expat
	cd apr-1.5.0 && CC="${CC_cfg_3rd_party}" CXX="${CPP_cfg_3rd_party}" ./configure \
	--host=arm-nto-qnx ac_cv_file__dev_zero=yes ac_cv_func_setpgrp_void=yes \
	apr_cv_process_shared_works=yes apr_cv_mutex_recursive=yes \
	apr_cv_mutex_robust_shared=no apr_cv_tcp_nodelay_with_cork=no \
	ac_cv_sizeof_struct_iovec=8 && cd ..
	$(MAKE) -C apr-1.5.0 OPTIMIZATION=NONE TARGET_FLG=
	mkdir -p $(CURDIR)/log4cxx/include
	cp -rf apr-1.5.0/include/* $(CURDIR)/log4cxx/include
	cp apr-1.5.0/.libs/libapr-1.so.5 $(SLIB_PATH)libapr-1$(DEBUG_EXT).so.5
	ln -sf $(SLIB_PATH)libapr-1$(DEBUG_EXT).so.5 $(SLIB_PATH)libapr-1$(DEBUG_EXT).so

apr-util: expat apr
	cd apr-util-1.5.3 && CC="${CC_cfg_3rd_party}" CXX="${CPP_cfg_3rd_party}" ./configure \
	--host=arm-nto-qnx --with-apr=../apr-1.5.0 --with-expat-source=../expat-2.1.0 \
	--with-expat-build=../expat-2.1.0 && cd ..
	$(MAKE) -C apr-util-1.5.3 OPTIMIZATION=NONE TARGET_FLG=
	cp apr-util-1.5.3/.libs/libaprutil-1.so.5 $(SLIB_PATH)libaprutil-1$(DEBUG_EXT).so.5
	ln -sf $(SLIB_PATH)libaprutil-1$(DEBUG_EXT).so.5 $(SLIB_PATH)libaprutil-1$(DEBUG_EXT).so

log4cxx: expat apr apr-util
	cd apache-log4cxx-0.10.0 && CC="${CC_cfg_3rd_party}" CXX="${CPP_cfg_3rd_party}" ./configure \
	--host=arm-nto-qnx --with-apr=../apr-1.5.0 --with-apr-util=../apr-util-1.5.3 LDFLAGS=-L../expat-2.1.0/.libs/ && cd ..
	$(MAKE) -C apache-log4cxx-0.10.0 OPTIMIZATION=NONE TARGET_FLG=
	cp -r apache-log4cxx-0.10.0/src/main/include/log4cxx $(CURDIR)/log4cxx/include
	cp apache-log4cxx-0.10.0/src/main/cpp/.libs/liblog4cxx.so.10.0.0 $(SLIB_PATH)liblog4cxx$(DEBUG_EXT).so.10.0.0
	ln -sf $(SLIB_PATH)liblog4cxx$(DEBUG_EXT).so.10.0.0 $(SLIB_PATH)liblog4cxx$(DEBUG_EXT).so

clean-3rd_party:
	$(MAKE) clean -C expat-2.1.0 OPTIMIZATION=NONE TARGET_FLG=
	rm $(SLIB_PATH)libexpat$(DEBUG_EXT).*
	$(MAKE) clean -C apr-1.5.0 OPTIMIZATION=NONE TARGET_FLG=
	rm $(SLIB_PATH)libapr-1$(DEBUG_EXT).*
	$(MAKE) clean -C apr-util-1.5.3 OPTIMIZATION=NONE TARGET_FLG=
	rm $(SLIB_PATH)libaprutil-1$(DEBUG_EXT).*
	$(MAKE) clean -C apache-log4cxx-0.10.0 OPTIMIZATION=NONE TARGET_FLG=
	rm $(SLIB_PATH)liblog4cxx$(DEBUG_EXT).*
	rm -rf $(CURDIR)/log4cxx/include