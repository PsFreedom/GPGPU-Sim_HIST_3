#include <math.h> 
#include "gpu-sim.h"

unsigned SM_sqrt;
unsigned hist_line_sz;
unsigned hist_line_sz_log2;

unsigned total_SM;
unsigned total_cluster;
unsigned SM_per_cluster;


class hist_network *hist_nw;

hist_network::hist_network( unsigned flit, unsigned queue ): n_flit( flit ), n_queue( queue )
{
	SM_sqrt = ceil(sqrt(total_SM));
	printf("HIST >> hist_network -> Cluster %u SM %u(%u) SM/cluster %u\n", total_cluster, total_SM, SM_sqrt, SM_per_cluster);
	printf("HIST >> hist_network -> flit %u queue %u\n", flit, queue );

	/// Init HIST queue
	hist_out = new std::list<mem_fetch*>*[total_Q];
	hist_in  = new std::list<mem_fetch*>*[total_Q];
	for( int i =0; i < total_Q; i++ ){
		printf("   HIST >> Initialized hist_queue[%d]\n", i);
		hist_in[i]  = new std::list<mem_fetch*>[Q_ports];
		hist_out[i] = new std::list<mem_fetch*>[Q_ports];
	}

	/// Pointers to original miss/fifo queue
	sm_miss_queue = new std::list<mem_fetch*>*[total_SM];
	sm_fifo_queue = new std::list<mem_fetch*>*[total_cluster];
	sm_tag_array  = new tag_array*[total_SM];
	
	/// Init Status Variables
	stat_probe  = 0;
	stat_insert = 0;
	stat_merge  = 0;
	stat_full   = 0;
	stat_source = 0;
	stat_source_hit = 0;
	stat_source_miss = 0;
	max_nw_time = 0;
	tot_nw_time = 0;
	tot_nw_count = 0;
}

void hist_network::miss_init( int core_id, std::list<mem_fetch*> *queue )
{
	printf("   HIST >> %s %d\n", __FUNCTION__, core_id);
	sm_miss_queue[core_id] = queue;
}

void hist_network::fifo_init( int core_id, std::list<mem_fetch*> *queue )
{
	printf("   HIST >> %s %d\n", __FUNCTION__, core_id);
	sm_fifo_queue[core_id] = queue;
}

void hist_network::tagA_init( int core_id, tag_array *ptr )
{
	printf("   HIST >> %s %d\n", __FUNCTION__, core_id);
	sm_tag_array[core_id] = ptr;
}

void hist_network::miss_queue_push( int core_id, mem_fetch *mf )
{
	//printf("   HIST >> %s %d\n", __FUNCTION__, core_id);
	sm_miss_queue[core_id]->push_back( mf );
}

void hist_network::fifo_queue_push( int core_id, mem_fetch *mf )
{
	//printf("   HIST >> %s %d\n", __FUNCTION__, core_id);
	sm_fifo_queue[core_id/SM_per_cluster]->push_back( mf );
}

bool hist_network::hist_out_push( int core_id, mem_fetch *mf )
{
	int next_id = hist_out_next( core_id, mf->hist_dst() );
	if( hist_out[core_id][next_id].size() < n_queue ){
		hist_out[core_id][next_id].push_back( mf );
		return true;
	}
	return false;
}

bool hist_network::hist_out_fush( int core_id, mem_fetch *mf )
{
	int next_id = hist_out_next( core_id, mf->hist_dst() );
	hist_out[core_id][next_id].push_back( mf );
	return true;
}

bool hist_network::hist_out_full( int core_id, int dest_id)
{
	int next_id = hist_out_next( core_id, dest_id );
	return hist_out[core_id][next_id].size() >= n_queue;
}

int hist_network::hist_out_next( int core_id, int dest_id )
{
    int dX = mesh_dx( core_id, dest_id );
    int dY = mesh_dy( core_id, dest_id );

    if( dX > 0 )
		return 0;
    if( dX < 0 )
		return 1;
    if( dY > 0 )
		return 2;
    if( dY < 0 )
		return 3;
	
	// Arrived
	assert( dX == 0 && dY == 0 );
	return 4;
}

