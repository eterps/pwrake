require "thread"

class SSH
  CHARS='0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!"#=~{*}?_-^@[],./'
  TLEN=78

  OPEN_LIST={}

  def self.nice=(nice)
    @@nice=nice
  end

  @@nice="nice"

  def initialize(host)
    @lock = Mutex.new
    @terminator = ""
    TLEN.times{ @terminator << CHARS[rand(CHARS.length)] }
    @host = host
    @lock.synchronize do
      @io = IO.popen("ssh -x -T -q #{host}","r+")
      @io.puts("export PATH='#{ENV['PATH']}'")
      @io.puts("#{@@nice} sh")
      #@io.puts("cd #{Dir::pwd}")
      _get
    end
    OPEN_LIST[__id__] = self
  end

  attr_reader :host, :status

  def close
    @lock.synchronize do
      @io.puts("exit")
      @io.puts("exit")
      @io.close
    end
    OPEN_LIST.delete(__id__)
  end

  def exec(command)
    @lock.synchronize do
      @io.puts(command)
      _get
    end
  end

  def cd(dir)
    exec("cd #{dir}")
  end

  def cd_cwd
    exec("cd #{Dir.pwd}")
  end

  #def fs_pwd
  #  exec("pwd")[0]
  #end

  END{
    OPEN_LIST.map{|k,v| Thread.new(v){|s| s.close}}.each{|t| t.join}
  }

  private
  def _get
    @io.puts "\necho '#{@terminator}':$? "
    a = []
    while x = @io.gets
      x.chomp!
      if x[0,TLEN] == @terminator
	@status = Integer(x[TLEN+1..-1])
	break
      end
      puts x
      a << x
    end
    a
  end
end


