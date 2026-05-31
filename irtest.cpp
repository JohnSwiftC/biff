#include "ir.h"
#include <fstream>
#include <iostream>
#include <ostream>

int main() {
  std::ofstream out{"out.txt"};
  IRFileWriter writer{out};

  writer.mov(1, 2);
  writer.mov(4, 10);

  writer.add(1, 4);

  writer.mov(2, 3);
  writer.sub(1, 2);

  writer.insert_string(10, "Hello, World!");
  writer.ouz(10, ".");

  return 0;
}
