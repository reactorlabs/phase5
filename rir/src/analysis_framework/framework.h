#pragma once

#include "ir/BC.h"
#include "ir/CodeEditor.h"

namespace rir {  

/** Dispatcher prototype.

  Dispatchers are used to determine the current situation and execute the
  appropriate code. Each dispatcher must override the doDispatch() method. If
  the dispatch fails, the dispatcher is supposed to call the fail() method, so
  that the dispatch() can return proper result.

  Note that failing the dispatch does not necessarily mean an error, it merely
  signifies that the dispatcher did not recognize the situation, or that the
  code dispatched to wanted to override the failure.

  The default dispatcher has no functionality of its own but to provide an 
  API base for real dispatchers implemented as its subclasses. These dispatchers
  can the be used in analysis & optimization drivers. 
 */
class Dispatcher {
  public:
    /** Dispatchers can be virtualized. */
    virtual ~Dispatcher() {}

    /** Dispatches on the given cursor, and returns true if the dispatch was
        successful, false if not. This is useful for chaining dispatchers, where
        fialure in one dispatcher does not necessarily mean a problem.  

        Note that it is not the job of the dispatcher to determine the next
        instruction and advance the cursor accordingly, but that of the analysis
        driver. The dispatcher should not modify the cursor. 
     */
    bool dispatch(CodeEditor::Iterator ins) {
      // reset the success tag
      success_ = true;
      doDispatch(ins);
      return success_;
    }

  protected:

    /** As a base class dispatcher should not be created directly. Instead,
        only subclasses of Dispatcher should be created by end users. 
     */
    Dispatcher() = default;

    /** Called by actual dispatchers when they want to notify the dispatching
      that it has failed.

      When this method is called from a dispatched routine, the dispatch()
      method will then return false.
     */
    void fail() { success_ = false; }

  private:
    /** Actual dispatch code.

      The actual implementation in subclasses must dispatch to proper method
      depending on the current instruction given by the cursor. If the 
      dispatcher fails, it should return and call the fail() method. Exceptions
      should not be used to drive the dispatching process.
     */
    virtual void doDispatch(CodeEditor::Iterator ins) = 0;

    /** The success tag. 
     */
    bool success_;
};
} // namespace rir
