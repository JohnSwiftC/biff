#include <array>
#include <fstream>
#include <iostream>
#include <istream>
#include <stack>
#include <string>
#include <vector>

void run_bf(const std::string &program) {
  size_t len{program.size()};
  size_t ptr{};
  size_t max_ptr{};
  size_t curr_instr{};
  std::array<unsigned char, 10000> tape{};

  std::vector<size_t> match(len);
  {
    std::stack<size_t> opening_loops;
    for (size_t i{}; i < len; ++i) {
      if (program[i] == '[') {
        opening_loops.push(i);
      } else if (program[i] == ']') {
        size_t open = opening_loops.top();
        opening_loops.pop();
        match[open] = i;
        match[i] = open;
      }
    }
  }

  while (curr_instr != len) {
    switch (program[curr_instr]) {
    case '>': {
      ptr++;
      if (ptr > max_ptr) {
        max_ptr = ptr;
      }
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
      if (tape[ptr] == 0) {
        curr_instr = match[curr_instr] + 1;
      } else {
        curr_instr++;
      }
      break;
    }
    case '.': {
      std::cout << tape[ptr];
      curr_instr++;
      break;
    }
    case ']': {
      if (tape[ptr] != 0) {
        curr_instr = match[curr_instr] + 1;
      } else {
        curr_instr++;
      }
      break;
    }
    case ',': {
      // EOF leaves a zero so programs can loop until input runs out
      int c = std::cin.get();
      if (c == EOF) {
        tape[ptr] = 0;
      } else {
        tape[ptr] = static_cast<unsigned char>(c);
      }
      curr_instr++;
      break;
    }

    default: {
      curr_instr = len;
      break;
    }
    }
  }

  std::cerr << "\ncells used: " << max_ptr + 1 << "\n";
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
