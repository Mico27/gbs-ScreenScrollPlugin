#pragma bank 255

#include "scene_transition.h"

#include <string.h>

#include "system.h"
#include "actor.h"
#include "camera.h"
#include "data_manager.h"
#include "game_time.h"
#include "math.h"
#include "fade_manager.h"
#include "parallax.h"
#include "scroll.h"
#include "ui.h"
#include "bankdata.h"
#include "input.h"
#include "projectiles.h"
#include "shadow.h"
#include "music_manager.h"
#include "vm.h"
#include "macro.h"
#include "interrupts.h"
#include "data/game_globals.h"

#define TILE_FRACTION_MASK         0b11111111
#define ONE_TILE_DISTANCE          256

INT16 player_transition_right_dist;
INT16 player_transition_left_dist;
INT16 player_transition_top_dist;
INT16 player_transition_bottom_dist;

INT16 player_transition_right_threshold;
INT16 player_transition_left_threshold;
INT16 player_transition_top_threshold;
INT16 player_transition_bottom_threshold;

UBYTE scroll_right_margin;
UBYTE scroll_bottom_margin;
UBYTE scroll_top_offset;

UBYTE scene_transition_enabled;
UBYTE is_transitioning_scene;
far_ptr_t up_scene;
far_ptr_t right_scene;
far_ptr_t down_scene;
far_ptr_t left_scene;
UBYTE round_position_flags;
INT16 transitioning_player_pos_x;
INT16 transitioning_player_pos_y;

INT16 transitioning_cam_pos_x;
INT16 transitioning_cam_pos_y;

void enable_transition_to_scene(void) BANKED {
    //camera_settings &= ~(CAMERA_LOCK_FLAG);
    //camera_x = SCROLL_CAM_X;
    //camera_y = SCROLL_CAM_Y;
    scene_transition_enabled = 1;
}

void scene_transition_reset(void) BANKED {
    if (image_height < SCREENHEIGHT){
        camera_settings &= ~(CAMERA_LOCK_Y_FLAG);
    }
    if (image_width < SCREENWIDTH){
        camera_settings &= ~(CAMERA_LOCK_X_FLAG);
    }
    up_scene.bank = 0;
    right_scene.bank = 0;
    down_scene.bank = 0;
    left_scene.bank = 0;
    scene_transition_enabled = 0;
}

void check_transition_to_scene_collision(void) BANKED {	
    if (scene_transition_enabled && !is_transitioning_scene && !CHK_FLAG(PLAYER.flags, ACTOR_FLAG_DISABLED)) {		
        // Check for scene scroll
        if (transitioning_player_pos_y != PLAYER.pos.y)
        {
            transitioning_player_pos_y = 0x7FFF;
            if (((WORD)PLAYER.pos.y) < player_transition_top_threshold){
                transition_to_scene_modal(DIRECTION_UP);				
                } else if ((WORD)PLAYER.pos.y > (TILE_TO_SUBPX(image_tile_height) - player_transition_bottom_threshold)){
                transition_to_scene_modal(DIRECTION_DOWN);		
            }
        }
        if (transitioning_player_pos_x != PLAYER.pos.x)
        {
            transitioning_player_pos_x = 0x7FFF;
            if (((WORD)PLAYER.pos.x) < player_transition_left_threshold){
                transition_to_scene_modal(DIRECTION_LEFT);
                } else if ((WORD)PLAYER.pos.x > (TILE_TO_SUBPX(image_tile_width) - player_transition_right_threshold)){
                transition_to_scene_modal(DIRECTION_RIGHT);
            }
        }
    }
}

void transition_to_scene_modal(UBYTE direction) BANKED {
    UBYTE scene_bank = 0;
    const scene_t * scene = NULL;
    switch(direction){
        case DIRECTION_UP:
            scene_bank = up_scene.bank;
            scene = up_scene.ptr;
        break;
        case DIRECTION_RIGHT:
            scene_bank = right_scene.bank;
            scene = right_scene.ptr;
        break;
        case DIRECTION_DOWN:
            scene_bank = down_scene.bank;
            scene = down_scene.ptr;
        break;
        case DIRECTION_LEFT:
            scene_bank = left_scene.bank;
            scene = left_scene.ptr;
        break;    
    }
    if (scene_bank && scene){
        camera_settings &= ~(CAMERA_LOCK_FLAG);
        is_transitioning_scene = direction;
        transition_load_scene(scene_bank, scene, direction);
        uint8_t camera_arrived = FALSE;
        uint8_t player_arrived = FALSE;
		do {
			script_runner_update();
        } while (VM_ISLOCKED());
        switch(direction){
            case DIRECTION_UP:
                if (scroll_bottom_margin){
                    scroll_render_rows(draw_scroll_x, 0, image_tile_height - 1, scroll_bottom_margin);
                }
                scroll_y = (SUBPX_TO_PX(camera_y) - (SCREENHEIGHT >> 1)) + 8;
            break;
            case DIRECTION_RIGHT:                
                if (scroll_right_margin){                    
                    scroll_render_cols(0, draw_scroll_y, 0, scroll_right_margin);
                }
                scroll_x = (SUBPX_TO_PX(camera_x) - (SCREENWIDTH >> 1)) - 8;
            break;
            case DIRECTION_DOWN:
                if (scroll_bottom_margin){
                    scroll_render_rows(draw_scroll_x, 0, 0, scroll_bottom_margin);
                }
                scroll_y = (SUBPX_TO_PX(camera_y) - (SCREENHEIGHT >> 1)) - 8;	
            break;
            case DIRECTION_LEFT:
                scroll_x = (SUBPX_TO_PX(camera_x) - (SCREENWIDTH >> 1));
            break;
        
        }
		do {
            script_runner_update();	
            
            if (!VM_ISLOCKED()){
                camera_arrived = transition_camera_to();
                player_arrived = transition_player_to();
            }
            input_update();		
            ui_update();
            
            toggle_shadow_OAM();
            camera_update();
            scroll_update();
            actors_update();
            actors_render();
            if (projectiles_active_head) {
                projectiles_update();
                projectiles_render();
            }
            activate_shadow_OAM();
            
            game_time++;
            wait_vbl_done();
            
            if (camera_arrived && player_arrived) {	
                camera_settings |= CAMERA_LOCK_FLAG;
                
                is_transitioning_scene = 0;
            }
        } while (is_transitioning_scene);
    }
}

