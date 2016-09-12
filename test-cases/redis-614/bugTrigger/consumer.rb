require 'bundler/setup'
require 'redis'

class Consumer
  def initialize(redis_url)
    @redis = Redis.new(:url => redis_url)
    @worker_id = "worker:#{$$}"
  end

  def run
    puts "Starting consumer #{@worker_id}"
    @redis.del(@worker_id)
    loop do
      element = @redis.brpoplpush('queue', @worker_id)
      puts "Got element: #{element}"
      @redis.lrem(@worker_id, 0, element)
    end
  end
end
Consumer.new('redis://127.0.0.1:6379/8').run
