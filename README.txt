Disable potentially conflicting versions of wine:

If one has previously installed some version of wine, try temporarily disabling it by;

sudo mv /opt /opt-tmp
mv ~/.wine ~/.wine-tmp

Install X11

Simply install the X11 application that ships with Mac OS X. Wacvst has only been tested with the Apple version of X11.

Build and install Wine

Uninstall any previously built from source versions of wine:

cd (previous wine build directory)
make uninstall

Download wine.1.1.33 from winehq.org

Build and install wine from the command line:

./configure
make depend
make (this will take a while)
sudo make install

Configure wine:

/usr/local/bin/wine winecfg. You'll get a warning dialog regarding the lack of the gecko package. You can safely ignore this.

Configure wacvst:

Copy your vst.dll to wacvst.vst/Contents/Resources

Edit wacvst.vst/Contents/Resources/config.plist.
  Change the plugin-name entry  to reflect name of Windows dll to be wrapped.
  Note that one should NOT include the .dll extension in the name.

  Change the wine-path entry to point to where wine is installed. If one installed
  wine in the default location, this entry does not need edited.

One may also need to copy support dlls to wine's windows\system32 directory.

If you want to wrap multiple vsts:

  Rename the vst bundle

  Change the uniqeID entry in Contents/Resources/config.plist. Insure that each individual vs. has a different uniqueID.

[The wacvst.vst folder has a compiled version, and the wacvst folder is the source code]

Install wacvst:

Move the configured wacvst  to ~/Library/Audio/Plug-Ins/VST/ or
/Library/Audio/Plug-Ins/VST

Troubleshooting:

Try starting the X11 server before attempting to load wacvst. Some hosts refuse to load a VST if it takes to long to start up. 

If wacvst shows up as a plugin within Live, but then Live reports it as being incompatible when dragged to a track, try dragging it a second time. 

From the command line, try loading wacvst via the misswatson host which can be downloaded from: MissWatson-v1.0-mac.zip

Start misswatson with the command line: MissWatson -editor -plugin=wacvst

Both misswatson and wacvst will send debugging information to the terminal window. If you are having problems, redirect this output to a file and email it to retroware99@gmail.com.

