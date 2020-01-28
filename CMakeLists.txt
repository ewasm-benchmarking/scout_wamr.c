cmake_minimum_required (VERSION 3.0)
project (scout.c)


# adjust these if wabt or libyaml are not subdirectories of this main repo directory
set(WAMR_DIR ./wasm-micro-runtime)
set(LIBYAML_DIR ./libyaml)


add_subdirectory(${WAMR_DIR}/product-mini/platforms/linux)
add_subdirectory(${LIBYAML_DIR})


add_executable(scout.exec scout.c)
add_dependencies(scout.exec vmlib yaml)
target_link_libraries(scout.exec vmlib yaml -lm -ldl -lpthread)


include_directories(${WAMR_DIR}/core/shared/platform/linux ${WAMR_DIR}/core/shared/platform/include ${WAMR_DIR}/core/shared/include ${WAMR_DIR}/core/iwasm/include ${LIBYAML_DIR}/include)
