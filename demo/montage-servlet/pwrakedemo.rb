require 'socket'

if defined? Pwrake

  begin
    MUTEX = Mutex.new
    SOCKET = TCPSocket.open("localhost", 13391)
    END{SOCKET.close}
  rescue
  end

  Pwrake.manager.scheduler_class.module_eval do

    def on_task_start(t)
      return unless t.kind_of?(Rake::FileTask)
      fn = t.name
      if $g and tag = $g.tasknode_id[fn]
        MUTEX.synchronize do
          #puts "start #{tag} #{fn}"
          SOCKET.puts("start #{tag}") if defined? SOCKET
        end
      end
    end

    def on_task_end(t)
      return unless t.kind_of?(Rake::FileTask)
      fn = t.name
      if $g 
        if tag = $g.tasknode_id[fn]
          MUTEX.synchronize do
            #puts "end #{tag} #{fn}"
            SOCKET.puts("end #{tag}") if defined? SOCKET
          end
        end
        if tag = $g.filenode_id[fn]
          MUTEX.synchronize do
            #puts "end #{tag} #{fn}"
            SOCKET.puts("end #{tag}") if defined? SOCKET
          end
        end
      end
      #
      res = fn
      shrink = 10
      case fn
      when /\.fits$/
        if /\.t\.fits$/!~fn and fn!="shrunk.fits" and File.exist?(fn)
          if /\.s\.fits$/=~fn
            fits = fn
          else
            fits = fn.sub(/\.fits$/,'.s.fits')
            sh "mShrink #{fn} #{fits} #{shrink}"
          end
          jpg = fn.sub(/\.fits$/,'.jpg')
          sh "mJPEG -ct 0 -gray #{fits} -0.1s '99.8%' gaussian -out #{jpg}" do |ok2,st2| end
          res = "img #{fn}"
        end
      when /\.jpg$/
        if File.exist?(fn)
          res = "img #{fn}"
        end
      end
      MUTEX.synchronize do
        #puts res
        SOCKET.puts(res) if defined? SOCKET
      end
    end

  end # Pwrake.manager.scheduler_class
  
  TMPFILES=[]

  def show_graph
    $g = Pwrake::Graphviz.new
    $g.trace
    $g.write("/tmp/graph.dot")
    system 'dot -T svg -Eweight=1 -Gstylesheet="file.css" -o /tmp/graph0.svg /tmp/graph.dot'
    
    s = File.read("/tmp/graph0.svg")
    s.gsub!(/<g ([^>]*\bclass="node"[^>]*)><title>([^<]*)<\/title>(.*?)<\/g>/m) do |l|
      a = $1
      t = $2
      e = $3
      a.sub!(/\bid="([^"]*)"/, %[id="#{t}"])
      e.sub!(/<(polygon|ellipse)([^>]*) fill="none"([^>]*)>/, '<\1\2\3>')
      e.sub!(/<text /, '<text fill="black" ')

      if name = $g.node_name[t]
        task = Rake.application[name]
        if task.prerequisites.empty?
          c = "input"
        elsif task.already_invoked
          c = "done"
        else
          c = "yet"
        end
        e.sub!(/<(polygon|ellipse) /, "<\\1 class='#{c}' ")
      end
      %[<g onmouseover="nodeIn(evt);" onmouseout="nodeOut(evt);" #{a}><title>#{t}</title>#{e}</g>]
    end

    File.open("/tmp/graph.svg","w") do |w|
      w.write s
      w.close
    end

    MUTEX.synchronize do
      #puts "reload"
      SOCKET.puts("reload") if defined? SOCKET
    end
  end

else

  def show_graph
  end

end
