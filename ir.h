#ifndef IR_H
#define IR_H

#include <cstddef>
#include <ostream>
class IRFileWriter {
private:
  std::ostream &m_out;

  void shift(size_t from, size_t to) {
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
  void add_const(size_t dest, unsigned char val);

  void sub(size_t dest, size_t src);
  void sub_const(size_t dest, unsigned char val);

  // Sets a null terminated string starting at dest
  // within the tape. Must start at a zero, because strings
  // must have a zero both at the beginning and end,
  // ensuring it works with ouz. therefore a biff string
  // has two bytes of padding
  void insert_string(size_t dest, const std::string &str);

  // Operate until zero. moves to addr, over one, executes
  // the bf operation (op) in a loop which moves forward
  // and performs the operation on the current cell until
  // it hits zero.
  //
  // It is crucial that the start of this is a 0,
  // otherwise the pointer cannot be reliabily returned to 0.
  void ouz(size_t addr, const std::string &op);

  // Starts a loop in control flow, with the value at counter
  // being used for a loop. must be ended with an end loop
  // see irtest for an example
  void loop(size_t counter);

  // End a loop in control flow. Must also
  // know where the loop counter is located
  void endloop(size_t counter);

  // Like loop, does runs the instructions
  // inbetween if the value at flag addr
  // is greater than 0. leaves the flag set to
  // after the statement executes
  void doif(size_t flag);

  // End an if statement
  void endif(size_t flag);

  // Mul must know where it can use two addresses to act as counters.
  // these counter are reset to zero after the end of the loop,
  // so this memory is completely released following the end of the operation
  void mul(size_t dest, size_t src, size_t counter_one, size_t counter_two);

  // Flips the value in a cell. Any non-zero value will become zero,
  // any zero cell will become one
  void flip(size_t addr);

  // NOT EQUAL between a and b.
  // sets flag to ONE if a and b are not equal
  // so flag can be used with doif
  //
  // ALSO, this REQUIRES two bytes, the
  // byte after flag MUST be zero
  void neq(size_t a, size_t b, size_t flag);

  // sets flag to ONE if a and b are equal
  void eq(size_t a, size_t b, size_t flag);

  // flag here requires 6 zero bytes
  // flag will be ONE if a < b, ZERO else
  void less(size_t a, size_t b, size_t flag);

  // Has the same 6 byte flag requirement as less,
  // it just an inverted less, flag will be ONE if a > b
  void greater(size_t a, size_t b, size_t flag);

  // This is very clever, and is taken from
  // an esolang wiki algorithm. Finds both the division
  // and modulus in a divion
  //
  // This REQUIRES 4 empty bytes to function correctly
  // starting at dump. these will be zeroed once the operation
  // is complete
  void div(size_t a, size_t b, size_t dump);

  // Same 4 byte zero requirement at dump as
  // div. uses the same algorithm as well,
  // a = a % b
  void mod(size_t a, size_t b, size_t dump);

  // Display a number as a string
  // this is expensive af
  //
  // Requires 8 zeroed bytes at dump. These are all
  // freed when the operation ends
  void itoa(size_t addr, size_t dump);
};

#endif
