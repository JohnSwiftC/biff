#ifndef IR_H
#define IR_H

#include <cstddef>
#include <ostream>
class IRFileWriter {
private:
  std::ostream &m_out;

  void move_to_address(size_t from, size_t to) {
    bool higher = to > from;

    size_t diff{};
    if (higher) {
      diff = to - from;
    } else {
      diff = from - to;
    }

    for (auto i{0}; i < diff; ++i) {
      if (higher) {
        m_out << '>';
      } else {
        m_out << '<';
      }
    }
  }

public:
  IRFileWriter(std::ostream &out) : m_out{out} {}

  void mov(size_t addr, unsigned char val);
  void add(size_t dest, size_t src);
  void sub(size_t dest, size_t src);
};

#endif
