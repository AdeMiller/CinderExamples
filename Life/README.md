Conway's Game of Life
=====================

Building
--------

Download CINDER. Currently the samples are built with 0.8.5 for VS2012. This will include
compatible version of the Boost library.

Set the following environment variables to refer to the appropriate directories

* CINDER_INC - CINDER include path, , cinder_0.8.5_vc2012/include
* CINDER_LIB - CINDER library path, cinder_0.8.5_vc2012/lib
* BOOST_INC - Boost include path, cinder_0.8.5_vc2012/boost

This should build in Visual Studio 2012 and 2013.

Running
-------

Move around the map using the mouse and left button to drag the center of the map view.

Keys

* s - Start/stop the simulation.
* r - Reset the map. This will populate the map with randomly generated creatures from the creature library.
* up - Zoom in.
* down arrow - Zoom out.
* q - Quit.

Adding  to the Creature Library
-------------------------------

Maps are randomly generate by adding Life creatures from the creature_library folder. 
The .life files use # characters to represent populated cells and whitespace unpopulated ones.
The following is an example, the infinite_1 creature:

<pre>
          #
        # ##
        # #
      #
    # #
</pre>

More examples of Life creatures can be [found on Wikipedia][1].

[1]: http://en.wikipedia.org/wiki/Conway's_Game_of_Life "Wikipedia"
