#pragma bank 255

#include <gbdk/platform.h>
#include <string.h>
#include "scene_stack_ex.h"
#include "system.h"
#include "vm.h"
#include "events.h"
#include "music_manager.h"
#include "actor.h"
#include "projectiles.h"
#include "camera.h"
#include "gbs_types.h"
#include "bankdata.h"
#include "data_manager.h"
#include "vm_exceptions.h"
#include "input.h"
#include "meta_tiles.h"
#include "scene_transition.h"

typedef struct vm_stack_item_t {    
    // VM contexts
    SCRIPT_CTX CTXS[VM_MAX_CONTEXTS];
    UWORD context_stacks[(VM_MAX_CONTEXTS * VM_CONTEXT_STACK_SIZE)];
    SCRIPT_CTX* first_ctx;
    SCRIPT_CTX* free_ctxs;
    SCRIPT_CTX* old_executing_ctx;
    SCRIPT_CTX* executing_ctx;
    UBYTE vm_lock_state;
} vm_stack_item_t;

typedef struct event_stack_item_t {
    //input events
    script_event_t input_events[8];
    UBYTE input_slots[8];
    script_event_t timer_events[MAX_CONCURRENT_TIMERS];
    timer_time_t timer_values[MAX_CONCURRENT_TIMERS];
} event_stack_item_t;

typedef struct music_stack_item_t {
    // music
    volatile uint8_t music_current_track_bank;
    const TRACK_T * music_current_track;
    script_event_t music_events[4];
} music_stack_item_t;

typedef struct actor_stack_item_t {
    // actors
    actor_t actors[MAX_ACTORS];
    actor_t * actors_active_head;
    actor_t * actors_active_tail;
    actor_t * actors_inactive_head;
    UBYTE player_moving;
    UBYTE player_iframes;
    actor_t * player_collision_actor;
} actor_stack_item_t;

typedef struct projectile_stack_item_t {
    //projectiles
    projectile_t projectiles[MAX_PROJECTILES];
    projectile_def_t projectile_defs[MAX_PROJECTILE_DEFS];
    projectile_t *projectiles_active_head;
    projectile_t *projectiles_inactive_head;
} projectile_stack_item_t;

typedef struct camera_stack_item_t {
    //camera
    UINT16 camera_x;
    UINT16 camera_y;
    UINT16 camera_clamp_x;
    UINT16 camera_clamp_y;
    BYTE camera_offset_x;
    BYTE camera_offset_y;
    BYTE camera_deadzone_x;
    BYTE camera_deadzone_y;
    UBYTE camera_settings;    
} camera_stack_item_t;

typedef struct meta_tiles_stack_item_t {
    //meta tiles
    UBYTE metatile_bank;
    unsigned char* metatile_ptr;
    UBYTE metatile_attr_bank;
    unsigned char* metatile_attr_ptr;
    UBYTE metatile_collision_bank;
    unsigned char* metatile_collision_ptr;
    UBYTE image_tile_width_bit;
} meta_tiles_stack_item_t;

typedef struct scene_transition_stack_item_t {
    //scene transition
    UBYTE scene_transition_enabled;
    far_ptr_t up_scene;
    far_ptr_t right_scene;
    far_ptr_t down_scene;
    far_ptr_t left_scene;    
} scene_transition_stack_item_t;

typedef struct scene_stack_item_ex_t {
    far_ptr_t scene;
    upoint16_t player_pos;
    direction_e player_dir;
    vm_stack_item_t vm_stack_item;
    event_stack_item_t event_stack_item;
    music_stack_item_t music_stack_item;
    actor_stack_item_t actor_stack_item;
    projectile_stack_item_t projectile_stack_item;
    camera_stack_item_t camera_stack_item;
    meta_tiles_stack_item_t meta_tiles_stack_item;
    scene_transition_stack_item_t scene_transition_stack_item;
    uint16_t rand_seed;
} scene_stack_item_ex_t;

extern uint16_t __rand_seed;

scene_stack_item_ex_t __at(0xA000u) scene_stacks_ex[MAX_SCENE_STACK_EX_COUNT];
scene_stack_item_ex_t * scene_stack_ex_ptr;
UBYTE scene_stack_ex_count;

UBYTE vm_pop_scene_stack_state;

void scene_stack_init(void) BANKED {
    scene_stack_ex_ptr = scene_stacks_ex;
    scene_stack_ex_count = 0;
    vm_pop_scene_stack_state = FALSE;
}

