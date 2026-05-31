#include "ir.h"
#include <cstddef>
#include <ostream>

void IRFileWriter::mov(size_t addr, unsigned char val) {
  for (auto i{0}; i < addr; ++i) {
    m_out << '>';
  }

  for (auto i{0}; i < val; ++i) {
    m_out << '+';
  }

  for (auto i{0}; i < addr; ++i) {
    m_out << '<';
  }

  m_out.flush();
}

void IRFileWriter::add(size_t dest, size_t src) {

  if (src == dest) {
    // actually a pretty cool trick.
    // because i add one to both the src and the dest,
    // this ends up adding 2xsrc into addr 0,
    // and then as part of the refill step in normal addition,
    // moves the value in addr 0 back into src.

    add(0, src);

    return;
  }

  move_to_address(0, src);

  m_out << '[';

  move_to_address(src, dest);

  m_out << '+';

  move_to_address(dest, 0);

  m_out << '+';

  move_to_address(0, src);

  m_out << '-';
  m_out << ']';

  move_to_address(src, 0);

  m_out << '[';

  move_to_address(0, src);

  m_out << '+';

  move_to_address(src, 0);

  m_out << '-';
  m_out << ']';

  m_out.flush();
}

void IRFileWriter::sub(size_t dest, size_t src) {

  if (src == dest) {

    mov(src, 0);

    return;
  }

  move_to_address(0, src);

  m_out << '[';

  move_to_address(src, dest);

  m_out << '-';

  move_to_address(dest, 0);

  m_out << '+';

  move_to_address(0, src);

  m_out << '-';
  m_out << ']';

  move_to_address(src, 0);

  m_out << '[';

  move_to_address(0, src);

  m_out << '+';

  move_to_address(src, 0);

  m_out << '-';
  m_out << ']';

  m_out.flush();
}
