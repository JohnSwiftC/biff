#ifndef TYPES_H
#define TYPES_H

#include <cstddef>
#include <string>
#include <unordered_map>

struct Type {
  size_t size;

  virtual ~Type() = default;
  Type(size_t size);
  size_t get_size();
};

struct IntegerType : Type {
  IntegerType();
};
struct ArrayType : Type {
  ArrayType(size_t array_length);
};

struct StructType : Type {
  struct Field {
    size_t offset;
    Type *type;
  };
  std::unordered_map<std::string, Field> field_offsets;

  StructType();
};

#endif
