#!/usr/bin/env ruby

begin
  require 'rake'
rescue LoadError
  require 'rubygems'
  require 'rake'
end

$LOAD_PATH.push "../../lib"
require "pwrake/load"

#cc = Pwrake::ConnectNode.connect({'localhost'=>3})
#cc = Pwrake::Channel.connect([nil]*3)
list = [nil,"les01"]*3
cc = Pwrake::Channel.connect(list)
#p cc


threads = (0...list.size).map do |i|
  Thread.new(i) do |j|
    cc[j].execute("hostname")
    cc[j].execute("sleep 1")
    cc[j].execute("hostname")
    puts cc[j].backquote("pgrep -l ruby")
    cc[j].execute("ls")
    #cc[j].close
    #cc[j].close
  end
end
j=1
sleep 0.5
#p "kill"
cc[j].kill("TERM")
#cc[j].execute("exit")

threads.each {|t| t.join}
Pwrake::Channel.close_all

