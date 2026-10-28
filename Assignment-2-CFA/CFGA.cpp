/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    // Export the graph so that recorded node IDs can be inspected.
    icfg->dump(moduleNameVec.front() + ".icfg");
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    reachablePaths.clear();
    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
        for (auto snk : sinks)
        {
            callStack = std::stack<unsigned>();
            std::set<State> visited;
            std::vector<unsigned> path;
            dfs(icfg->getICFGNode(src), snk, visited, path);
        }
}

void CFGAnalysis::dfs(const SVF::ICFGNode *node, unsigned sink,
                      std::set<State> &visited, std::vector<unsigned> &path)
{
    // Path-local cycle detection: a second call site has a different context.
    const State state(node->getId(), callStack);
    if (!visited.insert(state).second)
        return;
    path.push_back(node->getId());

    if (node->getId() == sink)
    {
        if (callStack.empty())
            recordPath(path);
    }
    else
    {
        for (const auto *edge : node->getOutEdges())
        {
            if (edge->isCallCFGEdge())
            {
                // A call edge starts at its unique CallICFGNode.
                callStack.push(node->getId());
                dfs(edge->getDstNode(), sink, visited, path);
                callStack.pop();
            }
            else if (edge->isRetCFGEdge())
            {
                const auto *ret = SVFUtil::dyn_cast<RetICFGNode>(edge->getDstNode());
                if (ret && !callStack.empty() &&
                    callStack.top() == ret->getCallICFGNode()->getId())
                {
                    const unsigned callSite = callStack.top();
                    callStack.pop();
                    dfs(edge->getDstNode(), sink, visited, path);
                    callStack.push(callSite);
                }
            }
            else if (edge->isIntraCFGEdge())
            {
                // Includes external calls modeled by a call-to-return edge.
                dfs(edge->getDstNode(), sink, visited, path);
            }
        }
    }

    path.pop_back();
    visited.erase(state);
}
