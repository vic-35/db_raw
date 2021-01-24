#!/usr/bin/python3
#-*- coding: utf-8 -*-

import sys
import time


sys.path.append('lib/')


import _db_simple_p
 
print(sys.version)

print('db_open: ', _db_simple_p.db_open('test.bin'))


_db_simple_p.db_append(0,"test _string")
_db_simple_p.db_append(1,"test 44444444444")


_db_simple_p.db_init_view()
 
ret=_db_simple_p.db_get_next() 
while ret[0] == 1 :
	print(ret)
	ret=_db_simple_p.db_get_next() 

_db_simple_p.db_close()

print("end")

