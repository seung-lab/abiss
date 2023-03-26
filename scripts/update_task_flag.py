import os
import sys
import redis
from cloudfiles import CloudFiles


TASK_KEY = sys.argv[1]
STATE = sys.argv[2]

try:
    r = redis.Redis(host=os.environ["REDIS_SERVER"], db=int(os.environ["REDIS_DB"]))
    r.set(TASK_KEY, STATE)
except:
    cf = CloudFiles(os.environ["SCRATCH_PATH"])
    if STATE == "DONE":
        cf.put(f'done/{TASK_KEY}.txt', b"")