static void push_vm_stack_item(void){
    vm_stack_item_t* vm_stack_item = &scene_stack_ex_ptr->vm_stack_item;
    memcpy(&vm_stack_item->CTXS, &CTXS, sizeof(CTXS));    
    memcpy(&vm_stack_item->context_stacks, &script_memory[VM_HEAP_SIZE], sizeof(vm_stack_item->context_stacks));    
    vm_stack_item->first_ctx = first_ctx;
    vm_stack_item->free_ctxs = free_ctxs;
    vm_stack_item->old_executing_ctx = old_executing_ctx;
    vm_stack_item->executing_ctx = executing_ctx;
    vm_stack_item->vm_lock_state = vm_lock_state;
}

static void push_event_stack_item(void){
    event_stack_item_t* event_stack_item = &scene_stack_ex_ptr->event_stack_item;        
    memcpy(&event_stack_item->input_events, &input_events, sizeof(input_events));
    memcpy(&event_stack_item->input_slots, &input_slots, sizeof(input_slots));
    memcpy(&event_stack_item->timer_events, &timer_events, sizeof(timer_events));
    memcpy(&event_stack_item->timer_values, &timer_values, sizeof(timer_values));
}

static void push_music_stack_item(void){
    music_stack_item_t* music_stack_item = &scene_stack_ex_ptr->music_stack_item; 
    music_stack_item->music_current_track_bank = music_current_track_bank;
    music_stack_item->music_current_track = music_current_track;    
    memcpy(&music_stack_item->music_events, &music_events, sizeof(music_events));
}

static void push_actor_stack_item(void){
    actor_stack_item_t* actor_stack_item = &scene_stack_ex_ptr->actor_stack_item; 
    memcpy(&actor_stack_item->actors, &actors, sizeof(actors));
    actor_stack_item->actors_active_head = actors_active_head;
    actor_stack_item->actors_active_tail = actors_active_tail;
    actor_stack_item->actors_inactive_head = actors_inactive_head; 
    actor_stack_item->player_moving = player_moving; 
    actor_stack_item->player_iframes = player_iframes;
    actor_stack_item->player_collision_actor = player_collision_actor;
}

static void push_projectile_stack_item(void){
    projectile_stack_item_t* projectile_stack_item = &scene_stack_ex_ptr->projectile_stack_item; 
    memcpy(&projectile_stack_item->projectiles, &projectiles, sizeof(projectiles));
    memcpy(&projectile_stack_item->projectile_defs, &projectile_defs, sizeof(projectile_defs));
    projectile_stack_item->projectiles_active_head = projectiles_active_head;
    projectile_stack_item->projectiles_inactive_head = projectiles_inactive_head;    
}

static void push_camera_stack_item(void){
    camera_stack_item_t* camera_stack_item = &scene_stack_ex_ptr->camera_stack_item;
    camera_stack_item->camera_x = camera_x;
    camera_stack_item->camera_y = camera_y; 
    camera_stack_item->camera_clamp_x = camera_clamp_x; 
    camera_stack_item->camera_clamp_y = camera_clamp_y; 
    camera_stack_item->camera_offset_x = camera_offset_x; 
    camera_stack_item->camera_offset_y = camera_offset_y; 
    camera_stack_item->camera_deadzone_x = camera_deadzone_x; 
    camera_stack_item->camera_deadzone_y = camera_deadzone_y; 
    camera_stack_item->camera_settings = camera_settings;
}

static void push_meta_tiles_stack_item(void){
    meta_tiles_stack_item_t* meta_tiles_stack_item = &scene_stack_ex_ptr->meta_tiles_stack_item;        
    meta_tiles_stack_item->metatile_bank = metatile_bank;    
    meta_tiles_stack_item->metatile_ptr = metatile_ptr;
    meta_tiles_stack_item->metatile_attr_bank = metatile_attr_bank;
    meta_tiles_stack_item->metatile_attr_ptr = metatile_attr_ptr;
    meta_tiles_stack_item->metatile_collision_bank = metatile_collision_bank;
    meta_tiles_stack_item->metatile_collision_ptr = metatile_collision_ptr; 
    meta_tiles_stack_item->image_tile_width_bit = image_tile_width_bit;
}

static void push_scene_transition_stack_item(void){
    scene_transition_stack_item_t* scene_transition_stack_item = &scene_stack_ex_ptr->scene_transition_stack_item;
    scene_transition_stack_item->scene_transition_enabled = scene_transition_enabled;    
    scene_transition_stack_item->up_scene = up_scene;
    scene_transition_stack_item->right_scene = right_scene;
    scene_transition_stack_item->down_scene = down_scene;
    scene_transition_stack_item->left_scene = left_scene;    
}

