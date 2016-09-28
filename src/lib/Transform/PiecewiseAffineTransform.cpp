//===- PiecewiseAffineTransform.cpp - Applies a sequence of piecewise affine transformations  ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The polyhedral piecewise affine transformation pass applies a sequence of piecewise affine 
// transformations to the SCoP.
// The transformation are specified in a separate file input to the compiler.
//
//===----------------------------------------------------------------------===//

#include "polly/DependenceInfo.h"
#include "polly/LinkAllPasses.h"
#include "polly/ScopInfo.h"
#include "polly/Options.h"
#include "polly/Support/GICHelper.h"
#include "polly/OptParser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "isl/flow.h"
#include "isl/aff.h"
#include "isl/band.h"
#include "isl/constraint.h"
#include "isl/set.h"
#include "isl/map.h"
#include "isl/options.h"
#include "isl/printer.h"
#include "isl/schedule.h"
#include "isl/schedule_node.h"
#include "isl/space.h"
#include "isl/union_map.h"
#include "isl/union_set.h"
#include <string>
#include <fstream>

using namespace llvm;
using namespace polly;

#define DEBUG_TYPE "polly-pwaff"

extern "C" void yy_scan_string(const char *);
extern "C" void yyparse();
extern "C" void yylex_destroy();


namespace {
cl::opt<std::string> TransformFilename(
		"polly-trans", 
		cl::desc("Piecewise affine transformation filename"), 
		cl::value_desc("filename"),
		cl::cat(PollyCategory));


class PiecewiseAffineTransform : public ScopPass {
public:
  static char ID;
  explicit PiecewiseAffineTransform() : ScopPass(ID) {}

  bool runOnScop(Scop &S) override;

  void printScop(raw_ostream &OS, Scop &S) const override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
    static __isl_give isl_map *iterToScheduleMap(Scop &, unsigned, unsigned);
    static __isl_give isl_map *zeroIterDims(Scop &, isl_space *);
    static __isl_give isl_map *applyUnitTransform(Scop &, const char *, __isl_take isl_map *);
    static __isl_give isl_map *scheduleLexGt(Scop &, unsigned, unsigned);
    static __isl_give isl_map *scheduleLexEq(Scop &, unsigned, unsigned);
    static int computeScheduleGap(__isl_keep isl_set *, __isl_keep isl_set *, unsigned);
    __isl_give isl_map *getTransform(Scop &S);
};
}

char PiecewiseAffineTransform::ID = 0;

/*
 * Map to transform iteration vector of the form [i, j, k] to a schedule vector [o1, i, o2, j, o3, k, o4, i5, o5]
 *
 * iterSize = number of iteration dimensions
 * scheduleSize = number of schedule dimensions
 
 * Note: Additional non-constant schedule dimensions are set to new variables.
 */
__isl_give isl_map *PiecewiseAffineTransform::iterToScheduleMap(Scop &S, unsigned iterSize, unsigned scheduleSize){
    
    isl_space *iterSpace = isl_space_set_alloc(S.getIslCtx(), 0, iterSize);
    isl_space *scheduleSpace = isl_space_set_alloc(S.getIslCtx(), 0, scheduleSize);
    
    isl_map *trans = isl_map_from_domain_and_range(isl_set_universe(iterSpace),
                                                   isl_set_universe(scheduleSpace));
    
    for (unsigned i = 0; i < iterSize; ++i)
        trans = isl_map_equate(trans, isl_dim_out, 2 * i + 1, isl_dim_in, i);
    
    trans = isl_map_align_params(trans, S.getParamSpace());
    
    return trans;
}

/*
 * Lexicographic greater than ordering on first n constant Schedule Dimensions 
 * scheduleSize = number of schedule dimensions
 */
