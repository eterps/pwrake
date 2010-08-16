require "pwrake/log"

module FileUtils
  include Pwrake::Log
  alias rake_system_orig :rake_system

  def rake_system(*cmd)
    cmd_log = cmd.join(" ").inspect
    start_time = Time.now
    Pwrake.log("sh[start]:%s cmd=%s"%[Pwrake.time_str(start_time),cmd_log])

    if conn = Thread.current[:connection]
      res    = conn.system(*cmd)
      status = Rake::PseudoStatus.new(conn.status)
      status = Rake::PseudoStatus.new(1) if !res && status.nil?
    else
      res    = Rake::AltSystem.system(*cmd)
      status = $?
      status = Rake::PseudoStatus.new(1) if !res && status.nil?
    end

    end_time = Time.now
    elap_time = end_time - start_time
    Pwrake.log("sh[end]:%s elap=%.3f status=%s cmd=%s"%
        [Pwrake.time_str(end_time),elap_time,status.exitstatus,cmd_log])
    res
  end

end
