module Rake

  class Task
    attr_accessor :ssh
    attr_accessor :partition

    def set_weight(w)
      @weight=w
    end

    def weight=(w)
      @weight=w
    end

    def weight
      @weight || 0
    end

    alias orig_invoke_prerequisites :invoke_prerequisites

    def invoke_prerequisites(task_args, invocation_chain)
      @prerequisites.each { |n|
        prereq = application[n, @scope]
        prereq_args = task_args.new_scope(prereq.arg_names)
        prereq.ssh = ssh
        prereq.invoke_with_call_chain(prereq_args, invocation_chain)
      }
    end

    def rsh(*cmd, &block)
      options = (Hash === cmd.last) ? cmd.pop : {}
      unless block_given?
        show_command = cmd.join(" ")
        show_command = show_command[0,42] + "..." unless $trace
        # TODO code application logic heref show_command.length > 45
        block = lambda { |ok, status|
          ok or fail "Command failed with status (#{status.exitstatus}): [#{show_command}]"
        }
      end
      if RakeFileUtils.verbose_flag == :default
        options[:verbose] = true
      else
        options[:verbose] ||= RakeFileUtils.verbose_flag
      end
      options[:noop]    ||= RakeFileUtils.nowrite_flag
      rake_check_options options, :noop, :verbose
      rake_output_message cmd.join(" ") if options[:verbose]
      unless options[:noop]
	start_time = Time.now.to_f
	application.logger.puts "pw_rsh[start]: name=%s,cmd=\"%s\",end=%.3f"%[name,cmd,start_time] 
        if ssh
          res = ssh.exec(*cmd)
        else
          res = system(*cmd)
        end
	end_time = Time.now.to_f
	elap_time = end_time - start_time
	application.logger.puts "pw_rsh[end]: name=%s,cmd=\"%s\",end=%.3f,elap=%.3f"%[name,cmd,end_time,elap_time] 
	res
      end
    end


    # Trace the task if it is needed.  Prerequites are traced first.
    def dag_trace
      trace_with_call_chain(InvocationChain::EMPTY)
    end

    # Same as trace, but explicitly pass a call chain to detect
    # circular dependencies.
    def trace_with_call_chain(invocation_chain) # :nodoc:
      new_chain = InvocationChain.append(self, invocation_chain)
        return if @already_dag_traced
        @already_dag_traced = true
        trace_prerequisites(new_chain)
    end
    protected :trace_with_call_chain

    # Trace all the prerequisites of a task.
    def trace_prerequisites(invocation_chain) # :nodoc:
      @prerequisites.each { |n|
        prereq = application[n, @scope]
        prereq.trace_with_call_chain(invocation_chain)
      }
    end

  end # Task


  class PwMultiTask < Task

    private

    def invoke_prerequisites_in_queue(mqueue)
      # Spawn worker threads
      pool = []
      Rake.application.ssh_list.each_with_index do |ssh,idx|
	th = Thread.new do
	  if application.options.trace
	    puts "** Worker ##{idx}(#{ssh.host}) is ready."
	  end
	  while job = mqueue.pop(ssh.host)
	    job.call(ssh)
	    if application.options.trace
	      puts "** Worker ##{idx}(#{ssh.host}) is called."
	    end
	  end
	end
	pool.push(th)
      end

      # shutdown
      mqueue.finish
      pool.each{|th| th.join}
    end


    def invoke_prerequisites_affinity(task_args, invocation_chain)
      time_start = Time.now

      # Gfarm affinity
      gfwhere_result = {}
      if Rake.application.options.gfarm # and @@affinity
	list = []
	@prerequisites.each{|r|
	  if a=application[r].prerequisites[0]
	    list << a
	  end
	}
	gfwhere_result = GfarmSSH.gfwhere(list)
      end

      application.logger.finish("multitask_ssh(#{name})[gfwhere]",time_start)

      mqueue = AffinityQueue.new(Rake.application.core_list, @prerequisites.size)

      # create processes
      @prerequisites.each do |r|
        prereq = application[r]
	if Rake.application.options.gfarm and afile = prereq.prerequisites[0]
	  gfwhere_list = gfwhere_result[GfarmSSH.gf_path(afile)]
	else
	  gfwhere_list = nil
	end
	block = proc{|ssh|
	  Rake.application.counter.count( gfwhere_list, ssh.host ) 
	  if Rake.application.options.gfarm and ssh #  if @@affinity and ssh
	    if gfwhere_list and gfwhere_list.include? ssh.host
	      compare = "==" 
	    else
	      compare = "!="
	    end
	    s = "** Affinity for #{afile}: gfwhere=#{gfwhere_list.inspect} #{compare} ssh.host=#{ssh.host}"
	    application.logger.puts s
	    puts s if application.options.trace
	  end
	  prereq.ssh = ssh
	  prereq.invoke_with_call_chain(task_args, invocation_chain)
	}
	mqueue.push( Rake.application.options.affinity && gfwhere_list, block )
      end

      application.logger.finish("multitask_ssh(#{name})[prepare]",time_start)

      invoke_prerequisites_in_queue(mqueue)

      application.logger.finish("multitask_ssh(#{name})[end]",time_start)
    end


    def invoke_prerequisites_graph(task_args, invocation_chain)
      time_start = Time.now
    
      application.logger.finish("pw_multitask(#{name})[start]",time_start) 
    
      puts "Rake.application.core_list=#{Rake.application.core_list.inspect}"
      mqueue = AffinityQueue.new(Rake.application.core_list, @prerequisites.size)
    
      # create processes
      @prerequisites.each do |r|
        prereq = application[r]
        afile = prereq.prerequisites[0]
	if application.options.mode == "graph_partition" && prereq.partition
	  dispatch_list = Rake.application.node_group[ prereq.partition ]
	else
	  dispatch_list = nil
	end
	application.logger.puts "** name=#{prereq.name} dispatch_list=#{dispatch_list.inspect}" 
    
        block = proc{|ssh|
          Rake.application.counter.count( dispatch_list, ssh.host ) 
          if Rake.application.options.gfarm and ssh
            if dispatch_list and dispatch_list.include? ssh.host
              compare = "==" 
            else
              compare = "!="
            end
            s = "** Grouping '#{afile}': dispatch=#{dispatch_list.inspect} #{compare} ssh.host=#{ssh.host}"
            application.logger.puts s if @@dag_done
            puts s if application.options.trace
          end
          prereq.ssh = ssh
          prereq.invoke_with_call_chain(task_args, invocation_chain)
        }
        mqueue.push( dispatch_list, block )
      end
    
      application.logger.finish("pw_multitask(#{name})[prepare]",time_start) 

      invoke_prerequisites_in_queue(mqueue)

      application.logger.finish("pw_multitask(#{name})[end]",time_start) 
    end


    def invoke_prerequisites(task_args, invocation_chain)
      case application.options.mode
      when "affinity"
	invoke_prerequisites_affinity(task_args, invocation_chain)
      else
	invoke_prerequisites_graph(task_args, invocation_chain)
      end
    end

  end # PwMultiTask

  class Application
    attr_accessor :node_group
    attr_accessor :core_list
    attr_accessor :ssh_list
    attr_accessor :counter
  end

  class << PwMultiTask
    @logfile = nil
    Rake.application.counter = Counter.new

    def setup(mode=nil)
      if !defined? @prepare_done
        prepare_logger
        load_nodelist
        set_filesystem(mode)
        prepare_ssh
        @prepare_done = true
      end
    end

    def setup_dag
      if Rake.application.options.mode == "graph_partition"
        if Task.task_defined?("pwrake_dag")
          Task["pwrake_dag"].invoke
        end
        dag
      end
    end

    def prepare_logger
      if logfile = ENV["LOGFILE"] || ENV["LOG"]
        @logfile = "log/" + logfile + ".log"
        mkdir_p "log"
        Rake.application.logger.open(@logfile)
        puts "logfile=#{@logfile}"
      else
        Rake.application.logger.open($stdout)
      end
    end

    def load_nodelist
      node_group = []
      if nodefile = ENV["NODEFILE"] || ENV["NODELIST"] || ENV["NODE"]
        nodelist = []
        open(nodefile){|f|
          while l=f.gets
            host, ncore, group = l.split
            ncore = (ncore || 1).to_i
            group = group.to_i
            nodelist << ([host] * ncore.to_i)
            node_group[group] ||= []
            node_group[group] << host
          end
        }
        core_list = []
        begin # alternative order
          sz=0
          nodelist.each do |a|
            core_list << a.shift if !a.empty?
            sz += a.size
          end
        end while sz>0
      else
        core_list = ["localhost"]
      end
      puts "HOSTS=\n" + core_list.join("\n") if core_list
      Rake.application.core_list = core_list
      Rake.application.node_group = node_group
      Rake.application.options.npart = node_group.size
    end

    def set_filesystem(mode=nil)
      #case mode || ENV["INVOKE_MODE"] || ENV["MODE"]
      #when /^group|graph|partition/
      #  Rake.application.options.mode = "graph_partition"
      #when /^af/
      #  Rake.application.options.mode = "affinity"
      #  Rake.application.options.affinity = true
      #else
      #  Rake.application.options.mode = "none"
      #end
      #puts "INVOKE_MODE=#{Rake.application.options.mode}"

      case ENV["FILESYSTEM"] || ENV["FS"]
      when "gfarm"
        Rake.application.options.gfarm = true
        Rake.application.options.mode = "affinity"
        Rake.application.options.affinity = true
        puts "FILESYSTEM=Gfarm"

        mountpoint = ENV["MOUNTPOINT"] || ENV["MP"]
        Rake.application.options.single_mp = mountpoint && /single/=~mountpoint
        puts "MOUNTPOINT=#{mountpoint}"
      else
        Rake.application.options.gfarm = false
        puts "FILESYSTEM=non-Gfarm"
      end

      Rake.application.options.gfarm_mountpoint = 
        ENV["GFARM_MOUNTPOINT"] || ENV["GFARM_MP"]
    end

    def prepare_ssh
      time_init_ssh = Time.now
      ssh_list = []
      if Rake.application.options.gfarm
        GfarmSSH.set_mountpoint
        th = []
        Rake.application.core_list.each_with_index {|h,i|
          mnt_dir = "%s%03d" % [GfarmSSH.mountpoint,i]
          th << Thread.new(h,mnt_dir) {|x,y|
            if Rake.application.options.single_mp
              #puts "# create SSH to #{x}"
              ssh = GfarmSSH.new(x) 
            else
              #puts "# create SSH to #{x}:#{y}"
              ssh = GfarmSSH.new(x,y)
            end
            ssh.cd_cwd
            ssh_list << ssh
          }
        }
        th.each{|t| t.join}
        system "ps x"
      else
        Rake.application.core_list.map {|h|
          Thread.new(h) {|x|
            ssh = SSH.new(x) 
            ssh.cd_cwd
            ssh_list << ssh
          }
        }.each{|t| t.join}
      end
      Rake.application.ssh_list = ssh_list
      Rake.application.logger.finish("prepare SSH",time_init_ssh)
    end


    def dag
      time_start = Time.now
      Rake.application.logger.finish("partition[start]",time_start) 

      @vertex_list = Metis::VertexList.new
      n_part = Rake.application.options.npart
      if n_part && n_part > 1
	@part = @vertex_list.partition(n_part)
	count_same = 0
	count_cut  = 0
	@vertex_list.each do |v|
	  Rake.application[v.name].partition = part = @part[v.index]
	  v.adj.each{|adj_vtx, wei|
	    if part == @part[adj_vtx.index]
	      count_same += 1
	    else
	      count_cut += 1
	    end
	  }
	end
	Rake.application.logger.puts "***partition: nocut edges = #{count_same/2}"
	Rake.application.logger.puts "***partition:   cut edges = #{count_cut/2}"
      else
	@vertex_list.each do |v|
          Rake.application[v.name].partition = 0
	end
      end
      @@dag_done = true
      Rake.application.logger.finish("partition[end]",time_start) 
    end


    def finish
      if @prepare_done
        puts "finish pwrake"
        Rake.application.counter.print
        Rake.application.ssh_list.map{|v| Thread.new(v){|s| s.close}}.each{|t| t.join}
        Rake.application.logger.close
        if @logfile
          #mkdir_p "log"
          #cp @logfile, "log/"
          $stderr.puts "log file : "+@logfile
        end
      end
    end

  end # class << PwMultiTask


  class Task
    @@dag_done=nil

    alias execute_orig :execute

    def execute(args=nil)
      execute_orig(args)
      dag = (@@dag_done) ? "done" : "yet"
      Rake.application.logger.puts "node=[#{@ssh.host}] input=[#{self.prerequisites.join(';')}] output=[#{self.name}] dag=#{dag} weight=#{weight}" if @ssh
    end
  end


  class Application

    alias orig_top_level :top_level
    def top_level
      PwMultiTask.setup
      PwMultiTask.setup_dag
      orig_top_level
      PwMultiTask.finish
    end
  end

end # module Rake


def pw_multitask(*args, &block)
  Rake::PwMultiTask.define_task(*args, &block)
end

def pw_weight(taskname,weight)
  Rake.application[taskname].weight = weight
end
