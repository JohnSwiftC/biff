#include "ir.h"
#include <fstream>

int main() {
  std::ofstream out{"out.txt"};
  IRFileWriter writer{out};

  writer.mov(1, 10);
  writer.mov(2, 10);

  // neq with a flag in 3. it should be 0,
  // should return pointer to addr 0
  writer.neq(2, 1, 3);

  writer.mov(4, 3);
  writer.mov(5, 12);

  // 6 should be 1, should also
  // return to addr 0
  writer.neq(4, 5, 6);

  // Division is also possible,
  // set a counter to 0, we loop on the
  // numerator, every iteration subtract the divisor and
  // add one to our answer counter. Then to keep addresses
  // coherant, mov 0 into the numerator address, add the result,
  // mov 0 into the result.

  return 0;
}
