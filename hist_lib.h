#include "../abstract_hardware_model.h"

int MAX( int A, int B );
int MIN( int A, int B );
int AB( int val );

new_addr_type hist_key( new_addr_type addr );
unsigned hist_home( new_addr_type addr );

int mesh_dx( int cur, int des );
int mesh_dy( int cur, int des );
int mesh_dist( int src, int des );
int mesh_next( int src, int des );
bool mesh_in_range( int sid, new_addr_type addr, int range );

/*
int torus_x( int cur, int des );
int torus_y( int cur, int des );
int torus_distance( int A, int B );
int torus_next( int cur, int des );
bool torus_in_range( mem_fetch *mf, unsigned range );
void torus_tracrt( int cur, int des );
*/
