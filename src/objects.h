#ifndef INCLUDE_OBJECTS_H
#define INCLUDE_OBJECTS_H

typedef unsigned int objid;

void new_object() objid;
void update(objid id, float dt);

#endif

#ifdef OBJECTS_IMPLEMENTATION

void update(objid id, float dt) {
  // pos = load_object_position(id)
  // pos_func = load_pos_func(id)
  // new_pos = pos_fun(pos, dt)
  //
  // boundry = load_object_boundries(id)
  // new_boundry = translate(boundry, new_pod)
  // num_collisions = collision_detection(new_boundry, &collisions)
  // handle collision
  // assign new position
}

#endif
