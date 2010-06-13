class GfarmSSH < SSH

  OPEN_LIST={}

  def initialize(host,mnt_dir=nil)
    a = []
    @mnt_dir = mnt_dir
    @gf_dir = self.class.gf_pwd
    a << super(host)
    if @mnt_dir
      @fs_dir = @mnt_dir + @gf_dir
      a << exec("cd")
      a << exec("mkdir -p #{@mnt_dir}")
      a << exec("gfarm2fs #{@mnt_dir}")
      exec("cd #{@fs_dir}")
    else
      #p [host,mnt_dir,self.class.gf_pwd]
    end
    OPEN_LIST[__id__] = self
    #puts a
    self
  end

  def close
    a = []
    if @mnt_dir
      exec("cd")
      a << exec("fusermount -u #{@mnt_dir}")
      a << exec("rmdir #{@mnt_dir}")
    end
    a << super
    OPEN_LIST.delete(__id__)
    #puts a
    self
  end

  def gf_pwd
    Utils.trim_prefix(@mnt_dir||@@top_dir, fs_pwd)
  end

  def gf_cd(dir)
    exec("cd #{@mnt_dir+dir}")
  end

  def self.top_dir=(d)
    @@top_dir = d
  end

  def self.top_dir
    @@top_dir
  end

  def self.gf_pwd
    Utils.trim_prefix(@@top_dir, Dir::pwd)
  end

  def self.gf_path(path)
    if %r|^\/| =~ path
      Utils.trim_prefix(@@top_dir, path)
    else
      gf_pwd + '/' + path
    end
  end

  module Utils
    def trim_prefix(t,d)
      if t != d[0,t.size]
	raise "directory '#{d}' is not under Gfarm mount point: '#{t}'"
      end
      d[t.size..-1]
    end
    module_function :trim_prefix
  end

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
    puts "result.size=#{result.size}"
    result
  end


  def self.gfwhereold(list)
    cmd = "gfwhere"
    gfwhere_result = {}
	result = ""
	list.each do |a|
	  if a # = application[r].prerequisites[0]
	    #puts "---- GfarmSSH.gf_path(a)=#{GfarmSSH.gf_path(a)}"
	    path = GfarmSSH.gf_path(a)
	    if cmd.size + path.size + 1 > 20480 # 131000
	      result << `#{cmd}`
	      result << "\n"
	      cmd = "gfwhere"
	    end
	    cmd << " "
	    cmd << path
	  end
	end
	result << `#{cmd}`

	result.split("\n\n").each{|x|
	  if /(\S+):\n(.+)/m =~ x
	    f,h = $1,$2
	    h = h.split
	    if !h.empty?
	      gfwhere_result[f] = h
	    end
	  end
	}
    gfwhere_result
  end

  END{
    OPEN_LIST.map{|k,v| Thread.new(v){|s| s.close}}.each{|t| t.join}
  }
end



#GfarmSSH.top_dir = "/tmp/tanaka"
#list = Dir.glob('*.fits')
#where = GfarmSSH.gfwhere(list)
#list.each{|x| w=where[GfarmSSH.gf_path(x)]; p [x,w] if !w}
