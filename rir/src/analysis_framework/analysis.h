#pragma once

#include "State.h"
#include "framework.h"

#include "R/Funtab.h"
namespace rir {

/** Base class for all analyses.

  In the future, this is where the API for analysis scheduling, retrieval and
  invalidation will live.
 */
class Analysis {
  public:
    virtual ~Analysis() {}

    /** Analyzes the given code.

      Internally sets the state and then calls the doAnalyze() virtual method
      that should implement the actual analysis.
     */
    void analyze(CodeEditor& code) {
        if (code_ != nullptr)
            invalidate();
        code_ = &code;
        doAnalyze();
    }

    /** Invalidates the data computed by the analysis.

      This is to be overwritten in child classes to proviode the appropriate
      cleanup functionality.
     */
    virtual void invalidate() { code_ = nullptr; }

    /** Returns true if the analysis contains valid data.
     */
    bool good() const { return code_ != nullptr; }

    /** Override this for pretty printing.
     */
    virtual void print() = 0;

  protected:
    /** Override this method to provide the actual analysis.

      This should mostly mean invoking the appropriate driver and collecting the
      state.
     */

    virtual void doAnalyze() = 0;

    CodeEditor* code_ = nullptr;
};

/** Forward driver analysis updates the basic analysis API with forward driver.

  ForwardAnalysis does not deal with visibility of any results, that is the
  domain of its descelndants.
 */
template <typename ASTATE>
class ForwardAnalysis : public Analysis {
  public:
    /** When analysis object is destroyed it invalidates its data.
     */
    ~ForwardAnalysis() { invalidate(); }

    /** Deletes all computed data and frees the resources.
     */
    void invalidate() override {
        Analysis::invalidate();
        delete currentState_;
        delete initialState_;
        delete finalState_;
        initialState_ = nullptr;
        currentState_ = nullptr;
        finalState_ = nullptr;
        for (auto& s : mergePoints_)
            delete s.second;
        mergePoints_.clear();
    }

    /** Prettyprinting.
     */
    void print() override {}

  protected:
    /** Default constructor for the forward analysis.
     */
    ForwardAnalysis() {}

    /** Returns the current state of the analysis.
     */
    ASTATE& current() { return *reinterpret_cast<ASTATE*>(currentState_); }

    /** Returns the dispatcher to drive the analysis.

        This method must be implemented in descendants.
     */
    virtual Dispatcher& dispatcher() = 0;

    /** Returns the initial state of the analysis.

        The default implementation returns an empty state, however it can be changed to return something more meaningful, such as setting function arguments to T, etc.
     */
    virtual ASTATE* initialState() { return new ASTATE(); }

    /** Prforms the analysis itself using the forrward pass algorithm.
     */
    void doAnalyze() override {
        initialState_ = initialState();
        currentState_ = initialState_->clone();
        q_.push_front(code_->begin());
        Dispatcher& d = dispatcher();
        while (not q_.empty()) {
            currentIns_ = q_.front();
            q_.pop_front();
            while (true) {
                // if current instruction is label, deal with state merging
                if (code_->isLabel(currentIns_)) {
                    // if state not stored, store copy of incoming
                    State*& stored = mergePoints_[currentIns_];
                    if (stored == nullptr) {
                        assert(currentState_ != nullptr);
                        stored = currentState_->clone();
                    } else {
                        // if incoming not present, copy stored
                        if (currentState_ == nullptr) {
                            currentState_ = stored->clone();
                            // otherwise merge incoming with stored
                        } else if (stored->mergeWith(currentState_)) {
                            delete currentState_;
                            currentState_ = stored->clone();
                            // and terminate current branch if there is no need
                            // to continue
                        } else {
                            delete currentState_;
                            currentState_ = nullptr;
                            break;
                        }
                    }
                }
                // user dispatch method
                d.dispatch(currentIns_);

                if (code_->isJmp(currentIns_)) {
                    auto l = code_->target(currentIns_);
                    if (shouldJump(l)) {
                        q_.push_front(l);
                    }
                    if (code_->isUncondJmp(currentIns_)) {
                        delete currentState_;
                        currentState_ = nullptr;
                        break;
                    }
                } else if (code_->isExitPoint(currentIns_)) {
                    if (finalState_ == nullptr) {
                        finalState_ = currentState_;
                    } else {
                        finalState_->mergeWith(currentState_);
                        delete currentState_;
                    }
                    currentState_ = nullptr;
                    break;
                }

                // move to next instruction
                ++currentIns_;
            }
        }
    }

