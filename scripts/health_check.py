import psutil
import sys

cpu_percent = sum(psutil.cpu_percent(5, percpu=True))

if cpu_percent > 5:
    sys.exit(0)
else:
    sys.exit(1)
