tclxqilla
=====

[XQilla] (http://xqilla.sourceforge.net/HomePage) is an XQuery and XPath 2 library
and command line utility written in C++,
implemented on top of the [Xerces-C] (http://xerces.apache.org/xerces-c/index.html) library.
It is made available under the terms of the Apache License v2.

XQilla's [XQC API] (http://xqc.sourceforge.net/) implements a standard C API for XQuery implementations,
defined in collaboration with the Zorba project.

tclxqilla is a Tcl extension to use XQilla XQC API to execute XQuery expression.

This extension is using [Tcl_LoadFile] (https://www.tcl.tk/man/tcl/TclLib/Load.htm) to
load XQilla library.

Before using this extension, please setup XQilla library path environment variable.
Below is an example on Linux platform to setup LD_LIBRARY_PATH environment variable (if need):

    export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH

Below is an example on Windows platform:

    set PATH=C:\Program Files\MonetDB\MonetDB5\bin;%PATH%


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

I do not test this extension on Windows platform.


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

