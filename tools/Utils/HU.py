#! /usr/bin/env python3
from argparse import ArgumentParser
from ftplib import FTP
import telnetlib
import time
from os import listdir, walk, makedirs
from os.path import isdir, isfile, join
import errno
import shutil
import sys
import os

DEFAULT_USER = b"root"
DEFAULT_PASSWD = b"#Pasa3Ford"
rtc_bin_folder = "bin/armle-v7/release/"
rtc_dll_folder = "dll/armle-v7/release/"
rtc_etc_folder = "src/appMain/"


def find_recursively(file_name, directory):
    list_dir = listdir(directory)
    for entry in list_dir:
        full_path = join(directory, entry)
        if isdir(full_path):
            res = find_recursively(file_name, full_path)
            if res:
                return res
        else:
            if entry == file_name:
                return full_path
    return None


def validate_ip(ip):
    if not ip:
        return False
    blocks = ip.split(".")
    if len(blocks) != 4:
        return False
    for ip_block in blocks:
        try:
            ip_block = int(ip_block)
            if ip_block not in range(0, 255):
                return False
        except(ValueError):
            return False
    return True


def telnet_login(ip, user, passwd):
    telnetConnection = None
    while telnetConnection == None:
        try:
            print("Connection to : ", ip)
            telnetConnection = telnetlib.Telnet(ip)
            print("Connection is established successfully")
        except:
            print("Connection failure occurs. Try Again")
            time.sleep(1)
    telnetConnection.read_until(b"login: ")
    telnetConnection.write(user + b"\n")
    telnetConnection.read_until(b"Password:")
    telnetConnection.write(passwd + b"\n")
    telnetConnection.read_until(b'#')
    print("Login Success")
    return telnetConnection


def create_path(filename, base=""):
    """
    create path for filename if it not exits
    """
    index = filename.rfind("/")
    path = filename[:index]
    try:
        makedirs(base + path)
    except OSError as exception:
        if exception.errno != errno.EEXIST:
            raise exception


HU_files_path = {
    # binaries
    "SmartDeviceLink": "/fs/mp/apps/SmartDeviceLink",
    "libPolicy.so": "/fs/mp/apps/usr/lib/libPolicy.so",
    "liblog4cxx.so": "/fs/mp/apps/usr/lib/liblog4cxx.so",
    "liblog4cxx.so.10": "/fs/mp/apps/usr/lib/liblog4cxx.so.10",
    "libapr-1.so": "/fs/mp/apps/usr/lib/libapr-1.so",
    "libapr-1.so.5": "/fs/mp/apps/usr/lib/libapr-1.so.5",
    "libaprutil-1.so": "/fs/mp/apps/usr/lib/libaprutil-1.so",
    "libaprutil-1.so.5": "/fs/mp/apps/usr/lib/libaprutil-1.so.5",
    "libexpat.so": "/fs/mp/apps/usr/lib/libexpat.so",
    "libexpat.so.7": "/fs/mp/apps/usr/lib/libexpat.so.7",
    "libappenders.so": "/fs/mp/apps/usr/lib/libappenders.so",
    # config
    "log4cxx.properties": "/fs/mp/etc/AppLink/log4cxx.properties",
    "hmi_capabilities.json": "/fs/mp/etc/AppLink/hmi_capabilities.json",
    "init_policy.sh": "/fs/mp/etc/AppLink/init_policy.sh",
    "init_policy_pasa.sh": "/fs/mp/etc/AppLink/init_policy_pasa.sh",
    "policy.cfg": "/fs/mp/sql/policy.cfg",
    "policy.ini": "/fs/mp/etc/AppLink/policy.ini",
    "policy_usb.cfg": "/fs/mp/etc/AppLink/policy_usb.cfg",
    "sdl_preloaded_pt.json": "/fs/mp/etc/AppLink/sdl_preloaded_pt.json",
    "smartDeviceLink.ini": "/fs/mp/etc/AppLink/smartDeviceLink.ini",
    "policy.sql": "/fs/mp/sql/policy.sql"
}

file_list = []
# path on HU file system
for x in HU_files_path:
    file_list.append(HU_files_path[x])

usage = "--to_target or --collect_rtc option must be used. \n use -h fo help"
parser = ArgumentParser(description='v1.0')
parser.add_argument("--ip", dest="ip", metavar="IP",
                  help="IP Address of target")
parser.add_argument("--rtc", dest="rtc_path", metavar="RTC PATH",
                  help="Path to SmasrDeviceLink RTC workspace")
parser.add_argument("--bin", dest="bin_path", metavar="BIN PATH",
                  help="Binaries folder path")
parser.add_argument("--to_target", dest="to_target", action="store_true", default=False,
                  help="Load binaries to target, --ip and --bin options required")
