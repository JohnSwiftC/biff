#ifndef TYPES_H
#define TYPES_H

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>

struct TypeException : std::exception {
  std::string message;

  TypeException(std::string message);

  virtual const char *what() const noexcept;
};

enum class TypeClass { BUILTIN, USERDEF };

struct Type {
  size_t size;
  TypeClass type_class;
  virtual ~Type() = default;
  Type(size_t size, TypeClass type_class);
  size_t get_size() const;
  TypeClass get_type_class() const;
};

using TypePtr = std::unique_ptr<Type>;

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
  std::unordered_map<std::string, Field> fields;

  StructType();

  void add_field(std::string name, Type *type);
};

#endif
