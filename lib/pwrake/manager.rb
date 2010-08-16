module Rake

  class Application
    alias top_level_orig :top_level

    def top_level
      pwrake = nil
      begin
        pwrake = Pwrake.manager
        #pwrake.setup
        #pwrake.setup_dag
        top_level_orig
        #end
      ensure
        puts "** ensure"
        pwrake.finish if pwrake
      end
    end

    attr_reader :pwrake
  end
end


module Pwrake

  def self.manager
    if !@manager
      @manager = Manager.new
      @manager.setup_logger
      @manager.setup
      @manager.setup_dag
    end
    @manager
  end

  class Manager
    include Log
    attr_reader :node_group
    attr_reader :core_list
    attr_reader :counter
    attr_reader :threads
    attr_reader :logger
    attr_reader :gfarm

    #attr_accessor :scheduler_class

    def scheduler_class=(a)
      @scheduler_class=a
    end

    def scheduler_class
      @scheduler_class ||= Scheduler
    end

    def connection_list
      setup_connection if @connection_list.nil?
      @connection_list
    end

    def initialize
      @logfile = nil
      @logger = Logger.new
      @prepare_done = false
      @connection_list = nil
    end


    def setup(scheduling=nil)
      if !@prepare_done
        @counter = Counter.new
        setup_hostlist
        setup_filesystem(scheduling)
        # setup_connection
        @prepare_done = true
      end
    end


    def setup_dag
      if @scheduling == "graph_partition"
        if Task.task_defined?("pwrake_dag")
          Task["pwrake_dag"].invoke
        end
        dag
      end
    end


    def setup_logger
      if @logfile = ENV["LOGFILE"] || ENV["LOG"]
        # @logfile = "log/" + logfile + ".log"
        # mkdir_p "log"
        mkdir_p File.dirname(@logfile)
        @logger.open(@logfile)
        log "logfile=#{@logfile}"
      else
        @logger.open($stdout)
      end
    end


    def setup_hostlist
      @hostfile = ENV["HOSTFILE"] || ENV["HOSTLIST"] || ENV["HOSTS"] ||
        ENV["NODEFILE"] || ENV["NODELIST"] || ENV["NODES"]
      # @hostlist = HostList.new(@hostfile)
      #
      @host_group = []
      if @hostfile
        tmplist = []
        File.open(@hostfile) {|f|
          while l = f.gets
            l = $1 if /^([^#]+)#/ =~ l
            host, ncore, group = l.split
            ncore = (ncore || 1).to_i
            group = (group || 0).to_i
            tmplist << ([host] * ncore.to_i)
            @host_group[group] ||= []
            @host_group[group] << host
          end
        }
        #
        @core_list = []
        begin # alternative order
          sz = 0
          tmplist.each do |a|
            @core_list << a.shift if !a.empty?
            sz += a.size
          end
        end while sz>0
      else
        @core_list = ["localhost"]
      end
      #
      @threads = @core_list.size
      log "HOSTS=\n" + @core_list.join("\n")
    end


    def setup_filesystem(scheduling=nil)
      case ENV["FILESYSTEM"] || ENV["FS"]
      when "gfarm"
        @gfarm = true
        @scheduling = :affinity
        @affinity = true
        log "FILESYSTEM=Gfarm"
        require "pwrake/affinity"
        #
        #mountpoint = ENV["MOUNTPOINT"] || ENV["MP"]
        #@single_mp = mountpoint && /single/=~mountpoint
        #log "MOUNTPOINT=#{mountpoint}"
      else
        @gfarm = false
        log "FILESYSTEM=non-Gfarm"
      end
      #
      @gfarm_mountpoint = ENV["GFARM_MOUNTPOINT"] || ENV["GFARM_MP"]
    end


    def setup_connection
      if @core_list.all?{|x| x=="localhost" }
        @connection_class = Shell
      elsif @gfarm
        @connection_class = GfarmSSH
      else
        @connection_class = SSH
        #require "pwrake/torque"
        #@connection_class = Torque # SSH
      end
      time_init_ssh = Time.now
      log "@connection_class = #{@connection_class}"
      @connection_list = @connection_class.connect_list(@core_list)
    end


    def finish
      if @prepare_done
        @counter.print
        #if @connection_list.size > 2
        #  @connection_list.map{|v| Thread.new(v){|s| s.close }}.each{|t| t.join}
        #else
        #  @connection_list.map{|v| v.close }
        #end
        @logger.close
        if @logfile
          #mkdir_p "log"
          #cp @logfile, "log/"
          $stderr.puts "log file : "+@logfile
        end
      end
    end

  end # class Pwrake::Manager

end
