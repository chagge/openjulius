Setup
=====

This is fork of http://julius.sourceforge.jp/en_index.php (started with snapshot 2013.6.30 Julius rev.4.2.3)

Read the openjulius's `00readme.txt` or `Release.txt` for a general info and `COPYING` for the License.

Modification
============

This fork adds support for the confusion networks being send over sockets into the output module. 
Also, this fork allows to reuse a running Julius instance for another call session. To use this feature, you need 
to tell Julius to get ready for it by sending it USR1 signal::

 $ killall -USR1 julius
 

The rest of openjulius is unmodified.                                                                    

Installation
============

To install openjulius, just build the openjulius, e.g.

.. code-block:: bash

    $ git clone git@github.com:UFAL-DSG/openjulius.git
    $ cd openjulius
    $ ./configure
    $ make
    $ make install

All the best,
Filip Jurcicek    



