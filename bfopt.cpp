#include <fstream>
#include <iostream>
#include <string>

// Optimizes my output
// rn is just removes the redundant < and > that
// my ir generates when it jumps to and from 0 between instructions

namespace {

constexpr int kLineWidth = 30;

void emit(std::ostream &out, char c, int &col) {
  out.put(c);
  if (c == '\n') {
    col = 0;
  } else if (++col >= kLineWidth) {
    out.put('\n');
    col = 0;
  }
}

void flush(std::ostream &out, long &net, int &col) {
  char c = net > 0 ? '>' : '<';
  for (long n = net > 0 ? net : -net; n > 0; --n) {
    emit(out, c, col);
  }
  net = 0;
}

} // namespace

int main(int argc, char **argv) {
  std::istream *in = &std::cin;
  std::ostream *out = &std::cout;
  std::ifstream infile;
  std::ofstream outfile;

  if (argc >= 2 && std::string(argv[1]) != "-") {
    infile.open(argv[1]);
    if (!infile) {
      std::cerr << "error: cannot open input file '" << argv[1] << "'\n";
      return 1;
    }
    in = &infile;
  }

  if (argc >= 3 && std::string(argv[2]) != "-") {
    outfile.open(argv[2]);
    if (!outfile) {
      std::cerr << "error: cannot open output file '" << argv[2] << "'\n";
      return 1;
    }
    out = &outfile;
  }

  long net = 0;
  int col = 0;
  char c;
  while (in->get(c)) {
    if (c == '>') {
      ++net;
    } else if (c == '<') {
      --net;
    } else {
      flush(*out, net, col);
      emit(*out, c, col);
    }
  }
  flush(*out, net, col);

  return 0;
}
