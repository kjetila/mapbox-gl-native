#include <mbgl/style/function/identity_stops.hpp>
#include <mbgl/util/color.hpp>

namespace mbgl {
namespace style {

template <>
float IdentityStops<float>::evaluate(const Value& value) const {
    return numericValue<float>(value)
        .value_or(0.0f);
}

template <>
Color IdentityStops<Color>::evaluate(const Value& value) const {
    if (!value.is<std::string>()) {
        return Color::black();
    }

    return Color::parse(value.get<std::string>())
        .value_or(Color::black());
}

} // namespace style
} // namespace mbgl
