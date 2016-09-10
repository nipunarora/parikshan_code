
url: https://docs.google.com/document/d/1lG_xPiaWwpEHcz558Kort7KGV8-3QYctNTNwluaqtLw/pub

github-417 Report
https://github.com/antirez/redis/pull/417


1. Symptom

Memory leak in the master server.
1.1 Severity

Severe. Mentioned in redis.io/topics/problems
1.2 Was there exception thrown?

Yes. There will be when memory is exhausted.
1.2.1 Were there multiple exceptions?

Yes.
1.3 Scope of the failure

Affect all clients.
2. How to reproduce this failure

2.0 Version

2.4.9
2.1 Configuration

Standard master-slave configuration.
2.2 Reproduction procedure

1. Start the cluster (master + slave)
2. Observe the master’s mem usage (this is the normal usage)
3. executes the following commands concurrently (i.e. from its own terminal).
term1 $ redis-cli -r 1000000 set foo bar
term2 $ redis-cli -n 15 -r 1000000 set foo bar
4. After the two sets: check master redis instance’s memory usage again
Observe the tremendous increase of mem usage.
In fact, even if you just execute
“term2 $ redis-cli -n 15 -r 1000000 set foo bar”
You can still see the leak, but just not as dramatic. The trick is to specify a database ID > 10.
2.2.1 Timing order

Order doesn’t matter in fact.
2.2.2 Events order externally controllable?

Yes.
2.3 Can the logs tell how to reproduce the failure?

Yes.
DB = 15:
[4517] 06 Aug 17:34:04 - DB 15: 1 keys (0 volatile) in 4 slots HT.
2.4 How many machines needed?

1 (Master + slave + client)
3. Diagnosis procedure

3.1 Detailed Symptom (where you start)

Leak. Can be seen from the log msg:
[4517] 06 Aug 17:37:04 - 0 clients connected (1 slaves), 47479944 bytes in use
3.2 Backward inference

But the hard thing is to know the source of the leak. This is very hard. The key to trigger the bug is that DB id = 15, which is logged:
[4517] 06 Aug 17:34:04 - DB 15: 1 keys (0 volatile) in 4 slots HT.
But I doubt the user can truly pick this out of a large number of other msgs.
replicationFeedSlaves() {
 while((ln = listNext(&li))) {
  redisClient *slave = ln->value;
  /* Don't feed slaves that are still waiting for BGSAVE to start */
  if (slave->replstate == REDIS_REPL_WAIT_BGSAVE_START) continue;
  switch(dictid) {
      ---> ‘dictid’ is database ID
  case 0: selectcmd = shared.select0; break;
  case 1: selectcmd = shared.select1; break;
  case 2: selectcmd = shared.select2; break;
  case 3: selectcmd = shared.select3; break;
  case 4: selectcmd = shared.select4; break;
  case 5: selectcmd = shared.select5; break;
  case 6: selectcmd = shared.select6; break;
  case 7: selectcmd = shared.select7; break;
  case 8: selectcmd = shared.select8; break;
  case 9: selectcmd = shared.select9; break;
  default:
    selectcmd = createObject(REDIS_STRING,
     sdscatprintf(sdsempty(),"select %d\r\n",dictid));
    ---> when database ID is greater than 9, it will create a new object to send to slave. But this object is never freed...
    selectcmd->refcount = 0;
    break;
  }
 }
}
This function is to prepare the request to be sent to the slave. The intention was that for each DB number, it needs a string object. Since ID 0~9 are commonly used, it creates them in advance. For DB IDs that are greater than 9, it will create on-demand. But it never freed this object after the creation.
Note: the reason that we need a concurrent request:
term1 $ redis-cli -r 1000000 set foo bar
to run in parallel with the id=15 requests is that if we only run the “-n 15” set requests, the leak won’t be that dramatic, because the master will batch a lot of requests together before creating the object: slave->replstate will be REDIS_REPL_WAIT_BGSAVE_START.
4. Root cause

Redis forgot to free the created selectcmd object.
4.1 Category:

Semantic (not releasing resource).
5. Fix

5.1 How?

Immediately free the newly created object after “addReply”.
https://github.com/antirez/redis/commit/3c38b0876e62dfc8944261ee51632c2784caddf0

