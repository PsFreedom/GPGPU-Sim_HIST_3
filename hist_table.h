#include "../abstract_hardware_model.h"

extern class hist_table *hist_tb;

enum hist_entry_status {
    HIST_INVALID,
    HIST_WAIT,
    HIST_READY
};
enum hist_request_status {
    HIST_HIT_WAIT,
    HIST_HIT_READY,
    HIST_MISS,
    HIST_FULL
};
struct hist_entry_t;

class hist_table{
public:
	hist_table( unsigned nset, unsigned assoc, unsigned range );
	void print( int SM ) const;

	hist_request_status probe( new_addr_type addr ) const;
    hist_request_status probe( new_addr_type addr, unsigned &idx ) const;
	
	void   uptime( mem_fetch *mf );
	void allocate( mem_fetch *mf );
	void add_flag( mem_fetch *mf );
	void   add_mf( mem_fetch *mf );
	void  fill_mf( new_addr_type addr );
	void invalidate( int sid, new_addr_type addr );

	int  near_fwd( mem_fetch *mf, int target );
    void shortest_trip( mem_fetch *mf );
	unsigned get_range() const{ return hist_range; };
	unsigned hist_set( new_addr_type addr ) const;
	
private:
	unsigned hist_nset;
	unsigned hist_assoc;
	unsigned hist_range;
	hist_entry_t **m_table;
};
