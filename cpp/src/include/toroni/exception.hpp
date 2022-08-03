#ifndef TORONI_EXCEPTION_HPP
#define TORONI_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace toroni {
/**
 * @brief  This class is the base class of all exceptions thrown by toroni
 * @note
 * @retval None
 */
class exception : public std::exception {
public:
  explicit exception(const char *err) noexcept : m_str(err) {}

  const char *what() const noexcept override { return m_str.c_str(); }

private:
  std::string m_str;
};
} // namespace toroni
#endif