void push_scene_stack_ex(void) BANKED {
    if (scene_stack_ex_count < MAX_SCENE_STACK_EX_COUNT){
        SWITCH_RAM_BANK(1, RAM_BANKS_ONLY);
        scene_stack_ex_ptr->scene = current_scene;
        scene_stack_ex_ptr->player_pos = PLAYER.pos;
        scene_stack_ex_ptr->player_dir = PLAYER.dir;
        push_vm_stack_item();
        push_event_stack_item();
        push_music_stack_item();
        push_actor_stack_item();
        push_projectile_stack_item();
        push_camera_stack_item();
        push_meta_tiles_stack_item();
        push_scene_transition_stack_item();
        scene_stack_ex_ptr->rand_seed = __rand_seed;
        scene_stack_ex_ptr++;
        scene_stack_ex_count++;
        SWITCH_RAM_BANK(0, RAM_BANKS_ONLY);
    }
}

static void pop_vm_stack_item(void){
    vm_stack_item_t* vm_stack_item = &scene_stack_ex_ptr->vm_stack_item;
    memcpy(&CTXS, &vm_stack_item->CTXS, sizeof(CTXS));
    memcpy(&script_memory[VM_HEAP_SIZE], &vm_stack_item->context_stacks, sizeof(vm_stack_item->context_stacks)); 
    first_ctx = vm_stack_item->first_ctx;
    free_ctxs = vm_stack_item->free_ctxs;
    old_executing_ctx = vm_stack_item->old_executing_ctx;
    executing_ctx = vm_stack_item->executing_ctx;
    vm_lock_state = vm_stack_item->vm_lock_state;
}

static void pop_event_stack_item(void){
    event_stack_item_t* event_stack_item = &scene_stack_ex_ptr->event_stack_item;        
    memcpy(&input_events, &event_stack_item->input_events, sizeof(input_events));
    memcpy(&input_slots, &event_stack_item->input_slots, sizeof(input_slots));
    memcpy(&timer_events, &event_stack_item->timer_events, sizeof(timer_events));
    memcpy(&timer_values, &event_stack_item->timer_values, sizeof(timer_values));
}

static void pop_music_stack_item(void){
    //store current music
    volatile uint8_t prev_music_current_track_bank = music_current_track_bank;
    const TRACK_T * prev_music_current_track = music_current_track;
        
    music_stack_item_t* music_stack_item = &scene_stack_ex_ptr->music_stack_item; 
    music_current_track_bank = music_stack_item->music_current_track_bank;
    music_current_track = music_stack_item->music_current_track;    
    memcpy(&music_events, &music_stack_item->music_events, sizeof(music_events));
    
    if (prev_music_current_track_bank != music_current_track_bank || prev_music_current_track != music_current_track){
        // Restart music
        if (music_current_track_bank != MUSIC_STOP_BANK) {
            music_next_track = music_current_track;
        } else {
            music_sound_cut();
        }
    }
}

static void pop_actor_stack_item(void){
    actor_stack_item_t* actor_stack_item = &scene_stack_ex_ptr->actor_stack_item; 
    memcpy(&actors, &actor_stack_item->actors, sizeof(actors));
    actors_active_head = actor_stack_item->actors_active_head;
    actors_active_tail = actor_stack_item->actors_active_tail;
    actors_inactive_head = actor_stack_item->actors_inactive_head; 
    player_moving = actor_stack_item->player_moving; 
    player_iframes = actor_stack_item->player_iframes;
    player_collision_actor = actor_stack_item->player_collision_actor;
}

static void pop_projectile_stack_item(void){
    projectile_stack_item_t* projectile_stack_item = &scene_stack_ex_ptr->projectile_stack_item; 
    memcpy(&projectiles, &projectile_stack_item->projectiles, sizeof(projectiles));
    memcpy(&projectile_defs, &projectile_stack_item->projectile_defs, sizeof(projectile_defs));
    projectiles_active_head = projectile_stack_item->projectiles_active_head;
    projectiles_inactive_head = projectile_stack_item->projectiles_inactive_head;    
}

