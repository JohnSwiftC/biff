#include <array>
#include <fstream>
#include <iostream>
#include <istream>
#include <stack>

void run_bf(const std::string &program) {
  size_t len{program.size()};
  size_t ptr{};
  size_t curr_instr{};
  std::array<unsigned char, 10000> tape;

  for (size_t i{}; i < 5000; ++i) {
    tape[i] = 0;
  }

  std::stack<size_t> opening_loops;

  while (curr_instr != len) {
    switch (program[curr_instr]) {
    case '>': {
      ptr++;
      curr_instr++;
      break;
    }
    case '<': {
      ptr--;
      curr_instr++;
      break;
    }
    case '+': {
      if (tape[ptr] == 255) {
        tape[ptr] = 0;
      } else {
        tape[ptr]++;
      }
      curr_instr++;
      break;
    }
    case '-': {
      if (tape[ptr] == 0) {
        tape[ptr] = 255;
      } else {
        tape[ptr]--;
      }
      curr_instr++;
      break;
    }
    case '[': {
      opening_loops.push(curr_instr);
      curr_instr++;
      break;
    }
    case '.': {
      std::cout << tape[ptr];
      curr_instr++;
      break;
    }
    case ']': {
      size_t jump = opening_loops.top();
      if (tape[ptr] != 0) {
        curr_instr = jump;
      } else {
        curr_instr++;
      }
      opening_loops.pop();
      break;
    }

    default: {
      return;
    }
    }
  }
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cerr << "error: requires input file\n";
    return 1;
  }

  std::istream *in;
  std::ifstream infile;

  infile.open(argv[1]);

  if (!infile) {
    std::cerr << "error: failed to open input file\n";
    return 1;
  }

  in = &infile;

  std::string program;
  // just gonna assume im using my compiled output lol
  std::getline(*in, program);

  run_bf(program);

  return 0;
}
