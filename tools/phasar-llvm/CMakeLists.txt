set(LLVM_LINK_COMPONENTS
  Analysis
  BitWriter
  Core
  Demangle
  IRReader
  Linker
  Passes
  Support
)

# Build a stand-alone executable
if(PHASAR_IN_TREE)
  add_phasar_executable(phasar-llvm
    phasar-llvm.cpp
  )
else()
  add_executable(phasar-llvm
    phasar-llvm.cpp
  )
endif()

find_package(Boost COMPONENTS program_options graph ${BOOST_THREAD} REQUIRED)
# Warning! There is a another listing of libraries inside cmake/phasar_macros.cmake.
# If this list is altered the other one should be altered accordingly.
target_link_libraries(phasar-llvm
  LINK_PUBLIC
  phasar_config
  phasar_controller
  phasar_controlflow
  phasar_phasarllvm_utils
  phasar_analysis_strategy
  phasar_ifdside
  phasar_utils
  phasar_mono
  phasar_db
  phasar_experimental
  phasar_passes
  phasar_pointer
  phasar_typehierarchy
  ${SQLITE3_LIBRARY}
  ${Boost_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
  LINK_PRIVATE
  ${PHASAR_STD_FILESYSTEM}
)

if (NOT PHASAR_IN_TREE)
  if(USE_LLVM_FAT_LIB)
    llvm_config(phasar-llvm USE_SHARED ${LLVM_LINK_COMPONENTS})
  else()
    llvm_config(phasar-llvm ${LLVM_LINK_COMPONENTS})
  endif()

  install(TARGETS phasar-llvm
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()
