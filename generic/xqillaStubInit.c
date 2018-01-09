/*
 * xqillaStubInit.c --
 *
 *	Stubs tables for the foreign XQilla libraries so that
 *	Tcl extensions can use them without the linker's knowing about them.
 *
 * @CREATED@ 2018-01-09 14:15:51Z by genExtStubs.tcl from xqillaStubDefs.txt
 *
 *-----------------------------------------------------------------------------
 */

#include <tcl.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <xqilla/xqilla-xqc.h>
#include "xqillaStubs.h"

/*
 * Static data used in this file
 */

/*
 * ABI version numbers of the XQilla API that we can cope with.
 */

static const char *const xqillaSuffixes[] = {
    "", ".3", NULL
};

/*
 * Names of the libraries that might contain the XQilla API
 */

static const char *const xqillaStubLibNames[] = {
    /* @LIBNAMES@: DO NOT EDIT THESE NAMES */
    "libxqilla", NULL
    /* @END@ */
};

/*
 * Names of the functions that we need from XQilla
 */

static const char *const xqillaSymbolNames[] = {
    /* @SYMNAMES@: DO NOT EDIT THESE NAMES */
    "createXQillaXQCImplementation",
    NULL
    /* @END@ */
};

/*
 * Table containing pointers to the functions named above.
 */

static xqillaStubDefs xqillaStubsTable;
xqillaStubDefs* xqillaStubs = &xqillaStubsTable;

/*
 *-----------------------------------------------------------------------------
 *
 * XQillaInitStubs --
 *
 *	Initialize the Stubs table for the XQilla API
 *
 * Results:
 *	Returns the handle to the loaded XQilla client library, or NULL
 *	if the load is unsuccessful. Leaves an error message in the
 *	interpreter.
 *
 *-----------------------------------------------------------------------------
 */

MODULE_SCOPE Tcl_LoadHandle
XQillaInitStubs(Tcl_Interp* interp)
{
    int i, j;
    int status;			/* Status of Tcl library calls */
    Tcl_Obj* path;		/* Path name of a module to be loaded */
    Tcl_Obj* shlibext;		/* Extension to use for load modules */
    Tcl_LoadHandle handle = NULL;
				/* Handle to a load module */

    /* Determine the shared library extension */

    status = Tcl_EvalEx(interp, "::info sharedlibextension", -1,
			TCL_EVAL_GLOBAL);
    if (status != TCL_OK) return NULL;
    shlibext = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(shlibext);

    /* Walk the list of possible library names to find an XQilla client */

    status = TCL_ERROR;
    for (i = 0; status == TCL_ERROR && xqillaStubLibNames[i] != NULL; ++i) {
	for (j = 0; status == TCL_ERROR && xqillaSuffixes[j] != NULL; ++j) {
	    path = Tcl_NewStringObj(xqillaStubLibNames[i], -1);
	    Tcl_AppendObjToObj(path, shlibext);
	    Tcl_AppendToObj(path, xqillaSuffixes[j], -1);
	    Tcl_IncrRefCount(path);

	    /* Try to load a client library and resolve symbols within it. */

	    Tcl_ResetResult(interp);
	    status = Tcl_LoadFile(interp, path, xqillaSymbolNames, 0,
			      (void*)xqillaStubs, &handle);
	    Tcl_DecrRefCount(path);
	}
    }

    /* 
     * Either we've successfully loaded a library (status == TCL_OK), 
     * or we've run out of library names (in which case status==TCL_ERROR
     * and the error message reflects the last unsuccessful load attempt).
     */
    Tcl_DecrRefCount(shlibext);
    if (status != TCL_OK) {
	return NULL;
    }
    return handle;
}
