#! /usr/bin/env ruby19
require 'webrick'
include WEBrick

require "socket"

QUEUE = Queue.new
$wf_running = false

PROGRESS = TCPServer.open(13391)

thread = Thread.start do
  socks = [PROGRESS]
  addr = PROGRESS.addr
  addr.shift
  printf("server is on %s\n", addr.join(":"))

  while true
    nsock = select(socks)
    next if nsock == nil
    for s in nsock[0]
      if s == PROGRESS
        socks.push(s.accept)
        print(s, " is accepted\n")
        $wf_running = true
      else
        if s.eof?
          print(s, " is gone\n")
          QUEUE.push "finish"
          s.close
          socks.delete(s)
          $wf_running = false
        else
          str = s.gets
          QUEUE.push str
        end
      end
    end
  end

end


SERVER = HTTPServer.new({ :DocumentRoot => '.',
                          :Port => 13390 })

class StartServlet < HTTPServlet::AbstractServlet
  def do_POST(req, res)
    if $wf_running
      res.body = 'workflow is already running'
    else
      if $wf_detach
        if $wf_detach.alive?
          res.body = 'workflow is exiting, please wait'
        else
          $wf_detach = nil
        end
      end
      if !$wf_detach
        nodelist = req.body
        system("rake -f Rakefile clobber")
        $wf_pid = spawn("./pwrake FS=gfarm NODES=nodes/#{nodelist}")
        res.body = 'workflow started'
      end
    end
    res['Content-Type'] = "text/plain"
  end
end

class StopServlet < HTTPServlet::AbstractServlet
  def do_POST(req, res)
    if $wf_running
      $wf_running = false
      if !$wf_detach
        puts "Process.kill(:KILL, $wf_pid)"
        Process.kill(:KILL, $wf_pid)
        $wf_detach = Process.detach($wf_pid)
      end
    end
    if $wf_detach
      if $wf_detach.alive?
        res.body = 'workflow is exiting'
      else
        $wf_detach = nil
        $wf_pid = nil
      end
    end
    if !$wf_detach
      res.body = 'workflow is not running'
    end
    res['Content-Type'] = "text/plain"
  end
end

class ProgressServlet < HTTPServlet::AbstractServlet
  def do_GET(req, res)
    begin
      img_count = 0
      item = nil
      list = []
      20.times do |i|
        break if i>0 and QUEUE.empty?
        timeout(20) do
          item = QUEUE.pop
        end
        list << item
        break if ["reload","finish"].include? item
        img_count += 1 if /^img /=~item
        #puts "i=#{i} img_count=#{img_count} item=#{item}"
        break if img_count>5
      end
      res.body = list.join("\n")
    rescue Timeout::Error
      p $!
      if $wf_running
        res.body = ""
      else
        res.body = "finish"
      end
    end
    res['Content-Type'] = "text/plain"
  end
end

class SourceServlet < HTTPServlet::AbstractServlet
  def do_GET(req, res)
    res.body = "
<html>
<head>
  <title>Rakefile</title>
</head>
<body>
#{`source-highlight -n -f html -s ruby -i Rakefile`}
</body>
</html>
"
    res['Content-Type'] = 'text/html'
  end
end


SERVER.mount("/start", StartServlet)
SERVER.mount("/stop",  StopServlet)
SERVER.mount("/progress", ProgressServlet)
SERVER.mount("/rakefile", SourceServlet)

SERVER.mount_proc( '/graph.svg' ){ |req,res|
  res['Content-Type'] = 'application/xml'
  res.body = File.open("/tmp/graph.svg"){|file|
    file.binmode
    file.read
  }
}

trap("INT"){ 
  SERVER.shutdown
}
SERVER.start
