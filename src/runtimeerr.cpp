#include "runtimeerr.hpp"

namespace statpascal {

TRuntimeResult::TRuntimeResult (TCode result, std::size_t programCounter, const std::string &additionalMessage):
  result (result), programCounter (programCounter) {
    static const std::string msg [] = {
        "Program terminated without error",
        "Breakpoint",
        "Halt",
        "Invalid opcode encountered",
        "Division by zero",
        "Range check error",
        "Invalid argument",
        "Overflow error",
        "Stack overflow",
        "FFI error",
        "I/O error",
        "Out of memory"};
                                
    message = msg [static_cast<std::size_t> (result)];
    if (!additionalMessage.empty ())
        message.append (": " + additionalMessage);
}

TRuntimeResult::TCode TRuntimeResult::getCode () const {
    return result;
}

std::string TRuntimeResult::getMessage () const {
    return message;
}

std::size_t TRuntimeResult::getProgramCounter () const {
    return programCounter;
}


TRuntimeError::TRuntimeError (TRuntimeResult::TCode result, const std::string &additionalMessage):
  result (result), additionalMessage (additionalMessage) {
}

TRuntimeResult::TCode TRuntimeError::getRuntimeResultCode () const {
    return result;
}

std::string TRuntimeError::getAdditionalMessage () const {
    return additionalMessage;
}

}
