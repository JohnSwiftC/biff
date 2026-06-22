#include "types.h"

Type::Type(size_t size) : size{size} {}
size_t Type::get_size() { return size; }

IntegerType::IntegerType() : Type(1) {}
ArrayType::ArrayType(size_t array_length) : Type(array_length + 4) {}

StructType::StructType() : Type(0), field_offsets{} {}
