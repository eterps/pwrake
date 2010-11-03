require "thread"

module Pwrake

  class Channel
    CONN={}
    LOCK={}
    #MUTEX=Mutex.new

    def localhost?(host)
      IPSocket.getaddress(host) == "127.0.0.1"
    end

    def initialize(host)
      @status = nil
      @host = host
      LOCK[host] ||= Mutex.new
      LOCK[host].synchronize do
        @con = (CONN[host] ||= Connection.new(host))
      end
      @idx = @con.new_channel
      on_start
      cd_cwd
    end

    attr_reader :host, :status

    def close
      execute("cd")
      on_close
      @con.close(@idx)
    end

    def on_start
    end

    def on_close
    end

    def cd(dir=nil)
      execute("cd #{dir}")
    end

    def cd_cwd
      execute("cd #{Dir.pwd}")
    end

    def execute(cmd)
      res,@status = @con.execute(@idx,cmd)
      puts "res=#{res},@status=#{@status}" if $debug
      res
    end

    def backquote(cmd)
      res,@status = @con.backquote(@idx,cmd)
      puts "res=#{res},@status=#{@status}" if $debug
      res
    end

    def kill(signal)
      @con.kill(@idx,signal)
    end

    #require 'pp'
    class << self

      def connect( hosts )
        core_list = []
        hosts.map do |h|
          Thread.new(h) do |g|
            c = self.new(g)
            core_list << c
          end
        end.each{|t| t.join}
        #pp core_list
        core_list
      end

      def close_all
        CONN.each do |h,con|
          con.close
        end
      end

    end
  end
end