__isl_give isl_map *PiecewiseAffineTransform::scheduleLexGt(Scop &S, unsigned scheduleSize, unsigned n){
    
    isl_space *scheduleSpace = isl_space_set_alloc(S.getIslCtx(), 0, scheduleSize);
    
    isl_map *lex = isl_map_from_domain_and_range(isl_set_empty(isl_space_copy(scheduleSpace)),
                                                   isl_set_empty(isl_space_copy(scheduleSpace)));
    isl_constraint *c;

    for (unsigned i = 0; i < n; ++i){
        isl_map *t = isl_map_from_domain_and_range(isl_set_universe(isl_space_copy(scheduleSpace)),
                                                   isl_set_universe(isl_space_copy(scheduleSpace)));

        for (unsigned j = 0; j < i; j++){
            t = isl_map_equate(t, isl_dim_out, 2*j, isl_dim_in, 2*j);
	}
       	
    	c = isl_constraint_alloc_inequality(isl_local_space_from_space(isl_map_get_space(t)));
        c = isl_constraint_set_coefficient_si(c, isl_dim_in, 2*i, 1);
        c = isl_constraint_set_coefficient_si(c, isl_dim_out, 2*i, -1);
        c = isl_constraint_set_constant_si(c, -1);
	t = isl_map_add_constraint(t, c);
	
	lex = isl_map_union(lex, t);
    }

    lex = isl_map_align_params(lex, S.getParamSpace());
    
    return lex;
}
/*
 * Lexicographic equal ordering on first n constant Schedule Dimensions 
 * scheduleSize = number of schedule dimensions
 */
__isl_give isl_map *PiecewiseAffineTransform::scheduleLexEq(Scop &S, unsigned scheduleSize, unsigned n){
    
    isl_space *scheduleSpace = isl_space_set_alloc(S.getIslCtx(), 0, scheduleSize);
    isl_map *lex = isl_map_from_domain_and_range(isl_set_universe(isl_space_copy(scheduleSpace)),
                                                   isl_set_universe(isl_space_copy(scheduleSpace)));

    for (unsigned i = 0; i < n; ++i){
        lex = isl_map_equate(lex, isl_dim_out, 2*i, isl_dim_in, 2*i);
    }

    lex = isl_map_align_params(lex, S.getParamSpace());
    
    return lex;
}

/*
 */
__isl_give isl_map *PiecewiseAffineTransform::zeroIterDims(Scop &S, isl_space *space){
    
    isl_map *map = isl_map_universe(isl_space_map_from_set(space));

    unsigned n = isl_space_dim(space, isl_dim_set);

    for (unsigned i = 0; i < n ; i = i+2){
        map = isl_map_equate(map, isl_dim_out, i, isl_dim_in, i);
	if (i+1 < n)
	    map = isl_map_fix_si(map, isl_dim_out, i+1, 0);
    }

    map = isl_map_align_params(map, S.getParamSpace());
    
    return map;
}

/*
 * Applies a unit transform given by @str to @transform
 */
