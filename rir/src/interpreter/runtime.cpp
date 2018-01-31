#include "runtime.h"
#include "interp.h"

SEXP envSymbol;
SEXP callSymbol;
SEXP execName;
SEXP promExecName;
Context* globalContext_;

DispatchTable* isValidDispatchTableSEXP(SEXP wrapper) {
    return isValidDispatchTableObject(wrapper);
}

Function* isValidFunctionSEXP(SEXP wrapper) {
    return isValidFunctionObject(wrapper);
}

/** Checks if given closure should be executed using RIR.

  If the given closure is RIR function, returns its Function object, otherwise
  returns nullptr.
 */
Function* isValidClosureSEXP(SEXP closure) {
    if (TYPEOF(closure) != CLOSXP)
        return nullptr;
    DispatchTable* t = isValidDispatchTableObject(BODY(closure));
    if (t == nullptr)
        return nullptr;
    Function* f = t->first();
    if (f->magic != FUNCTION_MAGIC)
        return nullptr;
    return f;
}

Code* isValidPromiseSEXP(SEXP promise) {
    return isValidCodeObject(PRCODE(promise));
}

// for now, we will have to rewrite this when it goes to GNU-R proper

void printFunction(Function* f) {
    Rprintf("Function object (%p):\n", f);
    Rprintf("  Magic:           %x (hex)\n", f->magic);
    Rprintf("  Size:            %u\n", f->size);
    Rprintf("  Origin:          %p %s\n", f->origin(), f->origin() ? "" : "(unoptimized)");
    Rprintf("  Next:            %p\n", f->next());
    Rprintf("  Signature:       %p\n", f->signature());
    Rprintf("  Code objects:    %u\n", f->codeLength);
    Rprintf("  Fun code offset: %x (hex)\n", f->foffset);
    Rprintf("  Invoked:         %u\n", f->invocationCount);

    if (f->magic != FUNCTION_MAGIC)
        Rf_error("Wrong magic number -- not rir bytecode");

    // print respective code objects
    for (Code* c : *f)
        c->print();
}

// TODO change gnu-r to expect ptr and not bool and we can get rid of the
// wrapper
int isValidFunctionObject_int_wrapper(SEXP closure) {
    return isValidFunctionObject(closure) != nullptr;
}

int isValidCodeObject_int_wrapper(SEXP code) {
    return isValidCodeObject(code) != nullptr;
}

void initializeRuntime(CompilerCallback compiler, OptimizerCallback optimizer) {
    envSymbol = Rf_install("environment");
    callSymbol = Rf_install(".Call");
    execName = Rf_mkString("rir_executeWrapper");
    R_PreserveObject(execName);
    promExecName = Rf_mkString("rir_executePromiseWrapper");
    R_PreserveObject(promExecName);
    // initialize the global context
    globalContext_ = context_create(compiler, optimizer);
    registerExternalCode(rirEval_f, compiler, rirExpr);
}

Context* globalContext() { return globalContext_; }
