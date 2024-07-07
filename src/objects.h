#ifndef INCLUDE_OBJECTS_H
#define INCLUDE_OBJECTS_H

#include <stddef.h>

#include "arena.h"

typedef unsigned int objid;

void init_motion_tables(arena *a, size_t max_objects);
objid new_object(float initial[12]);

void calc_next_pos(size_t id, float dt, float values[15]);

void update_acceleration(objid id, float vel[3]);
void update_velocity(objid id, float vel[3]);
void update_position(objid id, float vel[3]);

#endif

#ifdef OBJECTS_IMPLEMENTATION

static float *acceleration_table;
static float *velocity_table;
static float *position_table;
static float *dimension_table;

objid new_object(float initial[12]) {
  static size_t next_id = 0;

  size_t offset = next_id * 3;

  acceleration_table[offset + 0] = initial[0];
  acceleration_table[offset + 1] = initial[1];
  acceleration_table[offset + 2] = initial[2];

  velocity_table[offset + 0] = initial[3];
  velocity_table[offset + 1] = initial[4];
  velocity_table[offset + 2] = initial[5];

  position_table[offset + 0] = initial[6];
  position_table[offset + 1] = initial[7];
  position_table[offset + 2] = initial[8];

  dimension_table[offset + 0] = initial[9];
  dimension_table[offset + 1] = initial[10];
  dimension_table[offset + 2] = initial[11];

  return ++next_id;
}

void init_motion_tables(arena *a, size_t max_objects) {
  size_t num_bytes = max_objects * 3 * sizeof(float);
  acceleration_table = arena_alloc(a, num_bytes);
  velocity_table = arena_alloc(a, num_bytes);
  position_table = arena_alloc(a, num_bytes);
  dimension_table = arena_alloc(a, num_bytes);
}

void calc_next_pos(size_t id, float dt, float values[15]) {

  size_t offset = id * 3;
  float *current_acc = &acceleration_table[offset];
  float *current_vel = &velocity_table[offset];
  float *current_pos = &position_table[offset];
  float *current_dim = &dimension_table[offset];

  // current acceleration
  values[0] = current_acc[0];
  values[1] = current_acc[1];
  values[2] = current_acc[2];

  // next velocity
  values[3] = values[0] * dt + current_vel[0];
  values[4] = values[1] * dt + current_vel[1];
  values[5] = values[2] * dt + current_vel[2];

  // current position
  values[6] = current_pos[0];
  values[7] = current_pos[1];
  values[8] = current_pos[2];

  // next position
  values[9] = values[3] * dt + values[6];
  values[10] = values[4] * dt + values[7];
  values[11] = values[5] * dt + values[8];

  // current dimension
  values[12] = current_dim[0];
  values[13] = current_dim[1];
  values[14] = current_dim[2];
}

void update_acceleration(objid id, float vel[3]) {
  float *current_acc = &acceleration_table[id * 3];
  current_acc[0] = vel[0];
  current_acc[1] = vel[1];
  current_acc[2] = vel[2];
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

#endif
