class AffinityQueue

  @@find_max = true

  def initialize(hosts,num_jobs)
    num_hosts = hosts.size
    @hosts = hosts.uniq
    @count_jobs = 0
    @q = {}
    @finished = false
    @exist = ConditionVariable.new
    @mutex = Mutex.new
    @hosts.each{|h| @q[h]=[]}
  end

  def parse_hostname(host)
    /^([^.]*)\.?(.*)$/ =~ host
    [$1,$2]
  end

  def push(afin,job)
    if @finished
      raise "finished queue: cannot append job"
    end
    @mutex.synchronize{
      if !afin.kind_of? Array
	afin = [afin]
      end
      found = []
      j = [found,job]
      afin.each do |h|
	if q = @q[h]
	  found.push(h)
	  q.push(j)
	end
      end
      if found.empty?
	@q[@hosts[rand(@hosts.size)]].push(j)
      end
      @exist.signal
    }
  end

  def pop(host=nil)
    @mutex.synchronize{
      @count_jobs += 1

      q = @q[host]
      loop do
	if !q || q.empty?
	  if @@find_max
	    h_short, h_domain = parse_hostname(host)
	    max_host = nil
	    max_size = 0
	    @q.each{|h,a|
	      if !a.empty?
		h_s, h_d = parse_hostname(h)
		if h_domain == h_d
		  size = a.size
		else
		  size = a.size * 0.1
		end
		if size > max_size
		  max_host = h
		  max_size = size
		end
	      end
	    }
	    if max_host
	      j = @q[max_host].shift
	      j[0].each{|x| @q[x].delete_if{|x| j.equal? x}}
	      return j[1]
	    end
	  else
	    # old algorithm
	    @hosts.each do |h|
	      if !@q[h].empty?
		j = @q[h].shift
		j[0].each{|x| @q[x].delete_if{|x| j.equal? x}}
		return j[1]
	      end
	    end
	  end

	  # all empty
	  if @finished
	    @exist.signal
	    return false
	  else
	    @exist.wait(@mutex)
	  end
	else
	  j = q.shift
	  j[0].each{|x| @q[x].delete_if{|x| j.equal? x}}
	  return j[1]
	end
      end
    }
  end

  def finish
    @finished = true
    @exist.signal
  end
end


if __FILE__ == $0
  require "pp"
  require "thread"
  # test
  hosts = (0..10).map{|i| "host%02d"%i}
  q = AffinityQueue.new(hosts)

  q.push("host00", x=proc{puts "-- proc1 called --"})
  q.push("host09", x)
  q.push(%w[host01 host05], proc{puts "-- proc2 called --"})
  q.push(%w[host03 host04 host08], proc{puts "-- proc3 called --"})
  q.push(nil, proc{puts "-- proc4 called --"})
  pp q

  q.pop("host01").call
  pp q
  q.pop("host06").call
  pp q
  q.pop("host03").call
  pp q
  q.pop("hostx").call
  pp q
  q.pop.call
  pp q
end
