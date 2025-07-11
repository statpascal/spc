/** runtimerr.hpp
*/

#pragma once

#include <string>

namespace statpascal {

class TRuntimeResult {
public:
    enum class TCode {Done, Breakpoint, Halted, InvalidOpcode, DivisionByZero, RangeCheck, InvalidArgument, Overflow, StackOverflow, FFIError, IOError, OutOfMemory};
    
    TRuntimeResult (TCode result, std::size_t programCounter, const std::string &additionalMessage);

    TCode getCode () const;
    std::string getMessage () const;
    std::size_t getProgramCounter () const;
private:        
    TCode result;
    std::size_t programCounter;
    std::string message;
};


class TRuntimeError {
public:
    TRuntimeError (TRuntimeResult::TCode result, const std::string &additionalMessage = std::string ());

    TRuntimeResult::TCode getRuntimeResultCode () const;
    std::string getAdditionalMessage () const;
    
private:
    const TRuntimeResult::TCode result;
    const std::string additionalMessage;
};

}
