module Pwrake

  class GfarmAffinityScheduler < Scheduler
    include Log

    def on_trace(tasks)
      #puts "--- tasks = #{tasks.inspect}"
      if Pwrake.manager.gfarm # and @@affinity
        gfwhere_result = {}
        filenames = []
        tasks.each do |t|
          if t.kind_of? Rake::FileTask and name = t.prerequisites[0]
            filenames << name
          end
        end
        #x.name } #GfarmSSH.local_to_gfarm_path(x.name)}
        #puts "--- filenames = #{filenames.inspect}"
        gfwhere_result = GfarmSSH.gfwhere(filenames)
        #puts "--- gfwhere_result = #{gfwhere_result.inspect}"
        tasks.each do |t|
          if t.kind_of? Rake::FileTask and prereq_name = t.prerequisites[0]
            t.locality = gfwhere_result[GfarmSSH.gf_path(prereq_name)]
          end
        end
        #new_tasks = tasks.map do |t|
        #  if t.kind_of? Rake::FileTask and prereq_name = t.prerequisites[0]
        #    hosts = gfwhere_result[GfarmSSH.gf_path(prereq_name)]
        #    QueuedTask.new(t,hosts)
        #  else
        #    t
        #  end
        #end
        #tasks = new_tasks
      end
      #puts "--- tasks = #{tasks.inspect}"
      tasks
    end

    def on_execute(task)
      #return task unless task.kind_of? QueuedTask
      #t = task.task
      if task.kind_of? Rake::FileTask and prereq_name = task.prerequisites[0]
        #puts "--- task = #{task.inspect}"
        conn = Thread.current[:connection]
        #puts "--- conn = #{conn.inspect}"
        scheduled = task.locality
        #puts "--- scheduled = #{scheduled.inspect}"
        if conn
          exec_host = conn.host
          #puts "--- exec_host = #{exec_host.inspect}"
          Pwrake.manager.counter.count( scheduled, exec_host )
          if Pwrake.manager.gfarm and conn
            if scheduled and scheduled.include? exec_host
              compare = "==" 
            else
              compare = "!="
            end
            #puts "compare = #{compare}"
            log "-- access to #{prereq_name}: gfwhere=#{scheduled.inspect} #{compare} execed=#{exec_host}"
          end
        end
      end
      task
    end

    def on_thread_end
      puts "-- $cv.broadcast"
      # $cv.broadcast
    end

    def queue_class
      AffinityQueue
    end
  end

  manager.scheduler_class = GfarmAffinityScheduler


  class AffinityQueue < TaskQueue

    class Throughput

      def initialize(list=nil)
        @interdomain_list = {}
        @interhost_list = {}
        if list
          values = []
          list.each do |x,y,v|
            hash_x = (@interdomain_list[x] ||= {})
            hash_x[y] = n = v.to_f
            values << n
          end
          @min_value = values.min
        else
          @min_value = 1
        end
      end

      def interdomain(x,y)
        hash_x = (@interdomain_list[x] ||= {})
        if v = hash_x[y]
          return v
        elsif v = (@interdomain_list[y] || {})[x]
          hash_x[y] = v
        else
          if x == y
            hash_x[y] = 1
          else
            hash_x[y] = 0.1
          end
        end
        hash_x[y]
      end

      def interhost(x,y)
        return @min_value if !x
        hash_x = (@interhost_list[x] ||= {})
        if v = hash_x[y]
          return v
        elsif v = (@interhost_list[y] || {})[x]
          hash_x[y] = v
        else
          x_short, x_domain = parse_hostname(x)
          y_short, y_domain = parse_hostname(y)
          v = interdomain(x_domain,y_domain)
          hash_x[y] = v
        end
        hash_x[y]
      end

      def parse_hostname(host)
        /^([^.]*)\.?(.*)$/ =~ host
        [$1,$2]
      end
    end # class Throughput


    class HostQueue

      def initialize(hosts)
        @hosts = hosts
        @q = {}
        @hosts.each{|h| @q[h]=[]}
        @q[nil]=[]
        @throughput = Throughput.new
        @size = 0
      end

      attr_reader :size

      def push(j)
        stored = false
        if j.respond_to? :locality
          j.locality.each do |h|
            if q = @q[h]
              j.assigned.push(h)
              q.push(j)
              stored = true
            end
          end
        end
        if !stored
          @q[@hosts[rand(@hosts.size)]].push(j)
        end
        @size += 1
      end

      #require "pp"
      def pop(host)
        q = @q[host]
        if q && !q.empty?
          j = q.shift
          #if j.respond_to? :assigned
            j.assigned.each{|x| @q[x].delete_if{|x| j.equal? x}}
          #end
          @size -= 1
          return j
        else
          nil
        end
      end

      def pop_alt(host)
        # select a task based on many and close
        max_host = nil
        max_num  = 0
        @q.each do |h,a|
          if !a.empty?
            d = @throughput.interhost(host,h) * a.size
            if d > max_num
              max_host = h
              max_num  = d
            end
          end
        end
        if max_host
          pop(max_host)
        else
          pop(nil)
        end
      end

      def clear
        @hosts.each{|h| @q[h].clear}
      end

    end # class HostQueue

    def initialize(hosts=[])
      @q = HostQueue.new(hosts.uniq)
      super(hosts)
    end

  end # class AffinityQueue

end # module Pwrake


if __FILE__ == $0
  require "pp"
  require "thread"
  # test
  hosts = (0..3).map{|i| "host%02d.a"%i}
  hosts.concat( (0..3).map{|i| "host%02d.b"%i} )
  q = Pwrake::AffinityQueue.new(hosts)

  def push(q,host,task)
    q.push(Pwrake::QueuedTask.new(task,host))
  end

  push(q, "host00.a", x=proc{puts "-- proc1 called --"})
  push(q, %w[host01.a host03.a], proc{puts "-- proc2 called --"})
  push(q, %w[host03.a host04.a host08.a], proc{puts "-- proc3 called --"})
  push(q, nil, proc{puts "-- proc4 called --"})
  q.push(proc{puts "-- proc4 called --"})
  push(q, %w[host01.b host05.b], proc{puts "-- proc5 called --"})
  pp q

  q.pop("host01.a").call
  q.pop("host02.b").call
  q.pop("host02.a").call
  q.pop("host05.a").call
  q.pop.call
  push(q, "host02.a", proc{puts "-- proc.end called --"})
  q.finish
  q.pop("host02.a").call
  pp q
end
