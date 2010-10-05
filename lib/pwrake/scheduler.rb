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

  class TaskQueue

    class SimpleQueue
      def initialize(hosts=nil)
        @q = []
      end
      def push(x)
        @q.push(x)
      end
      def pop(h)
        @q.shift
      end
      def pop_alt(h)
        nil
      end
      def clear
        @q.clear
      end
    end

    def initialize(hosts=[])
      @finished = false
      @m = Mutex.new
      @q = SimpleQueue.new if !defined? @q
      @cv = ConditionVariable.new
      @th_end = []
    end

    def push(tasks)
      @m.synchronize do
        tasks.each do |task|
          @q.push(task)
        end
        @cv.signal
      end
    end

    def pop(host=nil)
      loop do
        @m.synchronize do
          if @th_end.first == Thread.current
            @th_end.shift
            return false
          elsif task = @q.pop(host)
            @cv.signal
            return task
          elsif @finished # no task in queue
            @cv.signal
            return false
          elsif task = @q.pop_alt(host)
            @cv.signal
            return task
          else
            @cv.wait(@m)
          end
        end
      end
    end

    def finish
      @finished = true
      @cv.signal
    end

    def stop
      @m.synchronize do
        @q.clear
        finish
      end
    end

    def thread_end(th)
      @th_end.push(th)
      @cv.broadcast
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
