/**
 * ICFG.h
 * @author kisslune 
 */

#ifndef ANSWERS_ICFG_H
#define ANSWERS_ICFG_H

#include "Graphs/SVFG.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include <set>
#include <stack>
#include <utility>
#include <vector>

class CFGAnalysis
{
public:
    explicit CFGAnalysis(SVF::ICFG *icfg);
    void analyze(SVF::ICFG *icfg);
    void dumpPaths();

protected:
    void recordPath(const std::vector<unsigned> &path);
    using State = std::pair<unsigned, std::stack<unsigned>>;
    void dfs(const SVF::ICFGNode *node, unsigned sink,
             std::set<State> &visited, std::vector<unsigned> &path);

    std::stack<unsigned> callStack;
    std::set<unsigned> sources;
    std::set<unsigned> sinks;
    std::set<std::vector<unsigned>> reachablePaths;
};

#endif //ANSWERS_ICFG_H
