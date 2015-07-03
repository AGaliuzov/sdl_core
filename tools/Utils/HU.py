#!/usr/bin/env python3

from optparse import OptionParser
from ftplib import FTP
import telnetlib
import time
import os
import errno
import shutil


rtc_to_fs = {
#binaries
	"SmartDeviceLink/bin/armle-v7/release/SmartDeviceLink" : "/fs/mp/apps/SmartDeviceLink",
	"SmartDeviceLink/dll/armle-v7/release/libPolicy.so" : "/fs/mp/apps/usr/lib/libPolicy.so",
	"SmartDeviceLink/dll/armle-v7/release/liblog4cxx.so" : "/fs/mp/apps/usr/lib/liblog4cxx.so",
	"SmartDeviceLink/dll/armle-v7/release/liblog4cxx.so.10" : "/fs/mp/apps/usr/lib/liblog4cxx.so.10",
	"SmartDeviceLink/dll/armle-v7/release/libapr-1.so" : "/fs/mp/apps/usr/lib/libapr-1.so",
	"SmartDeviceLink/dll/armle-v7/release/libapr-1.so.5" : "/fs/mp/apps/usr/lib/libapr-1.so.5",
	"SmartDeviceLink/dll/armle-v7/release/libaprutil-1.so" : "/fs/mp/apps/usr/lib/libaprutil-1.so",
	"SmartDeviceLink/dll/armle-v7/release/libaprutil-1.so.5" : "/fs/mp/apps/usr/lib/libaprutil-1.so.5",
	"SmartDeviceLink/dll/armle-v7/release/libexpat.so" : "/fs/mp/apps/usr/lib/libexpat.so",
	"SmartDeviceLink/dll/armle-v7/release/libexpat.so.7" : "/fs/mp/apps/usr/lib/libexpat.so.7",
	"SmartDeviceLink/dll/armle-v7/release/libappenders.so" : "/fs/mp/apps/usr/lib/libappenders.so",
#config
	"SmartDeviceLink/src/appMain/log4cxx.properties" : "/fs/mp/etc/AppLink/log4cxx.properties",
	"SmartDeviceLink/src/appMain/hmi_capabilities.json" : "/fs/mp/etc/AppLink/hmi_capabilities.json",
	"SmartDeviceLink/src/appMain/init_policy.sh" : "/fs/mp/etc/AppLink/init_policy.sh",
	"SmartDeviceLink/src/appMain/init_policy_pasa.sh" : "/fs/mp/etc/AppLink/init_policy_pasa.sh",
	"SmartDeviceLink/src/appMain/policy.cfg" : "/fs/mp/etc/AppLink/policy.cfg",
	"SmartDeviceLink/src/appMain/policy.ini" : "/fs/mp/etc/AppLink/policy.ini",
	"SmartDeviceLink/src/appMain/policy_usb.cfg" : "/fs/mp/etc/AppLink/policy_usb.cfg",
	"SmartDeviceLink/src/appMain/sdl_preloaded_pt.json" : "/fs/mp/etc/AppLink/sdl_preloaded_pt.json",
	"SmartDeviceLink/src/appMain/smartDeviceLink.ini" : "/fs/mp/etc/AppLink/smartDeviceLink.ini",
	"SmartDeviceLink/src/appMain/policy.sql" : "/fs/mp/sql/policy.sql"
}

def create_path(filename, base = ""):
	index = filename.rfind("/")
	path = filename[:index]
	try:
		os.makedirs(base + path)
	except OSError as exception:
		if exception.errno != errno.EEXIST:
			raise exception

file_list = []
#path on HU file system
for x in rtc_to_fs:
	file_list.append(rtc_to_fs[x])

usage = "--to_target or --collect_rtc or --rtc_to_target option must be used. use -h fo help"
DEFAULT_USER = b"root"
DEFAULT_PASSWD = b"#Pasa3Ford"
parser = OptionParser()
parser.add_option("--ip", dest = "ip", metavar = "IP",
		help = "IP Address of target") 
parser.add_option("--rtc",dest = "rtc_path", metavar = "RTC PATH",
		help = "Path to RTC workspace")
parser.add_option("--bin", dest="bin_path", metavar = "BIN PATH",
		help = "Binaries folder path")
parser.add_option("--to_target", dest="to_target", action = "store_true", default = False,
		help = "Load binaries to target, --ip and --bin options required")
parser.add_option("--collect_rtc", dest = "collect_rtc", action = "store_true", default = False,
		help = "Collect binaries from RTC workspace. --rtc and --bin options required")
parser.add_option("--rtc_to_target", dest = "rtc_to_target", action="store_true", default = False,
		help = "Collect binaries from rtc workspace and load them on target, --rtc and --ip options required")

