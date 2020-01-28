
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "yaml.h"

#include "bh_platform.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "bh_memory.h"
#include "wasm_export.h"


#define verbose 0



///////////////////////////
//#include "wasm_runtime.h" causes errors since things are duplicated with wasm_export.h, so just hard-code it here (this is a hack)

//this struct was copy/pasted so the original license applies.
/* 
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
typedef struct WASMMemoryInstance {
    /* Current page count */
    uint32 cur_page_count;
    /* Maximum page count */
    uint32 max_page_count;

    /* Heap data base address */
    uint8 *heap_data;
    /* Heap data end address */
    uint8 *heap_data_end;
    /* The heap created */
    void *heap_handle;
    /* Heap base offset of wasm app */
    int32 heap_base_offset;

    /* Memory data */
    uint8 *memory_data;
    /* Global data of global instances */
    uint8 *global_data;
    uint32 global_data_size;

    /* End address of memory */
    uint8 *end_addr;

    /* Base address, the layout is:
       thunk_argv data + thunk arg offsets +
       memory data + global data
       memory data init size is: NumBytesPerPage * cur_page_count
       global data size is calculated in module instantiating
       Note: when memory is re-allocated, the thunk argv data, thunk
             argv offsets and memory data must be copied to new memory also.
     */
    uint8 base_addr[1];
} WASMMemoryInstance;

// some dummys
typedef struct WASMTableInstance{int a;} WASMTableInstance;
typedef struct WASMGlobalInstance{int a;} WASMGlobalInstance;
typedef struct WASMFunctionInstance{int a;} WASMFunctionInstance;
typedef struct WASMExportFuncInstance{int a;} WASMExportFuncInstance;
typedef struct WASMModule{int a;} WASMModule;
typedef struct WASMExecEnv{int a;} WASMExecEnv;

//this struct was copy/pasted so the original license applies.
/* 
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
typedef struct WASMModuleInstance {
    /* Module instance type, for module instance loaded from
       WASM bytecode binary, this field is Wasm_Module_Bytecode;
       for module instance loaded from AOT file, this field is
       Wasm_Module_AoT, and this structure should be treated as
       AOTModuleInstance structure. */
    uint32 module_type;

    uint32 memory_count;
    uint32 table_count;
    uint32 global_count;
    uint32 function_count;
    uint32 export_func_count;

    WASMMemoryInstance **memories;
    WASMTableInstance **tables;
    WASMGlobalInstance *globals;
    WASMFunctionInstance *functions;
    WASMExportFuncInstance *export_functions;

    WASMMemoryInstance *default_memory;
    WASMTableInstance *default_table;

    WASMFunctionInstance *start_function;

    WASMModule *module;

#if WASM_ENABLE_LIBC_WASI != 0
    WASIContext *wasi_ctx;
#endif

    uint32 DYNAMICTOP_PTR_offset;
    uint32 temp_ret;
    uint32 llvm_stack;

    /* Default WASM stack size of threads of this Module instance. */
    uint32 default_wasm_stack_size;

    /* The exception buffer of wasm interpreter for current thread. */
    char cur_exception[128];

    /* The custom data that can be set/get by
     * wasm_set_custom_data/wasm_get_custom_data */
    void *custom_data;

    /* Main exec env */
    WASMExecEnv *main_exec_env;
} WASMModuleInstance;






/////////////////
// Host functions

// some globals for now
uint8_t* stateRoot = NULL; // this is the state root for the module, global for now, should put it into WASMModuleInstance or the EE instance or something
uint8_t* blockData = NULL; // this is the blockData for this call, global for now, should put it into WASMModuleInstance or the EE instance or something
uint32_t blockDataSize = 0; // this is needed with blockData

void eth2_loadPreStateRoot(wasm_exec_env_t exec_env, uint32_t offset){
  if(verbose) printf("eth2_loadPreStateRoot(%u)\n",offset);
  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
  WASMMemoryInstance *mem = ((WASMModuleInstance*)module_inst)->default_memory;
  //if (mem->memory_data+offset+32 < end_addr)
  //  wasm_runtime_set_exception(module_inst, buf);
  for (int i=0; i<32; i++){
    mem->memory_data[offset+i] = stateRoot[i];
  }
}

