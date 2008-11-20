#include <opencog/util/platform.h>
#include "../../PLN.h"

#include "../Rule.h"
#include "../Rules.h"
#include "../../AtomSpaceWrapper.h"
#include "../../PLNatom.h"
#include "../../BackInferenceTreeNode.h"

namespace reasoning
{

CrispTheoremRule::CrispTheoremRule(reasoning::iAtomSpaceWrapper *_destTable)
: Rule(_destTable, true, true, "RewritingRule")
{
    /// SHOULD NOT BE USED FOR FORWARD INFERENCE!
    
    inputFilter.push_back(meta(
        new tree<Vertex>(mva(
            (Handle)ATOM))));
}

map<vtree, vector<vtree> ,less_vtree> CrispTheoremRule::thms;
BBvtree bind_vtree(vtree &targ, map<Handle, vtree>& binds)
{
                BBvtree thm_substed(new BoundVTree(targ));
                
                bool changes;
                do
                {               
                    changes = false;
                    
					cprintf(4,"Next change...\n");
                    
                    for(vtree::pre_order_iterator vit = thm_substed->begin(); vit != thm_substed->end(); vit++)
                    {
						cprintf(4,"Next change check...\n");
                        
                        Handle *ph = v2h(&*vit);
                        if (ph)
                        {
							cprintf(4,"(ph)");
                            
                            map<Handle, vtree>::iterator it = binds.find(*ph);
                            if (it != binds.end()) {
                            
                                cprintf(4,"replacing...[%lu]\n", v2h(*vit));
                                cprintf(4,"with...\n");
                                NMPrinter printer(NMP_HANDLE|NMP_TYPE_NAME);
                                printer.print(it->second.begin(),4);
                                
                                thm_substed->replace(vit, it->second.begin());

								cprintf(4,"replace ok\n");

                                changes = true;
                                goto break_inner; //vit = thm_substed->end(); //break inner loop                            
                            }
                        }
                        else
							cprintf(4,"NOT (ph)");
                    }
break_inner:                    
					cprintf(4,"1 change run ok");
                } while (changes);
                
    return thm_substed;             
}

Rule::setOfMPs CrispTheoremRule::o2iMetaExtra(meta outh, bool& overrideInputFilter) const
{
    AtomSpaceWrapper *nm = GET_ATW;
    set<MPs> ret;   
bool htemp=false;
    
/*  for (map<vtree, vector<vtree> ,less_vtree>::iterator thm=thms.begin(); thm != thms.end(); thm++)
    {
        rawPrint(thm->first
    }*/

    for (map<vtree, vector<vtree> ,less_vtree>::iterator thm=thms.begin(); thm != thms.end(); thm++)
    {
        map<Handle, vtree> binds;
        set<Handle> vars;
        
        /// Grab the FW_VARs
        
        long count1=0, count2=0;
        
        //vector<vtree> thms2(thms->second);
        //vtree thms1(thms->first);
        
        foreach(vtree& targ, thm->second)
        {
			cprintf(1, "Producing thm: checking...\n");
            
            vtree tmp(thm->first);

			NMPrinter printer(NMP_HANDLE|NMP_TYPE_NAME);
            printer.print(tmp.begin(),1);

            //foreach(Vertex v, targ)
            for(vtree::iterator v  = targ.begin(); v != targ.end(); v++)
                if (nm->getType(v2h(*v)) == FW_VARIABLE_NODE)
                {
                    vars.insert(v2h(*v));
                    count1++;
                }
        }
        //foreach(Vertex v, thm->first)
        for(vtree::iterator v  = thm->first.begin(); v != thm->first.end(); v++)
                if (nm->getType(v2h(*v)) == FW_VARIABLE_NODE)
                {
                    vars.insert(v2h(*v));   
                    count1++;
                }
                        
        /// Rename the vars
        
        bindingsT newPreBinds;
        
        foreach(Handle h, vars)
            newPreBinds[h] = v2h(CreateVar(destTable));
                
         typedef pair<Handle,Handle> hpair;
                
		 cprintf(4,"Bindings are:\n");
                
         foreach(hpair hp, newPreBinds)
			cprintf(4, "%s => %s\n", nm->getName(hp.first).c_str(), nm->getName(hp.second).c_str());
		 cprintf(4,"<<< Bindings\n");
                
        /// Replace the old vars with renamed counterparts
                
        vtree           thm_target(thm->first);
        vector<vtree>   thm_args(thm->second);
                
        foreach(vtree& targ, thm_args)
            //foreach(Vertex& v, targ)
            for(vtree::iterator v  = targ.begin(); v != targ.end(); v++)
            {
                bindingsT::iterator bit = newPreBinds.find(v2h(*v));
                if (bit != newPreBinds.end())
                {
                    *v = Vertex(bit->second);
                    count2++;
                }
            }

        //foreach(Vertex& v, thm_target)
            for(vtree::iterator v  = thm_target.begin(); v != thm_target.end(); v++)
            {
                bindingsT::iterator bit = newPreBinds.find(v2h(*v));
                if (bit != newPreBinds.end())
                {
                    *v = Vertex(bit->second);
                    count2++;
                }
            }
		 cprintf(4,"From,to\n");
            
         vtree tmp2(thm->first);

		 NMPrinter printer(NMP_HANDLE|NMP_TYPE_NAME);
         printer.print(tmp2.begin(),4);
         tmp2 = thm_target;
         printer.print(tmp2.begin(),4);
        
         cprintf(4,"counts: %ld %ld\n", count1, count2);
        
         cprintf(4,"Unifies to %lu:\n",v2h(*outh->begin()));

         printer.print(outh->begin(), 4);

/// haxx:: warning!
/// should have different binds for left and right!!!

        if (unifiesTo(thm_target, *outh, binds, binds, true))
        {
             cprintf(4,"Unifies!\n");
             vtree tmp(thm_target);

             printer.print(tmp.begin(), 4);
            
             for (map<Handle, vtree>::iterator ii=binds.begin(); ii!=binds.end(); ii++)
             {
                cprintf(4,"%lu=>\n", ii->first);
                printer.print(ii->second.begin(), 4);
             }

 
            MPs new_thm_args;

            foreach(vtree targ, thm_args)
            {
                 cprintf(4,"Subst next...\n");
                bool changes = false;
                
                BBvtree thm_substed = bind_vtree(targ, binds);

				 cprintf(4,"FOR:\n");

                 printer.print(outh->begin(), 4);
                 cprintf(4,"thm_substed\n");
                 printer.print(thm_substed->begin(), 4);
                
                 htemp =true;
                
                new_thm_args.push_back(thm_substed);
            }

            new_thm_args.push_back(BBvtree(new BoundVTree(mva((Handle)HYPOTHETICAL_LINK,
                *outh))));

            ret.insert(new_thm_args);
			cprintf(1, "Producing thm found.\n");

            // Now, fix the inputFilter to correspond to the new nr of input args.
            const_cast<MPsIn*>(&inputFilter)->clear();
            for (uint i = 0; i < new_thm_args.size(); i++)
                const_cast<MPsIn*>(&inputFilter)->push_back(meta(
                    new tree<Vertex>(mva(
                    (Handle)ATOM))));
        }

    }


    overrideInputFilter = true;
    
    return ret;
}


BoundVertex CrispTheoremRule::compute(const vector<Vertex>& premiseArray, Handle CX) const
{
    AtomSpaceWrapper *nm = GET_ATW;

    /*vtree res(mva((Handle)IMPLICATION_LINK,
        mva(nm->getOutgoing(v2h(premiseArray[0]))[0]),
        mva((Handle)AND_LINK,
            mva(nm->getOutgoing(v2h(premiseArray[0]))[1]),
            mva(nm->getOutgoing(v2h(premiseArray[1]))[1])
        )
    ));*/

    int real_args = premiseArray.size()-1;
    
    //vtree res(mva(nm->getOutgoing(v2h(premiseArray[1]))[1]));
    
    cprintf(3,"CrispTheoremRule::compute... [%lu]\n", nm->getOutgoing(v2h(premiseArray[real_args]))[0]);
    
/*  printTree(nm->getOutgoing(v2h(premiseArray[0]))[0],0,0);
    printTree(nm->getOutgoing(v2h(premiseArray[1]))[0],0,0);
    printTree(nm->getOutgoing(v2h(premiseArray[2]))[0],0,0);
    
    cprintf(0,"<<<\n");*/
    
    /// Unravel the HYPOTHETICAL_LINK:
    
    vtree res(make_vtree(nm->getOutgoing(v2h(premiseArray[real_args]))[0]));
    
//  cprintf(0,"CrispTheoremRule::compute... make_vtree ok\n");
        
    /*for (int i=0;i<premiseArray.size();i++)
        res.append_child(res.begin(), premiseArray[i]*/
    
    TruthValue** tvs = new TruthValue*[premiseArray.size()];
    int i=0;
    foreach(const Vertex& v, premiseArray)
    {
        tvs[i++] = (TruthValue*) &(nm->getTV(v2h(v)));
    }
    
    bool use_AND_rule = (real_args>1);
    
//  cprintf(0,"CrispTheoremRule::compute... TV ok\n");
    
    TruthValue* tv = (use_AND_rule?SymmetricANDFormula().compute(tvs,real_args):tvs[0]->clone());
    Handle ret_h = destTable->addAtom(res, *tv, true,true);
    delete tv;
    
	cprintf(3,"CrispTheoremRule::compute produced:\n");

    NMPrinter printer(NMP_HANDLE|NMP_TYPE_NAME);
    printer.print(ret_h);
    
    delete[] tvs;
    
    return Vertex(ret_h);
}

} // namespace reasoning
