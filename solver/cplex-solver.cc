#include <iostream>
#include <vector>

#include <ilcplex/ilocplex.h>
#include <ilconcert/iloenv.h>
ILOSTLBEGIN

#include "ns3/flow-field.h"
#include "ns3/matrix-encoder.h"

namespace ns3{

int CplexSolveMtxBlock(const MtxBlock& block, std::vector<uint16_t>& flowsSizes);

int CplexSolve(Ptr<MatrixEncoder> target, FlowInfo_t& measuredFlows)
{
  std::cout << "CplexSolve switch " << target->GetID() << std::endl;
  int status = 0;

  //TODO: parallelize in to different threads
  const std::vector<MtxBlock>& mtxBlocks = target->GetMtxBlocks();
  for(size_t idBlock = 0; idBlock < MTX_NUM_BLOCK; ++idBlock)
    {
      std::cout << "Block " << idBlock << std::endl;

      //Construct and solve the matrix using cplex;
      const MtxBlock& curBlock = mtxBlocks[idBlock];
      std::vector<uint16_t> flowsSizes;
      status |= CplexSolveMtxBlock(curBlock, flowsSizes);
      
      
      //std::cout << "FlowCnt " << curBlock.m_flowTable.size() << std::endl;       
      //Add the solved flow to measuredFlows
      for(size_t iflow = 0; iflow < curBlock.m_flowTable.size(); ++iflow)
	{
	  const FlowField& flow = curBlock.m_flowTable[iflow].m_flow;
	  measuredFlows[flow] = flowsSizes[iflow];
	}
      
    }
      

  return status;
}

int CplexSolveMtxBlock(const MtxBlock& block, std::vector<uint16_t>& flowsSizes)
{
  IloEnv env;
  try
    {
      size_t         flowCnt = block.m_flowTable.size();
      if(flowCnt == 0) return 0;

      IloModel       model(env);

      //Unknown vars.
      IloNumVarArray vars(env, flowCnt, 0.0, IloInfinity); 

      //Objective
      model.add(IloMinimize(env, IloSum(vars))); 

      //Constraints      
      const std::vector<MtxFlowField>&    flowTable    = block.m_flowTable;
      const std::vector<uint16_t>&        countTable   = block.m_countTable;
      std::vector<std::vector<uint16_t> > countIdToFlowId(countTable.size(), std::vector<uint16_t>());
      for(size_t i = 0; i < flowCnt; ++i)
	{
	  const MtxFlowField& flowField = flowTable[i];

	  for(size_t ithCountTableIDX = 0; 
	      ithCountTableIDX < flowField.m_countTableIDXs.size(); 
	      ++ithCountTableIDX)
	    {
	      size_t countTableIdx = flowField.m_countTableIDXs[ithCountTableIDX];
	      countIdToFlowId[ countTableIdx ].push_back(i);
	    }
	}
      
      for(size_t ic = 0; ic < countTable.size(); ++ic)
	{
	  if(countTable[ic] > 0)
	    {
	     
	      NS_ASSERT(countIdToFlowId[ic].size() > 0);
	      //Add contraint to the model
	      IloNumExpr expr(env);
	      for(size_t ithflow = 0; ithflow < countIdToFlowId[ic].size(); ++ithflow)
		{
		  expr = expr + vars[ countIdToFlowId[ic][ithflow] ];
		}
	      
	      model.add( expr == countTable[ic] );
	    }
	  else
	    {
	      NS_ASSERT(countIdToFlowId[ic].size() == 0);
	    }
	}

      //Solve
      IloCplex cplex(model);
      if ( !cplex.solve() )
	{
	  std::cerr << "Failed to solve" << std::endl;
	  return -1;
	}
      
      //Copy the value to flowSizes
      IloNumArray vals(env);
      cplex.getValues(vals, vars);
      flowsSizes.resize(flowCnt, 0);
      for(size_t iflow = 0; iflow < flowCnt; ++iflow)
	{
	  flowsSizes[iflow] = vals[iflow];
	}

    }
  catch(IloException& e)
    {
      std::cerr << "Concert exception " << e << std::endl; 
    }
  catch(...)
    {
      std::cerr << "Unknown Exception" << std::endl;
    }

  return 0;
} 

}


