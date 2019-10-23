#include "../Player.hpp"
#include "../random.hpp"
#include "../Simulator/all.hpp"
#include <vector>
#include <iostream>

#ifndef PLAYOUT_COUNT
#define PLAYOUT_COUNT 1000
#endif


class MonteCarloPlayer: public Player{
    Simulator0 sim;
    int playoutCount = PLAYOUT_COUNT;
    cpprefjp::random_device rd;
    std::mt19937 mt; 
public:
    MonteCarloPlayer(): mt(rd()){
    }

    virtual std::string decideRed(){
        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    // std::vector<std::string> getLegalPattern(Geister game){
    //     std::vector<std::string> res;
    //     std::vector<char> blue;
    //     std::vector<char> red;
    //     std::vector<char> unknown;
    //     for(int u = 8; u < 16; ++u){
    //         if(game.allUnit()[u].color.toInt() == 1)
    //             blue.push_back(u - 8 + 'A');
    //         else if(brd.units[u].color.toInt() == 3)
    //             red.push_back(u - 8 + 'A');
    //         else
    //             unknown.push_back(u - 8 + 'A');
    //     }
    //     for(auto p: pattern){
    //         int checkB = 0;
    //         for(auto b: blue){
    //             if(p.find(b) != std::string::npos){
    //                 checkB++;
    //                 break;
    //             }
    //         }
    //         if(checkB != 0) continue;
    //         int checkR = 0;
    //         for(auto r: red){
    //             if(p.find(r) == std::string::npos){
    //                 checkR++;
    //                 break;
    //             }
    //         }
    //         if(checkR != 0) continue;
    //         res.push_back(p);
    //     }
    //     return res;
    // }

    std::string decideHand_Random(){
        auto legalMoves = game.getLegalMove1st();
        std::vector<double> winCounts;
        for(int i = 0; i < legalMoves.size(); ++i){
            winCounts.push_back(0.0);
        }
        for(int i = 0; i < playoutCount; ++i){
            auto m = legalMoves[i % legalMoves.size()];
            sim.init(game);
            sim.geister.move(m.unit.name, m.direct.toChar());
            winCounts[i % legalMoves.size()] += sim.playout();
        }
        double maxWin = -playoutCount;
        auto action = legalMoves[0];
        for(int i = 0; i < legalMoves.size(); ++i){
            if(winCounts[i] > maxWin){
                maxWin = winCounts[i];
                action = legalMoves[i];
            }
        }
        return action;
    }

    // Hand decideHand_Average(const Board& brd){
    //     auto patterns = getLegalPattern(brd);
    //     int patternCounts = patterns.size();
    //     UseSimulateBoard simbrd(brd);
    //     auto legalMoves = simbrd.getLegalMove();
    //     int playout = playoutCount / legalMoves.size();
    //     vector<double> winCounts;
    //     for(int i = 0; i < legalMoves.size(); ++i){
    //         winCounts.push_back(0.0);
    //     }
    //     for(int l = 0; l < legalMoves.size(); ++l){
    //         auto m = legalMoves[l];
    //         for(int i = 0; i < playout; ++i){
    //             auto pattern = patterns[i % patternCounts];
    //             UseSimulateBoard sbrd(brd, pattern);
    //             sbrd.move(m[0], m[1]);
    //             winCounts[l] += sbrd.run_simulate(1);
    //         }
    //     }
    //     double maxWin = -playoutCount;
    //     auto action = legalMoves[0];
    //     for(int i = 0; i < legalMoves.size(); ++i){
    //         if(winCounts[i] > maxWin){
    //             maxWin = winCounts[i];
    //             action = legalMoves[i];
    //         }
    //     }
    //     return Hand({action[0], action[1]});
    // }

    // Hand decideHand_MinMax(const Board& brd){
    //     auto patterns = getLegalPattern(brd);
    //     UseSimulateBoard simbrd(brd);
    //     auto legalMoves = simbrd.getLegalMove();
    //     int playout = playoutCount / legalMoves.size();
    //     vector<double> winCounts;
    //     for(int i = 0; i < legalMoves.size(); ++i){
    //         winCounts.push_back(0.0);
    //     }
    //     vector<double> patternCounts;
    //     for(int p = 0; p < patterns.size(); ++p){
    //         patternCounts.push_back(0.0);
    //     }
    //     for(int l = 0; l < legalMoves.size(); ++l){
    //         for(int p = 0; p < patterns.size(); ++p){
    //             patternCounts[p] = 0.0;
    //         }
    //         for(int i = 0; i < playout; ++i){
    //             auto m = legalMoves[l];
    //             auto win = 0.0;
    //             UseSimulateBoard sbrd(brd, patterns[i % patterns.size()]);
    //             sbrd.move(m[0], m[1]);
    //             patternCounts[i % patterns.size()] += sbrd.run_simulate(1);
    //         }
    //         int minWin = playout;
    //         for(int i = 0; i < patternCounts.size(); ++i){
    //             if(patternCounts[i] < minWin){
    //                 minWin = patternCounts[i];
    //                 winCounts[l] = minWin;
    //             }
    //         }
    //     }
    //     double maxWin = -playoutCount;
    //     auto action = legalMoves[0];
    //     for(int i = 0; i < legalMoves.size(); ++i){
    //         if(winCounts[i] > maxWin){
    //             maxWin = winCounts[i];
    //             action = legalMoves[i];
    //         }
    //     }
    //     return Hand({action[0], action[1]});
    // }

    // Hand decideHand_Vote(const Board& brd){
    //     auto patterns = getLegalPattern(brd);
    //     const int playout = playoutCount / patterns.size();
    //     UseSimulateBoard simbrd(brd);
    //     auto legalMoves = simbrd.getLegalMove();
    //     auto action = legalMoves[0];
    //     vector<int> voteCounts;
    //     for(int i = 0; i < legalMoves.size(); ++i){
    //         voteCounts.push_back(0);
    //     }
    //     for(auto p: patterns){
    //         vector<double> winCounts;
    //         for(int i = 0; i < legalMoves.size(); ++i){
    //             winCounts.push_back(0.0);
    //         }
    //         for(int i = 0; i < playout; ++i){
    //             auto m = legalMoves[i % legalMoves.size()];
    //             auto win = 0.0;
    //             UseSimulateBoard sbrd(brd, p);
    //             sbrd.move(m[0], m[1]);
    //             winCounts[i % legalMoves.size()] += sbrd.run_simulate(1);
    //         }
    //         double maxWin = -playoutCount;
    //         int vote = 0;
    //         for(int i = 0; i < legalMoves.size(); ++i){
    //             if(winCounts[i] > maxWin){
    //                 maxWin = winCounts[i];
    //                 vote = i;
    //             }
    //         }
    //         voteCounts[vote]++;
    //     }
    //     int maxVote = 0;
    //     for(int i = 0; i < legalMoves.size(); ++i){
    //         if(voteCounts[i] > maxVote){
    //             maxVote = voteCounts[i];
    //             action = legalMoves[i];
    //         }
    //     }
    //     return Hand({action[0], action[1]});
    // }

    virtual std::string decideHand(std::string res){
        game = Geister(res);
        return decideHand_Random();
    }
};