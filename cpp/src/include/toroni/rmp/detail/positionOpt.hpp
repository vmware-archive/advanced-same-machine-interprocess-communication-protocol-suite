#ifndef TORONI_RMP_DETAIL_POSITIONOPT_HPP
#define TORONI_RMP_DETAIL_POSITIONOPT_HPP

#include <optional>

namespace toroni {
namespace rmp {
namespace detail {

template <typename T> using Optional = std::optional<T>;
using PositionOpt = Optional<Position>;
static const PositionOpt EMPTY_POSITION = {};
} // namespace detail
} // namespace rmp
} // namespace toroni
#endif // TORONI_RMP_DETAIL_POSITIONOPT_HPP
