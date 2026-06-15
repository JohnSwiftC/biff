#include "compexcept.h"

CompilerException::CompilerException(int line_number, std::string message)
    : line_number{line_number},
      message{"Error on line " + std::to_string(line_number) + ": " +
              std::move(message)} {}

const char *CompilerException::what() const noexcept { return message.c_str(); }