void transition_load_scene(UBYTE scene_bank, const scene_t * scene, UBYTE direction) BANKED {
    // hide actors (except player)
    actor_t *actor = actors_active_tail;
    while (actor) {
        if (actor != &PLAYER){
            SET_FLAG(actor->flags, ACTOR_FLAG_HIDDEN);
        }		
        actor = actor->prev;
    }
    UBYTE tmp_show_actors_on_overlay = show_actors_on_overlay;
    show_actors_on_overlay = TRUE;
    // hide projectiles
    projectiles_init();
    // Update sprites before scene change
    toggle_shadow_OAM();
    actors_update();
    actors_render();
    projectiles_render();
    activate_shadow_OAM();
    wait_vbl_done();
    show_actors_on_overlay = tmp_show_actors_on_overlay;
    switch (direction){
        case DIRECTION_RIGHT:
            camera_x = -TILE_TO_SUBPX(20 - scroll_right_margin) + SCROLL_CAM_X;
            PLAYER.pos.x -= TILE_TO_SUBPX(image_tile_width);
            bkg_offset_x = (bkg_offset_x + image_tile_width) & 31; 
            transitioning_cam_pos_x = SCROLL_CAM_X;
            transitioning_cam_pos_y = camera_y;
            transitioning_player_pos_x = PLAYER.pos.x + player_transition_right_dist;
            transitioning_player_pos_y = PLAYER.pos.y;
        break;
        case DIRECTION_DOWN:
            camera_y = -TILE_TO_SUBPX(18 - scroll_bottom_margin) + SCROLL_CAM_Y;
            PLAYER.pos.y -= TILE_TO_SUBPX(image_tile_height);
            bkg_offset_y = (bkg_offset_y + image_tile_height) & 31;
            transitioning_cam_pos_y = SCROLL_CAM_Y;
            transitioning_cam_pos_x = camera_x;
            transitioning_player_pos_y = PLAYER.pos.y + player_transition_bottom_dist;
            transitioning_player_pos_x = PLAYER.pos.x;
        break;
    }
    // kill all threads, but don't clear variables 
    script_runner_init(FALSE);
    // reset timers on scene change
    timers_init(FALSE);
    // reset input events on scene change
    events_init(FALSE);
    // reset music events
    music_init_events(FALSE);
    
    load_scene(scene, scene_bank, TRUE);
    
    switch (direction){
        case DIRECTION_LEFT:
            camera_x = TILE_TO_SUBPX(image_tile_width) + SCROLL_CAM_X;
            PLAYER.pos.x += TILE_TO_SUBPX(image_tile_width);
            bkg_offset_x = (bkg_offset_x - image_tile_width) & 31;
            transitioning_cam_pos_x = TILE_TO_SUBPX(image_tile_width - (20 - scroll_right_margin)) + SCROLL_CAM_X;
            transitioning_cam_pos_y = camera_y;
            transitioning_player_pos_x = PLAYER.pos.x - player_transition_left_dist;
            transitioning_player_pos_y = PLAYER.pos.y;
        break;
        case DIRECTION_UP:
            camera_y = TILE_TO_SUBPX(image_tile_height) + SCROLL_CAM_Y;
            PLAYER.pos.y += TILE_TO_SUBPX(image_tile_height);
            bkg_offset_y = (bkg_offset_y - image_tile_height) & 31;
            transitioning_cam_pos_y = TILE_TO_SUBPX(image_tile_height - (18 - scroll_bottom_margin)) + SCROLL_CAM_Y;
            transitioning_cam_pos_x = camera_x;
            transitioning_player_pos_y = PLAYER.pos.y - player_transition_top_dist;
            transitioning_player_pos_x = PLAYER.pos.x;
        break;
    }
        
    if (round_position_flags & direction){		
        transitioning_player_pos_x = (transitioning_player_pos_x  & ~TILE_FRACTION_MASK);
        transitioning_player_pos_y = (transitioning_player_pos_y  & ~TILE_FRACTION_MASK);
        if (direction == DIRECTION_UP){
            transitioning_player_pos_y += ONE_TILE_DISTANCE;
            } else if (direction == DIRECTION_LEFT){
            transitioning_player_pos_x += ONE_TILE_DISTANCE;
        }
    }
}

