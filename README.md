orion-mod
=========

Personal test field on tweaking Orion Viewer (mainly for Nook Simple Touch)

This project is a tweak on the Orion Viewer by  Michael Bogdanov:

http://code.google.com/p/orion-viewer/

Currently it is based on Orion-viewer 0.38.5

Current tweaks
--------------

* Pdf reflow using k2pdfopt at:

  http://www.willus.com/k2pdfopt/

  and its mod from kindlepdfviewer at:

  https://github.com/hwhw/kindlepdfviewer

* Minor fixes in key binding activity and light themes on Nook Simple Touch

How to build
------------
* Build the mupdf library
  cd mupdf-1.1-source/android  
  ndk-build -j4

* copy libmupdf.so, liblept.so, libtess.so to orion-viewer/orion_viewer/libs/armeabi/

* In orion-viewer/orion_viewer, configure your sdk_dir and run ant debug

