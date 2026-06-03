#include "ir.h"
#include <cstddef>
#include <ostream>
#include <stdexcept>

void IRFileWriter::mov(size_t addr, unsigned char val) {
  shift(0, addr);

  m_out << "[-]";

  for (auto i{0}; i < val; ++i) {
    m_out << '+';
  }

  shift(addr, 0);

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

  shift(0, src);

  m_out << '[';

  shift(src, dest);

  m_out << '+';

  shift(dest, 0);

  m_out << '+';

  shift(0, src);

  m_out << '-';
  m_out << ']';

  shift(src, 0);

  m_out << '[';

  shift(0, src);

  m_out << '+';

  shift(src, 0);

  m_out << '-';
  m_out << ']';

  m_out.flush();
}

void IRFileWriter::add_const(size_t dest, unsigned char val) {
  shift(0, dest);

  for (auto i{0}; i < val; ++i) {
    m_out << '+';
  }

  shift(dest, 0);

  m_out.flush();
}

void IRFileWriter::sub(size_t dest, size_t src) {

  if (src == dest) {

    mov(src, 0);

    return;
  }

  shift(0, src);

  m_out << '[';

  shift(src, dest);

  m_out << '-';

  shift(dest, 0);

  m_out << '+';

  shift(0, src);

  m_out << '-';
  m_out << ']';

  shift(src, 0);

  m_out << '[';

  shift(0, src);

  m_out << '+';

  shift(src, 0);

  m_out << '-';
  m_out << ']';

  m_out.flush();
}

void IRFileWriter::sub_const(size_t dest, unsigned char val) {
  shift(0, dest);

  for (auto i{0}; i < val; ++i) {
    m_out << '-';
  }

  shift(dest, 0);

  m_out.flush();
}

void IRFileWriter::insert_string(size_t dest, const std::string &str) {
  shift(0, dest + 1);

  size_t end{dest + 1};
  for (const unsigned char &c : str) {
    for (auto i{0}; i < c; ++i) {
      m_out << '+';
    }

    m_out << '>';
    end++;
  }

  shift(end, 0);
}

void IRFileWriter::ouz(size_t addr, const std::string &op) {
  shift(0, addr);

  m_out << '>';
  m_out << '[';
  m_out << op;
  m_out << ">]<";
  m_out << "[<]";

  shift(addr, 0);
}

void IRFileWriter::loop(size_t counter) {
  shift(0, counter);

  m_out << '[';

  shift(counter, 0);
}

void IRFileWriter::endloop(size_t counter) {
  shift(0, counter);
  m_out << "-]";
  shift(counter, 0);
}

void IRFileWriter::doif(size_t flag) {
  shift(0, flag);

  m_out << '[';

  shift(flag, 0);
}

