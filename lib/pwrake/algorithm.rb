require "thread"
#require "pp"

module Pwrake

  class Tracer
    include Log

    def initialize
      @mutex = Mutex.new
      @fetched = {}
    end

    def available_task( root )
      @footprint = {}
      @fetched_tasks = []
      @mutex.synchronize do
        tm = timer("trace")
        status = find_task( root, [] )
        msg = [ "num_tasks=%i" % [@fetched_tasks.size] ]
        tk = @fetched_tasks[0]
        msg << "task[0]=%s" % tk.name.inspect if tk.kind_of?(Rake::Task)
        tm.finish(msg.join(' '))
        if status
          return @fetched_tasks
        else
          return nil
        end
      end
    end

    def find_task( task, chain )
      name = task.name

      if task.already_invoked
        return nil
      end

      if chain.include?(name)
        fail RuntimeError, "Circular dependency detected: #{chain.join(' => ')} => #{name}"
      end

      if @footprint[name] || @fetched[name]
        return :traced
      end
      @footprint[name] = true

      chain.push(name)
      prerequisites = task.prerequisites
      all_invoked = true
      i = 0
      while i < prerequisites.size
        prereq = task.application[prerequisites[i], task.scope]
        if find_task( prereq, chain )
          all_invoked = false
        end
        i += 1
      end
      chain.pop

      if all_invoked
        @fetched[name] = true
        if task.needed?
          @fetched_tasks << task
        else
          task.already_invoked = true
          return nil
        end
      end

      :fetched
    end
  end



  class Operator
    include Log

    def initialize
      Thread.abort_on_exception = true
      connections = Pwrake.manager.connection_list
      @scheduler = Pwrake.manager.scheduler_class.new
      log "@scheduler.class = #{@scheduler.class}"
      log "@scheduler.queue_class = #{@scheduler.queue_class}"
      @input_queue = @scheduler.queue_class.new(Pwrake.manager.core_list)
      @scheduler.on_start
      @workers = []
      connections.each_with_index do |conn,j|
        @workers << Thread.new(conn,j) do |c,i|
          begin
            thread_loop(c,i)
          ensure
            log "-- worker[#{i}] ensure : closing #{conn.host}"
            conn.close
          end
        end
      end
      @tracer = Tracer.new
    end

    def thread_loop(conn,i)
      Thread.current[:connection] = conn
      Thread.current[:id] = i
      host = conn.host
      standard_exception_handling do
        while task = @input_queue.pop(host)
          tm = timer("task","worker##{i} task=#{task}")
          task = @scheduler.on_execute(task)
          task.already_invoked = true
          task.execute #if task.needed?
          task.output_queue.push(task)
          tm.finish("worker##{i} task=#{task}")
        end
      end
      log "-- worker[#{i}] loopout : #{task}"
      # @scheduler.on_thread_end
    end

    # Provide standard execption handling for the given block.
    def standard_exception_handling
      begin
        yield
      rescue SystemExit => ex
        # Exit silently with current status
        @input_queue.stop
        raise
      rescue OptionParser::InvalidOption => ex
        # Exit silently
        @input_queue.stop
        exit(false)
      rescue Exception => ex
        # Exit with error message
        name = "pwrake"
        $stderr.puts "#{name} aborted!"
        $stderr.puts ex.message
        if Rake.application.options.trace
          $stderr.puts ex.backtrace.join("\n")
        else
          $stderr.puts ex.backtrace.find {|str| str =~ /#{@rakefile}/ } || ""
          $stderr.puts "(See full trace by running task with --trace)"
        end
        @input_queue.stop
        exit(false)
      end
    end

    def invoke(root, args)
      log "--- Task # invoke #{root.inspect}, #{args.inspect} thread=#{Thread.current.inspect}"
      if conn = Thread.current[:connection]
        j = Thread.current[:id]
        thread = Thread.new(conn,j) {|c,i|
          log "-- new worker[#{i}] created"
          thread_loop(c,i)
        }
      else
        thread = nil
      end
      output_queue = Queue.new
      while a = @tracer.available_task(root)
        a = @scheduler.on_trace(a)
        a.each{|task| task.output_queue = output_queue }
        @input_queue.push(a)
        a.each do |x|
          b = output_queue.pop
          if !a.include?(b)
            puts "b= #{b.class.inspect}"
            p b
            a.each_with_index do |x,i|
              puts "a[#{i}]="
              puts "#{a[i].class.inspect}"
              p a[i]
            end
            raise "b is not included in a"
          end
        end
      end
      log "--- End of Task # invoke #{root.inspect}, #{args.inspect} thread=#{Thread.current.inspect}"
      if thread
        @input_queue.thread_end(thread)
        thread.join
        log "-- new worker[#{j}] exit"
      end
    end

    def finish
      log "-- Operator#finish called ---"
      @scheduler.on_finish
      @input_queue.finish
      @workers.each{|t| t.join }
    end
  end


  class Manager
    def operator
      @operator ||= Operator.new
    end
  end

end # module Pwrake




module Rake

  class Application
    include Pwrake::Log
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

    attr_reader :operator
  end


  class Task
    include Pwrake::Log
    attr_accessor :already_invoked
    alias invoke_orig :invoke

    def invoke(*args)
      log "--- Task#invoke(#{args.inspect}) Pwrake.manager.threads=#{Pwrake.manager.threads}"
      if Pwrake.manager.threads == 1
        invoke_orig(*args)
      else
        task_args = TaskArguments.new(arg_names, args)
        Pwrake.manager.operator.invoke(self,task_args)
      end
    end
  end

end