    // initial state
    State* initialState_ = nullptr;
    // current state
    State* currentState_ = nullptr;
    // final state of the analysis
    State* finalState_ = nullptr;
    // current instruction the analysis operates on
    CodeEditor::Iterator currentIns_;
    // map of states at mergepoints in the analyzed code
    std::unordered_map<CodeEditor::Iterator, State*> mergePoints_;

  private:
    /** When faced with a jump, determinesd whether the jump should be taken, or not based on whether the state at the target mergepoint would change or not.
     */
    bool shouldJump(CodeEditor::Iterator label) {
        State*& stored = mergePoints_[label];
        if (stored == nullptr) {
            stored = currentState_->clone();
            return true;
        }
        return stored->mergeWith(currentState_);
    }

    /** Queue of mergepoints to analyze. Merge points are identified by their code instructions.
     */
    std::deque<CodeEditor::Iterator> q_;
};
/** Forward analysis which report the final state of the analysis.

    This is useful for simpler analyses where the forward pass is important to reach the fixpoint, but the actual fixpoint is valid over the entire code range. 
 */
template <typename ASTATE>
class ForwardAnalysisFinal : public ForwardAnalysis<ASTATE> {
    using ForwardAnalysis<ASTATE>::finalState_;

  public:
    /** Default destructor,. i.e. taken from ForwardAnalysis.
     */
    ~ForwardAnalysisFinal() = default;

    /** Returns the final state computed by the analysis.
     */
    ASTATE const& finalState() {
        return *reinterpret_cast<ASTATE*>(finalState_);
    }
};

/** Forward analysis with abstract state for each instruction.

    The analysis first reaches fixpoint and stores abstract states at all mergepoints. In the retrieval phase, abstract state after each instruction can be reconstructed in linear time from its nearest upstream mergepoint. The analysis is optimized for a linear retrieval of the instruction states within each basic block. 
 */
template <typename ASTATE>
class ForwardAnalysisIns : public ForwardAnalysisFinal<ASTATE> {
    using ForwardAnalysis<ASTATE>::currentIns_;
    using ForwardAnalysis<ASTATE>::currentState_;
    using ForwardAnalysis<ASTATE>::initialState_;
    using ForwardAnalysis<ASTATE>::dispatcher;
    using ForwardAnalysis<ASTATE>::mergePoints_;

  protected:
    using ForwardAnalysis<ASTATE>::current;
    using ForwardAnalysis<ASTATE>::code_;

  public:
    /** Destructor is the same as ForwardAnalysis.
     */
    ~ForwardAnalysisIns() = default;

    /** Returns abstract state after given instruction.
     */
    ASTATE const& operator[](CodeEditor::Iterator ins) {
        if (ins != currentIns_)
            seek(ins);
        return current();
    }
    
    /** Returns abstract state after given instruction expressed as editor's cursor.
     */
    ASTATE const& operator[](CodeEditor::Cursor cur) {
        auto ins = cur.asItr();
        if (ins != currentIns_)
            seek(ins);
        return current();
    }

  protected:

    /** Performs the analysis and initializes the cache for the retrieval phase.
     */
    void doAnalyze() override {
        ForwardAnalysis<ASTATE>::doAnalyze();
        initializeCache();
    }

    /** Initializes the cache for the retrieval state and sets the current state to initial.
     */
    void initializeCache() {
        delete currentState_;
        currentState_ = initialState_->clone();
        currentIns_ = code_->begin();
    }

    /** Advances the current retrieval state by the next instruction.
     */
    void advance() {
        dispatcher().dispatch(currentIns_);
        ++currentIns_;
        // if the cached instruction is label, dispose of the state and create a
        // copy of the fixpoint
        if (code_->isLabel(currentIns_)) {
            auto fixpoint = mergePoints_[currentIns_];
            // if we reach dead code there is no merge state available
            if (fixpoint) {
                delete currentState_;
                currentState_ = fixpoint->clone();
            }
        }
    }

    /** Seeks to any instruction.

        This is just a very ineffective proof of concept solution.
     */
    void seek(CodeEditor::Iterator ins) {
        while (currentIns_ != code_->end()) {
            if (currentIns_ == ins)
                return;
            if (code_->isExitPoint(currentIns_))
                break;
            // advance the state using dispatcher
            advance();
        }
        // if we haven't found it, let's start over
        initializeCache();
        while (currentIns_ != code_->end()) {
            if (currentIns_ == ins)
                return;
            advance();
        }
        assert(false and "not reachable");
    }
};


/** Backward analysis driver

    Implements the backwards analysis driver. 
 */
template <typename ASTATE>
class BackwardAnalysis : public Analysis {
  public:
    /** Destroying the analysis invalidates the analysis results.
     */
    ~BackwardAnalysis() { invalidate(); }

