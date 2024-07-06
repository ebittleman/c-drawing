#ifndef INCLUDE_OBJECTS_H
#define INCLUDE_OBJECTS_H

#include <stddef.h>

#include "arena.h"

typedef unsigned int objid;

void init_motion_tables(arena *a);
objid new_object(float initial[12]);
void calc_next_pos(size_t id, float dt, float values[15]);
void update_velocity(objid id, float vel[3]);
void update_position(objid id, float vel[3]);
void commit_postion(objid id);
void update_object(objid id, float dt);

#endif

#ifdef OBJECTS_IMPLEMENTATION

static float *acceleration_table;
static float *velocity_table;
static float *position_table;
static float *dimension_table;

objid new_object(float initial[12]) {
  static size_t next_id = 0;

  acceleration_table[next_id * 3 + 0] = initial[0];
  acceleration_table[next_id * 3 + 1] = initial[1];
  acceleration_table[next_id * 3 + 2] = initial[2];

  velocity_table[next_id * 3 + 0] = initial[3];
  velocity_table[next_id * 3 + 1] = initial[4];
  velocity_table[next_id * 3 + 2] = initial[5];

  position_table[next_id * 3 + 0] = initial[6];
  position_table[next_id * 3 + 1] = initial[7];
  position_table[next_id * 3 + 2] = initial[8];

  dimension_table[next_id * 3 + 0] = initial[9];
  dimension_table[next_id * 3 + 1] = initial[10];
  dimension_table[next_id * 3 + 2] = initial[11];

  return ++next_id;
}

void init_motion_tables(arena *a) {
  acceleration_table = arena_alloc(a, 10000 * 3 * sizeof(float));
  velocity_table = arena_alloc(a, 10000 * 3 * sizeof(float));
  position_table = arena_alloc(a, 10000 * 3 * sizeof(float));
  dimension_table = arena_alloc(a, 10000 * 3 * sizeof(float));
}

void calc_next_pos(size_t id, float dt, float values[15]) {
  //  float ax, ay, az, vx, vy, vz, px, py, pz;

  float *current_acc = &acceleration_table[id * 3];
  float *current_vel = &velocity_table[id * 3];
  float *current_pos = &position_table[id * 3];
  float *current_dim = &dimension_table[id * 3];

  values[0] = current_acc[0];
  values[1] = current_acc[1];
  values[2] = current_acc[2];

  values[3] = values[0] * dt + current_vel[0];
  values[4] = values[1] * dt + current_vel[1];
  values[5] = values[2] * dt + current_vel[2];

  values[6] = current_pos[0];
  values[7] = current_pos[1];
  values[8] = current_pos[2];

  values[9] = values[3] * dt + values[6];
  values[10] = values[4] * dt + values[7];
  values[11] = values[5] * dt + values[8];

  values[12] = current_dim[0];
  values[13] = current_dim[1];
  values[14] = current_dim[2];
}

void update_velocity(objid id, float vel[3]) {
  float *current_vel = &velocity_table[id * 3];
  current_vel[0] = vel[0];
  current_vel[1] = vel[1];
  current_vel[2] = vel[2];
}

void update_position(objid id, float pos[3]) {
  float *current_pos = &position_table[id * 3];
  current_pos[0] = pos[0];
  current_pos[1] = pos[1];
  current_pos[2] = pos[2];
}

void commit_postion(objid id) {
  float *current_pos = &position_table[id * 3];
  float *next_pos = &dimension_table[id * 3];

  current_pos[0] = next_pos[0];
  current_pos[1] = next_pos[1];
  current_pos[2] = next_pos[2];
}

void update_object(objid id, float dt) {
  // pos = load_object_position(id)
  // pos_func = load_pos_func(id)
  // new_pos = pos_fun(pos, dt)
  //
  // boundry = load_object_boundries(id)
  // new_boundry = translate(boundry, new_pos)
  // collision_info = collision_detection(new_boundry, &collisions)
  //
  // collision_handler = lookup_collision_handler(id)
  // new_pos = collision_handler(id, collision_info)
  //
  // assign_ new_position(id, new_pos)
}

#endif
