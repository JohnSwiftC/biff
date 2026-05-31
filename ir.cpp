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
    // this ends up almost adding 2xsrc into addr 0,
    // and then as part of the refill step in normal addition,
    // moves the value in addr 0 back into src.

    add(0, src);

    return;
  }

  for (auto i{0}; i < src; ++i) {
    m_out << '>';
  }

  bool higher = src > dest;

  m_out << '[';

  size_t diff{};
  if (higher) {
    diff = src - dest;
  } else {
    diff = dest - src;
  }

  for (auto i{0}; i < diff; ++i) {
    if (higher) {
      m_out << '<';
    } else {
      m_out << '>';
    }
  }

  m_out << '+';

  for (auto i{0}; i < dest; ++i) {
    m_out << '<';
  }

  m_out << '+';

  for (auto i{0}; i < src; ++i) {
    m_out << '>';
  }

  m_out << '-';
  m_out << ']';

  for (auto i{0}; i < src; ++i) {
    m_out << '<';
  }

  m_out << '[';

  for (auto i{0}; i < src; ++i) {
    m_out << '>';
  }

  m_out << '+';

  for (auto i{0}; i < src; ++i) {
    m_out << '<';
  }

  m_out << '-';
  m_out << ']';

  m_out.flush();
}
