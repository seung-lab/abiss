import os
import sys
import redis
from cloudfiles import CloudFiles


TASK_KEY = sys.argv[1]

try:
    r = redis.Redis(host=os.environ["REDIS_SERVER"], db=int(os.environ["REDIS_DB"]))
    if r.exists(TASK_KEY) and r.get(TASK_KEY).decode() == "DONE":
        sys.exit(0)
    else:
        sys.exit(1)
except:
    cf = CloudFiles(os.environ["SCRATCH_PATH"])
    if cf.exists(f"done/{TASK_KEY}.txt"):
        sys.exit(0)
    else:
        sys.exit(1)
