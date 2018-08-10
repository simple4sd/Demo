#!/usr/bin/python

from socket import *
from subprocess import *
import sys
import time


if len(sys.argv) != 3:
	print "wrong parameter number!"
	sys.exit(-1)	
ip = sys.argv[1]
port = int(sys.argv[2])

s = socket(AF_INET, SOCK_STREAM)
s.connect((ip, port))

time.sleep(2)

p = Popen("./a.out", shell=True)



