import time
time.sleep(3.5)
gdb.execute("target remote :1234")
gdb.execute("set waiting = false")
