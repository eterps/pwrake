require "pathname"

module Pwrake

  class GfarmChannel < Channel

    @@local_mp = nil

=begin
    def initialize(host,remote_mp=nil)
      @remote_mp = Pathname.new(remote_mp)
      super(host)
      if @remote_mp
        system "mkdir -p #{@remote_mp}"
        system "gfarm2fs #{@remote_mp}"
        path = ENV['PATH'].gsub( /#{self.class.mountpoint}/, @remote_mp.to_s )
        system "export PATH=#{path}"
      end
      self
    end

    def close
      if @remote_mp
        system "cd"
        system "fusermount -u #{@remote_mp}"
        system "rmdir #{@remote_mp}"
      end
      super
      self
    end
=end

    def on_start
      @remote_mp = "%s%03d" % [mountpoint,@idx]
      path = ENV['PATH'].gsub( /#{self.class.mountpoint}/, @remote_mp.to_s )
      execute "mkdir -p #{@remote_mp}"
      execute "gfarm2fs #{@remote_mp}"
      execute "export PATH=#{path}"
      cd_cwd
    end

    def on_close
      execute "cd"
      execute "fusermount -u #{@remote_mp}"
      execute "rmdir #{@remote_mp}"
    end

    def cd_cwd
      dir = @remote_mp + Pathname.pwd.relative_path_from(GfarmChannel.mountpath)
      execute "cd #{dir}"
    end

    def cd(dir)
      path = Pathname.new(dir)
      if path.absolute?
        path = @remote_mp + path.relative_path_from(Pathname.new("/"))
      end
      execute "cd #{path}"
    end


    class << self

      def mountpoint=(d)
        @@local_mp = Pathname.new(d)
      end

      def gf_pwd
        "/" + Pathname.pwd.relative_path_from(mountpath).to_s
      end

      def gf_path(path)
        pn = Pathname(path)
        if pn.absolute?
          pn = pn.relative_path_from(mountpath)
        else
          pn = Pathname.pwd.relative_path_from(mountpath) + pn
        end
        "/" + pn.to_s
      end

      def local_to_gfarm_path(path)
        pn = Pathname(path)
        if pn.absolute?
          pn = pn.relative_path_from(mountpath)
        else
          pn = Pathname.pwd.relative_path_from(mountpath) + pn
        end
        "/" + pn.to_s
      end

      def mountpoint
        mountpath.to_s
      end

      def mountpath
        path = @@local_mp || ENV["GFARM_MOUNTPOINT"] || ENV["GFARM_MP"]
        if !path
          path = Pathname.new(Dir.pwd)
          while ! path.mountpoint?
            path = path.parent
          end
        end
        path
      end

      def gfwhere(list)
        result = {}
        parse_proc = proc{|x|
          if /(\S+):\n(.+)/m =~ x
    	f = $1
    	h = $2.split
    	result[f] = h if !h.empty?
          end
        }
        cmd = "gfwhere"
        list.each do |a|
          if a
            path = gf_path(a)
            if cmd.size + path.size + 1 > 20480 # 131000
              x = Kernel.backquote(cmd)
              x.split("\n\n").each(&parse_proc)
              cmd = "gfwhere"
            end
            cmd << " "
            cmd << path
          end
        end
        if cmd.size>8
          x = Kernel.backquote(cmd)
          x.split("\n\n").each(&parse_proc)
        end
        # puts "result.size=#{result.size}"
        result
      end

=begin
      def connect_list( hosts )
        # set_mountpoint
        tm = Pwrake.timer("connect_gfarm")
        th = []
        connections = []
        hosts.each_with_index{ |h,i|
          mnt_dir = "%s%03d" % [ mountpoint, i ]
          th << Thread.new(h,mnt_dir) {|x,y|
            if Rake.application.options.single_mp
              puts "# create Channel to #{x}"
              ssh = self.new(x)
            else
              puts "# create Channel to #{x}:#{y}"
              ssh = self.new(x,y)
            end
            ssh.cd_cwd
            connections << ssh
          }
        }
        th.each{|t| t.join}
        tm.finish
        connections
      end
=end

    end # class << self
    
  end # class GfarmChannel

end # module Pwrake

