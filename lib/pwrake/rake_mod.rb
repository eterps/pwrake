module Rake

  class Application
    include Pwrake::Log

    attr_reader :pwrake
    attr_reader :operator

    alias top_level_orig :top_level

    def top_level
      pwrake = nil
      begin
        pwrake = Pwrake.manager
        top_level_orig
      ensure
        $stderr.puts "** ensure"
        pwrake.finish if pwrake
      end
    end


    alias invoke_task_orig :invoke_task

    def invoke_task(task_string)
      name, args = parse_task_string(task_string)
      root = self[name]
      begin
        operator = Pwrake.manager.operator
        operator.invoke(root,args)
      ensure
        operator.finish if operator
      end
    end


    alias standard_rake_options_orig :standard_rake_options

    def standard_rake_options
      opts = standard_rake_options_orig
      opts.each_with_index do |a,i|
        if a[0] == '--version'
          a[3] = lambda { |value|
            puts "rake, version #{RAKEVERSION}"
            puts "pwrake, version #{Pwrake::PWRAKEVERSION}"
            exit
          }
        end
      end
      opts
    end

  end # class Application


  class Task
    include Pwrake::Log
    attr_accessor :already_invoked

    alias invoke_orig :invoke

    def invoke(*args)
      log "--- Task#invoke(#{args.inspect}) Pwrake.manager.threads=#{Pwrake.manager.threads}"
      #if Pwrake.manager.threads == 1
      #  invoke_orig(*args)
      #else
        task_args = TaskArguments.new(arg_names, args)
        Pwrake.manager.operator.invoke(self,task_args)
      #end
    end
  end

end # module Rake
