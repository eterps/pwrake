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
        if find_task( root, [] )
          return @fetched_tasks
        else
          return nil
        end
      end
    end

    def find_task( task, chain )
      name = task.name
      #log "-- trace #{name}."
      #log "-- trace #{task.inspect}."

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
      # @to_worker = TaskQueue.new(connections)
      # @from_worker = Queue.new
      @scheduler.on_start
      @workers = []
      connections.each_with_index do |conn,j|
        @workers << Thread.new(conn,j) do |c,i|
          begin
            #p [conn.host, Thread.current]
            thread_loop(c,i)
          ensure
            log "-- worker[#{i}] ensure : closing #{conn.host}"
            #p [:ensure, conn.host, Thread.current]
            #Thread.pass
            conn.close
            # @workers.each{|t| t.join if t.alive? }
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
          log "-- worker[#{i}] start : #{task.inspect}"
          task = @scheduler.on_execute(task)
          task.already_invoked = true
          task.execute #if task.needed?
          task.output_queue.push(task)
          log "-- worker[#{i}] end : #{task}"
        end
      end
      log "-- worker[#{i}] loopout : #{task.inspect}"
      # @scheduler.on_thread_end
    end

    # Provide standard execption handling for the given block.
    def standard_exception_handling
      begin
        yield
      rescue SystemExit => ex
        # Exit silently with current status
        #p ex
        #p "pass1"
        @input_queue.stop
        raise
      rescue OptionParser::InvalidOption => ex
        # Exit silently
        #p ex
        #p "pass2"
        @input_queue.stop
        exit(false)
        #raise
      rescue Exception => ex
        #p ex
        #p "pass3"
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
        #raise
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
        # log{ a.map{|x| "-- fetch #{x.inspect}"} }
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
        log "-- new worker[#{j}] exit"
        thread.exit
      end
    end

    def finish
      log "-- Operator#finish called ---"
      @scheduler.on_finish
      #puts "-- Operator # finish pass1 ---"
      @input_queue.finish
      #puts "-- Operator # finish pass2 ---"
      #Thread.pass
      #puts "-- Operator # finish pass3 ---"
      @workers.each{|t| t.join }
      #puts "-- Operator # finish pass4 ---"
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
        log "-- Application # invoke_task # invoke start ---"
        operator.invoke(root,args)
        log "-- Application # invoke_task # invoke end ---"
      ensure
        operator.finish if operator
      end
      #puts "end invoke"
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


#Rake::Task.include( Pwrake::Task )
#Rake::Application.include( Pwrake::Application )

#Rake::Task.module_eval { include Pwrake::Task }
#Rake::Application.module_eval { include Pwrake::Application }


#alias task_orig :task

#def task(*args, &block)
#  Rake::ParallelTask.define_task(*args, &block)
#end
