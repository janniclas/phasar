set(PHASAR_plugins_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS log filesystem program_options REQUIRED)

list(APPEND
  PHASAR_LLVM_DEPS
  Support
  Core)
