import os
import sys
import redis
from cloudfiles import CloudFiles


TASK_KEY = sys.argv[1]

done = False
try:
    r = redis.Redis(host=os.environ["REDIS_SERVER"], db=int(os.environ["REDIS_DB"]))
    if r.exists(TASK_KEY) and r.get(TASK_KEY).decode() == "DONE":
        done = True
except:
    cf = CloudFiles(os.environ["SCRATCH_PATH"])
    if cf.exists(f"done/{TASK_KEY}.txt"):
        done = True

if not done:
    sys.exit(1)
