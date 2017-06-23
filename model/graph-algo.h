#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdint.h>
#include <limits>
#include <vector>
#include <list>
#include <map>

namespace ns3 {

class Graph
{
  
public:
  /*The Adjacent List Node;
   */
  struct AdjNode_t
  {
    uint16_t from_port;
    uint16_t to_port;
    int      id;
    int      weight;

    AdjNode_t():from_port(0), to_port(0), id(0), weight(0)
    {}

    
  };
  typedef std::vector<AdjNode_t>       AdjListEntry_t;
  typedef std::vector<AdjListEntry_t>  AdjList_t;

  /*PathTable Entry Type
   */
#define MAX_DIST std::numeric_limits<int>::max()
  
  struct PathTableEntry_t
  {
    PathTableEntry_t():
      known(false),
      dist(MAX_DIST)
    {
    }
    bool known;
    int  dist;
    std::vector<int>  before;
  };
  typedef std::vector<PathTableEntry_t>  PathTable_t;

  struct Edge_t 
  {
    int src;
    int dst;
    uint16_t spt;
    uint16_t dpt;
  };

  typedef std::vector<Edge_t> Path_t;

  Graph();
  ~Graph();

  Path_t  GetPath (int from, int to) const;

  void BFS (int root);

  void SetAdjList(const AdjList_t& adjList);
  
private:

  /*Build all nodes' Path table.
   */
  void BuildPaths();

  /*Find all shortest path
   */
  
  
  /*Use Dijkstra algo to compute the short path from node root to other nodes, 
   *store the paths in root's path table.
   */
  /*
  void Dijkstra (int root);
  */

  /*Find the unknown node in root's pathTable with the shortest smallest dist,
   *If the founded unknown node's dist == MAX_DIST, we'll say that this node is 
   *not exist.
   */
  int  FindSmallestUnknown (PathTable_t& pathTable);
  
  AdjList_t                m_adjList;
  std::vector<PathTable_t> m_paths;
  int                      m_numNodes;
  mutable std::map<int, Path_t>    m_pathCache;
  //Path cache, the first time we GetPath(in easy controller),
  //the Path is generated randomly.
  //Then the cache is stored to ensured consistent.(is the same in FlowDecoder)
};

}

#endif