__isl_give isl_map *PiecewiseAffineTransform::applyUnitTransform(Scop &S, const char *str, __isl_take isl_map *transform){
    // Parse str to get unit transformation
    stmtPtr = NULL;
    
    yy_scan_string(str);
    yyparse();
    yylex_destroy();

    if (stmtPtr == NULL)
	return transform;
    
    // Stores the unit transformation
    isl_map *map = nullptr;
    
    // Original Schedule
    isl_union_map *schedule = isl_union_map_intersect_domain(S.getSchedule(), S.getDomains());
    
    // New schedule domain of the program (note after application of transform)
    isl_set *postDomain = isl_set_apply(isl_set_coalesce(isl_set_from_union_set(isl_union_map_range(schedule))), isl_map_copy(transform)); // schedule USED

    DEBUG(dbgs() << "------------------- Unit Transform -------------------\n"<< str << ";\n");

    switch(stmtPtr->type){
        case typeRealign:
	{
      	    isl_set *preLoopDomain1, *postLoopDomain1;
      	    isl_set *preLoopDomain2, *postLoopDomain2;
	    char *sl1, *sl2;
	    unsigned n;
    
            sl1 = (stmtPtr->m.l1)->name;
            sl2 = (stmtPtr->m.l2)->name;
            n = stmtPtr->m.n;
            
	    if (S.LoopDomainMap[sl1] == NULL || S.LoopDomainMap[sl1] == NULL) return transform;

            preLoopDomain1 = isl_set_copy(S.LoopDomainMap[sl1]);
	    postLoopDomain1 = isl_set_apply(preLoopDomain1, isl_map_copy(transform)); //  preLoopDomain1 USED
            preLoopDomain2 = isl_set_copy(S.LoopDomainMap[sl2]);
	    postLoopDomain2 = isl_set_apply(preLoopDomain2, isl_map_copy(transform)); //  preLoopDomain2 USED

	    // ----------------------
	    // Left Domain : Stmts syntactically before loop l1 and l1
	    // ----------------------
	    isl_map *lexOrder = scheduleLexGt(S, isl_set_dim(postDomain, isl_dim_set), (isl_set_dim(postDomain, isl_dim_set)+1)/2);
	    isl_set *leftDomain = isl_set_apply(isl_set_copy(postLoopDomain1), lexOrder); // lexOrder USED
	    leftDomain = isl_set_union(leftDomain, isl_set_copy(postLoopDomain1));
	    leftDomain = isl_set_intersect(leftDomain, isl_set_copy(postDomain));

	    // ----------------------
	    // Right Domain: Realign to match with leftDomain and extend the schedule order by 1 for the nth index.
	    // ----------------------
	    isl_set *rightDomain = isl_set_complement(isl_set_copy(leftDomain)); 
	    rightDomain = isl_set_intersect(rightDomain, isl_set_copy(postDomain));

	    // left map and right map
	    isl_map *leftMap = isl_set_identity(isl_set_copy(leftDomain));
	    isl_map *rightMap = isl_set_identity(isl_set_copy(rightDomain));
	    
	    isl_map *zeroIters = zeroIterDims(S, isl_set_get_space(leftDomain));
	    isl_set *leftMax = isl_set_apply(leftDomain, isl_map_copy(zeroIters)); // leftDomain USED
	    isl_set *rightMin = isl_set_apply(isl_set_copy(rightDomain), zeroIters);
	    leftMax = isl_set_lexmax(leftMax);
	    rightMin = isl_set_lexmin(rightMin); 

    	    DEBUG(dbgs() << "  - left max := " << stringFromIslObj(leftMax) << ";\n");
    	    DEBUG(dbgs() << "  - right min := " << stringFromIslObj(rightMin) << ";\n");

	    // Difference
	    isl_set *gap = isl_set_sum(isl_set_neg(isl_set_copy(rightMin)), isl_set_copy(leftMax));
	    //gap = isl_set_apply(gap, zeroIterDims(S, isl_set_get_space(gap)));

	    // Increment gap by 1
	    isl_set *inc = isl_set_universe(isl_set_get_space(gap));
    	    for (unsigned i = 0; i < isl_set_dim(gap, isl_dim_set); i = i+1){
	        if (i!= n*2)
	            inc = isl_set_fix_si(inc, isl_dim_set, i, 0);
		else
	            inc = isl_set_fix_si(inc, isl_dim_set, i, 1);
	    }
	    gap = isl_set_sum(gap, inc);

    	    DEBUG(dbgs() << "  - gap := " << stringFromIslObj(gap) << ";\n");
	
	    // update right map
            isl_map *gapMap = isl_map_from_domain_and_range(rightDomain, gap);

	    rightMap = isl_map_sum(rightMap, gapMap);
 
    	    DEBUG(dbgs() << "\n  - left map := " << stringFromIslObj(leftMap) << ";\n");
    	    DEBUG(dbgs() << "  - right map := " << stringFromIslObj(rightMap) << ";\n");
	    
	    map = isl_map_union(leftMap, rightMap); // leftMap, rightMap USED

            isl_set_free(leftMax);
            isl_set_free(rightMin);
            isl_set_free(postLoopDomain1);
            isl_set_free(postLoopDomain2);

    	    break;
	}
        case typeISplit: 
	{
    	    isl_set *preLoopDomain, *postLoopDomain;
	    char *sl, *sr1, *sr2;
	    unsigned n;
    
            sl = (stmtPtr->is.l)->name;
            sr1 = (stmtPtr->is.r1)->name;
            sr2 = (stmtPtr->is.r2)->name;
            n = stmtPtr->is.n;
            
	    if (S.LoopDomainMap[sl] == NULL) return transform;

            preLoopDomain = isl_set_copy(S.LoopDomainMap[sl]);
	    postLoopDomain = isl_set_apply(isl_set_copy(preLoopDomain), isl_map_copy(transform));

	    // ----------------------
            // Read predicate 
	    // ----------------------
            isl_set *pred = isl_set_read_from_str(S.getIslCtx(), stmtPtr->is.pred);
	    // TODO: check that predicate corresponds to value of n
	    
	    // Extend predicate set to match schedule dimension
            pred = isl_set_apply(pred, iterToScheduleMap(S, isl_set_dim(pred, isl_dim_set), isl_set_dim(postDomain, isl_dim_set)));
	    pred = isl_set_intersect(pred, postLoopDomain); // postLoopDomain USED
             
	    // ----------------------
	    // Left Domain: Stmts syntactically before l and l with iterations given by pred
	    // ----------------------
	    isl_map *lexOrder = scheduleLexGt(S, isl_set_dim(postDomain, isl_dim_set), (isl_set_dim(postDomain, isl_dim_set)+1)/2);
	    isl_set *leftDomain = isl_set_apply(isl_set_copy(postLoopDomain), lexOrder); // lexOrder USED
	    leftDomain = isl_set_union(leftDomain, isl_set_copy(pred));
	    leftDomain = isl_set_intersect(leftDomain, isl_set_copy(postDomain));
	    // Identity map for leftDomain
	    isl_map *leftMap = isl_set_identity(isl_set_copy(leftDomain));

	    
	    // ----------------------
	    // Right Domain: Extend the schedule order by 1 for the nth index
	    // ----------------------
	    isl_set *rightDomain = isl_set_complement(leftDomain); // leftDomain USED
	    rightDomain = isl_set_intersect(rightDomain, isl_set_copy(postDomain));
            isl_map *rightMap  = isl_map_from_domain_and_range(isl_set_copy(rightDomain),
                                                   isl_set_universe(isl_set_get_space(rightDomain)));
	    
	    isl_constraint *c = isl_constraint_alloc_equality(isl_local_space_from_space(isl_map_get_space(rightMap)));
	    c = isl_constraint_set_coefficient_si(c, isl_dim_in, n*2, -1);
	    c = isl_constraint_set_coefficient_si(c, isl_dim_out, n*2, 1);
	    c = isl_constraint_set_constant_si(c, -1);
	    rightMap = isl_map_add_constraint(rightMap, c);

            // Equate input and output constant dimensions
    	    for (unsigned i = 0; i < n*2; i++)
		rightMap = isl_map_equate(rightMap, isl_dim_out, i, isl_dim_in, i);

    	    for (unsigned i = n*2 + 1; i < isl_set_dim(postDomain, isl_dim_set); i++)
		rightMap = isl_map_equate(rightMap, isl_dim_out, i, isl_dim_in, i);
	    // ----------------------
	    
	    map = isl_map_union(leftMap, rightMap); // leftMap, rightMap USED

	    // Assign new loop domains to labels. 
	    isl_map * invTrans = isl_map_reverse(isl_map_copy(transform));
	    isl_set * invPred = isl_set_apply(pred, invTrans); // pred USED

	    S.LoopDomainMap[sr1] = isl_set_coalesce(isl_set_intersect(isl_set_copy(preLoopDomain), isl_set_copy(invPred)));
	    S.LoopDomainMap[sr2] = isl_set_coalesce(isl_set_intersect(isl_set_copy(postDomain), 
					    isl_set_intersect(preLoopDomain, isl_set_complement(invPred)))); // preLoopDomain, invPred USED
	    S.LoopDimMap[sr1] = S.LoopDimMap[sl];
	    S.LoopDimMap[sr2] = S.LoopDimMap[sl];

            DEBUG(dbgs() << "  - Domain for "<< sr1 << " := " << stringFromIslObj(S.LoopDomainMap[sr1]) << ";\n");
            DEBUG(dbgs() << "  - Domain for "<< sr2 << " := " << stringFromIslObj(S.LoopDomainMap[sr2]) << ";\n");

            isl_set_free(rightDomain);

            break;
        }	
        case typeAffine:
        {	
    	    isl_set *preLoopDomain, *postLoopDomain;
	    char *sl;
    
            sl = (stmtPtr->a.l)->name;

	    if (S.LoopDomainMap[sl] == NULL) return transform;
            
            preLoopDomain = isl_set_copy(S.LoopDomainMap[sl]);
	    unsigned loopDim = S.LoopDimMap[sl];

	    // Get tranformed domain
	    postLoopDomain = isl_set_apply(isl_set_copy(preLoopDomain), isl_map_copy(transform));

	    // ----------------------
            // 1. Affine Transform
	    //
	    // Suppose [i, j] -> [i', j', k', l'], schInDim = 7, loopDim = 2, schOutDim = 2 * 2 + 1 + (4 - 2)*2 
	    // amap = [o1, i, o2, j, o3, p3, o4] -> [o1, i', o2, j', 0, k', 0, l', o3]  
	    // loopDim' = 2 - 2 + 4
	    //
	    // Suppose [i, j] -> [i', j', k', l'], schInDim = 7, loopDim = 3, schOutDim = 3 * 2 + 1 + (4 - 2)*2 
	    // amap = [o1, i, o2, j, o3, p3, o4] -> [o1, i', o2, j', 0, k', 0, l', o3, p3, o4]  
	    // loopDim' = 3 - 2 + 4
	    //
	    // Suppose [i, j] -> [i'], schInDim = 7, loopDim = 3, schOutDim = 7 
	    // amap = [o1, i, o2, j, o3, p3, o4] -> [o1, i', o3, p3, o4, 0, 0]  
	    // loopDim' = 3 - 2 + 1
	    // ----------------------
            isl_map *amap = isl_map_read_from_str(S.getIslCtx(), stmtPtr->a.trans);

	    // Input and Output Map Dimensions
            unsigned amapInDim = isl_map_dim(amap, isl_dim_in);
            unsigned amapOutDim = isl_map_dim(amap, isl_dim_out);
	   
	    // Input and Output Schedule Dimensions 
            unsigned schInDim = isl_set_dim(postDomain, isl_dim_set);
            unsigned schOutDim = schInDim;
	    unsigned offset = (amapOutDim - amapInDim)*2; 
            if(loopDim * 2 + 1 + offset > schInDim)
                schOutDim = loopDim * 2 + 1 + offset;

	    // New Loop Dimension
	    loopDim = loopDim + offset/2;
            
	    // Extend map to match input and output schedule dimensions
            amap = isl_map_apply_domain(amap, iterToScheduleMap(S, amapInDim, schInDim));
            amap = isl_map_apply_range(amap, iterToScheduleMap(S, amapOutDim, schOutDim));

	    // Initialize remaining dimensions
	    
            // Equate input and output dimensions
    	    for (unsigned i = 0; i < amapInDim*2 && i < amapOutDim*2; i = i+2)
		amap = isl_map_equate(amap, isl_dim_out, i, isl_dim_in, i);

 	    for (unsigned i = amapInDim*2; i < schInDim && i + offset < schOutDim; i = i+1)
		amap = isl_map_equate(amap, isl_dim_out, i + offset, isl_dim_in, i);

           // Set remaining dimensions to 0
            for (unsigned i = 0; i < offset; i+=2)
	        amap = isl_map_fix_si(amap, isl_dim_out, amapInDim*2 + i, 0);
	    
            for (unsigned i = schInDim + offset; i < schOutDim; i+=1)
	        amap = isl_map_fix_si(amap, isl_dim_out, i, 0);

            amap = isl_map_intersect_domain(amap, isl_set_copy(postLoopDomain));
            amap = isl_map_intersect_domain(amap, isl_set_copy(postDomain));

	    // -------------------------------------------
	    // 2. Identity Transform for remaining indices
	    // -------------------------------------------
    	    isl_space *newSpace = isl_space_set_alloc(S.getIslCtx(), 0, schOutDim);
    
            isl_map *cmap = isl_map_from_domain_and_range(isl_set_complement(postLoopDomain),
                                                   isl_set_universe(newSpace)); // postLoopDomain USED
            // Equate input and output dimensions
    	    for (unsigned i = 0; i < schInDim; i++)
		cmap = isl_map_equate(cmap, isl_dim_out, i, isl_dim_in, i);

            // Set remaining dimensions to 0
            for (unsigned i = schInDim; i < schOutDim; i += 1)
	        cmap = isl_map_fix_si(cmap, isl_dim_out, i, 0);

            cmap = isl_map_intersect_domain(cmap, isl_set_copy(postDomain));
	    // -------------------------------------------
	    
	    map = isl_map_union(amap, cmap); // amap, cmap USED

	    S.LoopDimMap[sl] = loopDim;

            DEBUG(dbgs() << "  - # Loops for "<< sl << " := " << S.LoopDimMap[sl] << ";\n");

	    break;
	}    
	case typeLift:
	{
    	    isl_set *preLoopDomain, *postLoopDomain;
	    char *sl, *sr;
	    unsigned n;
    
            sl = (stmtPtr->l.l)->name;
            sr = (stmtPtr->l.r)->name;
            n = stmtPtr->l.n;
            
	    if (S.LoopDomainMap[sl] == NULL) return transform;

            preLoopDomain = isl_set_copy(S.LoopDomainMap[sl]);
	    postLoopDomain = isl_set_apply(preLoopDomain, isl_map_copy(transform));

	    // ----------------------
	    // Lift loop to nth loop
	    // ----------------------
	    isl_map *lexOrder = scheduleLexEq(S, isl_set_dim(postDomain, isl_dim_set), n);
	    postLoopDomain = isl_set_apply(postLoopDomain, lexOrder);
	    
	    isl_map * invTrans = isl_map_reverse(isl_map_copy(transform));
	    S.LoopDomainMap[sr] = isl_set_apply(postLoopDomain, invTrans);
	    S.LoopDimMap[sr] = S.LoopDimMap[sl];

            DEBUG(dbgs() << "  - Domain for "<< sr << " := " << stringFromIslObj(S.LoopDomainMap[sr]) << ";\n");

	    map = isl_set_identity(isl_set_copy(postDomain));

            break;
 
	}
    }

    // Cleanup data-structures
    isl_set_free(postDomain);
    free(stmtPtr);

    map = isl_map_align_params(map, S.getParamSpace());

    map = isl_map_coalesce(map);
    DEBUG(dbgs() << "  - Map := " << stringFromIslObj(map) << ";\n");
    DEBUG(dbgs() << "------------------------------------------------------\n");
    map = isl_map_coalesce(isl_map_apply_range(transform, map));
    return map;
}


