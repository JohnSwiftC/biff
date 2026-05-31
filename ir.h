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
};

#endif
