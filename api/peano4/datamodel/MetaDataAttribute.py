# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
def _accessor_name(name):
    return name[:1].upper() + name[1:]


class Attribute:
    layout_priority = 100

    def __init__(self, name):
        self.name = name

    @property
    def accessor_name(self):
        return _accessor_name(self.name)

    @property
    def field_name(self):
        return "_" + self.name

    def get_enum_declaration(self):
        return None

    def get_method_declarations(self):
        raise NotImplementedError

    def get_method_definitions(self, qualified_class_name):
        raise NotImplementedError

    def get_member_declaration(self):
        raise NotImplementedError

    def get_to_string_expression(self):
        return f"get{self.accessor_name}()"


class _ScalarAttribute(Attribute):
    def __init__(self, name, initval=None):
        super().__init__(name)
        self.initial_value = self.default_value if initval is None else initval

    @property
    def cpp_type(self):
        return self.cpp_type_name

    def get_method_declarations(self):
        return [
            f"  INLINE_METHOD GPU_CALLABLE_METHOD {self.cpp_type} get{self.accessor_name}() const;",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD void set{self.accessor_name}({self.cpp_type} value);",
        ]

    def get_method_definitions(self, qualified_class_name):
        return f"""INLINE_METHOD GPU_CALLABLE_METHOD auto {qualified_class_name}::get{self.accessor_name}() const -> {self.cpp_type} {{
  return {self.field_name};
}}

INLINE_METHOD GPU_CALLABLE_METHOD void {qualified_class_name}::set{self.accessor_name}(const {self.cpp_type} value) {{
  {self.field_name} = value;
}}"""

    def get_member_declaration(self):
        return f"  {self.cpp_type} {self.field_name}{{{self.initial_value}}};"


class Boolean(_ScalarAttribute):
    layout_priority = 30
    cpp_type_name = "bool"
    default_value = "false"


class Double(_ScalarAttribute):
    layout_priority = 0
    cpp_type_name = "double"
    default_value = "0.0"


class Integer(_ScalarAttribute):
    layout_priority = 10
    cpp_type_name = "int"
    default_value = "0"


class _VectorAttribute(Attribute):
    def __init__(self, name, cardinality):
        super().__init__(name)
        self.cardinality = str(cardinality)

    @property
    def cpp_type(self):
        return f"tarch::la::Vector<{self.cardinality}, {self.element_type}>"

    def get_method_declarations(self):
        return [
            f"  INLINE_METHOD GPU_CALLABLE_METHOD {self.cpp_type} get{self.accessor_name}() const;",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD {self.element_type} get{self.accessor_name}(int index) const;",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD void set{self.accessor_name}(const {self.cpp_type}& value);",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD void set{self.accessor_name}({self.element_type} value);",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD void set{self.accessor_name}(int index, {self.element_type} value);",
        ]

    def get_method_definitions(self, qualified_class_name):
        return f"""INLINE_METHOD GPU_CALLABLE_METHOD {self.cpp_type} {qualified_class_name}::get{self.accessor_name}() const {{
  return {self.field_name};
}}

INLINE_METHOD GPU_CALLABLE_METHOD {self.element_type} {qualified_class_name}::get{self.accessor_name}(const int index) const {{
  return {self.field_name}(index);
}}

INLINE_METHOD GPU_CALLABLE_METHOD void {qualified_class_name}::set{self.accessor_name}(const {self.cpp_type}& value) {{
  {self.field_name} = value;
}}

INLINE_METHOD GPU_CALLABLE_METHOD void {qualified_class_name}::set{self.accessor_name}(const {self.element_type} value) {{
  {self.field_name} = value;
}}

INLINE_METHOD GPU_CALLABLE_METHOD void {qualified_class_name}::set{self.accessor_name}(
  const int index,
  const {self.element_type} value
) {{
  {self.field_name}(index) = value;
}}"""

    def get_member_declaration(self):
        return f"  {self.cpp_type} {self.field_name}{{}};"


class IntegerArray(_VectorAttribute):
    layout_priority = 10
    element_type = "int"


