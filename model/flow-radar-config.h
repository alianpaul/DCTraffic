#ifndef FLOW_RADAR_CONFIG_H
#define FLOW_RADAR_CONFIG_H

namespace ns3
{

/*************Flow radar config*****************/
/* Flow Decoder config
 */
//static const float PERIOD   = 0.01; //10ms
//static const float END_TIME = 10.0;  //10s

static const float PERIOD = 1.f;
static const float END_TIME = 0.f;
  
static const size_t NUM_THREAD = 4; //4 worker thread for counter lsqr decoding

/* Flow Encoder config
 * The count table entry has been divided into NUM_COUNT_HASH sections.
 * Each section has COUNT_TABLE_SUB_SIZE entries.
 */
static const int NUM_COUNT_HASH       = 4;    //num of counter table hash function
  
static const int COUNT_TABLE_SIZE = 10 * 1000;
static const int COUNT_TABLE_SUB_SIZE = 2500;
  
static const int NUM_FLOW_HASH    = 20;        //num of flow filter hash function
static const int FLOW_FILTER_SIZE = 400000000;    //num of bits of flow filter

}
#endif
