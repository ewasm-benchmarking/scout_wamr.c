
The C file `scout.c` implements the [Scout spec](https://ethresear.ch/t/phase-2-execution-prototyping-engine-ewasm-scout/5509) to parse yaml test files, and to execute the Wasm tests.


# Dependencies

We use `libyaml` to parse Scout test yaml format and `wamr` to execute Wasm. If the dependencies are not already present, then download them with `git clone`. If you don't download these dependencies into the main directory of this project, then you will have to adjust paths to them below. CMake is required to compile `libyaml` and `wamr`, and optional to compile `scout.c`.

```
cd scout.c
git clone https://github.com/yaml/libyaml.git
cd libyaml && git checkout 94ecadc50ebd19299c96ee96417d252a417c7816 . && cd ..
https://github.com/bytecodealliance/wasm-micro-runtime.git
cd wabt && git checkout 1e573664095f8931e3991c5757fbc8d870f22fa4 . && cd ..
```


# Compile

Two options.

Option 1) CMake for everything.

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
make	 # will output library libOFF.a
cd ../..

# compile wamr
cd wasm-micro-runtime/product-mini/platforms/linux
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make	# will output library libvmlib.a
cd ../../../../..

# compile scout.c, adjust paths to includes and libraries as needed
gcc scout.c -Iwasm-micro-runtime/core/shared/platform/linux/ -Iwasm-micro-runtime/core/shared/platform/include/ -Iwasm-micro-runtime/core/shared/include/ -Iwasm-micro-runtime/core/iwasm/include/ -Ilibyaml/include/ -Llibyaml/build/ -Lwasm-micro-runtime/product-mini/platforms/linux/build/ -lOFF -lvmlib -lpthread -lm -O4 -march=native -o scout.exec
# this outputs scout.exec
```


# Execute

```
# from scout.c repo directory
./scout.exec helloworld.yaml
# warning: yaml files specify path to wasm files relative to scout.exec, everything is in the same directory for now
```
