#include <tcl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xqilla/xqilla-xqc.h>
#include "xqillaStubs.h"

MODULE_SCOPE Tcl_LoadHandle XQillaInitStubs(Tcl_Interp* interp);

/*
 * Only the _Init function is exported.
 */
extern DLLEXPORT int Xqilla_Init(Tcl_Interp * interp);

typedef struct ThreadSpecificData {
  int initialized;                /* initialization flag */
  Tcl_HashTable *xqilla_hashtblPtr; /* per thread hash table. */
  int express_count;
  int result_count;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

TCL_DECLARE_MUTEX(myMutex);

/*
 * This struct is to record XQilla info,
 */
struct XQillaDATA {
    XQC_Implementation *impl;
    XQC_Sequence *doc;
    int has_doc;
    Tcl_Interp   *interp;
};

typedef struct XQillaDATA XQillaDATA;

struct XQillaExpression {
    XQC_Expression *expr;
    XQC_DynamicContext *context;
};

typedef struct XQillaExpression XQillaExpression;

struct XQillaResult {
    XQC_Sequence *result;
};

typedef struct XQillaResult XQillaResult;

static Tcl_Mutex xqillaMutex;
static int xqillaRefCount = 0;
static Tcl_LoadHandle xqillaLoadHandle = NULL;


void XQILLA_Thread_Exit(ClientData clientdata)
{
  /*
   * This extension records hash table info in ThreadSpecificData,
   * so we need delete it when we exit a thread.
   */
  ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
      Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

  if(tsdPtr->xqilla_hashtblPtr) {
    Tcl_DeleteHashTable(tsdPtr->xqilla_hashtblPtr);
    ckfree(tsdPtr->xqilla_hashtblPtr);
  }
}


static void DbDeleteCmd(void *db) {
  XQillaDATA *pDb = (XQillaDATA*)db;

  if(pDb->doc) {
	  pDb->doc->free(pDb->doc);
      pDb->doc = 0;
  }

  if(pDb->impl) {
	  pDb->impl->free(pDb->impl);
      pDb->impl = 0;
  }

  Tcl_Free((char*)pDb);
  pDb = 0;

  Tcl_MutexLock(&xqillaMutex);
  if (--xqillaRefCount == 0) {
      Tcl_FSUnloadFile(NULL, xqillaLoadHandle);
      xqillaLoadHandle = NULL;
  }
  Tcl_MutexUnlock(&xqillaMutex);
}


static int XQILLA_RESULT(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  XQillaResult *pResult;
  Tcl_HashEntry *hashEntryPtr;
  char *mHandle;
  int choice;
  int rc = TCL_OK;

  ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
      Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

  if (tsdPtr->initialized == 0) {
    tsdPtr->initialized = 1;
    tsdPtr->xqilla_hashtblPtr = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
    Tcl_InitHashTable(tsdPtr->xqilla_hashtblPtr, TCL_STRING_KEYS);
  }

  static const char *RSTS_strs[] = {
    "next",
    "string_value",
    "close",
    0
  };
  enum RSTS_enum {
    RSTS_NEXT,
    RSTS_STRING_VALUE,
    RSTS_CLOSE,
  };

  if( objc < 2 ){
      Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
      return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], RSTS_strs, "option", 0, &choice) ){
    return TCL_ERROR;
  }

  /*
   * Get back XQillaResult pointer
   */
  mHandle = Tcl_GetStringFromObj(objv[0], 0);
  hashEntryPtr = Tcl_FindHashEntry( tsdPtr->xqilla_hashtblPtr, mHandle );
  if( !hashEntryPtr ) {
    if( interp ) {
        Tcl_Obj *resultObj = Tcl_GetObjResult( interp );
        Tcl_AppendStringsToObj( resultObj, "invalid handle ", mHandle, (char *)NULL );
    }

    return TCL_ERROR;
  }

  pResult = Tcl_GetHashValue( hashEntryPtr );

  switch( (enum RSTS_enum)choice ){

    case RSTS_NEXT: {
      XQC_Error err;
      Tcl_Obj *return_obj;

      err = pResult->result->next(pResult->result);
      if(err == XQC_NO_ERROR) {
          return_obj = Tcl_NewBooleanObj(1);
      } else {
          return_obj = Tcl_NewBooleanObj(0);
      }

      Tcl_SetObjResult(interp, return_obj);
      break;
    }

    case RSTS_STRING_VALUE: {
      Tcl_Obj *return_obj;
      const char *value = NULL;

      pResult->result->string_value(pResult->result, &value);
      if(!value) {
          return_obj = Tcl_NewStringObj("", -1);
      } else {
          return_obj = Tcl_NewStringObj(value, -1);
      }

      Tcl_SetObjResult(interp, return_obj);
      break;
    }

    case RSTS_CLOSE: {
      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      /* Free used resource */
      if(pResult) {
          if(pResult->result) {
	         pResult->result->free(pResult->result);
             pResult->result = 0;
          }

          Tcl_Free((char*)pResult);
      }
      pResult = 0;

      Tcl_MutexLock(&myMutex);
      if( hashEntryPtr )  Tcl_DeleteHashEntry(hashEntryPtr);
      Tcl_MutexUnlock(&myMutex);

      break;
    }

  } /* End of the SWITCH statement */

  return rc;
}


