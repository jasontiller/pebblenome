pebblenome
==========

A metronome application for the Pebble smartwatch.  It runs directly
on the Pebble.


==========
Configuration
==========

Modify startpebdev.sh and specify the root installation directory of
the pebble tools (Pebble SDK and ARM compiler).  Run startpebdev.sh,
which will launch a subshell with the Pebble SDK in the PATH and
ensure the appropriate symlinks are created.


==========
Build
==========

The first time you build, run

./waf configure

After that, to build, run

./waf build


==========
Installation
==========

In the ./build directory, run

python -m SimpleHTTPServer

Connect to the server from the smartphone to which your Pebble is
tethered.  Click on the '.pbw' file in your smartphone's browser.  If
you have the Pebble app installed correctly, the Pebble app will ask
you if you want to download this application to your watch.  Say 'OK'.
You should see Pebblenome being downloaded to your Pebble.

==========
Running
==========

From the main menu, select 'metronome'.  Have fun.
