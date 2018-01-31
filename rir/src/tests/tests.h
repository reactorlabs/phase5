#pragma once

#include "helpers.h"


/** Defines a new test with given name belonging to the specified suite. The test creation should be immediately followed by the code of the test in curly braces.

    Internally this creates a class which inherits from the Test class with the overriden execute() function, whose body will be left for implementation when the macro stops.

    Tests are organized into suites. Each test must specify its suite and name. The name of the test must be unique within its suite.

    Note that this macro also instantiates the singleton variable of the test and registers it with the Test base class. 
 */
#define TEST(SUITE, NAME) \
class test_ ## SUITE ## _ ## NAME : public ::Test { \
public: \
    char const * suite() const override { return #SUITE; } \
    char const * name() const override { return #NAME; } \
private: \
    void execute() override; \
    static ::Test * singleton_; \
}; \
::Test * test_ ## SUITE ## _ ## NAME::singleton_ = Test::RegisterTestClass(new test_ ## SUITE ## _ ## NAME()); \
void test_ ## SUITE ## _ ## NAME::execute()

/** Checks that given expression is true, throws the TestError exception if it is not.
 */
#define CHECK(EXPR) \
if (!(EXPR)) throw ::TestError(__FILE__, __LINE__, STR("Assertion " << #EXPR << " failed."))

/** Checks that given code raises the given exception.
 */
#define CHECK_FAILS(ETYPE, ...) \
do { \
    bool finished = false; \
    try { \
		__VA_ARGS__; \
	    finished = true; \
    } catch (ETYPE const &) { \
    } catch (...) { \
        throw ::TestError(__FILE__, __LINE__, STR("Exception " << #ETYPE << " expected, but other thrown")); \
    } \
    if (finished) \
        throw ::TestError(__FILE__, __LINE__, STR("Exception " << #ETYPE << " expected, but none thrown")); \
} while (false)

/** Exception class for test related errors.

    On top of the std::exception class also remembers the location where the error was thrown for easier debugging.
*/
class TestError : public Exception {
public:
    /** Creates the test error with given message and sets the error location. The error location should be provided using the __FILE__ and __LINE__ preprocessor macros.
     */
    TestError(char const * file, unsigned line, std::string const & message) noexcept :
	Exception(message),
	file_(file),
	line_(line) {
    }

    /** Prints the test exception to an output stream. Prints the message and location at which the exception was thrown.
     */
    friend std::ostream & operator << (std::ostream & stream, TestError const & e);

private:

    /** File at which the eception has been thrown.
     */
    char const * file_;
    /** Line at which the exception has been thrown.
     */
    unsigned line_;
};

/** Base class for all tests.

    Provides the mechanisms to register new tests and to run & report all tests.
 */
class Test {
public:
    /** Runs all registered tests.

	Returns either EXIT_SUCCESS if all tests passed, or EXIT_FAILURE.
    */
    static int RunAll();

    /** Returns the name of the suite to which the test belongs.
     */
    virtual char const * suite() const = 0;

    /** Returns the name of the test. Test name has to be unique only within a particular suite. 
     */
    virtual char const * name() const = 0;

protected:

    /** Registers a new test so that it can be executed by the RunAll() method. 
     */
    static Test * RegisterTestClass(Test * test);

    /** The method which provides the actual implementation of the test. Must be provided is subclasses.
     */
    virtual void execute() = 0;
};