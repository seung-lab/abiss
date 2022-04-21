import os
import sys
import redis
from cloudfiles import CloudFiles


TASK_KEY = sys.argv[1]

if "REDIS_SERVER" in os.environ:
    r = redis.Redis(os.environ["REDIS_SERVER"])
    if r.exists(TASK_KEY) and r.get(TASK_KEY) == "DONE":
        sys.exit(0)
    else:
        sys.exit(1)
else:
    cf = CloudFiles(os.environ["SCRATCH_PATH"])
    if cf.exists(f"done/{TASK_KEY}.txt"):
        sys.exit(0)
    else:
        sys.exit(1)
