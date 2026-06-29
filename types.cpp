#include "types.h"

TypeException::TypeException(std::string message)
    : message{std::move(message)} {}

const char *TypeException::what() const noexcept { return message.c_str(); }

Type::Type(size_t size, TypeClass type_class)
    : size{size}, type_class{type_class} {}
size_t Type::get_size() const { return size; }
TypeClass Type::get_type_class() const { return type_class; }

IntegerType::IntegerType() : Type(1, TypeClass::BUILTIN) {}

ArrayType::ArrayType(size_t array_length)
    : Type(array_length + 4, TypeClass::ARRAY) {}

StructType::StructType() : Type(0, TypeClass::USERDEF), fields{} {}
void StructType::add_field(std::string name, Type *type) {
  if (fields.count(name) != 0) {
    throw TypeException("field: " + name +
                        " has already been defined within the struct");
  }

  fields[name] = Field{size, type};
  size += type->get_size();
}