uint8_t transition_camera_to(void) BANKED {
    // Move camera towards destination
    
    if ((camera_x == transitioning_cam_pos_x) && (camera_y == transitioning_cam_pos_y)) {        
        return TRUE;
    }
    if (camera_x > transitioning_cam_pos_x) {
        // Move left
        camera_x -= SCROLL_CAM_SPEED;
        if (camera_x <= transitioning_cam_pos_x) {
            camera_x = transitioning_cam_pos_x;
        }
        } else if (camera_x < transitioning_cam_pos_x) {
        // Move right
        camera_x += SCROLL_CAM_SPEED;
        if (camera_x >= transitioning_cam_pos_x) {
            camera_x = transitioning_cam_pos_x;
        }        
    }
    
    if (camera_y > transitioning_cam_pos_y) {
        // Move up
        camera_y -= SCROLL_CAM_SPEED;
        if (camera_y <= transitioning_cam_pos_y) {
            camera_y = transitioning_cam_pos_y;
        }        
        } else if (camera_y < transitioning_cam_pos_y) {
        // Move down
        camera_y += SCROLL_CAM_SPEED;
        if (camera_y >= transitioning_cam_pos_y) {
            camera_y = transitioning_cam_pos_y;
        }      
    }	
    if ((camera_x == transitioning_cam_pos_x) && (camera_y == transitioning_cam_pos_y)) { 
        return TRUE;
    }	
    return FALSE;
}


uint8_t transition_player_to(void) BANKED {	
    if ((PLAYER.pos.x == transitioning_player_pos_x) && (PLAYER.pos.y == transitioning_player_pos_y)) {
        return TRUE;
    }
    
    UINT16 oldPlayerPosX = PLAYER.pos.x + SCROLL_CAM_X;
    UINT16 oldPlayerPosY = PLAYER.pos.y + SCROLL_CAM_Y;
    UINT16 newPlayerPosX = transitioning_player_pos_x + SCROLL_CAM_X;
    UINT16 newPlayerPosY = transitioning_player_pos_y + SCROLL_CAM_Y;
    
    if (oldPlayerPosX > newPlayerPosX) {
        // Move left
        oldPlayerPosX -= SCROLL_PLAYER_SPEED;
        if (oldPlayerPosX <= newPlayerPosX) {
            oldPlayerPosX = newPlayerPosX;
        }
        } else if (oldPlayerPosX < newPlayerPosX) {
        // Move right
        oldPlayerPosX += SCROLL_PLAYER_SPEED;
        if (oldPlayerPosX >= newPlayerPosX) {
            oldPlayerPosX = newPlayerPosX;
        }        
    }
    
    if (oldPlayerPosY > newPlayerPosY) {
        // Move up
        oldPlayerPosY -= SCROLL_PLAYER_SPEED;
        if (oldPlayerPosY <= newPlayerPosY) {
            oldPlayerPosY = newPlayerPosY;
        }        
        } else if (oldPlayerPosY < newPlayerPosY) {
        // Move down
        oldPlayerPosY += SCROLL_PLAYER_SPEED;
        if (oldPlayerPosY >= newPlayerPosY) {
            oldPlayerPosY = newPlayerPosY;
        }      
    }
    
    PLAYER.pos.x = oldPlayerPosX - SCROLL_CAM_X;
    PLAYER.pos.y = oldPlayerPosY - SCROLL_CAM_Y;
    
    if ((PLAYER.pos.x == transitioning_player_pos_x) && (PLAYER.pos.y == transitioning_player_pos_y)) {
        return TRUE;
    }
    return FALSE;
}

void set_neighbour_scene(SCRIPT_CTX * THIS) OLDCALL BANKED {
    uint8_t scene_bank = *(uint8_t *) VM_REF_TO_PTR(FN_ARG0);
    const scene_t * scene = *(scene_t **) VM_REF_TO_PTR(FN_ARG1);
    uint8_t direction = *(uint8_t *)VM_REF_TO_PTR(FN_ARG2);
    uint8_t rounding = *(uint8_t *)VM_REF_TO_PTR(FN_ARG3);
    enable_transition_to_scene();	
    if (rounding){
        round_position_flags |= direction;
    }
    if (direction == DIRECTION_UP){
        up_scene.bank = scene_bank;
        up_scene.ptr = (void *)scene;
    } else if (direction == DIRECTION_RIGHT){
        right_scene.bank = scene_bank;
        right_scene.ptr = (void *)scene;
    } else if (direction == DIRECTION_DOWN){
        down_scene.bank = scene_bank;
        down_scene.ptr = (void *)scene;
    } else if (direction == DIRECTION_LEFT){
        left_scene.bank = scene_bank;
        left_scene.ptr = (void *)scene;
    }
}