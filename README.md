liquibook
=========

Open source limit order book matching engine from [OCI](http://ociweb.com)

## Features
* Low-level components for order matching and aggregate depth tracking
* Memory-efficiency: minimal copying of data to internal structures
* Speed: between __1.2 million__ and __1.5 million__ inserts per second.  See full [performance history](liquibook/blob/master/PERFORMANCE.md).

## Flexibility
* Works with or without aggregate depth tracking
* Optional aggregate depth tracking to any number of levels (static) or BBO only
* Works with smart or regular pointers

## Works with Your Design
* Preserves your order model, requiring only trivial interface
* Preserves your identifiers for securities, accounts, exchanges, orders, fills
* Use your threading system (or be single-threaded)
* Use your synchronization method

Build Dependencies
------------------

* [MPC](http://www.ociweb.com/products/mpc) for cross-platform builds
* BOOST for unit testing only

Build Instructions
------------------

## Linux

Make sure the $BOOST_ROOT environment variable is set, then open a shell

<pre>
$ cd liquibook
$ . env.sh
$ mwc.pl -type make liquibook.mwc
$ make depend
$ make all
</pre>

## Windows

Make sure the %(BOOST_ROOT) environment variable is set, then open the Visual Studio Command Prompt of choice (this example is for Visual Studio 2010):
<pre>
> cd \liquibook
> winenv.bat
> mwc.pl -type vc10 liquibook.mwc
</pre>

Then in the same window, start Visual Studio from the command line, opening liquibook.sln (This example is the Visual Studio 2010 Express Edition):
<pre>
> "%VC10100COMNTOOLS%\..\IDE\VCExpress.exe liquibook.sln
</pre>

See other [build notes](liquibook/blob/master/BUILD_NOTES.md).
