from dataclasses import dataclass
from typing import ClassVar


def _accessor_name(name):
    return name[:1].upper() + name[1:]


@dataclass
class Attribute:
    name: str
    layout_priority: ClassVar[int] = 100

    @property
    def accessor_name(self):
        return _accessor_name(self.name)

    @property
    def field_name(self):
        return "_" + self.name

    def get_public_declarations(self):
        raise NotImplementedError

    def get_enum_declaration(self):
        return None

    def get_static_declaration(self):
        return None

    def get_static_definition(self, qualified_class_name):
        return None

    def get_private_declaration(self):
        raise NotImplementedError

    def get_initialiser(self):
        raise NotImplementedError

    def get_definitions(self, qualified_class_name):
        raise NotImplementedError

    def get_to_string_expression(self):
        return f"get{self.accessor_name}()"


class Boolean(Attribute):
    layout_priority = 30

    def __init__(self, name, initval=None):
        super().__init__(name)
        self.initval = initval or "false"

    def get_public_declarations(self):
        return [
            f"""  INLINE_METHOD bool get{self.accessor_name}() const {{
    return {self.field_name};
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(bool value) {{
    {self.field_name} = value;
  }}""",
        ]

    def get_private_declaration(self):
        return f"  bool {self.field_name};"

    def get_initialiser(self):
        return self.initval

    def get_definitions(self, qualified_class_name):
        return ""

    def get_to_string_expression(self):
        return f'(get{self.accessor_name}() ? "true" : "false")'


class Double(Attribute):
    layout_priority = 0

    def __init__(self, name, initval=None):
        super().__init__(name)
        self.initval = initval or "0.0"

    def get_public_declarations(self):
        return [
            f"""  INLINE_METHOD double get{self.accessor_name}() const {{
    return {self.field_name};
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(double value) {{
    {self.field_name} = value;
  }}""",
        ]

    def get_private_declaration(self):
        return f"  double {self.field_name};"

    def get_initialiser(self):
        return self.initval

    def get_definitions(self, qualified_class_name):
        return ""


class Integer(Attribute):
    layout_priority = 10

    def __init__(self, name, initval=None):
        super().__init__(name)
        self.initval = initval or "0"

    def get_public_declarations(self):
        return [
            f"""  INLINE_METHOD int get{self.accessor_name}() const {{
    return {self.field_name};
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(int value) {{
    {self.field_name} = value;
  }}""",
        ]

    def get_private_declaration(self):
        return f"  int {self.field_name};"

    def get_initialiser(self):
        return self.initval

    def get_definitions(self, qualified_class_name):
        return ""


class IntegerArray(Attribute):
    layout_priority = 10

    def __init__(self, name, cardinality):
        super().__init__(name)
        self.cardinality = str(cardinality)

    def get_public_declarations(self):
        vector_type = f"tarch::la::Vector<{self.cardinality},int>"
        return [
            f"""  INLINE_METHOD {vector_type} get{self.accessor_name}() const {{
    return {self.field_name};
  }}""",
            f"""  INLINE_METHOD int get{self.accessor_name}(int index) const {{
    return {self.field_name}(index);
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(const {vector_type}& value) {{
    {self.field_name} = value;
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(int value) {{
    {self.field_name} = value;
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(int index, int value) {{
    {self.field_name}(index) = value;
  }}""",
        ]

    def get_private_declaration(self):
        return f"  tarch::la::Vector<{self.cardinality},int> {self.field_name};"

    def get_initialiser(self):
        return "0"

    def get_definitions(self, qualified_class_name):
        return ""


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

    def get_public_declarations(self):
        storage_type = self.storage_type
        return [
            f"""  INLINE_METHOD std::bitset<{self.cardinality}> get{self.accessor_name}() const {{
    return std::bitset<{self.cardinality}>(static_cast<unsigned long long>({self.field_name}));
  }}""",
            f"""  INLINE_METHOD bool get{self.accessor_name}(int index) const {{
    return (static_cast<std::uint64_t>({self.field_name}) & (std::uint64_t{{1}} << index)) != 0;
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(const std::bitset<{self.cardinality}>& value) {{
    {self.field_name} = static_cast<{storage_type}>(value.to_ullong());
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(bool value) {{
    {self.field_name} = value ? _BooleanArrayMask{self.accessor_name} : {storage_type}{{0}};
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(int index, bool value) {{
    const std::uint64_t bit = std::uint64_t{{1}} << index;
    if (value) {{
      {self.field_name} = static_cast<{storage_type}>(static_cast<std::uint64_t>({self.field_name}) | bit);
    }}
    else {{
      {self.field_name} = static_cast<{storage_type}>(static_cast<std::uint64_t>({self.field_name}) & ~bit);
    }}
  }}""",
        ]

    def get_private_declaration(self):
        storage_type = self.storage_type
        return f"""  static constexpr {storage_type} _BooleanArrayMask{self.accessor_name} =
    static_cast<{storage_type}>(({self.cardinality} >= 64) ? ~std::uint64_t{{0}} : ((std::uint64_t{{1}} << {self.cardinality}) - std::uint64_t{{1}}));
  static_assert({self.cardinality} <= 64, "BooleanArray supports at most 64 entries");
  {storage_type} {self.field_name};"""

    def get_initialiser(self):
        return "0"

    def get_definitions(self, qualified_class_name):
        return ""


