#include "ir.h"

#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

std::string trim(const std::string &s) {
  const char *ws = " \t\r\n";
  size_t start = s.find_first_not_of(ws);
  if (start == std::string::npos) {
    return "";
  }
  size_t end = s.find_last_not_of(ws);
  return s.substr(start, end - start + 1);
}

std::string to_lower(std::string s) {
  for (char &c : s) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return s;
}

std::vector<std::string> split_args(const std::string &s) {
  std::vector<std::string> out;
  if (trim(s).empty()) {
    return out;
  }

  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, ',')) {
    out.push_back(trim(item));
  }
  return out;
}

std::pair<std::string, std::string> split_first(const std::string &s) {
  size_t comma = s.find(',');
  if (comma == std::string::npos) {
    return {trim(s), ""};
  }
  return {trim(s.substr(0, comma)), s.substr(comma + 1)};
}

size_t parse_addr(const std::string &s) {
  if (s.empty()) {
    throw std::runtime_error("expected an address but got nothing");
  }
  size_t consumed = 0;
  unsigned long v = std::stoul(s, &consumed);
  if (consumed != s.size()) {
    throw std::runtime_error("invalid address: '" + s + "'");
  }
  return static_cast<size_t>(v);
}

unsigned char parse_byte(const std::string &s) {
  if (s.empty()) {
    throw std::runtime_error("expected a byte value but got nothing");
  }
  size_t consumed = 0;
  unsigned long v = std::stoul(s, &consumed);
  if (consumed != s.size()) {
    throw std::runtime_error("invalid byte value: '" + s + "'");
  }
  return static_cast<unsigned char>(v & 0xFF);
}

std::string parse_string(const std::string &s) {
  std::string t = trim(s);
  if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
    std::string inner = t.substr(1, t.size() - 2);
    std::string result;
    for (size_t i = 0; i < inner.size(); ++i) {
      if (inner[i] == '\\' && i + 1 < inner.size()) {
        char n = inner[++i];
        switch (n) {
        case 'n':
          result += '\n';
          break;
        case 't':
          result += '\t';
          break;
        case 'r':
          result += '\r';
          break;
        case '0':
          result += '\0';
          break;
        case '\\':
          result += '\\';
          break;
        case '"':
          result += '"';
          break;
        default:
          result += n;
          break;
        }
      } else {
        result += inner[i];
      }
    }
    return result;
  }
  return t;
}

void require(const std::vector<std::string> &args, size_t n,
             const std::string &name) {
  if (args.size() != n) {
    throw std::runtime_error(name + " expects " + std::to_string(n) +
                             " argument(s) but got " +
                             std::to_string(args.size()));
  }
}

