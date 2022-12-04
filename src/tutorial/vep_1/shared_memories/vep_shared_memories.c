#include <stddef.h>
#include <stdint.h>
#include <platform.h>
#include <vep_private_memory.h>
#include <vep_public_memory.h>

/***** DO NOT MODIFY THIS FILE *****/

#if(TILE_ID == 0)
#ifdef USE_VEP_PRIVATE_MEM01
volatile vep_private_mem01_t * const vep_private_mem01 =
  (vep_private_mem01_t *) VEP_PRIVATE_REGION_TILE0_MEM01_START;
#endif
#ifdef USE_VEP_PRIVATE_MEM02
volatile vep_private_mem02_t * const vep_private_mem02 =
  (vep_private_mem02_t *) VEP_PRIVATE_REGION_TILE0_MEM02_START;
#endif
#endif

#if(TILE_ID == 1)
#ifdef USE_VEP_PRIVATE_MEM01
volatile vep_private_mem01_t * const vep_private_mem01 =
  (vep_private_mem01_t *) VEP_PRIVATE_REGION_TILE1_MEM01_START;
#endif
#ifdef USE_VEP_PRIVATE_MEM12
volatile vep_private_mem12_t * const vep_private_mem12 =
  (vep_private_mem12_t *) VEP_PRIVATE_REGION_TILE2_MEM12_START;
#endif
#endif

#if(TILE_ID == 2)
#ifdef USE_VEP_PRIVATE_MEM02
volatile vep_private_mem02_t * const vep_private_mem02 =
  (vep_private_mem02_t *) VEP_PRIVATE_REGION_TILE2_MEM02_START;
#endif
#ifdef USE_VEP_PRIVATE_MEM12
volatile vep_private_mem12_t * const vep_private_mem12 =
  (vep_private_mem12_t *) VEP_PRIVATE_REGION_TILE2_MEM12_START;
#endif
#endif

#ifdef USE_VEP_PUBLIC_MEM01
#if(TILE_ID == 0 || TILE_ID == 1)
volatile vep_public_mem01_t * const vep_public_mem01 =
  (vep_public_mem01_t *) VEP_PUBLIC_REGION_MEM01_START;
#endif
#endif

#ifdef USE_VEP_PUBLIC_MEM02
#if(TILE_ID == 0 || TILE_ID == 2)
volatile vep_public_mem02_t * const vep_public_mem02 =
  (vep_public_mem02_t *) VEP_PUBLIC_REGION_MEM02_START;
#endif
#endif

#ifdef USE_VEP_PUBLIC_MEM12
#if(TILE_ID == 1 || TILE_ID == 2)
volatile vep_public_mem12_t * const vep_public_mem12 =
  (vep_public_mem12_t *) VEP_PUBLIC_REGION_MEM12_START;
#endif
#endif
