$LOAD_PATH.push "../lib"
require 'pwrake'
load "common.rake"

# task definition for Graph-partitioning scheduling

ENV["INVOKE_MODE"] = "partition"
Rake::PwMultiTask.setup

Metis.command = "pmetis5.0pre2"

rimages_tbl = "#{INPUT_DIR}/rimages.tbl"


### pre-workflow for DAG

# select input files
file(rimages_tbl) {
  sh "(cd #{INPUT_DIR}; mImgtbl . /tmp/rimages_all.tbl)"
  sh "(cd #{INPUT_DIR}; mSubset /tmp/rimages_all.tbl region.hdr /tmp/rimages.tbl)"
  sh "mv /tmp/rimages*.tbl ."
}

# diffs.tbl is required for task definition below
file("diffs.tbl" => rimages_tbl) {
  sh "mOverlaps #{rimages_tbl} diffs.tbl"
}.invoke


PRJ_FITS = []


### projection

rimgs = Montage.read_image_tbl(rimages_tbl)
P_IMGTBL = []
P_TASK = []
rimgs.each do |x|
  basename = File.basename(x["fname"])
  input = INPUT_DIR+"/"+basename

  pimg = "p/"+basename
  file( pimg => [input,REGION_HDR] ) {|t|
    t.rsh "mProjectPP #{t.prerequisites[0]} #{t.name} #{REGION_HDR}"
    Montage.collect_imgtbl( t, P_IMGTBL )
  }.set_weight( 1000 )

  P_TASK << pimg
end

pw_multitask( "pimages.tbl" => P_TASK ) {
  Montage.put_imgtbl( P_IMGTBL, "p", "pimages.tbl" )
}


### dif & fit

DIFF_FITS=[]
FIT_TXT=[]
FIT_TBL=[]
diffs = Montage.read_overlap_tbl("diffs.tbl")
diffs.each do |c|
  p1 = "p/"+c[2]
  p2 = "p/"+c[3]

  DIFF_FITS << dif_fit = "d/"+c[4]
  file( dif_fit => [c[2],c[3],REGION_HDR,"pimages.tbl"] ) {|t|
    x1,x2,rh = t.prerequisites
    t.rsh "mDiff #{x1} #{x2} #{t.name} #{REGION_HDR}"
  }.set_weight( 1000 )

  FIT_TXT << fit_txt = c[4].sub(%r|diff\.(.*)\.fits$|,'d/fit.\1.txt')
  task( fit_txt => dif_fit ) { |t|
    r = t.rsh "mFitplane #{t.prerequisites[0]}"
    FIT_TBL.push([t.name,r])
  }.set_weight( 1000 )
end


### concatfit

pw_multitask( "fitfits.tbl" => FIT_TXT ) { |t|
  Montage.write_fitfits_tbl(FIT_TBL, "fitfits.tbl") 
}

### bg-model

file( "corrections.tbl" => ["fitfits.tbl", rimages_tbl] ) {
  sh "mBgModel #{rimages_tbl} fitfits.tbl corrections.tbl"
}


### background correction

CORRECT_FITS=[]
C_IMGTBL=[]
rimgs.each do |x|
  src = File.basename(x["fname"])
  dst = "c/" + src
  CORRECT_FITS << dst
  file( dst => ["p/#{src}","corrections.tbl",rimages_tbl] ) { |t|
    t.rsh "(cd p; mBackground -t #{src} ../#{dst} ../#{rimages_tbl} ../corrections.tbl)"
    Montage.collect_imgtbl( t, C_IMGTBL )
  }.set_weight( 1000 )
end

pw_multitask( "cimages.tbl" => CORRECT_FITS ) { |t|
  Montage.put_imgtbl( C_IMGTBL, "c", "cimages.tbl" )
}


### tile region

mkdir_p "t"
TILE_HDRS = []
TILE_TBL_TEST = []
NY.times do |iy|
  NX.times do |ix|

    TILE_HDRS << tile_hdr = "t/tile_#{ix}_#{iy}.hdr"
    file( tile_hdr => [REGION_HDR] ) {|t|
      opt="#{NX} #{NY} #{ix} #{iy} 50 50"
      t.rsh "mTileHdr #{t.prerequisites[0]} #{t.name} #{opt}"
    }

    TILE_TBL_TEST << tile_tbl_test = tile_hdr.ext('tbl_test')
    file( tile_tbl_test => [tile_hdr,rimages_tbl] ) {|t|
      t.rsh "mCoverageCheck #{rimages_tbl} #{t.name} -header #{t.prerequisites[0]}"
    }
  end
end

# tile_*.tbl_test are required for task definition below
pw_multitask( "pwrake_dag" => TILE_TBL_TEST ).invoke


TILE_TBL = []
TILE_FITS = []
SHRUNK_FITS = []
S_IMGTBL = []

NY.times do |iy|
  NX.times do |ix|
    tile_hdr = "t/tile_#{ix}_#{iy}.hdr"

    TILE_TBL << tile_tbl = tile_hdr.ext('tbl')
    file( tile_tbl => [tile_hdr,"cimages.tbl"] ) {|t|
      t.rsh "mCoverageCheck cimages.tbl #{t.name} -header #{t.prerequisites[0]}"
    }

    img_tbl = Montage.read_image_tbl(tile_tbl+"_test")
    imgs = img_tbl.map{|x| "c/"+File.basename(x["fname"])}

    TILE_FITS << tile_fits = tile_hdr.ext('fits')
    file( tile_fits => [tile_tbl,tile_hdr,"cimages.tbl"]+imgs ) {|t|
      a = t.prerequisites[0..1].join(" ")
      t.rsh "mAdd -e -p c #{a} #{t.name} "
    }.set_weight( 1000 )

    SHRUNK_FITS << shrunk_fits = tile_fits.sub(%r{t/(.*)\.fits}, 's/\1.fits')
    file( shrunk_fits => [tile_fits,tile_tbl] ) {|t|
      t.rsh "mShrink #{t.prerequisites[0]} #{t.name} #{SHRINK_FACTOR}"
      Montage.collect_imgtbl( t, S_IMGTBL )
    }.set_weight( 1000 )
  end
end


pw_multitask( "simages.tbl" => SHRUNK_FITS ) {
  Montage.put_imgtbl( S_IMGTBL, "s", "simages.tbl" )
}