static void pop_camera_stack_item(void){
    camera_stack_item_t* camera_stack_item = &scene_stack_ex_ptr->camera_stack_item;
    camera_x = camera_stack_item->camera_x;
    camera_y = camera_stack_item->camera_y; 
    camera_clamp_x = camera_stack_item->camera_clamp_x; 
    camera_clamp_y = camera_stack_item->camera_clamp_y; 
    camera_offset_x = camera_stack_item->camera_offset_x; 
    camera_offset_y = camera_stack_item->camera_offset_y; 
    camera_deadzone_x = camera_stack_item->camera_deadzone_x; 
    camera_deadzone_y = camera_stack_item->camera_deadzone_y; 
    camera_settings = camera_stack_item->camera_settings;
}

static void pop_meta_tiles_stack_item(void){    
    meta_tiles_stack_item_t* meta_tiles_stack_item = &scene_stack_ex_ptr->meta_tiles_stack_item;        
    metatile_bank = meta_tiles_stack_item->metatile_bank;    
    metatile_ptr = meta_tiles_stack_item->metatile_ptr;
    metatile_attr_bank = meta_tiles_stack_item->metatile_attr_bank;
    metatile_attr_ptr = meta_tiles_stack_item->metatile_attr_ptr;
    metatile_collision_bank = meta_tiles_stack_item->metatile_collision_bank;
    metatile_collision_ptr = meta_tiles_stack_item->metatile_collision_ptr;
    image_tile_width_bit = meta_tiles_stack_item->image_tile_width_bit;
}

static void pop_scene_transition_stack_item(void){
    scene_transition_stack_item_t* scene_transition_stack_item = &scene_stack_ex_ptr->scene_transition_stack_item;
    scene_transition_enabled = scene_transition_stack_item->scene_transition_enabled;    
    up_scene = scene_transition_stack_item->up_scene;
    right_scene = scene_transition_stack_item->right_scene;
    down_scene = scene_transition_stack_item->down_scene;
    left_scene = scene_transition_stack_item->left_scene;    
}

UBYTE pop_scene_stack_ex(void) BANKED {
    if (scene_stack_ex_count > 0){
        SWITCH_RAM_BANK(1, RAM_BANKS_ONLY);
        scene_stack_ex_count--;
        scene_stack_ex_ptr--;
        
        current_scene = scene_stack_ex_ptr->scene;
        PLAYER.pos = scene_stack_ex_ptr->player_pos;
        PLAYER.dir = scene_stack_ex_ptr->player_dir;
        pop_vm_stack_item();
        pop_event_stack_item();
        pop_music_stack_item();
        pop_actor_stack_item();
        pop_projectile_stack_item();
        pop_camera_stack_item();
        pop_meta_tiles_stack_item();
        pop_scene_transition_stack_item();
        __rand_seed = scene_stack_ex_ptr->rand_seed;
        
        SWITCH_RAM_BANK(0, RAM_BANKS_ONLY);
        return TRUE;
    }
    return FALSE;
}

void vm_push_scene_stack_ex(void) OLDCALL BANKED {
    if (scene_stack_ex_count < MAX_SCENE_STACK_EX_COUNT){
        vm_exception_code = EXCEPTION_PUSH_SCENE_STACK;
    }
}

void vm_pop_scene_stack_ex(void) OLDCALL BANKED {
    if (scene_stack_ex_count > 0){
        vm_exception_code = EXCEPTION_POP_SCENE_STACK;
    }
}

void vm_pop_all_scene_stack_ex(void) OLDCALL BANKED {
    if (scene_stack_ex_count > 0){
        scene_stack_ex_ptr = scene_stacks_ex;
        scene_stack_ex_ptr++;
        scene_stack_ex_count = 1;
        vm_exception_code = EXCEPTION_POP_SCENE_STACK;
    }
}

void vm_reset_scene_stack_ex(void) OLDCALL BANKED {
    scene_stack_ex_ptr = scene_stacks_ex;
    scene_stack_ex_count = 0;
}

void vm_poll_stack_pop(SCRIPT_CTX * THIS) OLDCALL BANKED {
    int16_t idx = *(int16_t *)VM_REF_TO_PTR(FN_ARG0);
    UWORD * A;
    if (idx < 0) A = THIS->stack_ptr + idx - 1; else A = script_memory + idx;
    *A = vm_pop_scene_stack_state;
    vm_pop_scene_stack_state = FALSE;
}

void vm_get_stack_size(SCRIPT_CTX * THIS) OLDCALL BANKED {    
    script_memory[*(int16_t*)VM_REF_TO_PTR(FN_ARG0)] = sizeof(scene_stacks_ex);
}