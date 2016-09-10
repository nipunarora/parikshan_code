require 'bundler/setup'
require 'redis'

class Producer
  def initialize(redis_url)
    @redis = Redis.new(:url => redis_url)
        @script = <<EOS
    local elements = { 1, 2, 3, 4}
    return redis.call('lpush', KEYS[1], unpack(elements))
EOS
  end

  def run
    puts "Starting producer"
    loop do
      puts "Inserting 4 elements..."
      @redis.eval(@script, :keys => ['queue'])
      sleep 1
    end
  end
end
Producer.new('redis://127.0.0.1:6379/8').run
