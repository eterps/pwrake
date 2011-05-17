require "tempfile"
require "pathname"

hosts=IO.readlines("nodes").map{|x|x.chomp}

files=%w[
test1
test2
test3
test4
test5
test6
test7
test8
test9
test10
]

# get mountpoint
path = Pathname.pwd
while !path.mountpoint?
  path = path.parent
end
puts "mountpoint: #{path}"
pwd = Pathname.pwd.relative_path_from(path)

files.each_with_index do |f,i|
  h = hosts[ i % hosts.size ]
  t = Tempfile.open("temp."+ENV["USER"])
  t.puts h
  t.flush
  c = "gfreg -v -h #{h} #{t.path} #{pwd}/#{f}.in"
  t.close
  puts c
  system c
end