class Target:
	def __init__(self, ip, bin_path = None, rtc_path = None, zipped = False, user = DEFAULT_USER, passwd = DEFAULT_PASSWD):
		self.ip = ip
		self.bin_path = bin_path
		self.rtc_path = rtc_path
		self.zipped = zipped
		self.user = user
		self.passwd = passwd
		self.telnet_ = None
		self.ftp_ = None

	def validate_ip(self):
		if not self.ip:
			return False
		splited = self.ip.split(".")
		if len(splited) != 4:
			return False
		for x in splited:
			try:
				x = int(x)
				if x > 255 or x < 0:
					return False
			except(ValueError):
				return False
		return True

	def login(self):
		telnetConnection = None
		while telnetConnection == None:                                                             
			try:                                                            
				print("Connection to : ", self.ip)
				telnetConnection = telnetlib.Telnet(self.ip)           
				print("Connection is established successfully")
			except:                                                         
				print("Connection failure occurs. Try Again")
				time.sleep(1)                                           
		telnetConnection.read_until(b"login: ")                                  
		telnetConnection.write(self.user + b"\n")                                 
		telnetConnection.read_until(b"Password:")                        
		telnetConnection.write(self.passwd + b"\n")                         
		print ("Login Success")
		return telnetConnection

	def telnet(self):
		if not self.telnet_:
			self.telnet_ = self.login()
		return self.telnet_

	def ftp(self):
		if not self.ftp_:
			self.ftp_ = FTP(self.ip)
			self.ftp_.login("root", "#Pasa3Ford")
		return self.ftp_
	
	def mount(self):
		command = b"mount -uw /fs/mp\n"
		self.telnet().write(command)
		self.telnet().read_until(b'#')
		print("mount success")
	
	def sync(self):
		command = b"sync; sync; sync\n"
		self.telnet().write(command)
		self.telnet().read_until(b'#')
		print("FS synced")

	def kill_sdl(self):
		command = b"ps -A | grep -e Sma -e SD| awk '{print $1}' | xargs kill -9 \n"                                 
		self.telnet().write(command)                                    
		#self.telnet().read_until(b'#')
		#res = self.telnet().read_eager()
		time.sleep(0.5)
		print("kill sdl success")

	def load_binaries_on_target(self):
		print("Load binaries to target")
		if not self.validate_ip():
			print ("invalid ip address : ", self.ip)
			return False
		if not self.bin_path:
			print("Binaries path required")
			return False
		print(" IP: ", self.ip)
		print(" Binaries path: ", self.bin_path)
		self.mount()
		self.kill_sdl()
		self.copy_files_on_hu(self.bin_path)
		self.sync()
	
	def copy_files_on_hu(self, source):
		print("FTP login success")
		for filename in file_list:
			src_filename = source + filename
			try:
				f = open(src_filename, "rb")
				self.ftp().storbinary("STOR " + filename, f)
				print ("Copy " , src_filename, " --> ", self.ip+":"+filename)
			except(FileNotFoundError):
				print(src_filename + " Not found; ")

	def collect_rtc_binaries(self):
		print("Collect RTC binaries")
		if not self.bin_path:
			print("Binaries path required")
			return False
		if not self.rtc_path:
			print("RTC path required")
			return False
		print(" RTC workspace path: ", self.rtc_path)
		print(" Binaries path: ", self.bin_path)
		for f in rtc_to_fs:
			rtc_f_name = self.rtc_path + "/" + f
			bin_f_name = self.bin_path + "/" +  f
			print("copy : ", rtc_f_name, " --> ", bin_f_name)
			create_path(bin_f_name)
			shutil.copyfile(rtc_f_name, bin_f_name)

	def load_binaries_to_target_from_rtc(self):
		print("Load binaries from RTC to target")
		if not self.validate_ip():
			print ("invalid ip address : ", self.ip)
		if not self.rtc_path:
			print("RTC path required")
			return False
		print(" RTC workspace path: ", self.rtc_path)
		print(" IP: ", self.ip)
		self.mount()
		self.kill_sdl()
		for f in rtc_to_fs:
			rtc_f_name = self.rtc_path + "/" + f
			target_path = rtc_to_fs[f]
			try:
				f = open(rtc_f_name, "rb")
				self.ftp().storbinary("STOR " + target_path, f)
				print ("Copy " , rtc_f_name, " --> ", self.ip+":"+target_path)
			except(FileNotFoundError):
				print(src_filename + " Not found; ")
		self.sync()


def main():
	(options, args) = parser.parse_args()
	if not(options.to_target ^ options.collect_rtc ^ options.rtc_to_target):
		print(usage)
		return -1
	target = Target(options.ip, options.bin_path, options.rtc_path)
	if (options.to_target):
		target.load_binaries_on_target()
	if (options.collect_rtc):
		target.collect_rtc_binaries()
	if(options.rtc_to_target):
		target.load_binaries_to_target_from_rtc()
	return 0
	
if __name__ == "__main__":
	return main()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
