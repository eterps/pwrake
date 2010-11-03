require "thread"
require "socket"

module Pwrake

  class Connection

    def channel(idx)
      raise "idx=#{idx} does not exist" if !@queue[idx]
      Channel.new(self,idx)
    end

    def remote?
      #!@host.nil? and IPSocket.getaddress(@host) != "127.0.0.1"
      !@host.nil? and !["localhost","localhost.localdomain","127.0.0.1"].include?(@host)
    end

    def syschk(cmd)
      if !system(cmd)
        raise "fail on `#{cmd}'"
      end
    end

    def initialize(host=nil)
      @host = host
      servrb = "pwrake/serv.rb"

      fn = $:.find{|x| File.exist?(x+"/"+servrb)}
      if fn.nil?
        raise "#{servrb} is missing"
      end
      #puts "found !!! #{fn}"

      if remote?
        # puts "remote!!! #{host}"
        ssh_cmd = "ssh -x -T -q '#{@host}'"
        #if !system("#{ssh_cmd} 'test -e .#{servrb}'")
        syschk("#{ssh_cmd} 'mkdir -p .#{File.dirname(servrb)}'")
        syschk("scp -p -q '#{fn}/#{servrb}' '#{@host}:.#{servrb}'")
        #end
        @io = IO.popen( "#{ssh_cmd} ruby '.#{servrb}'", "r+" )
      else
        # puts "lopcall!!! #{host}"
        @io = IO.popen( "ruby '#{fn}/#{servrb}'", "r+" )
      end

      @last_channel = 0
      @queue = {} # (0...@n).map{Queue.new}
      @thread = Thread.new{ thread_loop() }
    end

    def enqueue(idx,cmd)
      if @queue[idx]
        @queue[idx].push(cmd)
      else
        $stderr.puts "Channel ID: #{idx} does not exist."
      end
    end

    def thread_loop
      while line = @io.gets
        line.chomp!
        #p line

        case line

        when /^(\d+): (.*)$/o
          idx = $1.to_i(10)
          cmd = $2
          enqueue(idx,cmd)

        when /^(end|err):(\d+)(:.*)?$/o
          cmd = $1
          idx = $2.to_i(10)
          msg = $3
          cmd = cmd+msg if msg
          enqueue(idx,cmd)

        else
          $stderr.puts "unknown result: #{line}"
        end
      end
    end

    def loop(idx)
      while x = @queue[idx].pop
        # $stderr.puts "Con#exe q.pop=#{x.inspect}" if $debug
        if /^end(:(.*))?/o =~ x
          return $2.to_i
        end
        yield(x)
        if /^err(:(\d+))?/o =~ x
          return $2
        end
      end
    end

    def new_channel
      idx = @last_channel
      @last_channel += 1
      @queue[idx] = Queue.new
      @io.write("new:#{idx}\n")
      @io.flush
      loop(idx){|x| puts x}
      idx
    end

    def execute(idx,cmd)
      if !@queue[idx]
        raise "key=#{@idx.inspect} does not exist in @queue={@queue.inspect}"
      end
      @io.write("#{idx}: #{cmd}\n")
      @io.flush
      status = loop(idx){|x| puts x}
      return status==0, status
    end

    def backquote(idx,cmd)
      if !@queue[idx]
        raise "key=#{@idx.inspect} does not exist in @queue={@queue.inspect}"
      end
      @io.write("#{idx}: #{cmd}\n")
      @io.flush
      a = ''
      loop(idx){|x| a << x}
      return a, status
    end

    def close(idx=nil)
      if idx.nil?
        @io.write("end\n")
        @io.flush
        #loop(idx){|x| puts x}
        @thread.join
      else
        execute(idx,"exit")
        @queue[idx] = nil
      end
    end

    def kill(idx,signal)
      @io.write("kill:#{idx} #{signal}\n")
      @io.flush
      loop(idx){|x| puts x}
    end
  end
end
