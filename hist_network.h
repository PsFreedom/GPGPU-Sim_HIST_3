#include "mem_fetch.h"

#define total_Q (SM_sqrt*SM_sqrt)
#define Q_ports 5

extern unsigned SM_sqrt;
extern unsigned hist_line_sz;
extern unsigned hist_line_sz_log2;

extern unsigned total_SM;
extern unsigned total_cluster;
extern unsigned SM_per_cluster;

extern class hist_network *hist_nw;

class tag_array;
class hist_network {
public:
	hist_network( unsigned flit, unsigned queue );

	void miss_init( int core_id, std::list<mem_fetch*> *queue );
	void fifo_init( int core_id, std::list<mem_fetch*> *queue );
    void tagA_init( int core_id, tag_array *ptr );
	
	void miss_queue_push( int core_id, mem_fetch *mf );
	void fifo_queue_push( int core_id, mem_fetch *mf );
	bool hist_out_push( int core_id, mem_fetch *mf );
	bool hist_out_fush( int core_id, mem_fetch *mf );
	bool hist_out_full( int core_id, int dest_id );
	int  hist_out_next( int core_id, int dest_id );
	
	mem_fetch* data_queue(std::list<mem_fetch*> **queue, int SM, int QID);
	
	void send_flit( int from, int to );
	void send_inv( int from, new_addr_type addr );
	void hist_cycle();
	bool process_arrived_mf( mem_fetch *mf );

	void stat_print();
	void print_queue( int SM );
	void print_queue_all();
	
private:
	std::list<mem_fetch*> **hist_out;
	std::list<mem_fetch*> **hist_in;
	
	std::list<mem_fetch*> **sm_miss_queue;
	std::list<mem_fetch*> **sm_fifo_queue;
    
	const unsigned n_flit;
	const unsigned n_queue;
    tag_array **sm_tag_array;
	
	unsigned max_nw_time;
	unsigned long long stat_probe;
	unsigned long long stat_insert;
	unsigned long long stat_merge;
	unsigned long long stat_full;
	unsigned long long stat_source;
	unsigned long long stat_source_hit;
	unsigned long long stat_source_miss;
	unsigned long long tot_nw_time;
	unsigned long long tot_nw_count;
};