    /** Invalidates the results computed by the analysis and frees up the resources.
     */
    void invalidate() override {
        Analysis::invalidate();
        delete currentState_;
        delete initialState_;
        delete finalState_;
        currentState_ = nullptr;
        initialState_ = nullptr;
        finalState_ = nullptr;
        for (auto& s : mergePoints_)
            delete s.second;
        mergePoints_.clear();
        jumpOrigins_.clear();
    }

    /** prettyprinting.
     */
    void print() override {}

  protected:
    /** Analyses have default constructors.
     */
    BackwardAnalysis() {}

    /** Returns the current abstract state.
     */
    ASTATE& current() {
        return *reinterpret_cast<ASTATE*>(currentState_);
    }

    /** Returns the dispatcher used to drive the analysis.
     */
    virtual Dispatcher& dispatcher() = 0;

    /** Returns the initial state of the analysis.
     */
    virtual ASTATE* initialState() {
        return new ASTATE();
    }

    /** Peforms the backward analysis pass until a fixpoint is reached.
     */
    void doAnalyze() override {
        // First, forward pass to find jump origins for labels
        // and to add exit points to the working list
        for (auto it = code_->begin(); it != code_->end(); ++it) {
            if (code_->isJmp(it)) {
                jumpOrigins_[code_->target(it)].push_back(it);
            }
            if (code_->isExitPoint(it)) {
                q_.push_front(it);
            }
        }

        initialState_ = initialState();
        Dispatcher& d = dispatcher();

        while (not q_.empty()) {
            currentIns_ = q_.front();
            q_.pop_front();

            while (true) {
                if (code_->isExitPoint(currentIns_)) {
                    // need initial state
                    assert(currentState_ == nullptr);
                    currentState_ = initialState_->clone();
                } else if (isMergePoint(currentIns_)) {
                    // if state not stored, store copy of incoming
                    State*& stored = mergePoints_[currentIns_];  // first call to [] initializes to nullptr
                    if (!stored) {
                        assert(currentState_ != nullptr);
                        stored = currentState_->clone();
                    } else {
                        // if incoming not present, copy stored
                        if (currentState_ == nullptr) {
                            currentState_ = stored->clone();
                        // otherwise merge incoming with stored
                        } else if (stored->mergeWith(currentState_)) {
                            delete currentState_;
                            currentState_ = stored->clone();
                        // and terminate current branch if there is no need to continue
                        } else {
                            delete currentState_;
                            currentState_ = nullptr;
                            break;
                        }
                    }
                }

                // user dispatch method
                d.dispatch(currentIns_);

                if (code_->isEntryPoint(currentIns_)) {
                    // backward analysis ends here
                    if (finalState_ == nullptr) {
                        finalState_ = currentState_;
                    } else {
                        finalState_->mergeWith(currentState_);
                        delete currentState_;
                    }
                    currentState_ = nullptr;
                    break;
                }

                if (code_->isLabel(currentIns_)) {
                    // merge all origins for this label
                    for (auto origin : jumpOrigins_[currentIns_]) {
                        if (shouldFollowJumpFrom(origin)) {
                            q_.push_front(origin);
                        }
                    }
                    // if previous instruction doesn't lead here
                    if (code_->isExitPoint(currentIns_ - 1) ||
                            code_->next(currentIns_ - 1).count(currentIns_) == 0) {
                        delete currentState_;
                        currentState_ = nullptr;
                        break;
                    }
                }

                // move to the previous instruction
                --currentIns_;
            }
        }
    }

    /** For a given instruction, returns true if the instruction itself is a mergepoint.
     */
    bool isMergePoint(CodeEditor::Iterator ins) const {
        return code_->isJmp(ins);
    }

    /** Initial state.
     */
    State* initialState_ = nullptr;
    /** Current state.
     */
    State* currentState_ = nullptr;
    /** Final state of the analysis.
     */
    State* finalState_ = nullptr;
    /** Currently analyzed instruction.
     */
    CodeEditor::Iterator currentIns_;
    /** Mergepoints for the analysis at which the abstract states are kept.
     */
    std::unordered_map<CodeEditor::Iterator, State*> mergePoints_;

