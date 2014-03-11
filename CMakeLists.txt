# The name of our project is "MANA". CMakeLists files in this project can
# refer to the root source directory of the project as ${MANA_SOURCE_DIR} and
# to the root binary directory of the project as ${MANA_BINARY_DIR}.
cmake_minimum_required (VERSION 2.6)
project (MANA)
add_subdirectory (src bin)
add_subdirectory (test)
#add_subdirectory (examples)
ENABLE_TESTING()
