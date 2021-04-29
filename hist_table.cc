#include <stdio.h> 
#include "gpu-sim.h"

#define total_entry (hist_nset*hist_assoc)

class hist_table *hist_tb;

struct hist_entry_t
{
    hist_entry_t(){
        m_status = HIST_INVALID;
        m_key    = 0;
		m_flag   = new bool[total_SM];

		for( int i=0; i < total_SM; i++){
			m_flag[i] = false;
		}

        m_alloc_time  = 0;
        m_access_time = 0;
        m_fill_time   = 0;
    }
    void allocate( mem_fetch *mf ){
        m_status = HIST_WAIT;
        m_key    = hist_key(mf->get_addr());
		
		for( int i=0; i < total_SM; i++){
			m_flag[i] = false;
		}

        m_alloc_time  = cur_time;
        m_access_time = cur_time;
        m_fill_time   = 0;
    }
	void uptime(){
		m_access_time = cur_time;
	}
	unsigned count(){
		unsigned counter = 0;
		for( int i=0; i < total_SM; i++){
			if( m_flag[i] == true )
				counter++;
		}
		return counter;
	}
	void print(){
		printf( "      HIST >> | %2u | %#010x | ", m_status, m_key);
		for( int i; i < total_SM; i++){
			if( m_flag[i] )
				printf("1");
			else
				printf("0");
		}
		printf(" [%u]\n", count());
	}

    // HIST entry fields (veriables) //
    hist_entry_status m_status;
    new_addr_type     m_key;
	bool             *m_flag;

    // For Replacement Policy
    unsigned m_alloc_time;
    unsigned m_access_time;
    unsigned m_fill_time;
    
    std::list<mem_fetch*> m_wait_list;
};

hist_table::hist_table( unsigned nset, unsigned assoc, unsigned range )
{
	printf("HIST >> hist_table -> Set %u Assoc %u Range %u\n", nset, assoc, range);
	hist_nset  = nset;
	hist_assoc = assoc;
	hist_range = range;
	
	m_table = new hist_entry_t*[total_SM];
	for( int i=0; i < total_SM; i++ ){
		printf("   HIST >> Table[%d]\n", i);
		m_table[i] = new hist_entry_t[total_entry];
	//	print( i );
	}
}

void hist_table::print( int SM ) const
{
	for( int i=0; i < hist_nset; i++ ){
		for( int j=0; j < hist_assoc; j++){
			m_table[SM][(i*hist_assoc)+j].print();
		}
		printf("      HIST >> ---------------\n");
	}
}

unsigned hist_table::hist_set( new_addr_type addr ) const {
	return hist_key(addr) % hist_nset;
}

hist_request_status hist_table::probe( new_addr_type addr ) const {
	unsigned tmp_idx;
	return probe( addr, tmp_idx );
}

enum hist_request_status hist_table::probe( new_addr_type addr, unsigned &idx ) const 
{
    new_addr_type tag = hist_key(  addr );	// Pisacha: HIST Key from address (Tag)
	unsigned home     = hist_home( addr );	// Pisacha: get HOME from address
    unsigned set_idx  = hist_set(  addr );	// Pisacha: Index HIST from address
	unsigned index;
	
    unsigned invalid_line = (unsigned)-1;	// Pisacha: This is MAX UNSIGNED
    unsigned valid_line   = (unsigned)-1;	// Pisacha: This is MAX UNSIGNED
    unsigned valid_time   = (unsigned)-1;	// Pisacha: This is MAX UNSIGNED
	
    unsigned count_line   = (unsigned)-1;	// Pisacha: This is MAX UNSIGNED
    unsigned count_num    = (unsigned)-1;	// Pisacha: This is MAX UNSIGNED
    unsigned count_time   = (unsigned)-1;	// Pisacha: This is MAX UNSIGNED
	unsigned max_time = 0;
	
	for( index = set_idx*hist_assoc; index < (set_idx+1)*hist_assoc; index++ )
	{
		hist_entry_t       *line = &m_table[home][index];
		new_addr_type        key = line->m_key;
		hist_entry_status status = line->m_status;
		
		if( line->m_access_time > max_time ) // Newest
			max_time = line->m_access_time;
			
		if( tag == key ){
			switch( status ){
				case HIST_WAIT:
					idx = index;
					return HIST_HIT_WAIT;
					break;
				case HIST_READY:
					idx = index;
					return HIST_HIT_READY;
					break;
				case HIST_INVALID:
					idx = index;
					return HIST_MISS;
					break;
				default:
					break;
			}
		}
		else{
			if( status == HIST_INVALID ){
				invalid_line = index;
			}
			if( status == HIST_READY ){
				if( line->count() <= 2 &&				// Count <= 2
				    line->count() <= count_num &&		// Least Count
					line->m_access_time < count_time )	// Older
				{
					count_line = index;
					count_num  = line->count();
					count_time = line->m_access_time;
				}
				if( line->m_access_time < valid_time ){ // Older
					valid_time = line->m_access_time;
					valid_line = index;
				}
			}
		}
	}
	