uint32_t eth2_blockDataSize(wasm_exec_env_t exec_env){
  if(verbose) printf("eth2_blockDataSize()\n");
  return blockDataSize;
}

void eth2_blockDataCopy(wasm_exec_env_t exec_env, uint32_t outputOffset, uint32_t offset, uint32_t length){
  if(verbose) printf("eth2_blockDataCopy(%u, %u, %u)\n", outputOffset, offset, length);
  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
  WASMMemoryInstance *mem = ((WASMModuleInstance*)module_inst)->default_memory;
  //if (mem->memory_data+offset+length < end_addr) error;
  //if (blockDataSize < offset+length ) error;
  for (int i=0; i<length; i++){
    mem->memory_data[outputOffset+i] = blockData[offset+i];
  }
}

void eth2_savePostStateRoot(wasm_exec_env_t exec_env, uint32_t offset){
  if(verbose) printf("eth2_savePostStateRoot(%u)\n", offset);
  wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
  WASMMemoryInstance *mem = ((WASMModuleInstance*)module_inst)->default_memory;
  //if (mem->memory_data+offset+32 < mem->end_addr) error;
  for (int i=0; i<32; i++){
    stateRoot[i] = mem->memory_data[offset+i];
  }
}

void eth2_pushNewDeposit(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length){
  if(verbose) printf("eth2_pushNewDeposit(%u, %u)\n", offset, length);
}

void eth2_debugPrintMem(wasm_exec_env_t exec_env, uint32_t offset, uint32_t length){
  if(verbose) printf("eth2_debugPrintMem(%u, %u)\n", offset, length);
}

// wamr registers host functions like this, so that they are available for import
#include "lib_export.h"
static NativeSymbol extended_native_symbol_defs[] = { 
  EXPORT_WASM_API(eth2_loadPreStateRoot),
  EXPORT_WASM_API(eth2_blockDataSize),
  EXPORT_WASM_API(eth2_blockDataCopy),
  EXPORT_WASM_API(eth2_savePostStateRoot),
  EXPORT_WASM_API(eth2_pushNewDeposit),
  EXPORT_WASM_API(eth2_debugPrintMem)
};
#include "ext_lib_export.h"






///////////////////////////
// EEs are each an Account.

// world_state is an array of account pointers.
struct Account** world_state;


struct Account {
  unsigned char* address;
  unsigned char* stateRoot;

  const char* wasm_filename;
};


struct Account* create_account(const char* wasm_filename, int envid){

  struct Account* new_account = malloc(sizeof(struct Account));
  new_account->address = malloc(32);
  new_account->stateRoot = malloc(32);
  for (int i=0;i<32;i++) new_account->address[i]=0; *((int*)new_account->address)=envid;
  for (int i=0;i<32;i++) new_account->stateRoot[i]=0;
  new_account->wasm_filename = wasm_filename;

  return new_account;
}