static int XQILLA_EXPR(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  XQillaExpression *pExpress;
  Tcl_HashEntry *hashEntryPtr;
  char *mHandle;
  int choice;
  int rc = TCL_OK;

  ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
      Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

  if (tsdPtr->initialized == 0) {
      tsdPtr->initialized = 1;
      tsdPtr->xqilla_hashtblPtr = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
      Tcl_InitHashTable(tsdPtr->xqilla_hashtblPtr, TCL_STRING_KEYS);
  }

  static const char *EXPR_strs[] = {
    "execute",
    "close",
    0
  };
  enum EXPR_enum {
    EXPR_EXECUTE,
    EXPR_CLOSE,
  };

  if( objc < 2 ){
      Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
      return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], EXPR_strs, "option", 0, &choice) ){
      return TCL_ERROR;
  }

  /*
   * Get back XQillaExpression pointer
   */
  mHandle = Tcl_GetStringFromObj(objv[0], 0);
  hashEntryPtr = Tcl_FindHashEntry( tsdPtr->xqilla_hashtblPtr, mHandle );
  if( !hashEntryPtr ) {
    if( interp ) {
        Tcl_Obj *resultObj = Tcl_GetObjResult( interp );
        Tcl_AppendStringsToObj( resultObj, "invalid handle ", mHandle, (char *)NULL );
    }

    return TCL_ERROR;
  }

  pExpress = Tcl_GetHashValue( hashEntryPtr );

  switch( (enum EXPR_enum)choice ){

    case EXPR_EXECUTE: {
      XQillaResult *pResult;
      XQC_Error err;
      Tcl_HashEntry *newHashEntryPtr;
      char handleName[16 + TCL_INTEGER_SPACE];
      Tcl_Obj *pResultStr = NULL;
      int newvalue;

      pResult = (XQillaResult *)Tcl_Alloc( sizeof(*pResult) );
      if( pResult==0 ){
        Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
        return TCL_ERROR;
      }
      err = pExpress->expr->execute(pExpress->expr, pExpress->context, &(pResult->result));
      if(err != 0) {
         if(pResult) Tcl_Free((char *)pResult);

         return TCL_ERROR;
      }

      Tcl_MutexLock(&myMutex);
      sprintf( handleName, "xqilla_result_%d", tsdPtr->result_count++ );

      pResultStr = Tcl_NewStringObj( handleName, -1 );

      newHashEntryPtr = Tcl_CreateHashEntry(tsdPtr->xqilla_hashtblPtr, handleName, &newvalue);
      Tcl_SetHashValue(newHashEntryPtr, pResult);
      Tcl_MutexUnlock(&myMutex);

      Tcl_CreateObjCommand(interp, handleName, (Tcl_ObjCmdProc *) XQILLA_RESULT,
                 (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

      Tcl_SetObjResult(interp, pResultStr);

      break;
    }

    case EXPR_CLOSE: {
      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      /* Free used resource */
      if(pExpress) {
          if(pExpress->context) {
	         pExpress->context->free(pExpress->context);
             pExpress->context = 0;
          }

          if(pExpress->expr) {
	         pExpress->expr->free(pExpress->expr);
             pExpress->expr = 0;
          }

          Tcl_Free((char*)pExpress);
      }
      pExpress = 0;

      Tcl_MutexLock(&myMutex);
      if( hashEntryPtr )  Tcl_DeleteHashEntry(hashEntryPtr);
      Tcl_MutexUnlock(&myMutex);

      break;
    }

  } /* End of the SWITCH statement */

  return rc;
}


static int DbObjCmd(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  XQillaDATA *pDb = (XQillaDATA *) cd;
  int choice;
  int rc = TCL_OK;

  ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
      Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

  if (tsdPtr->initialized == 0) {
      tsdPtr->initialized = 1;
      tsdPtr->xqilla_hashtblPtr = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
      Tcl_InitHashTable(tsdPtr->xqilla_hashtblPtr, TCL_STRING_KEYS);
  }

  static const char *DB_strs[] = {
    "prepare",
    "close",
    0
  };

  enum DB_enum {
    DB_PREPARE,
    DB_CLOSE,
  };

  if( objc < 2 ){
      Tcl_WrongNumArgs(interp, 1, objv, "SUBCOMMAND ...");
      return TCL_ERROR;
  }

  if( Tcl_GetIndexFromObj(interp, objv[1], DB_strs, "option", 0, &choice) ){
      return TCL_ERROR;
  }

  switch( (enum DB_enum)choice ){

    case DB_PREPARE: {
      char *zQuery =  NULL;
      int len;
      XQillaExpression *pExpress;
      XQC_Error err;
      Tcl_HashEntry *newHashEntryPtr;
      char handleName[16 + TCL_INTEGER_SPACE];
      Tcl_Obj *pResultStr = NULL;
      int newvalue;

      if( objc == 3){
        zQuery = Tcl_GetStringFromObj(objv[2], &len);

        if( !zQuery || len < 1 ) {
          return TCL_ERROR;
        }
      }else{
        Tcl_WrongNumArgs(interp, 2, objv, "XQuery_String");
        return TCL_ERROR;
      }

      pExpress = (XQillaExpression *)Tcl_Alloc( sizeof(*pExpress) );
      if( pExpress==0 ){
        Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
        return TCL_ERROR;
      }

      err = pDb->impl->prepare(pDb->impl, zQuery, 0, &(pExpress->expr));
      if(err != 0) {
          Tcl_SetResult (interp, "prepare fail", NULL);
          Tcl_Free((char*)pExpress);
          pExpress = 0;

          return TCL_ERROR;
      }

      if(pDb->has_doc) {
        err = pExpress->expr->create_context(pExpress->expr, &(pExpress->context));
        if(err != 0) {
            pExpress->context = 0;

            Tcl_SetResult (interp, "create context fail", NULL);
            Tcl_Free((char*)pExpress);
            pExpress = 0;

            return TCL_ERROR;
        }

        pExpress->context->set_context_item(pExpress->context, pDb->doc);
      } else {
        pExpress->context = 0;
      }

      Tcl_MutexLock(&myMutex);
      sprintf( handleName, "xqilla_expr_%d", tsdPtr->express_count++ );

      pResultStr = Tcl_NewStringObj( handleName, -1 );

      newHashEntryPtr = Tcl_CreateHashEntry(tsdPtr->xqilla_hashtblPtr, handleName, &newvalue);
      Tcl_SetHashValue(newHashEntryPtr, pExpress);
      Tcl_MutexUnlock(&myMutex);

      Tcl_CreateObjCommand(interp, handleName, (Tcl_ObjCmdProc *) XQILLA_EXPR,
                 (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

      Tcl_SetObjResult(interp, pResultStr);

      break;
    }

    case DB_CLOSE: {
      Tcl_DeleteCommand(interp, Tcl_GetStringFromObj(objv[0], 0));

      break;
    }

  } /* End of the SWITCH statement */

  return rc;
}


static int XQILLA_MAIN(void *cd, Tcl_Interp *interp, int objc,Tcl_Obj *const*objv){
  XQillaDATA *p;
  const char *zArg;
  int i;
  int len;
  char *xml_doc = NULL;
  XQC_Error err;

  if( (objc&1)!=0 ){
      Tcl_WrongNumArgs(interp, 1, objv, "HANDLE ?-doc XML_DOC? ");
      return TCL_ERROR;
  }

  for(i=2; i+1<objc; i+=2){
    zArg = Tcl_GetStringFromObj(objv[i], 0);

    if( strcmp(zArg, "-doc")==0 ){
        xml_doc = Tcl_GetStringFromObj(objv[i + 1], &len);

        if(!xml_doc || len < 1) {
             return TCL_ERROR;
        }
    }else{
      Tcl_AppendResult(interp, "unknown option: ", zArg, (char*)0);
      return TCL_ERROR;
    }
  }

  Tcl_MutexLock(&xqillaMutex);
  if (xqillaRefCount == 0) {
	if ((xqillaLoadHandle = XQillaInitStubs(interp)) == NULL) {
	    Tcl_MutexUnlock(&xqillaMutex);
	    return TCL_ERROR;
	}
  }
  ++xqillaRefCount;
  Tcl_MutexUnlock(&xqillaMutex);

  p = (XQillaDATA *)Tcl_Alloc( sizeof(*p) );
  if( p==0 ){
    Tcl_SetResult(interp, (char *)"malloc failed", TCL_STATIC);
    return TCL_ERROR;
  }

  memset(p, 0, sizeof(*p));

  // XQilla specific way to create an XQC_Implementation struct
  p->impl = createXQillaXQCImplementation(XQC_VERSION_NUMBER);
  if(p->impl == 0) {
    if(p) Tcl_Free((char *)p);
    return TCL_ERROR;
  }

  if(xml_doc) {
     err = p->impl->parse_document(p->impl, xml_doc, &(p->doc));
     if(err != 0) {
        p->impl->free(p->impl);

        if(p) Tcl_Free((char *)p);
        return TCL_ERROR;
     }

     p->doc->next(p->doc);
     p->has_doc = 1;
  } else {
      p->doc = 0;
      p->has_doc = 0;
  }
  p->interp = interp;

  zArg = Tcl_GetStringFromObj(objv[1], 0);
  Tcl_CreateObjCommand(interp, zArg, DbObjCmd, (char*)p, DbDeleteCmd);

  return TCL_OK;
}


int Xqilla_Init(Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     *  Tcl_GetThreadData handles the auto-initialization of all data in
     *  the ThreadSpecificData to NULL at first time.
     */
    Tcl_MutexLock(&myMutex);
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
        Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->initialized == 0) {
        tsdPtr->initialized = 1;
        tsdPtr->xqilla_hashtblPtr = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(tsdPtr->xqilla_hashtblPtr, TCL_STRING_KEYS);

        tsdPtr->express_count = 0;
        tsdPtr->result_count = 0;
    }
    Tcl_MutexUnlock(&myMutex);

    /* Add a thread exit handler to delete hash table */
    Tcl_CreateThreadExitHandler(XQILLA_Thread_Exit, (ClientData)NULL);

    Tcl_CreateObjCommand(interp, "xqilla", (Tcl_ObjCmdProc *) XQILLA_MAIN,
            (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

    return TCL_OK;
}
