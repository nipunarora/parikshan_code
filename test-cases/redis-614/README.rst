Instructions to recreate the bug:
========

Clone redis:
	git clone https://github.com/antirez/redis.git

Checkout the correct version of redis:
	git checkout bfc197c3b604baf0dba739ea174d5054284133f0

Install redis:
	make

Create two copies of the installed folders:
       redis and redis2 

Instructions on how to run proxy for redis-487
=========

0. Setup redis master and slaves

   cp -r redis redis-master
   cp -r redis redis-slave
   cp -r redis redis2-master
   cp -r redis redis2-slave

1. Install Gems

   gem install bundler
   bundle install

1. Run first redis service:

   cd redis-master
   ./src/redis-server --loglevel verbose

   cd redis-slave
   ./src/redis-server --port 7777 --slaveof 127.0.0.1 6379 --loglevel verbose

   cd redis2-master
   ./src/redis-server --port 8888 --loglevel verbose

   cd redis2-slave
   ./src/redis-server --port 9999 --slaveof 127.0.0.1 8888 --loglevel verbose
   
2. Run second redis service:

   cd redis2
   ./src/redis-server --port 7777 --loglevel verbose

3. Start proxy clone between the main master, and redis2-master

   ./proxyClone -l 5555 -h 127.0.0.1 -p 6379 -x 127.0.0.1 -d 8888 -a

4. Start bugTrigger

   ****Make sure that the ruby scripts connect to port 5555****
   
   cd bugTrigger

   In terminal 1
   ruby consumer.rb
   
   In terminal 2
   ruby producer.rb

   See memory leak in the verbose logs of both redis-slave and redis-slave2

   [2892] 12 Sep 02:41:04.386 - DB 8: 1 keys (0 volatile) in 4 slots HT.
   [2892] 12 Sep 02:41:04.386 - 1 clients connected (0 slaves), 806376 bytes in use
   [2892] 12 Sep 02:41:09.428 - DB 8: 1 keys (0 volatile) in 4 slots HT.
   [2892] 12 Sep 02:41:09.428 - 1 clients connected (0 slaves), 806392 bytes in use
   [2892] 12 Sep 02:41:14.467 - DB 8: 1 keys (0 volatile) in 4 slots HT.
   [2892] 12 Sep 02:41:14.467 - 1 clients connected (0 slaves), 806408 bytes in use
   [2892] 12 Sep 02:41:19.509 - DB 8: 1 keys (0 volatile) in 4 slots HT.
   [2892] 12 Sep 02:41:19.509 - 1 clients connected (0 slaves), 806408 bytes in use
   [2892] 12 Sep 02:41:24.552 - DB 8: 1 keys (0 volatile) in 4 slots HT.
   [2892] 12 Sep 02:41:24.552 - 1 clients connected (0 slaves), 806424 bytes in use
   [2892] 12 Sep 02:41:29.594 - DB 8: 1 keys (0 volatile) in 4 slots HT.
   [2892] 12 Sep 02:41:29.594 - 1 clients connected (0 slaves), 806424 bytes in use
   [2892] 12 Sep 02:41:33.643 - Client closed connection
   
   
