#ifndef TORONI_CPP_SYSTEM_TESTS_BURST_LOG_HPP
#define TORONI_CPP_SYSTEM_TESTS_BURST_LOG_HPP

#if defined(BOOST_FOUND) && defined(ENABLE_LOGGING)
#define LOG(lev, a) BOOST_LOG_TRIVIAL(lev) << a

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>

void InitLogger() {
  namespace logging = boost::log;

  logging::register_simple_formatter_factory<logging::trivial::severity_level,
                                             char>("Severity");
  logging::add_file_log(logging::keywords::file_name = "agent.log",
                        logging::keywords::open_mode = std::ios_base::app,
                        // logging::keywords::auto_flush = true,
                        logging::keywords::format =
                            "[%TimeStamp%] [%ThreadID%] [%Severity%] "
                            "[%ProcessID%] "
                            "%Message%");
  logging::add_common_attributes();

  logging::core::get()->set_filter(logging::trivial::severity >=
                                   logging::trivial::trace);
}
#else
#define LOG(lev, a)
void InitLogger(){};
#endif
#endif // TORONI_CPP_SYSTEM_TESTS_BURST_LOG_HPP