parser.add_argument("--collect_rtc", dest="collect_rtc", action="store_true", default=False,
                  help="Cllect binaries from RTC workspace to copy of target file system. --rtc and --bin options required")
parser.add_argument("--login", dest="login", action="store_true", default=False,
                  help="Automaticaly login on HU via telnet")


class Target:
    """
    Connection to target via telnet and ftp
    """

    def __init__(self, ip, bin_path=None, rtc_path=None, zipped=False, user=DEFAULT_USER, passwd=DEFAULT_PASSWD):
        self.ip = ip
        self.bin_path = bin_path
        self.rtc_path = rtc_path
        self.zipped = zipped
        self.user = user
        self.passwd = passwd
        self.telnet_ = None
        self.ftp_ = None

    def telnet(self):
        if not self.telnet_:
            self.telnet_ = telnet_login(self.ip, self.user, self.passwd)
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
        res = self.telnet().read_until(b'#')
        print("FS synced")

    def kill_sdl(self):
        command = b"ps -A | grep -e Sma -e SD| awk '{print $1}' | xargs kill -9 \n"
        self.telnet().write(command)
        self.telnet().read_until(b'#')
        time.sleep(0.5)
        print("kill sdl success")

    def get_remote_hash_and_size(self, hu_path):
        command = "cksum %s \n" % hu_path
        command = command.encode("ascii")
        self.telnet().write(command)
        tmp = self.telnet().read_until(b'#')
        tmp = tmp.decode("ascii")
        hash = tmp.split()[2]
        size = tmp.split()[3]
        return hash, int(size)

    def get_local_hash_and_size(self, local_path):
        command = "cksum %s" % local_path
        tmp = os.popen(command).read()
        hash = tmp.split(" ")[0]
        size= tmp.split(" ")[1]
        return hash, int(size)

    def get_local_size(self, local_path):
        command = "wc -c %s" % local_path
        tmp = os.popen(command).read()
        size = tmp.split(" ")[0]
        return int(size)

    def get_remote_size(self, hu_path):
        command = "wc -c %s \n" % hu_path
        command = command.encode("ascii")
        self.telnet().write(command)
        tmp = self.telnet().read_until(b'#')
        tmp = tmp.decode("ascii")
        size = tmp.split()[3]
        return int(size)

    def load_file_on_hu(self, src_file):
        try:
            src_path = find_recursively(src_file, self.bin_path)
            local_hash, local_size = self.get_local_hash_and_size(src_path)
            f = open(src_path, "rb")
            hu_path = HU_files_path[src_file]
            print("Copy \n", src_path, "\n", self.ip + ":" + hu_path)
            sys.stdout.flush()
            self.ftp().storbinary("STOR " + hu_path, f)
            hu_hash, hu_size = self.get_remote_hash_and_size(hu_path)
            if hu_size == local_size and local_hash == hu_hash:
                end_str = "  size : %s == %s &&  hash : %s == %s : OK"  % (local_size, hu_size, local_hash, hu_hash)
                self.sync()
            else:
                end_str = " \t ERROR : " \
                          "\n \t Files does not match:" \
                          "\n\t\t local size= %s, remote size = %s" \
                          "\n\t\t local hash = %s remote hash = %s" % (local_size, hu_size, local_hash, hu_hash)
            print(end_str)
            return src_path, hu_path

        except(FileNotFoundError):
            print(src_file + " Not found;")
        except Exception as e:
            print(src_file + " Process error : ", e)

    def load_binaries_on_target(self):
        print("Load binaries to target")
        if not validate_ip(self.ip):
            print("invalid ip address : ", self.ip)
            return False
        if not self.bin_path:
            print("Binaries path required")
            return False
        print(" IP: ", self.ip)
        print(" Binaries path: ", self.bin_path)
        self.mount()
        self.kill_sdl()
        for filename in HU_files_path:
            self.load_file_on_hu(filename)
        self.sync()

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
        for file_name in HU_files_path:
            src_path = find_recursively(file_name, self.rtc_path)
            dest_path = self.bin_path + HU_files_path[file_name]
            print("copy : ", src_path, " --> ", dest_path)
            create_path(dest_path)
            shutil.copyfile(src_path, dest_path)

    def interact(self):
        print("Interaction mode")
        self.telnet().interact()


def main():
    print(parser.description)
    args = parser.parse_args()
    if not (args.to_target ^ args.collect_rtc):
        print(usage)
        return -1
    target = Target(args.ip, args.bin_path, args.rtc_path)
    if (args.to_target):
        target.load_binaries_on_target()
    if (args.collect_rtc):
        target.collect_rtc_binaries()
    if (args.login):
        target.interact()
    return 0


if __name__ == "__main__":
    main()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
