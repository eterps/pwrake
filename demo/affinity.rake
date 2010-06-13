$LOAD_PATH.push "../lib"
require 'pwrake'
load "common.rake"

# task definition for Affinity scheduling

ENV["INVOKE_MODE"] = "affinity"


### projection

SRC_FITS = FileList["#{INPUT_DIR}/*.fits"]

file( "pimages.tbl" ) {
  P_IMGTBL = []
  PRJ_FITS=[]
  SRC_FITS.each do |src|
    desc prj = src.sub( %r|^(.*?)([^/]+).fits|, 'p/\2.p.fits' )
    file( prj => [src,REGION_HDR] ) do |t|
      t.rsh "mProjectPP #{src} #{prj} #{REGION_HDR}"
      Montage.collect_imgtbl( t, P_IMGTBL )
    end
    PRJ_FITS << prj
  end
  pw_multitask( "ProjectPP" => PRJ_FITS ).invoke
  Montage.put_imgtbl( P_IMGTBL, "p", "pimages.tbl" )
}


### dif & fit

file( "diffs.tbl" => "pimages.tbl" ) {
  sh "mOverlaps pimages.tbl diffs.tbl"
}

file "fitfits.tbl" => "diffs.tbl" do |this_task|
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
      r = t.rsh "mFitplane #{t.name}"
      FIT_TBL << [c[0..1],r]
    }
  end
  pw_multitask( "Diff" => DIFF_FITS ).invoke
  Montage.write_fitfits_tbl(FIT_TBL, "fitfits.tbl") 
end


### background-model

file( "corrections.tbl" => ["fitfits.tbl", "pimages.tbl"] ) {
  sh "mBgModel pimages.tbl fitfits.tbl corrections.tbl"
}


### background correction

C_IMGTBL=[]

file( "cimages.tbl" => ["corrections.tbl","pimages.tbl"] ) {
  pfiles = FileList["p/*.p.fits"]
  cfiles = pfiles.map do |s|    
    src = s.sub(%r{p/(.*)\.p\.fits}, '\1.p.fits')
    desc dst = src.sub(%r{(.*)\.p\.fits}, 'c/\1.c.fits')
    file( dst => ["p/#{src}","corrections.tbl","pimages.tbl"] ) { |t|
      t.rsh "(cd p; mBackground -t #{src} ../#{dst} ../pimages.tbl ../corrections.tbl)"
      #t.rsh "mBackground -t p/#{src} #{dst} pimages.tbl corrections.tbl"
      Montage.collect_imgtbl( t, C_IMGTBL )
    }
    dst
  end
  pw_multitask( "Background" => cfiles ).invoke
  Montage.put_imgtbl( C_IMGTBL, "c", "cimages.tbl" )
}


### tile region

file( "simages.tbl" => "cimages.tbl" ) {
  TILE_HDRS = []
  TILE_TBL = []
  TILE_FITS = []
  SHRUNK_FITS = []
  S_IMGTBL=[]

  NY.times do |iy|
    NX.times do |ix|
      TILE_HDRS << tile_hdr = "t/tile_#{ix}_#{iy}.hdr"
      file( tile_hdr => ["cimages.tbl",REGION_HDR,"t"] ) {|t|
	opt="#{NX} #{NY} #{ix} #{iy} 50 50"
	t.rsh "mTileHdr #{t.prerequisites[1]} #{t.name} #{opt}"
      }

      TILE_TBL << tile_tbl = tile_hdr.ext('tbl')
      file( tile_tbl => [tile_hdr,"cimages.tbl"] ) {|t|
	t.rsh "mCoverageCheck cimages.tbl #{t.name} -header #{t.prerequisites[0]}"
      }

      TILE_FITS << tile_fits = tile_hdr.ext('t.fits')
      file( tile_fits => [tile_tbl,tile_hdr] ) {|t|
	a = t.prerequisites.join(" ")
	t.rsh "mAdd -e -p c #{a} #{t.name} "
      }
 
      SHRUNK_FITS << shrunk_fits = tile_fits.sub(%r{t/(.*)\.t\.fits}, 's/\1.s.fits')
      file( shrunk_fits => [tile_fits,tile_tbl] ) {|t|
	t.rsh "mShrink #{t.prerequisites[0]} #{t.name} #{SHRINK_FACTOR}"
        Montage.collect_imgtbl( t, S_IMGTBL )
      }
    end
  end

  pw_multitask( "TileHdr" => TILE_HDRS ).invoke
  pw_multitask( "CoverageCheck" => TILE_TBL ).invoke
  pw_multitask( "Add.tile" => TILE_FITS ).invoke
  pw_multitask( "Shink" => SHRUNK_FITS ).invoke
  Montage.put_imgtbl( S_IMGTBL, "s", "simages.tbl" )
}
