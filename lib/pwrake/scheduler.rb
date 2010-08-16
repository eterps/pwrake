require "delegate"

module Rake
  class Task
    attr_accessor :output_queue

    def assigned
      @assigned ||= []
    end

    def locality
      @locality ||= []
    end

    def locality=(a)
      @locality = a
    end
  end
end


module Pwrake

=begin
  class QueuedTask < DelegateClass(Rake::Task)

    def initialize(task,locality=nil)
      @task = task
      super(@task)
      if locality.kind_of? Array
        @locality = locality
      else
        @locality = [locality]
      end
      @assigned = []
    end
    attr_reader :locality, :assigned, :task
  end

  class QueuedTask

    def initialize(task,locality=nil)
      @task = task
      if locality.kind_of? Array
        @locality = locality
      else
        @locality = [locality]
      end
      @assigned = []
    end

    attr_accessor :done_queue
    attr_reader :locality, :assigned, :task

    def already_invoked=(a)
      @task.already_invoked = a
    end

    def execute(*args)
      @task.execute(*args)
    end

    def name
      @task.name
    end

    def done_queue=(x)
      @task
  end
=end

  class TaskQueue
    def initialize(hosts=[])
      @n = hosts.size
      @n = 1 if @n==0
      #puts "taskqueue @n = #{@n}"
      @m = Mutex.new
      @q = Queue.new
    end

    def push(tasks)
      tasks.each do |task|
        #unless task.kind_of? QueuedTask
        #  task = QueuedTask.new(task)
        #end
        # puts "-- TaskQueue#push #{task.inspect}"
        @q.push(task)
      end
    end

    def pop(host=nil)
      @q.pop
    end

    def finish
      #puts "-- TaskQueue # finish called ---"
      @finished = true
      @m.synchronize do
        @n.times do
          @q.push(false)
        end
      end
      #puts "-- TaskQueue # finish end #{@q.size} ---"
    end

    def stop
      @m.synchronize do
        while ! @q.empty?
          @q.pop
        end
        @n.times do
          @q.push(false)
        end        
      end
    end
  end


  class Scheduler
    def initialize
    end

    def on_start
    end

    def on_trace(a)
      a
    end

    def on_execute(a)
      # puts "-- on_execute #{a.inspect}"
      if a.kind_of? TaskQueue
        a.task
      else
        a
      end
    end

    def on_finish
    end

    def queue_class
      TaskQueue
    end
  end

  #manager.scheduler_class = Scheduler
end
