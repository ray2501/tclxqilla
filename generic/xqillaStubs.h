/*
 *-----------------------------------------------------------------------------
 *
 * xqillaStubs.h --
 *
 *	Stubs for procedures in xqillaStubDefs.txt
 *
 * Generated by genExtStubs.tcl: DO NOT EDIT
 * 2023-12-15 04:27:13Z
 *
 *-----------------------------------------------------------------------------
 */

typedef struct xqillaStubDefs {

    /* Functions from libraries: libxqilla */

    XQC_Implementation* (*createXQillaXQCImplementationPtr)(int version);
} xqillaStubDefs;
#define createXQillaXQCImplementation (xqillaStubs->createXQillaXQCImplementationPtr)
MODULE_SCOPE xqillaStubDefs *xqillaStubs;
