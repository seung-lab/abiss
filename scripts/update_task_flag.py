import os
import sys
import redis
from cloudfiles import CloudFiles


TASK_KEY = sys.argv[1]
STATE = sys.argv[2]

if "REDIS_SERVER" in os.environ:
    r = redis.Redis(os.environ["REDIS_SERVER"])
    r.set(TASK_KEY, STATE)
else:
    cf = CloudFiles(os.environ["SCRATCH_PATH"])
    if STATE == "DONE":
        cf.put(f'done/{TASK_KEY}.txt', b"")