mem_fetch* hist_network::data_queue(std::list<mem_fetch*> **queue, int SM, int QID)
{
	std::list<mem_fetch*>::iterator it = queue[SM][QID].begin();
	
	while( it != queue[SM][QID].end() )
	{
		if( (*it)->hist_type() == HIST_DATA ||
			(*it)->hist_type() == HIST_FLIT )
		{
			return *it;
		}
		it++;
	}
	return *(queue[SM][QID].begin());
}

void hist_network::send_flit( int from, int to )
{
	for( unsigned i=1; i < n_flit; i++ )
	{
		mem_fetch *mf = (mem_fetch*)malloc(sizeof(mem_fetch));
		mf->hist_set_type( HIST_FLIT );
		mf->hist_set_src( from );
		mf->hist_set_dst( to );
		mf->hist_set_adr( 0 );
        mf->hist_set_stmp( cur_time );
		hist_out_fush( from, mf );
	}
}

void hist_network::send_inv( int from, new_addr_type addr )
{
	if( from < 0 )
		return;
	if( !mesh_in_range( from, addr, hist_tb->get_range() ) )
		return;
	if( hist_out_full(from, hist_home(addr)) )
		return;
	
	mem_fetch *mf = (mem_fetch*)malloc(sizeof(mem_fetch));
	mf->hist_set_type( HIST_INVALIDATE );
	mf->hist_set_src( from );
	mf->hist_set_dst( hist_home(addr) );
	mf->hist_set_adr( addr );
    mf->hist_set_stmp( cur_time );
	hist_out_fush( from, mf );
}

void hist_network::hist_cycle()
{
	int SM, NXT, qid;
	for( SM = 0; SM < total_Q; SM++ )
	{	
		for( qid = 0; qid < Q_ports-1; qid++ ){
			if( !hist_out[SM][qid].empty() )
			{
				mem_fetch *mf = data_queue( hist_out, SM, qid );
				NXT = mesh_next(SM, mf->hist_dst());
				
				if( hist_in[NXT][qid].size() < n_queue ){
					hist_in[NXT][qid].push_back(mf);
					hist_out[SM][qid].remove(mf);
				}
			}
		}
		
		for( qid = 0; qid < Q_ports-1; qid++ ){
			if( !hist_in[SM][qid].empty() )
			{
				mem_fetch *mf = data_queue( hist_in, SM, qid );
				if( hist_out_push(SM, mf) ){
					hist_in[SM][qid].remove(mf);
				}
			}
		}
		
		assert( qid == Q_ports-1 );
		if( !hist_out[SM][qid].empty() ){
			mem_fetch *mf = data_queue( hist_out, SM, qid );
			hist_out[SM][qid].remove(mf);
			if( process_arrived_mf(mf) )
				free( mf );
		}
	}
//	print_queue_all();
}

