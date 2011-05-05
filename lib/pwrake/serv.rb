require "shellwords"

$debug=false

class Invoker
  def initialize(idx,pipe)
    @idx=idx
    @cpid=nil
    @pipe=pipe
    [:TERM,:INT,:KILL].each{|k| self.trap(k)}
  end

  def trap(sig)
    Signal.trap(sig) do
      Process.kill(sig, @cpid) if @cpid
      fin
      exit
    end
  end

  def fin(status=nil)
    s = "end:#{@idx}"
    s += ":#{status}" if status
    puts s
    $stdout.flush
  end

  def wait
    while cmd = @pipe.gets
      cmd.strip!
      words = Shellwords.shellsplit(cmd)
      if cmd=="" || /^#/=~cmd
        $stderr.puts "empty command"
        fin(0)
      elsif words[0] == "exit"
        @pipe.close
        fin
        exit
        break
      elsif words[0] == "cd"
        dir = words[1]
        if dir
          Dir.chdir(dir)
        else
          Dir.chdir
        end
        fin(0)
      else
        cmd = "(#{cmd}) 2>&1"
        io = IO.popen(cmd)
        $stderr.puts "io.pid=#{io.pid}" if $debug
        @cpid = io.pid
        while x = io.gets
          puts "#{@idx}: #{x}"
          $stderr.puts "out:#{@idx}: #{x.inspect}" if $debug
        end
        Process.wait(@cpid) rescue nil
        status = $?
        @cpid = nil
        io.close
        $stderr.puts "status:#{@idx} #{status}" if $debug
        code = (status) ? status.exitstatus : "unknown"
        fin(code)
      end
    end
  end
end


class Dispatcher

  def initialize(n=0)
    @pid = {}
    @pipe_w = {}
    n.times{|i| fork(i)}
  end

  def finish
    $stderr.puts "finish" if $debug
    @pipe_w.each do |i,pipe|
      if pipe and !pipe.closed?
        pipe.puts("exit")
        pipe.flush
        close(i)
      end
    end
    Process.waitall
  end

  def fork(i)
    pipes = IO.pipe
    @pipe_w[i] = pipes[1]
    @pid[i] = Process.fork do
      Invoker.new(i,pipes[0]).wait
      exit
    end
  end

  def close(i)
    if @pipe_w[i]
      @pipe_w[i].close
      @pipe_w[i]=nil
      @pid[i]=nil
    end
  end

  def err(i,m)
    puts "err:#{i}:#{m}"
  end

  def loop
    while line = STDIN.gets
      $stderr.puts "in:"+line.inspect if $debug
      line.chomp!
      case line
      when /^(\d+): (.*)$/o
        i = Regexp.last_match[1].to_i(10)
        cmd = Regexp.last_match[2]
        if !@pipe_w[i]
          err i, "channel id: #{i} does not exist"
        elsif @pipe_w[i].closed?
          err i, "closed process : #{i}"
        else
          @pipe_w[i].puts(cmd)
          @pipe_w[i].flush
          if cmd=="exit"
            close(i)
          end
        end
      when /^new:(\d+)$/o
        i = Regexp.last_match[1].to_i(10)
        if !@pipe_w[i]
          fork(i)
          puts "end:#{i}"
        else
          err i, "channel id: #{i} already exists"
        end
      when /^kill:(\d+) ([A-Z]+)$/o
        i = Regexp.last_match[1].to_i(10)
        sig = Regexp.last_match[2]
        Process.kill(sig,@pid[i])
        $stderr.puts "Process.kill(#{sig},#{@pid[i]})"
        if @pipe_w[i]
          close(i)
          puts "end:#{i}"
        end
      when "end"
        finish
        exit
      else
        err i, "missing number in line: #{line}"
      end
      $stdout.flush
    end
  end
end

if ARGV[0]
  Dir.chdir ARGV[0]
end

Dispatcher.new.loop
