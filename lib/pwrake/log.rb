module Pwrake

  module Log

    def log(*args)
      if Rake.application.options.trace
        if block_given?
          a = yield(*args)
        elsif args.size > 1
          a = args
        else
          a = args[0]
        end
        if a.kind_of? Array
          a.each{|x| Pwrake.manager.logger.puts(x)}
        else
          Pwrake.manager.logger.puts(a)
        end
      end
    end

    def time_str(t)
      t.strftime("%Y-%m-%dT%H:%M:%S.%%06d") % t.usec
    end

    module_function :log, :time_str
  end

  # Pwrake.log
  def self.log(*args)
    Log.log(*args)
  end

  # Pwrake.time_str
  def self.time_str(t)
    Log.time_str(t)
  end
end
