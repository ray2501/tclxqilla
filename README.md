tclxqilla
=====

[XQilla](http://xqilla.sourceforge.net/HomePage) is an XQuery and XPath 2 library
and command line utility written in C++,
implemented on top of the [Xerces-C](http://xerces.apache.org/xerces-c/index.html) library.
It is made available under the terms of the Apache License v2.

XQilla's [XQC API](http://xqc.sourceforge.net/) implements a standard C API for XQuery implementations,
defined in collaboration with the Zorba project.

tclxqilla is a Tcl extension to use XQilla XQC API to execute XQuery expression.

This extension is using [Tcl_LoadFile](https://www.tcl.tk/man/tcl/TclLib/Load.htm) to
load XQilla library.

Before using this extension, please setup XQilla library path environment variable.
Below is an example on Linux platform to setup LD_LIBRARY_PATH environment variable (if need):

    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH

Below is an example on Windows platform:

    set PATH=C:\xqilla\bin;%PATH%


Documentation
=====

 * [XQC API (C API)](http://xqilla.sourceforge.net/XQCAPI)


License
=====

Apache License, Version 2.0


Commands
=====

xqilla HANDLE ?-doc XML_DOC?   
HANDLE prepare xquery_string  
HANDLE close  
EXPR_HANDLE execute  
EXPR_HANDLE close  
RESULT_HANDLE next  
RESULT_HANDLE string_value  
RESULT_HANDLE integer_value  
RESULT_HANDLE double_value  
RESULT_HANDLE type_name  
RESULT_HANDLE node_name  
RESULT_HANDLE close


UNIX BUILD
=====

It is necessary to install XQilla development files.
Below is an example in Ubuntu 14.04:

    sudo apt-get install libxqilla6 libxqilla-dev

Building under most UNIX systems is easy, just run the configure script and
then run make. For more information about the build process, see the
tcl/unix/README file in the Tcl src dist. The following minimal example will
install the extension in the /opt/tcl directory.

    $ cd tclxqilla
    $ ./configure --prefix=/opt/tcl
    $ make
    $ make install

If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

    $ cd tclxqilla
    $ ./configure --with-tcl=/opt/activetcl/lib
    $ make
    $ make install


WINDOWS BUILD
=====

I use MSYS2 to test build on Windows platform.

Step 1. Download a source distribution of Xerces-C 3.1.3

Step 2. Build Xerces-C

Need check [this link](http://mail-archives.apache.org/mod_mbox/xerces-c-users/201111.mbox/%3Cboris.20111103155709@codesynthesis.com%3E),
so do below steps:

cd xerces-c-3.1.3/  
./configure --prefix=/c/xqilla --enable-shared --with-gnu-ld  
make libxerces_c_la_LDFLAGS="-release 3.1 -no-undefined"  

Then use `make install` to install.

Step 3. Download a source distribution of XQilla 2.3.3.

Step 4. Build XQilla

cd XQilla-2.3.3  
CFLAGS="-DXQILLA_APIS" ./configure --prefix=/c/xqilla --with-xerces=\`pwd\`/../xerces-c-3.1.3/ --enable-shared --with-gnu-ld  

Update include/xqilla/xqilla-xqc.h, we need export `createXQillaXQCImplementation` function on Windows platform.

    #if defined(WIN32) && !defined(__CYGWIN__)
    #    if defined(XQILLA_APIS)
    #      define XQC_API __declspec(dllexport)
    #    else
    #      define XQC_API __declspec(dllimport)
    #    endif
    #else
    #  define XQC_API 
    #endif

    /**
     * Creates an XQC_Implementation object that uses XQilla.
     */
    XQC_API XQC_Implementation *createXQillaXQCImplementation(int version);

And update tests/xmark/xmark.cpp, to fix build fail (_MSC_VER to WIN32):

    #ifdef WIN32
      tm_p = localtime(&tt);
    #else
      struct tm tm;
      tm_p = &tm;
      localtime_r(&tt, &tm);
    #endif
    
make

Then use `make install` to install.

I don't know how to handle dll file name correctly. So just copy the `libxqilla-3.dll` file to `libxqilla.dll`. And copy the libgcc_s_seh-1.dll and libstdc++-6.dll files to the same location.

Step 5. Build this extension

$ cd tclxqilla  
$ CFLAGS="-I/c/xqilla/include" ./configure --with-tcl=/c/tcl/lib  
$ make  
$ make install


Examples
=====

Below is a simple example:

    package require xqilla

    xqilla db
    set exprs [db prepare  "1 to 100"]
    set result [$exprs execute]

    while {[$result next]} {
        puts [$result string_value]
    }

    $result close
    $exprs close
    db close

This example parses a document and sets it as the context item.

    package require xqilla

    set xml_doc {<foo><bar baz='hello'/></foo>}
    xqilla db -doc $xml_doc
    set exprs [db prepare {foo/bar/@baz}]
    set result [$exprs execute]

    while {[$result next]} {
        puts [$result string_value]
    }

    $result close
    $exprs close
    db close

