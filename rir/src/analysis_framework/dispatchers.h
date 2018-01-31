#pragma once

#include "framework.h"

namespace rir {

/** Instruction opcode based dispatcher. 
 
 The InstructionDispatcher is a Dispatcher specialization that implements a
 visitor pattern based on the instruction opcodes (because all the methods
 take the iterator as argument, the names of the methods differ).

 For cleaner use of the dispatcher in the analyses, the dispatcher is "split"
 between the dispatcher itself, which performs the dispatching on a Receiver. 
 This allows one class to implement different receivers at the same time for
 more complicated dispatchers. 
 */
class InstructionDispatcher : public Dispatcher {
  public:
    /** Receiver implements the visitor pattern over instructions.
     
      Contains a method for each instruction defined in RIR. Overriding the
      method defines the functionality for that instruction. 

      The receiver also provides the any() method to which all unhandled 
      bytecodes are sinked by their default implementations and the label()
      method that gets called when the label pseudoinstruction is encountered.
      */
    class Receiver {
      public:
        /** Default method for handling instructions. If instruction behavior
            is not refined in the implementing classes, it is sinked into this
            method. 
         */
        virtual void any(CodeEditor::Iterator ins) {}

        /** When a label is encouneterd in the code stream, this method is
            called. BIts default implementation also calls the any() method.
         */
        virtual void label(CodeEditor::Iterator ins) { any(ins); }

        /** Virtual methods for all RIR opcodes are generated using this
            macro from their definitions in the insns.h file. Default implementation
            of each opcode method passes control to the any() method. 
         */
#define DEF_INSTR(NAME, ...)                                                   \
    virtual void NAME(CodeEditor::Iterator ins) { any(ins); }
#include "ir/insns.h"

        /** Make sure that receivers are properly destroyed. 
         */
        virtual ~Receiver() {}
    };

    /** The dispatcher must be initialized with the appropriate receiver.

      NOTE: alternatively the dispatcher base class must be templated with the
      receiver, which would then become argument to doDispatch().
     */
    InstructionDispatcher(Receiver& receiver) : receiver_(receiver) {}

  private:
    /** Dispatches on the given instruction.

      If the instruction's opcode is not handler by the receiver, an assertion
      fails as the contract of instruction visitor is that it understand *all*
      instructions.

      The InstructionDispatcher always succeeds in the dispatch, not succeeding
      would mean an unknown instruction has been found. However, the failure to
      dispatch can still be triggered by the methods it dispatches to. 
     */
    void doDispatch(CodeEditor::Iterator ins) override {
        BC cur = *ins;
        switch (cur.bc) {
#define DEF_INSTR(NAME, ...)                                                   \
    case Opcode::NAME:                                                         \
        receiver_.NAME(ins);                                                   \
        break;
#include "ir/insns.h"
        case Opcode::label:
            receiver_.label(ins);
            break;
        default:
            assert(false and "Invalid instruction opcode");
        }
    }

    /** The receiver associated with the dispatcher. 
     */
    Receiver& receiver_;
};
}
