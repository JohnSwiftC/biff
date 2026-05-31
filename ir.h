#ifndef IR_H
#define IR_H

#include <cstddef>
#include <ostream>
class IRFileWriter {
private:
  std::ostream &m_out;

public:
  IRFileWriter(std::ostream &out) : m_out{out} {}

  void mov(size_t addr, unsigned char val);
  void add(size_t dest, size_t src);
  void sub(size_t dest, size_t src);
};

#endif
