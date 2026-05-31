#include "ir.h"
#include <fstream>
#include <iostream>
#include <ostream>

int main() {
  std::ofstream out{"out.txt"};
  IRFileWriter writer{out};

  writer.mov(1, 2);
  writer.mov(4, 10);

  writer.add(4, 4);

  return 0;
}
