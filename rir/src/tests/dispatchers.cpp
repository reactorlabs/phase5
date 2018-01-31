#include "../ir/CodeStream.h"
#include "../ir/Compiler.h"
#include "../analysis_framework/framework.h"
#include "../analysis_framework/dispatchers.h"

#include "tests.h"

using namespace rir;

SEXP testFunction;

unsigned result;

class SimpleDispatcher : public Dispatcher {
private:
    void doDispatch(CodeEditor::Iterator ins) {
       ++result;
    };
}; 

class FailDispatcher : public Dispatcher {
private:
    void doDispatch(CodeEditor::Iterator ins) {
       fail();
    };
}; 

class DefaultsReceiver : public InstructionDispatcher::Receiver {
public:
    void any(CodeEditor::Iterator ins) override {
        ++result;
    }
};

class GuardFunReceiver : public InstructionDispatcher::Receiver {
public:
    void any(CodeEditor::Iterator ins) override {
        result = 2;
    }

    void guard_fun_(CodeEditor::Iterator ins) override {
        result = 1;
    }
};


TEST(Dispatcher, doDispatchIsCalled) {
    CodeEditor ce(testFunction);
    result = 0;
    SimpleDispatcher sd;
    CodeEditor::Iterator ins = ce.begin();
    sd.dispatch(ins);
    CHECK(result == 1);
}

TEST(Dispatcher, CursorIsNotAdvanced) {
    CodeEditor ce(testFunction);
    result = 0;
    SimpleDispatcher sd;
    CodeEditor::Iterator ins = ce.begin();
    sd.dispatch(ins);
    CHECK(result == 1);
    CHECK(ins == ce.begin());
}

TEST(Dispatcher, ReturnsTrue) {
    CodeEditor ce(testFunction);
    result = 0;
    SimpleDispatcher sd;
    CodeEditor::Iterator ins = ce.begin();
    CHECK(sd.dispatch(ins));
}

TEST(Dispatcher, FailedReturnsFalse) {
    CodeEditor ce(testFunction);
    result = 0;
    FailDispatcher sd;
    CodeEditor::Iterator ins = ce.begin();
    CHECK(! sd.dispatch(ins));
}

TEST(InstructionDispatcher, DoesNotFail) {
    CodeEditor ce(testFunction);
    InstructionDispatcher::Receiver r;
    InstructionDispatcher id(r);
    CodeEditor::Iterator ins = ce.begin();
    CHECK(id.dispatch(ins));
}

TEST(InstructionDispatcher, DoesNotAdvanceCursor) {
    CodeEditor ce(testFunction);
    InstructionDispatcher::Receiver r;
    InstructionDispatcher id(r);
    CodeEditor::Iterator ins = ce.begin();
    id.dispatch(ins);
    CHECK(ins == ce.begin());
}

TEST(InstructionDispatcher, DefaultsToAny) {
    CodeEditor ce(testFunction);
    DefaultsReceiver r;
    result = 0;
    InstructionDispatcher id(r);
    CodeEditor::Iterator ins = ce.begin();
    id.dispatch(ins);
    CHECK(result == 1);
}

TEST(InstructionDispatcher, Dispatches) {
    CodeEditor ce(testFunction);
    GuardFunReceiver r;
    result = 0;
    InstructionDispatcher id(r);
    CodeEditor::Iterator ins = ce.begin();
    id.dispatch(ins);
    CHECK(result == 1);
    
}