class DoubleArray(_VectorAttribute):
    layout_priority = 0
    element_type = "double"


class BooleanArray(Attribute):
    layout_priority = 30

    def __init__(self, name, cardinality):
        super().__init__(name)
        self.cardinality = str(cardinality)

    @property
    def storage_type(self):
        cardinality_aliases = {
            "DIMENSIONS": 3,
            "TWO_POWER_D": 8,
        }
        try:
            cardinality = int(self.cardinality)
        except ValueError:
            cardinality = cardinality_aliases.get(self.cardinality, 64)

        if cardinality <= 8:
            return "std::uint8_t"
        if cardinality <= 16:
            return "std::uint16_t"
        if cardinality <= 32:
            return "std::uint32_t"
        return "std::uint64_t"

    @property
    def mask_name(self):
        return f"BooleanArrayMask{self.accessor_name}"

    def get_method_declarations(self):
        bitset_type = f"std::bitset<{self.cardinality}>"
        return [
            f"  INLINE_METHOD {bitset_type} get{self.accessor_name}() const;",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD bool get{self.accessor_name}(int index) const;",
            f"  INLINE_METHOD void set{self.accessor_name}(const {bitset_type}& value);",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD void set{self.accessor_name}(bool value);",
            f"  INLINE_METHOD GPU_CALLABLE_METHOD void set{self.accessor_name}(int index, bool value);",
        ]

    def get_method_definitions(self, qualified_class_name):
        bitset_type = f"std::bitset<{self.cardinality}>"
        return f"""INLINE_METHOD {bitset_type} {qualified_class_name}::get{self.accessor_name}() const {{
  return {bitset_type}{{static_cast<unsigned long long>({self.field_name})}};
}}

INLINE_METHOD GPU_CALLABLE_METHOD bool {qualified_class_name}::get{self.accessor_name}(const int index) const {{
  assertion(index >= 0, index);
  assertion(index < {self.cardinality}, index, {self.cardinality});
  return (static_cast<std::uint64_t>({self.field_name}) & (std::uint64_t{{1}} << index)) != 0;
}}

INLINE_METHOD void {qualified_class_name}::set{self.accessor_name}(const {bitset_type}& value) {{
  {self.field_name} = static_cast<{self.storage_type}>(value.to_ullong());
}}

INLINE_METHOD GPU_CALLABLE_METHOD void {qualified_class_name}::set{self.accessor_name}(const bool value) {{
  {self.field_name} = value ? {self.mask_name} : {self.storage_type}{{0}};
}}

INLINE_METHOD GPU_CALLABLE_METHOD void {qualified_class_name}::set{self.accessor_name}(const int index, const bool value) {{
  assertion(index >= 0, index);
  assertion(index < {self.cardinality}, index, {self.cardinality});
  const auto bit = std::uint64_t{{1}} << index;
  const auto bits = static_cast<std::uint64_t>({self.field_name});
  {self.field_name} = static_cast<{self.storage_type}>(value ? bits | bit : bits & ~bit);
}}"""

    def get_member_declaration(self):
        return f"""  static_assert({self.cardinality} > 0, "Boolean arrays require at least one entry.");
  static_assert({self.cardinality} <= 64, "Boolean arrays support at most 64 entries.");
  static constexpr {self.storage_type} {self.mask_name}{{
    static_cast<{self.storage_type}>(std::numeric_limits<std::uint64_t>::max() >> (64 - {self.cardinality}))
  }};
  {self.storage_type} {self.field_name}{{0}};"""


class Enumeration(_ScalarAttribute):
    layout_priority = 20

    def __init__(self, name, variants, initval=None):
        self.variants = list(variants)
        default_value = f"{_accessor_name(name)}::{self.variants[0]}"
        super().__init__(name, default_value if initval is None else initval)

    @property
    def cpp_type(self):
        return self.accessor_name

    def get_enum_declaration(self):
        variants = ", ".join(self.variants)
        return f"  enum class {self.accessor_name} : std::uint8_t {{ {variants} }};"

    def get_to_string_expression(self):
        return f"static_cast<int>(get{self.accessor_name}())"
