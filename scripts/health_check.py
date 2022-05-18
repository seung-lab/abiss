import psutil
import sys

cpu_percent = psutil.cpu_percent(5)

if cpu_percent > 5:
    sys.exit(0)
else:
    sys.exit(1)