  private:
    /** Determines whether jumps should be followed based on merging the abstract states of the target mergepoint and current incomming state.
     */
    bool shouldFollowJumpFrom(CodeEditor::Iterator ins) {
        State*& stored = mergePoints_[ins];
        if (!stored) {
            stored = currentState_->clone();
            return true;
        }
        return stored->mergeWith(currentState_);
    }
    /** Backward pass jump origins.
     */
    std::unordered_map<CodeEditor::Iterator, std::vector<CodeEditor::Iterator>> jumpOrigins_;
    /** Queue for analyzed instructions.
     */
    std::deque<CodeEditor::Iterator> q_;
};

/** Backward analysis with final state.

    A simpler version of the backward analysis which only remembers the final state reached when a fixpoint was found. This final state must then be valid for all instructions that were analyzed. 
 */        
template<typename ASTATE>
class BackwardAnalysisFinal : public BackwardAnalysis<ASTATE> {
    using BackwardAnalysis<ASTATE>::finalState_;
public:
    ~BackwardAnalysisFinal() = default;

    /** Returns the final abstract state.
     */
    ASTATE const& finalState() {
        return *reinterpret_cast<ASTATE*>(finalState_);
    }
};

/** A more complex backward analysis that supports retrieval of abstract state before any instruction.

    All mergepoints are stored and states for intermediate instructions can be recomputed in linear time from the nearest mergepoint. The retrieval is optimized for a reversed linear access. 
 */
template<typename ASTATE>
class BackwardAnalysisIns : public BackwardAnalysisFinal<ASTATE> {
    using BackwardAnalysis<ASTATE>::currentIns_;
    using BackwardAnalysis<ASTATE>::currentState_;
    using BackwardAnalysis<ASTATE>::initialState_;
    using BackwardAnalysis<ASTATE>::dispatcher;
    using BackwardAnalysis<ASTATE>::mergePoints_;

protected:
    using BackwardAnalysis<ASTATE>::current;
    using BackwardAnalysis<ASTATE>::code_;

public:
    ~BackwardAnalysisIns() = default;

    /** Returns abstract state before given instruction.
     */
    ASTATE const& operator[](CodeEditor::Iterator ins) {
        if (ins != currentIns_)
            seek(ins);
        return current();
    }
    /** Returns abstract state before given instruction represented as code editor cursor.
     */
    ASTATE const& operator[](CodeEditor::Cursor cur) {
        auto ins = cur.asItr();
        if (ins != currentIns_)
            seek(ins);
        return current();
    }

protected:

    /** Performs the analysis and initalizes the retrieval cache.
     */
    void doAnalyze() override {
        BackwardAnalysis<ASTATE>::doAnalyze();
        initializeCache();
    }

    /** Analyzes the retrieval cache.
     */
    void initializeCache() {
        delete currentState_;  // possibly non-null?
        currentState_ = initialState_->clone();
        currentIns_ = code_->rbegin();
        dispatcher().dispatch(currentIns_);
    }

    /** Advances the currently retrieved state to previous instruction.
     */
    void advance() {
        --currentIns_;
        // if entry point for the analysis, initial state is needed
        if (code_->isExitPoint(currentIns_)) {
            delete currentState_;
            currentState_ = initialState_->clone();
        // here we have stored fixpoint, so use it
        } else if (this->isMergePoint(currentIns_)) {
            auto fixpoint = mergePoints_[currentIns_];
            if (fixpoint) {
                delete currentState_;
                currentState_ = fixpoint->clone();
            }
        }
        dispatcher().dispatch(currentIns_);
    }

    /** Seeks the currently retrieved state to the given instruction.

        Note that this is very inefficient proof of concept implementation. 
     */
    void seek(CodeEditor::Iterator ins) {
        while (currentIns_ != code_->rend()) {
            if (currentIns_ == ins)
                return;
            // advance the state using dispatcher
            advance();
        }
        // if we haven't found it, let's start over
        initializeCache();
        while (currentIns_ != code_->rend()) {
            if (currentIns_ == ins)
                return;
            advance();
        }
        assert(false and "not reachable");
    }
};


static inline bool isSafeBuiltin(int i) {
    // We have reason to believe that those would not run arbitrary
    // code and not mess with the env

    // builtins for `is.*` where primval(op) not within [100,200[
    // (those do not dispatch)
    if ((i >= 362 && i < 376) || (i >= 379 && i <= 389))
        return true;

    switch (i) {
    case 62:  // identical
    case 88:  // c
    case 91:  // class
    case 107: // vector
    case 397: // rep.int
    case 555: // inherits
        return true;
    }
    return false;
}
}