int account_exec(struct Account* account, unsigned char* input_blockData, int input_blockDataSize){

  // set globals
  blockData = input_blockData;
  blockDataSize = input_blockDataSize;
  stateRoot = account->stateRoot;

  // allocate memory
  if (bh_memory_init_with_allocator(malloc, free)) {
    bh_printf("Init memory with memory allocator failed.\n");
    return -1;
  }

  // init runtime environment
  if (!wasm_runtime_init()){
    bh_memory_destroy();
    return -1;
  }

  bh_log_set_verbose_level(2);

  // get bytecode
  int8* bytecode = NULL;
  uint32 bytecodeSize = 0;
  if (!(bytecode = (uint8*) bh_read_file_to_buffer(account->wasm_filename, &bytecodeSize))){
    wasm_runtime_destroy();
    bh_memory_destroy();
    return -1;
  }

  // parse bytecode
  wasm_module_t wasm_module = NULL;
  char error_buffer[128] = {0};
  if (!(wasm_module = wasm_runtime_load(bytecode, bytecodeSize, error_buffer, sizeof(error_buffer)))) {
    bh_printf("%s\n", error_buffer);
    bh_free(bytecode);
    wasm_runtime_destroy();
    bh_memory_destroy();
    return -1;
  }

  // instantiate module
  //int stack_size = 64 * 1024;
  int stack_size = 1024 * 1024;
  //int heap_size = 64 * 1024;
  int heap_size = 1024 * 1024;
  wasm_module_inst_t wasm_module_inst;
  if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module, stack_size, 
			             heap_size, error_buffer, sizeof(error_buffer)))){
    bh_printf("%s\n", error_buffer);
    wasm_runtime_unload(wasm_module);
    bh_free(bytecode);
    wasm_runtime_destroy();
    bh_memory_destroy();
    return -1;
  }

  // get main func
  wasm_function_inst_t main_func;
  if (!(main_func = wasm_runtime_lookup_function(wasm_module_inst, "main", "()"))){
    wasm_runtime_unload(wasm_module);
    bh_free(bytecode);
    wasm_runtime_destroy();
    bh_memory_destroy();
    return -1;
  }

  // get exec env
  wasm_exec_env_t exec_env;
  if (!(exec_env = wasm_runtime_create_exec_env(wasm_module_inst, stack_size))){
    wasm_runtime_unload(wasm_module);
    bh_free(bytecode);
    wasm_runtime_destroy();
    bh_memory_destroy();
    return -1;
  }

  // call main
  if (!(wasm_runtime_call_wasm(exec_env, main_func, 0, NULL) ) ) {
    const char *exception;
    if ((exception = wasm_runtime_get_exception(wasm_module_inst)))
      bh_printf("%s\n", exception);
    wasm_runtime_unload(wasm_module);
    bh_free(bytecode);
    wasm_runtime_destroy();
    bh_memory_destroy();
    return -1;
  }

  // clean up
  bh_free(bytecode);
  wasm_runtime_destroy_exec_env(exec_env);
  wasm_runtime_deinstantiate(wasm_module_inst);
  wasm_runtime_unload(wasm_module);
  wasm_runtime_destroy();
  bh_memory_destroy();

}


int account_delete(struct Account* account){
  free(account->address);
  free(account->stateRoot);
  free(account);
}


///////////////////////////
// helpers for handling hex

// hex string to int array conversion
// input is string of hex characters, without 0x prefix
// also converts to little endian (ie least significant nibble first)
void hexstr_to_bytearray(uint8_t* out, char* in){
  if(verbose>2) printf("hexstr_to_bytearray(%s)\n",in);
  size_t len = strlen(in);
  if (len==0) return;
  uint8_t byte = 0;
  uint8_t nibble = 0;
  int i;
  for (i=0; i<len; i++){
    nibble = in[i];
    if (nibble >= '0' && nibble <= '9') nibble = nibble - '0';
    else if (nibble >= 'a' && nibble <='f') nibble = nibble - 'a' + 10;
    else if (nibble >= 'A' && nibble <='F') nibble = nibble - 'A' + 10;
    else printf("ERROR: %s is not hex string.\n",in);
    if (i%2) {
      byte = (byte<<4) + nibble;
      *(out+i/2) = byte;
      byte=0;
    }
    else byte = nibble;
  }
  if (byte){
    *(out+i/2) = byte;
  }
}