	if( invalid_line != (unsigned)-1 ){
		idx = invalid_line;
		return HIST_MISS;
	}
    if( count_line != (unsigned)-1 ){
		idx = count_line;
		return HIST_MISS;
	}
	if( valid_line != (unsigned)-1 ){
		idx = valid_line;
		return HIST_MISS;
	}
	
	idx = (unsigned)-1;
	return HIST_FULL;
}

void hist_table::uptime( mem_fetch *mf )
{
	unsigned idx;
	unsigned home = hist_home( mf->get_addr() );
	
	assert( probe( mf->get_addr(), idx ) != HIST_MISS );
	assert( probe( mf->get_addr(), idx ) != HIST_FULL );
	
	m_table[home][idx].uptime();
}

void hist_table::allocate( mem_fetch *mf )
{
	unsigned idx;
	unsigned home = hist_home( mf->get_addr() );
	
	assert( mesh_in_range( mf->get_sid(), mf->get_addr(), hist_range ) );
	assert( probe( mf->get_addr(), idx ) == HIST_MISS );
	
	m_table[home][idx].allocate( mf );
//	printf("      HIST >> %llu__%u_%u %s\n", hist_key(mf->get_addr()), home, idx, __FUNCTION__);
}

void hist_table::add_flag( mem_fetch *mf )
{
	unsigned idx;
	unsigned home = hist_home( mf->get_addr() );
	
	assert( mesh_in_range( mf->get_sid(), mf->get_addr(), hist_range ) );
	assert( probe( mf->get_addr(), idx ) != HIST_MISS );
	assert( probe( mf->get_addr(), idx ) != HIST_FULL );
	
	m_table[home][idx].m_flag[mf->get_sid()] = true;
//	printf("      HIST >> %llu__%u_%u %s count = %u\n", hist_key(mf->get_addr()), home, idx, __FUNCTION__, m_table[home][idx].count());
}

void hist_table::add_mf( mem_fetch *mf )
{
	unsigned idx;
	unsigned home = hist_home( mf->get_addr() );
	
//	assert( mesh_in_range( mf->get_sid(), mf->get_addr(), hist_range ) );
	assert( probe( mf->get_addr(), idx ) == HIST_HIT_WAIT );
	
	m_table[home][idx].m_wait_list.push_back( mf );
//	printf("      HIST >> %llu__%u_%u %s count = %u\n", hist_key(mf->get_addr()), home, idx, __FUNCTION__, m_table[home][idx].m_wait_list.size());
}

void hist_table::fill_mf( new_addr_type addr )
{
	unsigned idx;
	unsigned home = hist_home( addr );
	
	assert( probe( addr, idx ) == HIST_HIT_WAIT );
	m_table[home][idx].m_status = HIST_READY;
//	printf("      HIST >> %llu__%u_%u %s HIST_READY count %u\n", hist_key(mf->get_addr()), home, idx, __FUNCTION__, m_table[home][idx].count());

	while( m_table[home][idx].m_wait_list.size() > 0 )
	{
		mem_fetch *wait_mf = m_table[home][idx].m_wait_list.front();
		
		hist_nw->send_flit( home, wait_mf->get_sid() );
		wait_mf->hist_set_type( HIST_DATA );
		wait_mf->hist_set_src( home );
		wait_mf->hist_set_dst( wait_mf->get_sid() );
		wait_mf->hist_set_stmp( cur_time );
		hist_nw->hist_out_fush( home, wait_mf );
		
		m_table[home][idx].m_wait_list.pop_front();
	//	printf("      HIST >> %llu__%u_%u %s count = %u\n", hist_key(mf->get_addr()), home, idx, __FUNCTION__, m_table[home][idx].m_wait_list.size());
	}
}

void hist_table::invalidate( int sid, new_addr_type addr )
{
	unsigned idx;
	unsigned home = hist_home( addr );

	assert( mesh_in_range( sid, addr, hist_range ) );
	if( probe( addr, idx ) != HIST_HIT_READY )
		return;

//	printf("      HIST >> %s count %u\n", __FUNCTION__, m_table[home][idx].count());
	m_table[home][idx].m_flag[sid] = false;
	if( m_table[home][idx].count() == 0 ){
		m_table[home][idx].m_status = HIST_INVALID;
	//	printf("      HIST >> %s HIST_INVALID\n", __FUNCTION__);
	}
}

int hist_table::near_fwd( mem_fetch *mf, int target )
{
	int min_SM;
	unsigned idx, distance, min_dist;
	unsigned home = hist_home( mf->get_addr() );
	
//	assert( mesh_in_range( mf->get_sid(), mf->get_addr(), hist_range ) );
	assert( probe( mf->get_addr(), idx ) == HIST_HIT_READY );
//	printf("      HIST >> %llu__%u_%u %s available = %u\n", hist_key(mf->get_addr()), home, idx, __FUNCTION__, m_table[home][idx].count());
	
	min_SM   = total_SM;
	min_dist = total_SM;
	for( int SM = 0; SM < total_SM; SM++ ){
		if( m_table[home][idx].m_flag[SM] == true ){
			distance = mesh_dist( SM, target );
			if( distance <= min_dist ){
				min_dist = distance;
				min_SM   = SM;
			}
		}
	}
	assert( min_SM >= 0 );
	assert( min_SM < total_SM );
	assert( min_dist < total_SM );
	return min_SM;
}
