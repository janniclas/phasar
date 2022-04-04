set(PHASAR_controller_COMPONENT_FOUND 1)

find_package(Boost COMPONENTS log REQUIRED)

list(APPEND
  LLVM_DEPS
  Support
  Core)

list(APPEND
  CONTROLLER_DEPS
  ifdside
  mono
  plugins
  db
  pointer
  typehierarchy
  controlflow
  phasarllvm_utils
  utils
  analysis_strategy
)

foreach(dep ${CONTROLLER_DEPS})
  list(APPEND
    NEEDED_LIBS
    phasar::phasar_${dep}
  )
  if(NOT (${PHASAR_${dep}_COMPONENT_FOUND}))
    find_package(phasar COMPONENTS ${dep})
  endif()
endforeach()

list(APPEND
  PHASAR_NEEDED_LIBS
  phasar::phasar_controller
)
