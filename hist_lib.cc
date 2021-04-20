#include "../abstract_hardware_model.h"
#include "hist_network.h"

int MAX( int A, int B ){
	if( A >= B )
		return A;
	return B;
}
int MIN( int A, int B ){
	if( A < B )
		return A;
	return B;
}
int AB( int val ){
	if( val >= 0 )
		return val;
	return -val;
}

new_addr_type hist_key( new_addr_type addr ){
	return addr >> hist_line_sz_log2;
}
unsigned hist_home( new_addr_type addr ){
	return hist_key(addr) % total_SM;
}

int mesh_dx( int cur, int des )
{
	cur %= SM_sqrt;
	des %= SM_sqrt;
	return des - cur;
}
int mesh_dy( int cur, int des )
{
	cur /= SM_sqrt;
	des /= SM_sqrt;
	return des - cur;
}
int mesh_dist( int src, int des )
{
	return AB(mesh_dx(src, des)) + AB(mesh_dy(src, des));
}

int mesh_next( int src, int des )
{
	int dx = mesh_dx(src, des);
	int dy = mesh_dy(src, des);
	int curX = src % SM_sqrt;
	int curY = src / SM_sqrt;
	
	if( dx > 0 ){
		curX++;
	}
	else if( dx < 0 ){
		curX--;
	}
	else if( dy > 0 ){
		curY++;
	}
	else if( dy < 0 ){
		curY--;
	}
	else{
		assert( dx == 0 );
		assert( dy == 0 );
	}
	
	return (curY*SM_sqrt) + curX;
}

bool mesh_in_range( int sid, new_addr_type addr, int range )
{
	bool ret;
	int home = hist_home( addr );
	int *distance = new int[total_SM];
	int *counter  = new int[total_SM];
	int count = 0;
	
	for( int i=0; i<total_SM; i++ ){
		distance[i] = mesh_dist( i, home );
	}
	for( int dist = 0; dist <= 2*SM_sqrt; dist++ ){
		for( int SM = 0; SM < total_SM; SM++ ){
			if( distance[SM] == dist ){
				counter[SM] = count;
				count++;
			}
		}
	}
	
	ret = counter[sid] < range;
	delete distance;
	delete counter;
	return ret;
}

/*
int torus_x( int cur, int des )
{
	int ori_cur, ori_des;
	int east = 0;
	int west = 0;
	
	ori_cur = cur = cur % SM_sqrt;
	ori_des = des = des % SM_sqrt;
	
	// count east
	while( cur != des ){
		cur = (cur + 1) % SM_sqrt;
		east++;
	}

	// count west
	cur = ori_cur;
	while( cur != des ){
		if( cur == 0 )
			cur = SM_sqrt-1;
		else
			cur--;
		west--;
	}
	
	if( east <= AB(west) )
		return east;
	return west;
}

int torus_y( int cur, int des )
{
	int ori_cur, ori_des;
	int north = 0;
	int south = 0;
	
	ori_cur = cur = cur / SM_sqrt;
	ori_des = des = des / SM_sqrt;
	
	// count south
	while( cur != des ){
		cur = (cur + 1) % SM_sqrt;
		south++;
	}
	
	// count north
	cur = ori_cur;
	while( cur != des ){
		if( cur == 0 )
			cur = SM_sqrt-1;
		else
			cur--;
		north--;
	}
	
	if( south <= AB(north) )
		return south;
	return north;
}

int torus_distance( int A, int B ){
	return AB(torus_x(A,B)) + AB(torus_y(A,B));
}

int torus_next( int cur, int des )
{
	int dx = torus_x( cur, des );
	int dy = torus_y( cur, des );
	int curX = cur % SM_sqrt;
	int curY = cur / SM_sqrt;
	
	if( dx < 0 ){
		if( curX == 0 )
			curX = SM_sqrt-1;
		else
			curX--;
	}
	else if( dx > 0 ){
		curX++;
		curX %= SM_sqrt;
	}
	else if( dy < 0 ){
		if( curY == 0 )
			curY = SM_sqrt-1;
		else
			curY--;
	}
	else if( dy > 0 ){
		curY++;
		curY %= SM_sqrt;
	}
	
	return (curY*SM_sqrt) + curX;
}

void torus_tracrt( int cur, int des )
{
	if( cur == des ){
		printf(" ->%d Arrived!\n", cur);
	}
	else{
		printf(" ->%d", cur);
		torus_tracrt( torus_next(cur, des), des);
	}
}

bool torus_in_range( mem_fetch *mf, unsigned range )
{
	unsigned miss = mf->get_sid();
	unsigned home = hist_home(mf->get_addr());
	int SM, distance = 0;
	
	assert( range <= total_SM );
	while( range > 0 ){
		for( SM=0; SM < total_SM && range > 0; SM++ ){
			if( distance == torus_distance(home, SM) ){
				if( SM == miss )
					return true;
				range--;
			}
		}
		distance++;
	}
	return false;
}
*/
