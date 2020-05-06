
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "yaml.h"

#include "bh_platform.h"
#include "bh_assert.h"
#include "bh_log.h"
//#include "bh_memory.h"
#include "bh_read_file.h"
#include "wasm_export.h"
#include "wasm_runtime_common.h"

#include <time.h>



#define verbose 0



///////////////////////
// some globals for now

uint8_t* stateRoot = NULL; // this is the state root for the module, global for now, should put it into WASMModuleInstance or the EE instance or something
uint8_t* blockData = NULL; // this is the blockData for this call, global for now, should put it into WASMModuleInstance or the EE instance or something
uint32_t blockDataSize = 0; // this is needed with blockData



/////////////////
// Host functions

void eth2_loadPreStateRoot(wasm_exec_env_t exec_env, uint8_t* mem){
  if(verbose) printf("eth2_loadPreStateRoot(%p)\n",mem);
  //if (mem->memory_data+offset+32 < end_addr)
  //  wasm_runtime_set_exception(module_inst, buf);
  for (int i=0; i<32; i++){
    mem[i] = stateRoot[i];
  }
}

uint32_t eth2_blockDataSize(wasm_exec_env_t exec_env){
  if(verbose) printf("eth2_blockDataSize()\n");
  return blockDataSize;
}

void eth2_blockDataCopy(wasm_exec_env_t exec_env, uint8_t *mem, uint32_t offset, uint32_t length){
  if(verbose) printf("eth2_blockDataCopy(%p, %u, %u)\n", mem, offset, length);
  //if (mem->memory_data+offset+length < end_addr) error;
  //if (blockDataSize < offset+length ) error;
  for (int i=0; i<length; i++){
    mem[i] = blockData[offset+i];
  }
}

void eth2_savePostStateRoot(wasm_exec_env_t exec_env, uint8_t* mem){
  if(verbose) printf("eth2_savePostStateRoot(%p)\n", mem);
  //if (mem->memory_data+offset+32 < mem->end_addr) error;
  for (int i=0; i<32; i++){
    stateRoot[i] = mem[i];
  }
}



// Next are bigint host funcs. Hacky for now.

// bls12-381 values hard-coded
//mod = 0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab
//modinv = 0xceb06106feaafc9468b316fee268cf5819ecca0e8eb2db4c16ef2ef0c8e30b48286adb92d9d113e889f3fffcfffcfffd
//uint64_t mod[] = {0xb9feffffffffaaab, 0x1eabfffeb153ffff, 0x6730d2a0f6b0f624, 0x64774b84f38512bf, 0x4b1ba7b6434bacd7, 0x1a0111ea397fe69a};
//uint64_t modinv[] = {0x89f3fffcfffcfffd};

// bigint functions come from bigint.h, which requires #define's
/* included in wamr
*/
#define BIGINT_BITS 384
#define LIMB_BITS 64
#define LIMB_BITS_OVERFLOW 128
#include "bigint.h"
const uint64_t mod[] = {0xb9feffffffffaaab, 0x1eabfffeb153ffff, 0x6730d2a0f6b0f624, 0x64774b84f38512bf, 0x4b1ba7b6434bacd7, 0x1a0111ea397fe69a};
const uint64_t modinv = 0x89f3fffcfffcfffd;

void eth2_pushNewDeposit(wasm_exec_env_t exec_env, uint8_t* mem, uint32_t length){
  if(verbose) printf("eth2_pushNewDeposit(%p, %u)\n", mem, length);
}

void debug_printMemHex(wasm_exec_env_t exec_env, uint8_t* mem, uint32_t length){
  if(verbose) printf("eth2_debugPrintMem(%p, %u)\n", mem, length);
}

int f1m_mul_counter=0;
void bignum_f1m_mul(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* out){
  if(verbose) printf("bignum_f1m_mul()\n");
  f1m_mul_counter+=1;
  FUNCNAME(montmul)((UINT*)out,(UINT*)y,(UINT*)x,(UINT*)mod,modinv);
}

int f1m_add_counter=0;
void bignum_f1m_add(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* out){
  if(verbose) printf("bignum_f1m_add()\n");
  f1m_add_counter+=1;
  FUNCNAME(addmod)((UINT*)out,(UINT*)x,(UINT*)y,(UINT*)mod);
}

