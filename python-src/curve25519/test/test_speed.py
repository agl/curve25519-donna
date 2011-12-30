#! /usr/bin/python

from time import time
from curve25519 import Private

count = 10000
elapsed_get_public = 0.0
elapsed_get_shared = 0.0

def abbreviate_time(data):
    # 1.23s, 790ms, 132us
    if data is None:
        return ""
    s = float(data)
    if s >= 10:
        #return abbreviate.abbreviate_time(data)
        return "%d" % s
    if s >= 1.0:
        return "%.2fs" % s
    if s >= 0.01:
        return "%dms" % (1000*s)
    if s >= 0.001:
        return "%.1fms" % (1000*s)
    if s >= 0.000001:
        return "%.1fus" % (1000000*s)
    return "%dns" % (1000000000*s)

def nohash(key): return key

for i in range(count):
    p = Private()
    start = time()
    pub = p.get_public()
    elapsed_get_public += time() - start
    pub2 = Private().get_public()
    start = time()
    shared = p.get_shared_key(pub2) #, hashfunc=nohash)
    elapsed_get_shared += time() - start

print("get_public: %s" % abbreviate_time(elapsed_get_public / count))
print("get_shared: %s" % abbreviate_time(elapsed_get_shared / count))

# these take about 560us-570us each (with the default compiler settings, -Os)
# on my laptop, same with -O2
#  of which the python overhead is about 5us
#  and the get_shared_key() hash step adds about 5us
