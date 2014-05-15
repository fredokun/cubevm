CubeVm
======

The CubeVM interpeter for Pi-calculus programs


----

**Remark** : the CubeVM uses old scheduling and garbage collection schemes
and is thus not under active development anymore. It has been superseeded
 by the following implementations:

  - LuaPi: a (much) simpler and faster (!) library implementation based on
    a new scheduling scheme. It is implemented using Lua coroutines.

  - JavaPi: a parallel library  implementation based on Java threads.

  - Piccolo: the *real* successor of the CubeVM, not yet useable but under active
    development.

----

To compile the VM, go to the `lib/eXdbm` directory and type:

    ./configure
    make

Then go back to the root directory and type :

    make

To launch the cubevm frontend on an example program, type :

    ./copilot ./examples/display.pi

--
enjoy !(?)
--

