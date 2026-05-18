#include <string>
#include <string_view>

// clang-format off
static inline char32_t transform_to_base_char(char32_t c) {
  if (c >= U'A' && c <= U'Z') return c + (U'a' - U'A');
  if (c >= U'a' && c <= U'z') return c;
  if (c >= U'0' && c <= U'9') return c;

  switch (c) {
    // A
    case U'À': case U'Á': case U'Â': case U'Ã': case U'Ä': case U'Å': case U'Æ':
    case U'à': case U'á': case U'â': case U'ã': case U'ä': case U'å': case U'æ':
    case U'Ą': case U'ą': return U'a';
    // C
    case U'Ç': case U'Ć': case U'Č':
    case U'ç': case U'ć': case U'č': return U'c';
    // E
    case U'È': case U'É': case U'Ê': case U'Ë': case U'Ę':
    case U'è': case U'é': case U'ê': case U'ë': case U'ę': return U'e';
    // I
    case U'Ì': case U'Í': case U'Î': case U'Ï':
    case U'ì': case U'í': case U'î': case U'ï': return U'i';
    // L
    case U'Ł': case U'ł': return U'l';
    // N
    case U'Ñ': case U'Ń':
    case U'ñ': case U'ń': return U'n';
    // O
    case U'Ò': case U'Ó': case U'Ô': case U'Õ': case U'Ö': case U'Ø':
    case U'ò': case U'ó': case U'ô': case U'õ': case U'ö': case U'ø': return U'o';
    // S
    case U'Ś': case U'Š': case U'ß':
    case U'ś': case U'š': return U's';
    // U
    case U'Ù': case U'Ú': case U'Û': case U'Ü':
    case U'ù': case U'ú': case U'û': case U'ü': return U'u';
    // Y
    case U'Ý': case U'Ÿ':
    case U'ý': case U'ÿ': return U'y';
    // Z
    case U'Ź': case U'Ż': case U'Ž':
    case U'ź': case U'ż': case U'ž': return U'z';

    default:
      if (c <= 32) return U' ';
      return c;
  }
}
// clang-format on

static inline std::u32string sanitize_query(std::u32string_view input) {
  std::u32string result;
  result.reserve(input.size());

  bool last_was_space = true;

  for (char32_t c : input) {
    char32_t new_char = transform_to_base_char(c);

    if (new_char == U' ') {
      if (!last_was_space) {
        result += U' ';
        last_was_space = true;
      }
    } else {
      result += new_char;
      last_was_space = false;
    }
  }

  if (!result.empty() && result.back() == U' ') {
    result.pop_back();
  }

  return result;
}
