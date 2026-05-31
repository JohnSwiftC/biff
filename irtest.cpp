#include "ir.h"
#include <fstream>

int main() {
  std::ofstream out{"out.txt"};
  IRFileWriter writer{out};

  writer.mov(2, 3);
  writer.mov(3, 3);

  // finds 3 * 3
  writer.loop(3);
  writer.add(1, 2);
  writer.endloop(3);

  // executes, 1 is equal to 9 now
  writer.doif(1);
  writer.insert_string(5, "Biff IR!\n");
  writer.endif(1);

  // does not, 1 is now 0
  writer.doif(1);
  writer.insert_string(5, "Sad...");
  writer.endif(1);

  // At this point, 1 and 2 both hold the value three...
  // nested loops, prints 9 times!
  //
  // Also just realized, this is a great way
  // to multiply two numbers

  writer.loop(2);

  // This is a quirk of how loops work in BF,
  // when i nest loops, its asking the nested loop to run many times in our
  // loop, because a loop sets the value it works over to zero, in order to run
  // it again, we must also reset the counter it is using every time.
  //
  // There might be a clever way around this, but im not figuring it out now
  writer.mov(1, 3);
  writer.loop(1);

  writer.ouz(5, ".");

  writer.endloop(1);
  writer.endloop(2);

  writer.mov(1, 3);

  writer.mov(2, 4);

  return 0;
}