int f1m_sub_counter=0;
void bignum_f1m_sub(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* out){
  if(verbose) printf("bignum_f1m_sub()\n");
  f1m_sub_counter+=1;
  FUNCNAME(subtractmod)((UINT*)out,(UINT*)x,(UINT*)y,(UINT*)mod);
}

int int_mul_counter=0;
void bignum_int_mul(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* out){
  if(verbose) printf("bignum_int_mul()\n");
  int_mul_counter+=1;
  FUNCNAME(mul)((UINT*)out,(UINT*)x,(UINT*)y);
}

int int_add_counter=0;
uint32_t bignum_int_add(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* out){
  if(verbose) printf("bignum_int_add()\n");
  int_add_counter+=1;
  return FUNCNAME(add)((UINT*)out,(UINT*)x,(UINT*)y);
}

int int_sub_counter=0;
uint32_t bignum_int_sub(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* out){
  if(verbose) printf("bignum_int_sub()\n");
  int_sub_counter+=1;
  return FUNCNAME(subtract)((UINT*)out,(UINT*)x,(UINT*)y);
}


int int_div_counter=0;
//void bignum_int_div(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* remainder, uint8_t* quotient){
void bignum_int_div(wasm_exec_env_t exec_env, uint8_t* x, uint8_t* y, uint8_t* quotient, uint8_t* remainder) {
  int_div_counter++;
  if(verbose) printf("bignum_int_div()\n");

  /*
  for (int i=47;i>=0;i--){
    printf("%02x",x[i]);
  }
  printf("\n");
  for (int i=47;i>=0;i--){
    printf("%02x",y[i]);
  }
  printf("\n");
  */
  FUNCNAME(div)((UINT*)quotient,(UINT*)remainder,(UINT*)x,(UINT*)y);
  /*
  for (int i=47;i>=0;i--){
    printf("%02x",quotient[i]);
  }
  printf("\n");
  for (int i=47;i>=0;i--){
    printf("%02x",remainder[i]);
  }
  printf("\n");
  printf("bignum_int_div() returning\n");
  */
}

#undef BIGINT_BITS
#undef LIMB_BITS
#undef LIMB_BITS_OVERFLOW


// wamr registers host functions like this, so that they are available for import
// see https://github.com/bytecodealliance/wasm-micro-runtime/blob/master/doc/export_native_api.md
// also see code in the repo, since this doc may be outdated
static NativeSymbol native_symbols[] = {
  EXPORT_WASM_API_WITH_SIG(eth2_loadPreStateRoot,"(*)"),
  EXPORT_WASM_API_WITH_SIG(eth2_blockDataSize,"()i"),
  EXPORT_WASM_API_WITH_SIG(eth2_blockDataCopy,"(*ii)"),
  EXPORT_WASM_API_WITH_SIG(eth2_savePostStateRoot,"(*)"),
  EXPORT_WASM_API_WITH_SIG(eth2_pushNewDeposit,"(*~)"),
  EXPORT_WASM_API_WITH_SIG(debug_printMemHex,"(*~)"),
  EXPORT_WASM_API_WITH_SIG(bignum_f1m_mul,"(***)"),
  EXPORT_WASM_API_WITH_SIG(bignum_f1m_add,"(***)"),
  EXPORT_WASM_API_WITH_SIG(bignum_f1m_sub,"(***)"),
  EXPORT_WASM_API_WITH_SIG(bignum_int_mul,"(***)"),
  EXPORT_WASM_API_WITH_SIG(bignum_int_add,"(***)i"),
  EXPORT_WASM_API_WITH_SIG(bignum_int_sub,"(***)i"),
  EXPORT_WASM_API_WITH_SIG(bignum_int_div,"(****)")

};





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


  // wamr stuff
  //char *wasm_file = NULL;
  const char *func_name = NULL;
  uint8 *wasm_file_buf = NULL;
  uint32 wasm_file_size;
  uint32 stack_size = 16 * 1024, heap_size = 16 * 1024;
  wasm_module_t wasm_module = NULL;
  wasm_module_inst_t wasm_module_inst = NULL;
  RuntimeInitArgs init_args;
  char error_buf[128] = { 0 };

  memset(&init_args, 0, sizeof(RuntimeInitArgs));

  init_args.mem_alloc_type = Alloc_With_Allocator;
  init_args.mem_alloc_option.allocator.malloc_func = malloc;
  init_args.mem_alloc_option.allocator.realloc_func = realloc;
  init_args.mem_alloc_option.allocator.free_func = free;

  // init runtime environment
  if (!wasm_runtime_full_init(&init_args)) {
     printf("Init runtime environment failed.\n");
      return -1;
  }

  // register host funcs
  int num_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
  if (!wasm_runtime_register_natives("env",
                                     native_symbols, 
                                     num_native_symbols)) {
    goto fail1;
  }

  bh_log_set_verbose_level(0);

  // read bytecode
  if (!(wasm_file_buf = (uint8*) bh_read_file_to_buffer(account->wasm_filename,
                                                        &wasm_file_size)))
      goto fail1;

  // load WASM module
  if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                        error_buf, sizeof(error_buf)))) {
      printf("%s\n", error_buf);
      goto fail2;
  }

  //printf("loaded module\n");

  /* instantiate the module */
  if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                    stack_size,
                                                    heap_size,
                                                    error_buf,
                                                    sizeof(error_buf)))) {
      printf("%s\n", error_buf);
      goto fail3;
  }

  //printf("instantiated module\n");


  // call func
  struct timespec requestStart, requestEnd;
  clock_gettime(CLOCK_REALTIME, &requestStart);

  //printf("calling func\n");
  wasm_application_execute_func(wasm_module_inst, "main", 0, NULL);
  //printf("returned from func\n");

  clock_gettime(CLOCK_REALTIME, &requestEnd);
  double accum = ( requestEnd.tv_sec - requestStart.tv_sec )
                 + ( requestEnd.tv_nsec - requestStart.tv_nsec )
                 / 1E9;
  printf( "execution time: %lf\n", accum );



  /* destroy the module instance */
  wasm_runtime_deinstantiate(wasm_module_inst);



