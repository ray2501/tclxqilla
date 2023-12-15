# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded xqilla 0.1.2 \
	    [list load [file join $dir libtcl9xqilla0.1.2.so] [string totitle xqilla]]
} else {
    package ifneeded xqilla 0.1.2 \
	    [list load [file join $dir libxqilla0.1.2.so] [string totitle xqilla]]
}
