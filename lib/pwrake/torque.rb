require "thread"
require "pty"

module Pwrake

  class PTYIO
    CHARS='0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'
    TLEN=64

    def initialize(r,w)
      @r=r
      @w=w
      @initiator = ""
      TLEN.times{ @initiator << CHARS[rand(CHARS.length)] }
      @terminator = ""
      TLEN.times{ @terminator << CHARS[rand(CHARS.length)] }
      @tregexp=/^(.*):([0-9]*):#{@terminator}$/o
    end
    attr_reader :status

    def system(cmd)
      #puts "cmd="+cmd
      @w.puts "echo '#{@initiator}';#{cmd};echo :$?:'#{@terminator}'" # "
      while (l=@r.gets.chomp) != @initiator
        #puts "   ="+l
      end
      result = []
      while l = @r.gets.chomp
        if @tregexp =~ l
          rest = $1
          stat = $2
          if rest.size > 0
            #puts "#{rest.inspect}"
            #puts rest
            result << rest
          end
          @status = Integer(stat)
          #puts "status = #{@status}"
          break
        else
          #puts l
          result << l
        end
      end
      result
    end

    def close
      @w.puts "echo '#{@initiator}'; exit"
      while (l=@r.gets.chomp) != @initiator
        #puts "   ="+l
      end
      #puts "finished"
    end
  end

  class Torque < Shell

    CHARS='0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!"#=~{*}?_-^@[],./'
    TLEN=64

    OPEN_LIST={}

    @@nice = "nice"
    @@shell = "sh"

    def initialize(*args)
      case args.size
      when 1
        @submit_host = args[0]
        /^([^.]+)\./ =~ args[0]
        @exec_host = $1
      when 2
        @exec_host = args[0]
        @submit_host = args[1]
      else
        raise ArgumentError, "wrong number of argument (#{args.size} for 1 or 2)"
      end
      #puts "@submit_host=#{@submit_host}, @exec_host=#{@exec_host}"
      @lock = Mutex.new
      @input = Queue.new
      @output = Queue.new
      open # (system_cmd(*arg))
    end

    attr_reader :status

    def system_cmd
      [@@nice,@@shell].join(' ')
    end

    def thread_loop
      ssh_cmd = "ssh -x -t -q #{@submit_host} "
      qsub_cmd = "qsub -I -l walltime=1:00:00,nodes=#{@exec_host}"
      PTY.spawn(ssh_cmd+qsub_cmd) do |r,w|
        x = PTYIO.new(r,w)
        #p x.system("hostname")
        #x.system("qsub -I -l walltime=1:00:00,nodes=#{@exec_host}")
        x.system("export PATH='#{ENV['PATH']}'")
        while cmd = @input.deq
          #puts "cmd = #{cmd}"
          #puts x.system("hostname")
          r = x.system(cmd)
          @status = x.status
          @output.enq(r)
        end
        x.close
      end
    end

    def open
      @thread = Thread.new() do
        #puts "thread"
        thread_loop
      end
      OPEN_LIST[__id__] = self
    end

    def system(*command)
      if command.kind_of? Array
        command = command.join(' ')
      end
      @lock.synchronize do
        @input.enq(command)
        @output.deq
      end
    end

    def exec(command)  
      @lock.synchronize do
        @input.enq(command)
        @output.deq
      end
    end

    def close
      #puts "close"
      @lock.synchronize do
        # @input.enq("exit")
        # @input.enq("exit")
        @input.enq(nil)
        OPEN_LIST.delete(__id__)
      end
      @thread.join
    end

    def self.connect_list( hosts )
      connections = []
      hosts.map do |h|
        #Thread.new(h) {|x|
          x=h
          ssh = Torque.new(x) 
          ssh.cd_cwd
          connections << ssh
        #}
      end #.each{|t| t.join}
      connections
    end

  end

end # module Pwrake



if __FILE__ == $0
  # test code
  conn = Pwrake::Torque.new("chiba102.intrigger.nii.ac.jp")
  cmd="hostname"
  res = conn.system cmd
  puts "cmd=#{cmd} res=#{res} status=#{conn.status}"
  cmd="ls hoge"
  res = conn.system cmd
  puts "cmd=#{cmd} res=#{res} status=#{conn.status}"
  cmd="hoge"
  res = conn.system cmd
  puts "cmd=#{cmd} res=#{res} status=#{conn.status}"
  conn.close
end
