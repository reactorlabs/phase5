#pragma once
#include <sstream>

/** A convenience macro for concatenating strings and other value into string using the std::ostream formatters.

    Internally creates a stringstream, populates it with its arguments and returns the string value of the stream.
 */
#define STR(WHAT) static_cast<std::stringstream &&>(std::stringstream() << WHAT).str()

/** A simple exception class that implements the std::exception's protocol, but remembers a custom message.
 */
class Exception : public std::exception {
public:

    /** Creates the exception with given message.
     */
    Exception(std::string const & message):
        message_(message) {
    }

    /** Returns the message associated with the exception as per the std::exception protocol.
     */
    char const * what() const noexcept override {
        return message_.c_str();
    }

private:

    /** The message of the exception.
     */
    std::string message_;
};

/** Convenience macro for marking code that is not implemented. Throws the ::Exception class with appropriate message poiting to the origin of the exception.
 */
#define NOT_IMPLEMENTED throw ::Exception(STR("Not implemented section reached in file " << __FILE__ << ", line " << __LINE__))

/** Convenience macro for marking code that is unreachable. Throws the ::Exception class with appropriate message pointing to the code that was incorrectly marked as unreachable.
 */
#define UNREACHABLE throw ::Exception(STR("Unreachable section reached in file " << __FILE__ << ", line " << __LINE__))