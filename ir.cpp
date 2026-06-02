#include "ir.h"
#include <cstddef>
#include <ostream>

void IRFileWriter::mov(size_t addr, unsigned char val) {
  move_to_address(0, addr);

  m_out << "[-]";

  for (auto i{0}; i < val; ++i) {
    m_out << '+';
  }

  move_to_address(addr, 0);

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

void IRFileWriter::add_const(size_t dest, unsigned char val) {
  move_to_address(0, dest);

  for (auto i{0}; i < val; ++i) {
    m_out << '+';
  }

  move_to_address(dest, 0);

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

void IRFileWriter::sub_const(size_t dest, unsigned char val) {
  move_to_address(0, dest);

  for (auto i{0}; i < val; ++i) {
    m_out << '-';
  }

  move_to_address(dest, 0);

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

void IRFileWriter::doif(size_t flag) {
  move_to_address(0, flag);

  m_out << '[';

  move_to_address(flag, 0);
}

void IRFileWriter::endif(size_t flag) {
  move_to_address(0, flag);
  m_out << "[-]]";
  move_to_address(flag, 0);
}

void IRFileWriter::mul(size_t dest, size_t src, size_t counter_one,
                       size_t counter_two) {

  // Sets moves the value of dest to counter_one,
  // and sets dest to 0
  mov(counter_one, 0);
  add(counter_one, dest);
  mov(dest, 0);

  mov(counter_two, 0);

  loop(counter_one);
  add(counter_two, src);
  loop(counter_two);

  add_const(dest, 1);

  endloop(counter_two);
  endloop(counter_one);
}

// div hopefully

// requires two bytes.
// this implementation is objectively bad,
// but it works for now. can be havily optimized
// to easily remove unneeded returns to addr 0
void IRFileWriter::neq(size_t a, size_t b, size_t flag) {
  mov(flag, 0);
  add(flag, b);

  sub(flag, a);

  doif(flag);

  add_const(flag + 1, a);

  endif(flag);

  doif(flag + 1);

  mov(flag, 0);
  add_const(flag, 1);

  endif(flag + 1);
}