bool hist_network::process_arrived_mf( mem_fetch *mf )
{
	bool ret = false;
	int   SM = mf->hist_dst();
	enum hist_type_t mf_hist_type = mf->hist_type();

	if( max_nw_time < cur_time - mf->hist_time() )
		max_nw_time = cur_time - mf->hist_time();
	tot_nw_time += cur_time - mf->hist_time();
	tot_nw_count++;
	
	switch( mf_hist_type ){
		case HIST_PROBE:
		//	printf("HIST >> SM[%2u] received HIST_PROBE from %d\n", SM, mf->get_sid() );
			hist_request_status probe_res;
			probe_res = hist_tb->probe(mf->get_addr());
			
			stat_probe++;
			switch( probe_res ){
				case HIST_MISS:
				//	printf("   HIST >> HIST_MISS %#010x\n", hist_key(mf->get_addr()));
					if( !mesh_in_range(mf->get_sid(), mf->get_addr(), hist_tb->get_range()) ){
						mf->hist_set_type( HIST_REJECT_FULL );
						miss_queue_push( mf->get_sid(), mf );
						break;
					}
					
					hist_tb->allocate( mf );
					hist_tb->add_flag( mf );
					
					mf->hist_set_type( HIST_REJECT );
					miss_queue_push( mf->get_sid(), mf );
					stat_insert++;
					break;
				case HIST_HIT_WAIT:
				//	printf("   HIST >> HIST_HIT_WAIT %#010x\n", hist_key(mf->get_addr()));
					
					if( mesh_in_range(mf->get_sid(), mf->get_addr(), hist_tb->get_range()) )
						hist_tb->add_flag( mf );
					hist_tb->add_mf( mf );
					hist_tb->uptime( mf );
					stat_merge++;
					break;
				case HIST_HIT_READY:
				//	printf("   HIST >> HIST_HIT_READY\n");
					mf->hist_set_type( HIST_FORWARD );
					mf->hist_set_src( SM );
					mf->hist_set_dst( hist_tb->near_fwd(mf, mf->get_sid()) );
                    mf->hist_set_stmp( cur_time );
					hist_out_fush( SM, mf );
					
					if( mesh_in_range(mf->get_sid(), mf->get_addr(), hist_tb->get_range()) )
						hist_tb->add_flag( mf );
					hist_tb->uptime( mf );
					stat_source++;
					break;
				default:
					printf("   HIST >> HIST_FULL\n");
					assert( probe_res == HIST_FULL );
					mf->hist_set_type( HIST_REJECT_FULL );
					miss_queue_push( mf->get_sid(), mf );
					stat_full++;
					break;
			}
			break;
		case HIST_FILL:
			printf("HIST >> SM[%2u] received HIST_FILL\n", SM);
			hist_tb->fill_mf( mf->hist_adr() );
			ret = true;
			break;
		case HIST_DATA:
		//	printf("HIST >> SM[%2u] received HIST_DATA\n", SM);
			fifo_queue_push( mf->get_sid(), mf );
			break;
		case HIST_FORWARD:
			unsigned idx;
			printf("HIST >> SM[%2u] received HIST_FORWARD\n", SM);
		//	printf("   HIST >> Home %u Target %u\n", hist_home(mf->get_sid()), mf->get_sid());
			if( sm_tag_array[SM]->probe( mf->get_addr(), idx, mf ) == HIT ){
				printf("   HIST >> probe HIT\n");
				send_flit( SM, mf->get_sid() );
				mf->hist_set_type( HIST_DATA );
				mf->hist_set_src( SM );
				mf->hist_set_dst( mf->get_sid() );
				mf->hist_set_stmp( cur_time );
				hist_out_fush( SM, mf );
				stat_source_hit++;
			}
			else{
				printf("   HIST >> probe MISS\n");
				send_inv( SM, mf->get_addr() );
				mf->hist_set_type( HIST_REJECT_FULL );
				miss_queue_push( mf->get_sid(), mf );
				stat_source_miss++;
			}
			break;
		case HIST_INVALIDATE:
		//	printf("HIST >> SM[%2u] received HIST_INVALIDATE\n", SM);
			hist_tb->invalidate( mf->hist_src(), mf->hist_adr() );
			ret = true;
			break;
		default:
		//	printf("HIST >> SM[%2u] received HIST_FLIT\n", SM);
			assert( mf_hist_type == HIST_FLIT );
			ret = true;
			break;
	}
	return ret;
}

void hist_network::stat_print()
{
	printf("HIST >> Network Stat\n");
	printf("HIST >>    PROBE  %llu\n", stat_probe);
	printf("HIST >>    INSERT %llu\n", stat_insert);
	printf("HIST >>    MERGE  %llu\n", stat_merge);
	printf("HIST >>    FULL   %llu\n", stat_full);
	printf("HIST >>    SOURCE %llu\n", stat_source);
	printf("HIST >>    SRC_HT %llu\n", stat_source_hit);
	printf("HIST >>    SRC_MS %llu\n", stat_source_miss);
	printf("HIST >>    AVG_PKT_TIME %lf\n", (double)tot_nw_time / (double)tot_nw_count);
	printf("HIST >>    MAX_PKT_TIME %u\n", max_nw_time);
}

void hist_network::print_queue( int SM )
{
	assert( SM >= 0 );
	assert( SM < total_Q );
	
	printf("%2d[ ", SM );
	for( int i=0; i< Q_ports; i++ )
		printf(" %2d", hist_out[SM][i].size());
	printf(" |");
	for( int i=0; i< Q_ports; i++ )
		printf(" %2d", hist_in[SM][i].size());
	printf(" ]");
}

void hist_network::print_queue_all()
{
	printf("====================\n");
	for( int i=0; i<SM_sqrt; i++){
		printf("      ");
		for( int j=0; j<SM_sqrt; j++){
			print_queue( (i*SM_sqrt)+j );
			printf("   ");
		}
		printf("\n");
	}
	printf("====================\n");
}