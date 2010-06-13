module Rake

  class Logger

    def self.time_str(t)
      t.strftime("%Y-%m-%dT%H:%M:%S.%%06d") % t.usec
    end

    def initialize(arg=nil)
      @out=nil
      File.open(arg) if arg
    end

    def open(file)
      @out.close if @out && @cloasable
      case file
      when IO
        @out=file
        @closeable=false
      else
        @out=File.open(file,"w")
        @closeable=true
      end
      @start_time = Time.now
      @out.puts "LogStart="+Logger.time_str(@start_time)
    end

    def finish(str, start_time)
      if @out
        finish_time = Time.now
        t1 = Logger.time_str(start_time)
        t2 = Logger.time_str(finish_time)
        elap = finish_time - start_time
        @out.puts "#{str} : start=#{t1} end=#{t2} elap=#{elap}"
      end
    end

    def puts(s)
      @out.puts(s) if @out
    end

    def <<(s)
      @out.puts(s) if @out
    end

    def close
      finish "LogEnd", @start_time
      @out.close if @closeable
      @out=nil
      @closeable=nil
    end
  end # class Logger


  class Application
    def logger
      @logger ||= Logger.new
    end
  end

end # module Rake

