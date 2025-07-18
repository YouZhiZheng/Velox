set(sources
  src/log.cpp
  src/config.cpp
  src/util.cpp
)

set(exe_sources
		src/main.cpp
		${sources}
)

set(headers
  include/log.hpp
  include/config.hpp
  include/util.hpp
)

set(test_sources
  log/log_test.cpp
  config/config_test.cpp
)
