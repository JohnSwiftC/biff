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

void IRFileWriter::insert_string(size_t dest, const std::string &str) {
  move_to_address(0, dest + 1);

  size_t end{dest + 1};
  for (const unsigned char &c : str) {
    for (auto i{0}; i < c; ++i) {
      m_out << '+';
    }

    m_out << '>';
    end++;
  }

  move_to_address(end, 0);
}

void IRFileWriter::ouz(size_t addr, const std::string &op) {
  move_to_address(0, addr);

  m_out << '>';
  m_out << '[';
  m_out << op;
  m_out << ">]<";
  m_out << "[<]";

  move_to_address(addr, 0);
}

void IRFileWriter::loop(size_t counter) {
  move_to_address(0, counter);

  m_out << '[';

  move_to_address(counter, 0);
}

void IRFileWriter::endloop(size_t counter) {
  move_to_address(0, counter);
  m_out << "-]";
  move_to_address(counter, 0);
}
