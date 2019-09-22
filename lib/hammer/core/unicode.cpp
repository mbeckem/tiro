#include "hammer/core/unicode.hpp"

namespace hammer {

std::string_view to_string(GeneralCategory category) {
    switch (category) {
#define HAMMER_CASE(cat)       \
    case GeneralCategory::cat: \
        return #cat;

        HAMMER_CASE(Invalid)
        HAMMER_CASE(Cc)
        HAMMER_CASE(Cf)
        HAMMER_CASE(Cn)
        HAMMER_CASE(Co)
        HAMMER_CASE(Cs)
        HAMMER_CASE(Ll)
        HAMMER_CASE(Lm)
        HAMMER_CASE(Lo)
        HAMMER_CASE(Lt)
        HAMMER_CASE(Lu)
        HAMMER_CASE(Mc)
        HAMMER_CASE(Me)
        HAMMER_CASE(Mn)
        HAMMER_CASE(Nd)
        HAMMER_CASE(Nl)
        HAMMER_CASE(No)
        HAMMER_CASE(Pc)
        HAMMER_CASE(Pd)
        HAMMER_CASE(Pe)
        HAMMER_CASE(Pf)
        HAMMER_CASE(Pi)
        HAMMER_CASE(Po)
        HAMMER_CASE(Ps)
        HAMMER_CASE(Sc)
        HAMMER_CASE(Sk)
        HAMMER_CASE(Sm)
        HAMMER_CASE(So)
        HAMMER_CASE(Zl)
        HAMMER_CASE(Zp)
        HAMMER_CASE(Zs)

#undef HAMMER_CASE
    }

    HAMMER_UNREACHABLE("Invalid category.");
}

GeneralCategory general_category(CodePoint cp) {
    return unicode_data::find_value(unicode_data::cps_to_cat, cp);
}

} // namespace hammer
