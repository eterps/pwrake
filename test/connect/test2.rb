#!/usr/bin/env ruby

begin
  require 'rake'
rescue LoadError
  require 'rubygems'
  require 'rake'
end

$LOAD_PATH.push "../../lib"
require "pwrake/load"

#list = [nil,"les01","les02","les03","les04"]*8
list = [nil,"les01"]*2
cc = Pwrake::Channel.connect(list)

threads = (0...list.size).map do |i|
  Thread.new(i) do |j|
    cc[j].execute("hostname")
  end
end

system "ps x"

threads.each {|t| t.join}
Pwrake::Channel.close_all
