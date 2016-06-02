Source: https://docs.google.com/document/d/1JswQId0wvkitbr9uDI9mIcyk4QnLvtoQpAYoVjJsZUE/pub

https://github.com/antirez/redis/pull/487

1. Symptom

KEYS * command will return duplicated keys or omitting some keys.
1.1 Severity

Severe. Listed as the severe failures on:
http://redis.io/topics/problems
1.2 Was there exception thrown?

No error msg.
1.2.1 Were there multiple exceptions?

No.
1.3 Scope

Only for clients who set -> expire -> keys. t affect a whole lot of users as keys is not supposed to be used in production.
2. How to reproduce this failure

2.0 Version

2.6.14 then manually reversed the patch.
2.1 Configuration

Standard.
2.2 Reproduction procedure

Execute the following commands in a script:
flushall
set a c
set t c
set e c
set s c
set foo b
expire s 5
keys *
1. copy the above commands into bug.txt
2. cat bug.txt | redis-cli
2.2.1 Timing order

Call expire before keys.
2.2.2 Events order externally controllable?

Yes.
2.3 Can the logs tell how to reproduce the failure?

Error log has nothing. But if we have the client log we sure can reproduce it..
2.4 How many machines needed?

1.
2.5 How hard is the reproduction?

Easy.
3. Diagnosis procedure

This is a hard-to-diagnose failure. Especially that at the beginning it seemed like a concurrency bug (but turns out not). See details below.
3.1 Detailed Symptom (where you start)

ding@baker221:~/research/redis/redis-2.6.14/src$ cat ./bug487.txt | ./redis-cli
OK
OK
OK
OK
OK
OK
(integer) 1
1) "foo"
2) "s"
 --- It should show 4 keys, but instead only showed 2...
3.2 Backward inference

s quite straight-forward to locate the code handling keys command:
void keysCommand(redisClient *c) {
+   di = dictGetSafeIterator(c->db->dict);
-   di = dictGetIterator(c->db->dict);
    allkeys = (pattern[0] == '*' && pattern[1] == '\0');
    while((de = dictNext(di)) != NULL) {
        sds key = dictGetKey(de);
         ---- We are getting incorrect number of keys here!
        if (allkeys || stringmatchlen(pattern,plen,key,sdslen(key),0)) {
           keyobj = createStringObject(key,sdslen(key));
           if (expireIfNeeded(c->db,keyobj) == 0) {
                addReplyBulk(c,keyobj);
                numkeys++;
            }
            decrRefCount(keyobj);
        }
    }
    .. .. ..
}
In the while loop above we are iterating through all the keys. The question is why we get wrong number of keys.
Note that we have  version of iterator. Look at the code below:
/* If safe is set to 1 this is a safe iterator, that means, you can call
 * dictAdd, dictFind, and other functions against the dictionary even while
 * iterating. Otherwise it is a non safe iterator, and only dictNext()
 * should be called while iterating. */
typedef struct dictIterator {
    dict *d;
    int table, index, safe;
    dictEntry *entry, *nextEntry;
} dictIterator;
We know that this iterator is not safe if we are dictAdd, dictFind,  while we are iterating.
Now the question is, is it possible for us , etc in the middle?
If we have gdb, the answer is yes! Look at the callgraph at the beginning. Since we  before, so we will  in keys command, which will , which will start a rehashing process. This will mess up the iterator!
NOTE: this failure was in particular hard for me to diagnose because at the first glance, it appeared to be a concurrency bug: iterating  is called. I suspected that the handling of commands  were in parallel . But in the end, through some experiments, I was convinced that indeed redis is single threaded --- keys command starts only   finished.
4. Root  is called during the iterating process and messed up the iterator.
4.1 Category:

Semantic.

5. FixdictFindcause

expireand setafter keyswith expiresuch , setas dictFindwhile dictFindcall getExpirecall expirecalled dictFindto , dictAddcall etc.calling unsafethe ItWon