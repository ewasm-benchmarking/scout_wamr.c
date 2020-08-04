
The C file `scout.c` implements the [Scout spec](https://ethresear.ch/t/phase-2-execution-prototyping-engine-ewasm-scout/5509) to parse yaml test files, and to execute the Wasm tests.


# Dependencies

We use `libyaml` to parse Scout test yaml format and `wamr` to execute Wasm. If the dependencies are not already present, then download them with `git clone`. If you don't download these dependencies into the main directory of this project, then you will have to adjust paths to them below. CMake is required to compile `libyaml` and `wamr`, and optional to compile `scout.c`. Make sure to download submodules before compiling the project: `git submodule update --init`.


# Compile

Two options.

Option 1) CMake for everything.

NOTE: THIS OPTION WON'T WORK. WAMR IS AWKWARD TO COMPILE AGAINST. DON'T WANT TO SPEND TIME FIGURING IT OUT NOW. JUST USE OPTION 2.
```
# from scout.c repo directory
# make sure that dependencies are in this directory, otherwise adjust paths in CMakeLists.txt
mkdir build && cd build
cmake ..
make	# this outputs scout.exec, and also builds wamr and libyaml
cp scout.exec ..
cd ..
```

Option 2) gcc or clang for scout.c.

```
# from scout.c repo directory

# compile libyaml
cd libyaml/
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make	 # will output library libyaml.a
cd ../..

# compile wamr
cd wasm-micro-runtime/product-mini/platforms/linux
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWAMR_BUILD_AOT=0 -DWAMR_BUILD_JIT=0 -DWAMR_BUILD_LIBC_WASI=0 -DCMAKE_C_COMPILER=clang ..
make	# will output library libvmlib.a
cd ../../../../..

# compile scout.c, adjust paths to includes and libraries as needed (NOTE: THE LONG PATH MUST BE MANUALLY SET FOR YOUR LOCAL DIRECTORY PATH. I DON'T KNOW HOW TO MAKE THIS AUTOMATIC. WAITING FOR WAMR TO STABILIZE.)
gcc scout.c -Ibigint_experiments/ -Iwasm-micro-runtime/core/shared/platform/linux/ -Iwasm-micro-runtime/core/iwasm/include/ -Iwasm-micro-runtime/core/shared/utils/ -Iwasm-micro-runtime/core/shared/utils/uncommon/ -Iwasm-micro-runtime/core/iwasm/common/ -Ilibyaml/include/ -Llibyaml/build/ -Lwasm-micro-runtime/product-mini/platforms/linux/build/ wasm-micro-runtime/product-mini/platforms/linux/build/CMakeFiles/iwasm.dir/home/user/mnt/repos/ethereum/scout.c/wasm-micro-runtime/core/shared/utils/uncommon/bh_read_file.c.o -lyaml -lvmlib -lpthread -lm -O4 -march=native -o scout.exec
```


# Execute

```
# from scout.c repo directory
./scout.exec helloworld.yaml
# warning: yaml files specify path to wasm files relative to scout.exec, everything is in the same directory for now
```
