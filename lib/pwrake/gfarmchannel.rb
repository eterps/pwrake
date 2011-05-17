require "pathname"

module Pwrake

  class GfarmChannel < Channel

    @@local_mp = nil

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
        count = 0
        cmd = "gfwhere"
        parse_proc = proc{|x|
          if count==1
            result[cmd[8..-1]] = x.split
          else
            x.scan(/^([^\n]+):\n([^\n]*)$/m) do |file,hosts|
              h = hosts.split
              result[file] = h if !h.empty?
            end
          end
        }

        list.each do |a|
          if a
            path = GfarmSSH.gf_path(a)
            if cmd.size + path.size + 1 > 20480 # 131000
              x = Kernel.backquote(cmd)
              parse_proc.call(x)
              cmd = "gfwhere"
              count = 0
            end
            cmd << " "
            cmd << path
            count += 1
          end
        end
        if count > 0
          x = Kernel.backquote(cmd)
          parse_proc.call(x)
        end
        result
      end

    end # class << self

  end # class GfarmChannel

end # module Pwrake

