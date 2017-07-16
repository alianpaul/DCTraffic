#include <vector>
#include <stdint.h>
#include <cassert>
#include <iostream>

#include <ilcplex/ilocplex.h>
#include <ilconcert/iloenv.h>
ILOSTLBEGIN

namespace ns3
{

void CplexSolveEquations(const std::vector<std::vector<uint16_t> >& cntIdToVarId,
			 const std::vector<uint32_t>& cnt,
			 std::vector<uint32_t>& var)
{

  assert(cntIdToVarId.size() == cnt.size());

  IloEnv env;
  try 
    {
      size_t varSize = var.size();
      if(varSize == 0) return;

      IloModel model(env);

      //Unknown vars to solve
      IloNumVarArray ilovars(env, varSize, 0.0, IloInfinity);
      
      //Objective
      model.add(IloMinimize(env, IloSum(ilovars)));
      
      //constraints
      for(size_t ic = 0; ic < cnt.size(); ++ic)
	{
	  if(cnt[ic] > 0)
	    {
	      assert(cntIdToVarId[ic].size() > 0);
	      IloNumExpr expr(env);
	      for(size_t iv = 0; iv < cntIdToVarId[ic].size(); ++iv)
		{
		  expr = expr + ilovars[ cntIdToVarId[ic][iv] ];
		}
	      model.add( expr == cnt[ic] );
	    }
	  else 
	    {
	      assert(cntIdToVarId[ic].size() == 0);
	    }
	}

      //Solve
      IloCplex cplex(model);
      if( !cplex.solve() )
	{
	  std::cerr << "Fail to solve" << std::endl;
	  exit(0);
	}
   
      //Copy data back
      IloNumArray val(env);
      cplex.getValues(val, ilovars);
      for(size_t i = 0; i < varSize; ++i)
	{
	  var[i] = val[i];
	}
    } 
  catch (IloException& e) 
    {
      std::cerr << "Cplex exception " << e << std::endl;
      exit(0);
    }

}

}


