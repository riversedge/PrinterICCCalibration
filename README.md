# PrinterICCCalibration
Scripts that help with creating ICC printer calibrations using "poor man's" canner method

My old printer finally gave out and I bought an Epson ET-2800 in order to continue with some art work.   It was an admittedly cheap route but about the best thing I could pick up at the corner store.  While the overall quality of the prints was alright, the color matching was sometimes quite bad.  Not wanting to spend 3-5x as much on printer calibration software as this new printer cost, I set out to try and leverage the built in scanner to do color profile correction.

All-in-all it worked alright.  I didn't have any IT8.7/2 targets or anything handy and from a visible standpoint the scanner didn't do a terrible job, so I wanted to see how well I could run things through.   Results are definately better than the printer was doing before hand, so I'll call it success.

To install, add the scripts as a directory under the Argyll subdirectory and modify as needed.

To run: 
1) If you have targets, you can try to calibrate the printer with scannerICC.   If you don't, just copy the color managment ICC that the printer is using.   You need something to run the scripts.
2) Use createChart to make a custom chart.   Note the comments, you'll probably need to make one and do a calibration, then create a more refined one.   That refined one can be used from that point forward.
3) Print a chart and scan it back in.   Then run tiff2icc in order to create a custom profile.   

Your mileage may vary!  Mine seemed to turn out at least better than the printer defaults, but don't blame me if you end up with something weird.   Nobody *recommends* using a built-in flatbed scanner for doing printer color calibration, but if you want to give it a try, this is at least an instructional path on how to do that.

If you get a really weird profile (something that looks like an exploding star for instance) then it's probably a bad scan.   You may have to try multiple times to get a good result.  Some might never work, especially some textured papers where the scan might end up creating such significant issues to the block color that there's just no way to profile it.