void IRFileWriter::endif(size_t flag) {
  shift(0, flag);
  m_out << "[-]]";
  shift(flag, 0);
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

// Set addr 0 to one,
// if there is a value in addr,
// change addr 0 to zero, and set addr to 0
// because of the loop, nothing happens if
// addr is already 0
//
// then, if addr 0 has a value, add to addr. essentially flips
void IRFileWriter::flip(size_t addr) {

  m_out << '+';

  shift(0, addr);

  m_out << '[';

  shift(addr, 0);

  m_out << '-';

  shift(0, addr);

  m_out << "[-]]";

  shift(addr, 0);

  m_out << "[-";

  shift(0, addr);

  m_out << '+';

  shift(addr, 0);

  m_out << ']';
}

// requires two bytes.
// this implementation is objectively bad,
// but it works for now. can be havily optimized
// to easily remove unneeded returns to addr 0
void IRFileWriter::neq(size_t a, size_t b, size_t flag) {
  mov(flag, 0);
  add(flag, b);

  sub(flag, a);

  doif(flag);

  add_const(flag + 1, 1);

  endif(flag);

  doif(flag + 1);

  mov(flag, 0);
  add_const(flag, 1);

  endif(flag + 1);
}

// Literally just flips the output flag
// outputted by neq
void IRFileWriter::eq(size_t a, size_t b, size_t flag) {
  neq(a, b, flag);
  flip(flag);
}

void IRFileWriter::less(size_t a, size_t b, size_t flag) {
  size_t x = flag;
  size_t y = flag + 1;
  size_t temp0 = flag + 2;
  size_t temp1 = flag + 3; // This is actually three consecutive cells

  add(x, a);
  add(y, b);
  shift(0, temp0);
  m_out << "[-]";
  shift(temp0, temp1);
  m_out << "[-]>[-]+>[-]<<";
  shift(temp1, y);
  m_out << '[';
  shift(y, temp0);
  m_out << '+';
  shift(temp0, temp1);
  m_out << '+';
  shift(temp1, y);
  m_out << "-]";
  shift(y, temp0);
  m_out << '[';
  shift(temp0, y);
  m_out << '+';
  shift(y, temp0);
  m_out << "-]";
  shift(temp0, x);
  m_out << '[';
  shift(x, temp0);
  m_out << '+';
  shift(temp0, x);
  m_out << "-]+";
  shift(x, temp1);
  m_out << "[>-]>[<";
  shift(temp1, x);
  m_out << '-';
  shift(x, temp0);
  m_out << "[-]";
  shift(temp0, temp1);
  m_out << ">->]<+<";
  shift(temp1, temp0);
  m_out << '[';
  shift(temp0, temp1);
  m_out << "-[>-]>[<";
  shift(temp1, x);
  m_out << '-';
  shift(x, temp0);
  m_out << "[-]+";
  shift(temp0, temp1);
  m_out << ">->]<+<";
  shift(temp1, temp0);
  m_out << "-]";
  shift(temp0, 0);

  mov(y, 0);
  mov(temp0, 0);
  mov(temp1, 0);
  mov(temp1 + 1, 0);
  mov(temp1 + 2, 0);
}

void IRFileWriter::greater(size_t a, size_t b, size_t flag) {
  less(b, a, flag);
}

void IRFileWriter::div(size_t a, size_t b, size_t dump) {

  add(dump, a);
  add(dump + 1, b);

  shift(0, dump);

  m_out << "[->[->+>>]>[<<+>>[-<+>]>+>>]<<<<<]>[>>>]>[[-<+>]>+>>]<<<<<";

  shift(dump, 0);
  mov(a, 0);
  mov(dump + 1, 0);
  mov(dump + 2, 0);

  add(a, dump + 3);
  mov(dump + 3, 0);
}

void IRFileWriter::mod(size_t a, size_t b, size_t dump) {

  add(dump, a);
  add(dump + 1, b);

  shift(0, dump);

  m_out << "[->[->+>>]>[<<+>>[-<+>]>+>>]<<<<<]>[>>>]>[[-<+>]>+>>]<<<<<";

  shift(dump, 0);
  mov(a, 0);
  mov(dump + 1, 0);
  mov(dump + 3, 0);

  add(a, dump + 2);
  mov(dump + 2, 0);
}

void IRFileWriter::itoa(size_t addr, size_t dump) {
  add(dump, addr);

  shift(0, dump);

  // An algorithm from esolangs.org. This can also be written
  // with the current instruction set, but i believe this completes it faster
  m_out
      << ">[-]>[-]+>[-]+<[>[-<-<<[->+>+<<]>[-<+>]>>]++++++++++>[-]+>[-]>[-]>[-"
         "]<<<<<[->-[>+>>]>[[-<+>]+>+>>]<<<<<]>>-[-<<+>>]<[-]++++++++[-<+++++"
         "+>]>>[-<<+>>]<<]<[.[-]<]<";

  m_out << "[-]";

  shift(dump, 0);
}

void IRFileWriter::set_virtual_base(size_t base) {
  shift(m_base, base);
  m_base = base;
}
