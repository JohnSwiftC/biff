#ifndef COMPEXCEPT_H
#define COMPEXCEPT_H

#include <exception>
#include <string>
class CompilerException : public std::exception {

private:
  int line_number;
  std::string message;

public:
  CompilerException(int line_number, std::string message);

  virtual const char *what() const noexcept;
};

#endif