fail3:
  /* unload the module */
  wasm_runtime_unload(wasm_module);

fail2:
  /* free the file buffer */
  wasm_runtime_free(wasm_file_buf);

fail1:
  /* destroy runtime environment */
  wasm_runtime_destroy();





  return 0;
}


void account_delete(struct Account* account){
  free(account->address);
  free(account->stateRoot);
  free(account);
}


///////////////////////////
// helpers for handling hex

// hex string to int array conversion
// input is string of hex characters, without 0x prefix
// also converts to little endian (ie least significant nibble first)
void hexstr_to_bytearray(uint8_t* out, unsigned char* in){
  if(verbose>2) printf("hexstr_to_bytearray(%s)\n",in);
  size_t len = strlen((char*)in);
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
  if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "beacon_state"))
    goto error;
  // execution_scripts
  node = yaml_document_get_node(&document, 3);
  if(node->type != YAML_MAPPING_NODE)
    goto error;
  node = yaml_document_get_node(&document, 4);
  if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "execution_scripts"))
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
  if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "shard_pre_state"))
    goto error;
  // exec_env_states
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_MAPPING_NODE)
    goto error;
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "exec_env_states"))
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
  if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "shard_blocks"))
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
    if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "env"))
      goto error;
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE)
      goto error;
    // should also check that node->data.scalar.value is an integer less than the number of EEs
    // key "data" and its value
    node = yaml_document_get_node(&document, node_idx++);
    if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "data"))
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
  if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "shard_post_state"))
    goto error;
  // exec_env_states
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_MAPPING_NODE)
    goto error;
  node = yaml_document_get_node(&document, node_idx++);
  if(node->type != YAML_SCALAR_NODE || strcmp((char*)node->data.scalar.value, "exec_env_states"))
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
    char *wasm_filename = (char*)node->data.scalar.value;
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
    int env = atoi((char*)node->data.scalar.value);
    node = yaml_document_get_node(&document, i+4);
    if(verbose) printf("blockData %s\n",node->data.scalar.value);
    unsigned char* input_blockData_hex = node->data.scalar.value;
    struct Account* acct = world_state[env];
    size_t len_hexstr = node->data.scalar.length;
    int input_blockDataSize = len_hexstr/2;
    unsigned char* input_blockData = (unsigned char*) malloc(input_blockDataSize);
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


  printf("f1m_mul_counter %u\n",f1m_mul_counter);
  printf("f1m_add_counter %u\n",f1m_add_counter);
  printf("f1m_sub_counter %u\n",f1m_sub_counter);
  printf("int_mul_counter %u\n",int_mul_counter);
  printf("int_add_counter %u\n",int_add_counter);
  printf("int_sub_counter %u\n",int_sub_counter);
  printf("int_div_counter %u\n",int_div_counter);



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
