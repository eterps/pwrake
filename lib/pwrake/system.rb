require "pwrake/log"

module Kernel
  alias backquote :'`'
  module_function :backquote
end


module FileUtils
  include Pwrake::Log
  alias rake_system_orig :rake_system

  def rake_system(*cmd)
    cmd_log = cmd.join(" ").inspect
    tm = Pwrake.timer("sh",cmd_log)

    conn = Thread.current[:connection]
    if conn.kind_of?(Pwrake::Channel)
      res    = conn.execute(*cmd)
      status = Rake::PseudoStatus.new(conn.status)
      status = Rake::PseudoStatus.new(1) if !res && status.nil?
    else
      res    = system(*cmd)
      status = $?
      status = Rake::PseudoStatus.new(1) if !res && status.nil?
    end

    tm.finish("status=%s cmd=%s"%[status.exitstatus,cmd_log])
    res
  end
end

module PwrakeFileUtils
  module_function

  def `(cmd) #`
    cmd_log = cmd.inspect
    tm = Pwrake.timer("bq",cmd_log)

    conn = Thread.current[:connection]
    if conn.kind_of?(Pwrake::Channel)
      res    = conn.backquote(*cmd)
      status = Rake::PseudoStatus.new(conn.status)
      status = Rake::PseudoStatus.new(1) if !res && status.nil?
    else
      res    = Kernel.backquote(cmd)
      status = $?
      status = Rake::PseudoStatus.new(1) if !res && status.nil?
    end

    tm.finish("status=%s cmd=%s"%[status.exitstatus,cmd_log])
    res
  end
end

include PwrakeFileUtils

if !defined? FileUtils::PseudoStatus
  FileUtils::PseudoStatus = Rake::PseudoStatus
end
