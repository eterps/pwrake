require "pathname"

module Pwrake

  class GfarmSSH < SSH

    @@local_mp = nil

    def initialize(host,remote_mp=nil)
      a = []
      @remote_mp = Pathname.new(remote_mp)
      #@gf_dir = self.class.gf_pwd
      a << super(host)
      if @remote_mp
        #@fs_dir = @remote_mp + @gf_dir
        #a << exec("cd")
        a << exec("mkdir -p #{@remote_mp}")
        a << exec("gfarm2fs #{@remote_mp}")
        #a << exec("cd #{@fs_dir}")
        #path = exec("echo $PATH")
        path = ENV['PATH'].gsub( /#{self.class.mountpoint}/, @remote_mp.to_s )
        exec("export PATH=#{path}")
      else
        #p [host,remote_mp,self.class.gf_pwd]
      end

      #puts a
      self
    end

    def close
      a = []
      if @remote_mp
        exec("cd")
        a << exec("fusermount -u #{@remote_mp}")
        a << exec("rmdir #{@remote_mp}")
      end
      a << super
      #puts a
      self
    end

    #def gf_pwd
    #  Utils.trim_prefix(@remote_mp||@@local_mp, fs_pwd)
    #end

    def cd_cwd
      dir = @remote_mp + Pathname.pwd.relative_path_from(GfarmSSH.mountpath)
      #puts("cd #{dir}")
      exec("cd #{dir}")
    end

    def cd(dir)
      path = Pathname.new(dir)
      if path.absolute?
        path = @remote_mp + path.relative_path_from(Pathname.new("/"))
      end
      exec("cd #{path}")
    end

    def self.mountpoint=(d)
      @@local_mp = Pathname.new(d)
    end

#    def self.mountpoint
#      @@local_mp.to_s
#    end

    def self.gf_pwd
      #Utils.trim_prefix(@@local_mp, Dir.getwd)
      "/" + Pathname.pwd.relative_path_from(GfarmSSH.mountpath).to_s
    end

    def self.gf_path(path)
      pn = Pathname(path)
      if pn.absolute?
        pn = pn.relative_path_from(GfarmSSH.mountpath)
      else
        pn = Pathname.pwd.relative_path_from(GfarmSSH.mountpath) + pn
      end
      "/" + pn.to_s
    end

    def self.local_to_gfarm_path(path)
      pn = Pathname(path)
      if pn.absolute?
        pn = pn.relative_path_from(GfarmSSH.mountpath)
      else
        pn = Pathname.pwd.relative_path_from(GfarmSSH.mountpath) + pn
      end
      "/" + pn.to_s
    end

    #def self.gf_path(path)
    #  if %r|^\/| =~ path
    #    Utils.trim_prefix(@@local_mp, path)
    #  else
    #    gf_pwd + '/' + path
    #  end
    #end

    #def self.set_mountpoint
    #  mountpoint = ENV["GFARM_MOUNTPOINT"] || ENV["GFARM_MP"]
    #  if !mountpoint
    #    path = Pathname.new(Dir.pwd)
    #    while ! path.mountpoint?
    #      path = path.parent
    #    end
    #    mountpoint = path
    #  end
    #  @@local_mp = mountpoint
    #end

    def self.mountpoint
      mountpath.to_s
    end

    def self.mountpath
      path = @@local_mp || ENV["GFARM_MOUNTPOINT"] || ENV["GFARM_MP"]
      if !path
        path = Pathname.new(Dir.pwd)
        while ! path.mountpoint?
          path = path.parent
        end
      end
      path
    end

    #module Utils
    #  def trim_prefix(t,d)
    #    t = t.to_s
    #    d = d.to_s
    #    if t != d[0,t.size]
    #      raise "directory '#{d}' is not under Gfarm mount point: '#{t}'"
    #    end
    #    d[t.size..-1]
    #  end
    #  module_function :trim_prefix
    #end

    def self.gfwhere(list)
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
          path = GfarmSSH.gf_path(a)
          if cmd.size + path.size + 1 > 20480 # 131000
            `#{cmd}`.split("\n\n").each(&parse_proc)
            cmd = "gfwhere"
          end
          cmd << " "
          cmd << path
        end
      end
      if cmd.size>8
        `#{cmd}`.split("\n\n").each(&parse_proc)
      end
      # puts "result.size=#{result.size}"
      result
    end


    def self.connect_list( hosts )
      # GfarmSSH.set_mountpoint
      th = []
      connections = []
      hosts.each_with_index{ |h,i|
        mnt_dir = "%s%03d" % [ GfarmSSH.mountpoint, i ]
        th << Thread.new(h,mnt_dir) {|x,y|
          if Rake.application.options.single_mp
            puts "# create SSH to #{x}"
            ssh = GfarmSSH.new(x)
          else
            puts "# create SSH to #{x}:#{y}"
            ssh = GfarmSSH.new(x,y)
          end
          ssh.cd_cwd
          connections << ssh
        }
      }
      th.each{|t| t.join}
      #system "ps x"
      connections
    end
  end

end


#GfarmSSH.mountpoint = "/tmp/tanaka"
#list = Dir.glob('*.fits')
#where = GfarmSSH.gfwhere(list)
#list.each{|x| w=where[GfarmSSH.gf_path(x)]; p [x,w] if !w}
