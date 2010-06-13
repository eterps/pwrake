require 'montage_tools'
load 'clean.rake'

task( :default => "mosaic.jpg" )
#task( :default => "shrunk.jpg" )

require "pathname"
path = Dir.glob("../pkg/metis-5.0pre2/build/*") 
path << Pathname.new("../pkg/Montage_v3.2_beta4/bin").realpath.to_s
ENV['PATH'] += ":" + path.join(":")
puts "ENV['PATH']=#{ENV['PATH']}"

INPUT_DIR = ENV["INPUT_DIR"] || "r"
puts "INPUT_DIR=#{INPUT_DIR}"

REGION_HDR = "#{INPUT_DIR}/region.hdr"
SHRUNK_HDR = "#{INPUT_DIR}/shrunken.hdr"

TILE_NX = ENV["TILE_NX"]
TILE_NY = ENV["TILE_NY"]
TILE_PIXEL=2000
SHRINK_FACTOR=10

open(REGION_HDR) do |f|
  while l=f.gets
    eval l if /(NAXIS[12])\s*=\s*(\d+)/ =~ l
  end
end

NX = [ (TILE_NX || NAXIS1/TILE_PIXEL).to_i, 4 ].max
NY = [ (TILE_NY || NAXIS2/TILE_PIXEL).to_i, 4 ].max


file( "mosaic.fits" => ["cimages.tbl", REGION_HDR] ) {|t|
  sh "mAdd -p c #{t.prerequisites.join(' ')} #{t.name}"
}

file( "mosaic.jpg" => "mosaic.fits" ) {|t|
  sh "mJPEG -ct 0 -gray #{t.prerequisites[0]} -1.5s 60s gaussian -out #{t.name}"
}

file( "shrunk.fits" => ["simages.tbl", SHRUNK_HDR] ) {|t|
  sh "mAdd -n -e -p s #{t.prerequisites.join(' ')} #{t.name}"
}

file( "shrunk.jpg" => ["shrunk.fits"] ) {|t|
  sh "mJPEG -ct 0 -gray #{t.prerequisites[0]} -1.5s 60s gaussian -out #{t.name}"
}

mkdir_p "p"
mkdir_p "d"
mkdir_p "c"
mkdir_p "t"
mkdir_p "s"