void compile_line(IRFileWriter &w, const std::string &raw) {
  std::string line = trim(raw);
  if (line.empty() || line.rfind("#", 0) == 0 || line.rfind("//", 0) == 0) {
    return;
  }

  size_t colon = line.find(':');
  std::string name =
      to_lower(trim(colon == std::string::npos ? line : line.substr(0, colon)));
  std::string rest = colon == std::string::npos ? "" : line.substr(colon + 1);

  std::vector<std::string> args = split_args(rest);

  if (name == "mov") {
    require(args, 2, "mov");
    w.mov(parse_addr(args[0]), parse_byte(args[1]));
  } else if (name == "add") {
    require(args, 2, "add");
    w.add(parse_addr(args[0]), parse_addr(args[1]));
  } else if (name == "add_const") {
    require(args, 2, "add_const");
    w.add_const(parse_addr(args[0]), parse_byte(args[1]));
  } else if (name == "sub") {
    require(args, 2, "sub");
    w.sub(parse_addr(args[0]), parse_addr(args[1]));
  } else if (name == "sub_const") {
    require(args, 2, "sub_const");
    w.sub_const(parse_addr(args[0]), parse_byte(args[1]));
  } else if (name == "insert_string") {
    // dest, <string> -- the string may itself contain commas.
    auto [addr, str] = split_first(rest);
    w.insert_string(parse_addr(addr), parse_string(str));
  } else if (name == "ouz") {
    // addr, <op> -- op is a raw brainfuck snippet.
    auto [addr, op] = split_first(rest);
    w.ouz(parse_addr(addr), parse_string(op));
  } else if (name == "loop") {
    require(args, 1, "loop");
    w.loop(parse_addr(args[0]));
  } else if (name == "endloop") {
    require(args, 1, "endloop");
    w.endloop(parse_addr(args[0]));
  } else if (name == "doif") {
    require(args, 1, "doif");
    w.doif(parse_addr(args[0]));
  } else if (name == "endif") {
    require(args, 1, "endif");
    w.endif(parse_addr(args[0]));
  } else if (name == "mul") {
    require(args, 4, "mul");
    w.mul(parse_addr(args[0]), parse_addr(args[1]), parse_addr(args[2]),
          parse_addr(args[3]));
  } else if (name == "neq") {
    require(args, 3, "neq");
    w.neq(parse_addr(args[0]), parse_addr(args[1]), parse_addr(args[2]));
  } else if (name == "less") {
    require(args, 3, "less");
    w.less(parse_addr(args[0]), parse_addr(args[1]), parse_addr(args[2]));
  } else if (name == "greater") {
    require(args, 3, "greater");
    w.greater(parse_addr(args[0]), parse_addr(args[1]), parse_addr(args[2]));
  } else if (name == "flip") {
    require(args, 1, "flip");
    w.flip(parse_addr(args[0]));
  } else if (name == "eq") {
    require(args, 3, "eq");
    w.eq(parse_addr(args[0]), parse_addr(args[1]), parse_addr(args[2]));
  } else if (name == "div") {
    require(args, 3, "div");
    w.div(parse_addr(args[0]), parse_addr(args[1]), parse_addr(args[2]));
  } else if (name == "mod") {
    require(args, 3, "mod");
    w.mod(parse_addr(args[0]), parse_addr(args[1]), parse_addr(args[2]));
  } else if (name == "itoa") {
    require(args, 2, "itoa");
    w.itoa(parse_addr(args[0]), parse_addr(args[1]));
  } else {
    throw std::runtime_error("unknown instruction: '" + name + "'");
  }
}

class Compiler {
public:
  explicit Compiler(IRFileWriter &w) : writer_(w) {}

  void feed(const std::string &raw) {
    std::string line = trim(raw);

    size_t colon = line.find(':');
    std::string head = to_lower(
        trim(colon == std::string::npos ? line : line.substr(0, colon)));
    std::string rest = colon == std::string::npos ? "" : line.substr(colon + 1);

    if (defining_) {
      if (head == "end") {
        defining_ = false;
        return;
      }
      if (head == "def") {
        throw std::runtime_error("nested 'def' is not allowed (inside '" +
                                 current_ + "')");
      }
      macros_[current_].push_back(raw);
      return;
    }

    if (head == "def") {
      std::string label = to_lower(trim(rest));
      if (label.empty()) {
        throw std::runtime_error("'def' requires a label name");
      }
      current_ = label;
      macros_[label].clear();
      defining_ = true;
      return;
    }

    if (head == "use") {
      expand(to_lower(trim(rest)));
      return;
    }

    compile_line(writer_, line);
  }

  // Call once after the last line to catch a definition that was never closed.
  void finish() const {
    if (defining_) {
      throw std::runtime_error("unterminated 'def: " + current_ +
                               "' (missing 'end')");
    }
  }

private:
  void expand(const std::string &label) {
    if (label.empty()) {
      throw std::runtime_error("'use' requires a label name");
    }
    auto it = macros_.find(label);
    if (it == macros_.end()) {
      throw std::runtime_error("unknown label: '" + label + "'");
    }
    if (!active_.insert(label).second) {
      throw std::runtime_error("recursive use of label: '" + label + "'");
    }
    // Copy the body so expanding it can't be invalidated by new definitions.
    std::vector<std::string> body = it->second;
    for (const std::string &body_line : body) {
      feed(body_line);
    }
    active_.erase(label);
  }

  IRFileWriter &writer_;
  std::map<std::string, std::vector<std::string>> macros_;
  std::set<std::string> active_;
  std::string current_;
  bool defining_ = false;
};

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

  IRFileWriter writer{*out};
  Compiler compiler{writer};

  std::string line;
  size_t lineno = 0;
  while (std::getline(*in, line)) {
    ++lineno;
    try {
      compiler.feed(line);
    } catch (const std::exception &e) {
      std::cerr << "error on line " << lineno << ": " << e.what() << "\n";
      return 1;
    }
  }

  try {
    compiler.finish();
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