int main(int argc, char** argv){

  // command line arg is yaml filename
  char* yaml_filename = argv[1];

  //////////////////////////////
  // Lots of yaml stuff follows

  // parse yaml file into a yaml_document_t
  FILE *fp;
  yaml_parser_t parser;
  fp = fopen(yaml_filename, "rb");
  assert(fp);
  assert(yaml_parser_initialize(&parser));
  yaml_parser_set_input_file(&parser, fp);
  yaml_document_t document;
  if (!yaml_parser_load(&parser, &document)) {
    goto error;
  }
  yaml_parser_delete(&parser);
  assert(!fclose(fp));

  // will iterate over nodes of the yaml_document_t, checking structure, and getting node indices for wasm filenames, prestates, shard blocks, and poststates
  yaml_node_t *node;
  int node_idx = 1;

  // will collect idxs to later iterate over yaml sequences
  int start_node_idx_execution_scripts;
  int   end_node_idx_execution_scripts;
  int start_node_idx_pre_states;
  int   end_node_idx_pre_states;
  int start_node_idx_shard_blocks;
  int   end_node_idx_shard_blocks;
  int start_node_idx_post_states;
  int   end_node_idx_post_states;

  // beacon_state
  node = yaml_document_get_node(&document, 1);
  if(node->type != YAML_MAPPING_NODE)
    goto error;
  node = yaml_document_get_node(&document, 2);
  if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "beacon_state"))
    goto error;
  // execution_scripts
  node = yaml_document_get_node(&document, 3);
  if(node->type != YAML_MAPPING_NODE)
    goto error;
  node = yaml_document_get_node(&document, 4);
  if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "execution_scripts"))
    goto error;
  // get start and end node idx of execution script names sequence
  node = yaml_document_get_node(&document, 5);
  if(node->type != YAML_SEQUENCE_NODE)
    goto error;
  start_node_idx_execution_scripts = 6;
  size_t end_line_of_execution_scripts = node->end_mark.line;
  node_idx=6;
  while (node->start_mark.line != end_line_of_execution_scripts){
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE)
      goto error;
    // should also check that node->data.scalar.value is a string which ends in wasm
  }
  end_node_idx_execution_scripts = node_idx-2;
  int num_wasm_files = end_node_idx_execution_scripts-start_node_idx_execution_scripts+1;
  if(verbose) printf("got filenames\n");

  // shard_pre_state
  node_idx = end_node_idx_execution_scripts + 1;
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "shard_pre_state"))
    goto error;
  // exec_env_states
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_MAPPING_NODE)
    goto error;
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "exec_env_states"))
    goto error;
  // get start and end node idx of execution prestates sequence
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SEQUENCE_NODE)
    goto error;
  start_node_idx_pre_states = node_idx;
  size_t end_line_of_pre_states = node->end_mark.line;
  while (node->start_mark.line != end_line_of_pre_states){
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE)
      goto error;
    // should confirm they are all hex strings of length 64
  }
  end_node_idx_pre_states = node_idx-2;
  int num_pre_states = end_node_idx_pre_states-start_node_idx_pre_states;
  node_idx = node_idx-1;
  if(verbose) printf("got prestates\n");

  // get shard_blocks key
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "shard_blocks"))
    goto error;
  // get start and end of execution script names sequence
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SEQUENCE_NODE)
    goto error;
  start_node_idx_shard_blocks = node_idx;
  size_t end_line_of_shard_blocks = node->end_mark.line;
  node = yaml_document_get_node(&document, node_idx++);
  while (node->start_mark.line < end_line_of_shard_blocks){
    // each shard block starts with mapping node
    if(node->type != YAML_MAPPING_NODE)
      goto error;
    // key env and its value
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "env"))
      goto error;
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE)
      goto error;
    // should also check that node->data.scalar.value is an integer less than the number of EEs
    // key "data" and its value
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "data"))
      goto error;
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE)
      goto error;
    // should also check that node->data.scalar.value is hex string
    node = yaml_document_get_node(&document, node_idx++);
  }
  end_node_idx_shard_blocks = node_idx-2;
  int num_shard_blocks = (end_node_idx_shard_blocks-start_node_idx_shard_blocks)/5;
  if(verbose) printf("got shard blocks\n");

  // shard_post_state
  node_idx = end_node_idx_shard_blocks+1;
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "shard_post_state"))
    goto error;
  // exec_env_states
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_MAPPING_NODE)
    goto error;
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SCALAR_NODE || strcmp(node->data.scalar.value, "exec_env_states"))
    goto error;
  // get start and end node idx of execution poststates sequence
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SEQUENCE_NODE)
    goto error;
  start_node_idx_post_states = node_idx;
  size_t end_line_of_post_states = node->end_mark.line;
  do {
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE)
      goto error;
    // should confirm they are all hex strings of length 64
  }
  while (node->start_mark.line < end_line_of_post_states-1);
  end_node_idx_post_states = node_idx-1;
  int num_post_states = end_node_idx_pre_states-start_node_idx_pre_states;
  if(verbose) printf("got poststates %d %d\n", start_node_idx_post_states,end_node_idx_post_states);


  /////////////////////////////////////////////////////
  // Now have idxs of yaml sequences, can do wasm stuff

  // iterate over filenames, creating an account for each
  world_state = (struct Account**) malloc(sizeof(struct Account*)*num_wasm_files);
  for (int i=start_node_idx_execution_scripts, j=0; i<=end_node_idx_execution_scripts; i++, j++){
    node = yaml_document_get_node(&document, i);
    if(verbose) printf("file %s\n",node->data.scalar.value);
    // instantiate wasm
    char *wasm_filename = node->data.scalar.value;
    struct Account* acct = create_account(wasm_filename, j);
    world_state[j] = acct;
  }

  // iterate over prestates
  for (int i=start_node_idx_pre_states, j=0; i<=end_node_idx_pre_states; i++, j++){
    node = yaml_document_get_node(&document, i);
    if(verbose) printf("prestate %s\n",node->data.scalar.value);
    // give env a prestate
    struct Account* acct = world_state[j];
    // should check that node->data.scalar.value has length 64 chars
    hexstr_to_bytearray(acct->stateRoot, node->data.scalar.value);
  }

  // iterate over blocks
  for (int i=start_node_idx_shard_blocks; i<end_node_idx_shard_blocks; i+=5){
    node = yaml_document_get_node(&document, i+2);
    if(verbose) printf("shard_block env %s  ",node->data.scalar.value);
    int env = atoi(node->data.scalar.value);
    node = yaml_document_get_node(&document, i+4);
    if(verbose) printf("blockData %s\n",node->data.scalar.value);
    unsigned char* input_blockData_hex = node->data.scalar.value;
    struct Account* acct = world_state[env];
    size_t len_hexstr = node->data.scalar.length;
    int input_blockDataSize = len_hexstr/2;
    char* input_blockData = (unsigned char*) malloc(input_blockDataSize);
    if (len_hexstr)
      hexstr_to_bytearray(input_blockData, input_blockData_hex);
    // call this env with blockdata
    account_exec(acct, input_blockData, input_blockDataSize);
    free(input_blockData);
    blockDataSize=0;
  }

  // iterate over poststates
  int pass = 1;
  for (int i=start_node_idx_post_states, j=0; i<=end_node_idx_post_states; i++,j++){
    node = yaml_document_get_node(&document, i);
    if(verbose) printf("poststate %s\n", node->data.scalar.value);
    unsigned char* poststate_hex = node->data.scalar.value;
    unsigned char* poststate = malloc(32);
    struct Account* acct = world_state[j];
    hexstr_to_bytearray(poststate,poststate_hex);
    for(int k=0;k<32;k++){
      if(poststate[k]!=acct->stateRoot[k])  {
        printf("error: stateroot[%d]=%d but should be %d\n",k,acct->stateRoot[k],poststate[k]);
	pass = 0;
      }
    }
    free(poststate);
    
  }
  
  if (pass){
    printf("pass\n"); 
  }

  // clean up yaml nodes
  yaml_document_delete(&document);

  // clean up wasm stuff and accounts
  for (int i=0;i<num_wasm_files; i++){
    struct Account* acct = world_state[i];
    free(acct->address);
    free(acct->stateRoot);
    free(acct);
  }
  free(world_state);

  return 0;

error:
  printf("error with libyaml\n");
  return 0;
}




/* GPL3 license
#
#    Copyright 2020 Paul Dworzanski et al.
#
#    This file is part of scout.c.
#
#    scout.c is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    scout.c is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with scout.c.  If not, see <https://www.gnu.org/licenses/>.
#
*/