__isl_give isl_map *PiecewiseAffineTransform::getTransform(Scop &S){
     
    // Define identity transform
    isl_union_map *schedule = isl_union_map_intersect_domain(S.getSchedule(), S.getDomains());    
    isl_set *domain = isl_set_coalesce(isl_set_from_union_set(isl_union_map_range(schedule)));
    isl_map *Transform = isl_set_identity(domain);
    Transform = isl_map_align_params(Transform, S.getParamSpace());
 
    std::ifstream src;
    src.open(TransformFilename.c_str(), std::ios::in);
    if (src.is_open())
    {
 	std::string line;
        // char *tok = strtok(str, "\n");
        while (getline(src, line)) // tok != NULL)
        {
	    if (!(line.find("//") == 0))
        	Transform = applyUnitTransform(S, line.c_str(), Transform);
        } 
    }
    src.close();

    return Transform;
}
    


bool PiecewiseAffineTransform::runOnScop(Scop &S) {
    // Skip empty SCoPs but still allow code generation as it will delete the
    // loops present but not needed.
    if (S.getSize() == 0) {
        S.markAsOptimized();
        return false;
    }

	dbgs() << "\nTransforming function: " << S.getRegion().getEntry()->getParent()->getName() << "\n";
    
    const Dependences &D = getAnalysis<DependenceInfo>().getDependences();
   
    bool depsCheck = true; 
    if (!D.hasValidDependences()){
        dbgs() << "##### Invalid Dependences! Depedence Checking switched off\n";
        depsCheck = false;
    }
    
    // Build input data.
    int ValidityKinds =
    Dependences::TYPE_RAW | Dependences::TYPE_WAR | Dependences::TYPE_WAW;
    
    // Get Iteration Domain
    isl_union_set *Domain = S.getDomains();

    if (!Domain){
        dbgs() << "##### Domain not found\n";
        return false;
    }
    
    // Get Schedule
    isl_union_map *Schedule = S.getSchedule();
    

    // Get dependences
    isl_map *Deps;
    if (depsCheck){
        isl_union_map *Validity = D.getDependences(ValidityKinds);
        
        Validity = isl_union_map_apply_domain(Validity, isl_union_map_copy(Schedule));
        Validity = isl_union_map_apply_range(Validity, isl_union_map_copy(Schedule));
        Validity = isl_union_map_coalesce(Validity);
        Deps = isl_map_from_union_map(Validity); // Assuming Deps is in same space
    }
    
    
    DEBUG(dbgs() << "------------------------- SCOP -------------------------\n");
    DEBUG(dbgs() << "Domain := " << stringFromIslObj(Domain) << ";\n");
    if (depsCheck) DEBUG(dbgs() << "Dependences := " << stringFromIslObj(Deps) << ";\n");
    DEBUG(dbgs() << "Current Schedule := " << stringFromIslObj(Schedule) << ";\n");
    
    isl_options_set_on_error(isl_union_set_get_ctx(Domain), ISL_ON_ERROR_WARN);

    //******************************************
    // Update and Check Schedule
    //******************************************
     
    // Get Transformation   
    isl_map *Transform = getTransform(S);

    // Update the schedule
    isl_union_map *NewSchedule = isl_union_map_coalesce(isl_union_map_apply_range(isl_union_map_copy(Schedule), isl_union_map_from_map(isl_map_copy(Transform))));
    //DEBUG(dbgs() << "New Schedule := " << stringFromIslObj(NewSchedule) << ";\n");
    
    dbgs() << "------------------ Transform Checks ------------------\n";
	// boolean about whether it is safe to apply the transformation
	bool applyTransform = true;

    // Check that the transformation is injective
    isl_bool flag = isl_map_is_injective(Transform);
    dbgs() << "Is the transform Injective? " << (flag == isl_bool_true ? "True": "False") << ";\n";
	if (flag == isl_bool_false) applyTransform = false;
   
    if (depsCheck){ 
        //Check if dependences are preserved
        isl_map *NewDeps = isl_map_apply_range(isl_map_copy(Deps),isl_map_copy(Transform));
        NewDeps = isl_map_apply_domain(NewDeps, isl_map_copy(Transform));
        NewDeps = isl_map_coalesce(NewDeps);
        isl_map *LexOrder = isl_map_lex_ge(isl_set_get_space(isl_map_domain(isl_map_copy(NewDeps))));
        flag = isl_map_is_disjoint(isl_map_copy(NewDeps), isl_map_copy(LexOrder));
        dbgs() << "Does the transform preserve dependences? " << (flag == isl_bool_true ? "True": "False") << ";\n";
        if (flag == isl_bool_false){
			    applyTransform = false;
                dbgs() << " Counter Examples: " << stringFromIslObj(isl_map_coalesce(isl_map_intersect(NewDeps, LexOrder))) << ";\n";
        }        
	
        isl_map_free(NewDeps);
        isl_map_free(LexOrder);
    }
    dbgs() << "------------------------------------------------------\n";

    // TODO Generate feedback when transformation goes wrong
    // TODO Do incremental tranformation and dependence check rather than monolithic transformation

    // Update the Scop using new Schedule
    S.markAsOptimized();
    
	if(applyTransform){
		for (ScopStmt &Stmt : S) {
			isl_map *StmtSchedule;
			isl_set *Domain = Stmt.getDomain();
			isl_union_map *StmtBand;
			StmtBand = isl_union_map_intersect_domain(isl_union_map_copy(NewSchedule),
			                                          isl_union_set_from_set(Domain));
			if (isl_union_map_is_empty(StmtBand)) {
			    StmtSchedule = isl_map_from_domain(isl_set_empty(Stmt.getDomainSpace()));
			    isl_union_map_free(StmtBand);
			} else {
			    assert(isl_union_map_n_map(StmtBand) == 1);
			    StmtSchedule = isl_map_from_union_map(StmtBand);
			}
		    
		    Stmt.setSchedule(StmtSchedule);
		}
    }
	else {
		dbgs() << "ERROR!!! Transformation script does not preserve correctness! Transformation not implemented.\n";
	}

    // Free all data structures
    isl_union_set_free(Domain);
    isl_union_map_free(Schedule);
    isl_map_free(Transform);
    isl_union_map_free(NewSchedule);
    if (depsCheck) isl_map_free(Deps);

    return false;


}

void PiecewiseAffineTransform::printScop(raw_ostream &, Scop &) const {}

void PiecewiseAffineTransform::getAnalysisUsage(AnalysisUsage &AU) const {
  ScopPass::getAnalysisUsage(AU);
  AU.addRequired<DependenceInfo>();
}

Pass *polly::createPiecewiseAffineTransformPass() { return new PiecewiseAffineTransform(); }

INITIALIZE_PASS_BEGIN(PiecewiseAffineTransform, "polly-pwaff",
                      "Polly - Applies a sequence of piecewise affine Transform", false, false)
INITIALIZE_PASS_DEPENDENCY(DependenceInfo)
INITIALIZE_PASS_DEPENDENCY(ScopInfo)
INITIALIZE_PASS_END(PiecewiseAffineTransform, "polly-pwaff", "Polly - Applies a sequence of piecewise affine transform",
                    false, false)
