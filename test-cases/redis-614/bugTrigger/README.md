redis-lua-repl-bug
==================

Redis Lua script, multi-element LPUSH / BRPOPLPUSH replication bug

Affected Redis Version: 2.5.12 (Redis 2.6.0 Release Candidate 6)

## Update - Fixed Issue #614

@antirez reimplemented blocking operations in the 2.6 branch to fix this issue!
See the commit message below for a detailed explanation.

* https://github.com/antirez/redis/issues/614
* https://github.com/antirez/redis/commit/f444e2afdc2394b647c7310b5b249616fcc5b43e

## How to reproduce

1. Setup master/slave replication.
2. ruby consumer.rb
3. ruby producer.rb
4. Note how slave Redis receives wrong # of pop operations, and 'queue'
   continues to grow, out of sync with master Redis.

## Producer

1. connects to Redis instance (in this case on localhost, db 8)
2. evals a Lua script that pushes multiple elements onto a list 'queue'
3. sleep 1 second
4. repeat

## Consumer

1. connects to Redis instance
2. brpoplpush from 'queue' to processing queue.
3. remove from processing queue
4. repeat