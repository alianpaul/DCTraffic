#ifndef MATRIX_RADAR_CONFIG_H
#define MATRIX_RADAR_CONFIG_H

namespace ns3
{

static const size_t MTX_COUNT_TABLE_SIZE_IN_BLOCK = 1000;
static const size_t MTX_NUM_BLOCK                 = 10;
static const size_t MTX_NUM_IDX                   = 4;
  
static const size_t MTX_FLOW_FILTER_SIZE = 400000000;
static const size_t MTX_NUM_FLOW_HASH    = 20;

static const float MTX_PERIOD = 0.5f; //end 0.5s
static const float MTX_END_TIME = 0;

static const bool  IS_OFFLINE_DECODE = false; //
  
}

#endif
