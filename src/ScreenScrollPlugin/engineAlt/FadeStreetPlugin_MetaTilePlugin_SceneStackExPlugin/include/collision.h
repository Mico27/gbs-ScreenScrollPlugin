#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <gbdk/platform.h>

#include "math.h"
#include "bankdata.h"
#include "data/states_defines.h"

#define COLLISION_TOP 0x1
#define COLLISION_BOTTOM 0x2
#define COLLISION_LEFT 0x4
#define COLLISION_RIGHT 0x8
#define COLLISION_ALL 0xF
#define TILE_PROP_LADDER 0x10

#define METATILE_SIZE_8     0
#define METATILE_SIZE_16    1

#if METATILE_SIZE == METATILE_SIZE_16
    
#define METATILE_X_OFFSET(x) (x >> 1)
#define METATILE_Y_OFFSET(y) ((y >> 1) << image_tile_width_bit)
#define TILE_X_OFFSET(x) (x & 1)
#define TILE_Y_OFFSET(y) ((y & 1) << 5)
#define METATILE_MAP_OFFSET(x, y)  (METATILE_Y_OFFSET(y) + METATILE_X_OFFSET(x))
#define TILE_MAP_OFFSET(metatile_idx,x,y)  (UWORD)(((metatile_idx & 0xF0) << 2) + ((metatile_idx & 15) << 1) + TILE_Y_OFFSET(y) + TILE_X_OFFSET(x))
#define MAX_MAP_DATA_SIZE 0x1C00
#define COLLISION_DATA_SIZE 0x0400

#else

#define METATILE_Y_OFFSET(y) (y << image_tile_width_bit)
#define METATILE_MAP_OFFSET(x, y)  (METATILE_Y_OFFSET(y) + x)
#define MAX_MAP_DATA_SIZE 0x1F00
#define COLLISION_DATA_SIZE 0x0100

#endif

#define SRAM_MAP_DATA_PTR 0xA000
#define SRAM_COLLISION_DATA_PTR (SRAM_MAP_DATA_PTR + MAX_MAP_DATA_SIZE)

extern UBYTE collision_bank;
extern unsigned char *collision_ptr;
extern UBYTE image_tile_width;
extern UBYTE image_tile_height;

extern UBYTE tile_hit_x;
extern UBYTE tile_hit_y;

extern uint8_t __at(SRAM_COLLISION_DATA_PTR) sram_collision_data[COLLISION_DATA_SIZE];
extern uint8_t __at(SRAM_MAP_DATA_PTR) sram_map_data[MAX_MAP_DATA_SIZE];
extern UBYTE metatile_collision_bank;
extern UBYTE image_tile_width_bit;
extern UBYTE scene_transition_enabled;

/**
 * Check if point is within positioned bounding box.
 *
 * @param bb Pointer to bounding box
 * @param offset Pointer to position offset for bounding box (e.g Actor position)
 * @param point Pointer to position to look for within bounding box
 * @return Point is within bounding box
 */
inline UBYTE bb_contains(rect16_t *bb, upoint16_t *offset, upoint16_t *point) {
    if ((point->x < offset->x + bb->left) || 
        (point->x > offset->x + bb->right)) return FALSE;
    if ((point->y < offset->y + bb->top) || 
        (point->y > offset->y + bb->bottom)) return FALSE;
    return TRUE;
}

/**
 * Check if two positioned bounding boxes intersect.
 *
 * @param bb_a Pointer to bounding box A
 * @param offset_a Pointer to position offset for bounding box A
 * @param bb_b Pointer to bounding box B
 * @param offset_b Pointer to position offset for bounding box B
 * @return Positioned bounding boxes intersect
 */
inline UBYTE bb_intersects(rect16_t *bb_a, upoint16_t *offset_a, rect16_t *bb_b, upoint16_t *offset_b) {
    if ((offset_b->x + bb_b->left   > offset_a->x + bb_a->right) ||
        (offset_b->x + bb_b->right  < offset_a->x + bb_a->left)) return FALSE;    
    if ((offset_b->y + bb_b->top    > offset_a->y + bb_a->bottom) ||
        (offset_b->y + bb_b->bottom < offset_a->y + bb_a->top)) return FALSE;
    return TRUE;
}

/**
 * Return collision tile value at given tile x,y coordinate.
 *
 * @param tx Left tile
 * @param ty Top tile
 * @return Tile value, 0 if no collisions, COLLISION_ALL if out of bounds
 */
inline UBYTE tile_at(UBYTE tx, UBYTE ty) {
    if ((tx < image_tile_width) && (ty < image_tile_height)) {
        if (metatile_collision_bank) {
#if METATILE_SIZE == METATILE_SIZE_16
            return sram_collision_data[TILE_MAP_OFFSET(sram_map_data[METATILE_MAP_OFFSET(tx, ty)], tx, ty)];
#else
            return sram_collision_data[sram_map_data[METATILE_MAP_OFFSET(tx, ty)]];
#endif
        } else {
            return ReadBankedUBYTE(collision_ptr + (ty * (UINT16)image_tile_width) + tx, collision_bank);
        }
    }
    return (scene_transition_enabled)? 0: COLLISION_ALL;
}

/**
 * Test for a tile matching mask in a vertical range from ty_start to ty_end at column tx.
 * Updates globals tile_hit_x and tile_hit_y which can be read afterwards to determine which tile matched
 * 
 * @param tile_mask Tile bitmask to match
 * @param tx Tile x-coordinate
 * @param ty_start Starting tile y-coordinate
 * @param ty_end Ending tile y-coordinate
 * @return First matching tile value found, or 0 if none matched, COLLISION_ALL if out of bounds
 */
UBYTE tile_col_test_range_y(UBYTE tile_mask, UBYTE tx, UBYTE ty_start, UBYTE ty_end) BANKED;

/**
 * Test for a tile matching mask in a horizontal range from tx_start to tx_end at row ty.
 * Updates globals tile_hit_x and tile_hit_y which can be read afterwards to determine which tile matched
 *
 * @param tile_mask Tile bitmask to match
 * @param ty Tile y-coordinate
 * @param tx_start Starting tile x-coordinate
 * @param tx_end Ending tile x-coordinate
 * @return First matching tile value found, or 0 if none matched, COLLISION_ALL if out of bounds
 */
UBYTE tile_col_test_range_x(UBYTE tile_mask, UBYTE ty, UBYTE tx_start, UBYTE tx_end) BANKED;

#endif