class DoubleArray(Attribute):
    layout_priority = 0

    def __init__(self, name, cardinality):
        super().__init__(name)
        self.cardinality = str(cardinality)

    def get_public_declarations(self):
        vector_type = f"tarch::la::Vector<{self.cardinality},double>"
        return [
            f"""  INLINE_METHOD {vector_type} get{self.accessor_name}() const {{
    return {self.field_name};
  }}""",
            f"""  INLINE_METHOD double get{self.accessor_name}(int index) const {{
    return {self.field_name}(index);
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(const {vector_type}& value) {{
    {self.field_name} = value;
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(double value) {{
    {self.field_name} = value;
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}(int index, double value) {{
    {self.field_name}(index) = value;
  }}""",
        ]

    def get_private_declaration(self):
        return f"  tarch::la::Vector<{self.cardinality},double> {self.field_name};"

    def get_initialiser(self):
        return "0.0"

    def get_definitions(self, qualified_class_name):
        return ""


class Enumeration(Attribute):
    layout_priority = 20

    def __init__(self, name, variants, initval=None):
        super().__init__(name)
        self.variants = list(variants)
        self.initval = initval or f"{self.accessor_name}::{self.variants[0]}"

    def get_enum_declaration(self):
        variants = ", ".join(self.variants)
        return f"  enum class {self.accessor_name} : std::uint8_t {{ {variants} }};"

    def get_public_declarations(self):
        return [
            f"""  INLINE_METHOD {self.accessor_name} get{self.accessor_name}() const {{
    return {self.field_name};
  }}""",
            f"""  INLINE_METHOD void set{self.accessor_name}({self.accessor_name} value) {{
    {self.field_name} = value;
  }}""",
        ]

    def get_private_declaration(self):
        return f"  {self.accessor_name} {self.field_name};"

    def get_initialiser(self):
        return self.initval

    def get_definitions(self, qualified_class_name):
        return ""

    def get_to_string_expression(self):
        return f"static_cast<int>(get{self.accessor_name}())"


class StaticUserDefinedType(Attribute):
    layout_priority = 100

    def __init__(self, name, type, include):
        super().__init__(name)
        self.type = type
        self.include = include

    def get_public_declarations(self):
        return [
            f"""  INLINE_METHOD static {self.type}& get{self.accessor_name}() {{
    return {self.field_name};
  }}""",
        ]

    def get_static_declaration(self):
        return f"  static {self.type} {self.field_name};"

    def get_private_declaration(self):
        return ""

    def get_initialiser(self):
        return None

    def get_definitions(self, qualified_class_name):
        return ""

    def get_static_definition(self, qualified_class_name):
        return f"{self.type} {qualified_class_name}::{self.field_name};"

    def get_to_string_expression(self):
        return f"get{self.accessor_name}().toString()"
