set(sources
  src/log.cpp
  src/config.cpp
  src/util.cpp
  src/threadpool.cpp
)

set(exe_sources
		src/main.cpp
		${sources}
)

set(headers
  include/log.hpp
  include/config.hpp
  include/util.hpp
  include/threadpool.hpp
)

set(test_sources
  log/log_test.cpp
  config/config_test.cpp
  threadpool/threadpool_test.cpp
)
