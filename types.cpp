#include "types.h"

TypeException::TypeException(std::string message)
    : message{std::move(message)} {}

const char *TypeException::what() const noexcept { return message.c_str(); }

Type::Type(size_t size) : size{size} {}
size_t Type::get_size() { return size; }

IntegerType::IntegerType() : Type(1) {}
ArrayType::ArrayType(size_t array_length) : Type(array_length + 4) {}

StructType::StructType() : Type(0), fields{} {}
void StructType::add_field(std::string name, Type *type) {
  if (fields.count(name) != 0) {
    throw TypeException("field: " + name +
                        " has already been defined within the struct");
  }

  fields[name] = Field{size, type};
  size += type->get_size();
}
