set(CMAKE_C_COMPILER "/usr/local/opt/llvm/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/local/opt/llvm/bin/clang++")
set(CMAKE_EXE_LINKER_FLAGS -Wl)
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_INSTALL_RPATH "/usr/local/opt/llvm/lib")
set(CMAKE_EXE_LINKER_FLAGS "-L/usr/local/opt/llvm/lib")
# link_directories("/usr/local/opt/llvm/lib")