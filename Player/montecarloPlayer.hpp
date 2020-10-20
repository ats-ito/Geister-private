#include "player.hpp"
#include "random.hpp"
#include "Simulator/all.hpp"
#include <vector>
#include <iostream>

#ifndef PLAYOUT_COUNT
#define PLAYOUT_COUNT 1000
#endif

#ifndef SIMULATOR
#define SIMULATOR Simulator0
#endif

#define DECIDEHAND decideHand_Average


class MonteCarloPlayer: public Player{
    int playoutCount = PLAYOUT_COUNT;
    cpprefjp::random_device rd;
    std::mt19937 mt;

protected:
    // 可能な相手駒のパターンを列挙
    std::vector<std::string> getLegalPattern(Geister game){
        std::vector<std::string> res;
        std::vector<char> blue;
        std::vector<char> red;
        std::vector<char> unknown;
        // 色ごとに判明済みの駒をリスト化
        for(int u = 8; u < 16; ++u){
            if(game.allUnit()[u].color() == UnitColor::blue)
                blue.push_back(u - 8 + 'A');
            else if(game.allUnit()[u].color() == UnitColor::red)
                red.push_back(u - 8 + 'A');
            else
                unknown.push_back(u - 8 + 'A');
        }
        // 判明している情報と矛盾するパターンを除外
        for(auto p: pattern){
            // 青と分かっている駒を含むパターンを除外
            int checkB = 0;
            for(auto b: blue){
                if(std::string(p).find(b) != std::string::npos){
                    checkB++;
                    break;
                }
            }
            if(checkB != 0) continue;
            // 赤と分かっている駒を含まないパターンを除外
            int checkR = 0;
            for(auto r: red){
                if(std::string(p).find(r) == std::string::npos){
                    checkR++;
                    break;
                }
            }
            if(checkR != 0) continue;
            res.push_back(std::string(p));
        }
        return res;
    }

    std::string decideHand_Random(){
        // 合法手の列挙と着手の初期化
        auto legalMoves = game.getLegalMove1st();
        auto action = legalMoves[0];
        // 勝率記録用配列
        std::vector<double> rewards(legalMoves.size(), 0.0);
        for(size_t i = 0; i < legalMoves.size(); ++i){
            Geister tmp = game;
            tmp.move(legalMoves[i]);
            rewards[i] += Simulator(tmp).run(playoutCount);
        }
        //*/

        // 最大勝率の手を探す
        double maxReward = rewards[0];
        for(size_t i = 1; i < legalMoves.size(); ++i){
            if(rewards[i] > maxReward){
                maxReward = rewards[i];
                action = legalMoves[i];
            }
        }
        return action;
        // // 合法手の列挙
        // auto legalMoves = game.getLegalMove1st();
        // // 勝率計算用配列
        // std::vector<double> winCounts;
        // for(int i = 0; i < legalMoves.size(); ++i){
        //     winCounts.push_back(0.0);
        // }
        // // 規定回数のプレイアウトを実行
        // for(int i = 0; i < 10000; ++i){
        //     auto m = legalMoves[i % legalMoves.size()];// 全着手を順番に選択
        //     auto sim = SIMULATOR(game);// シミュレータの初期化（相手の駒パターンの設定）
        //     sim.root.move(m.unit.name, m.direct.toChar());// 指定の着手を指した後の局面を作成
        //     winCounts[i % legalMoves.size()] += sim.run();// プレイアウトの実行と結果の加算（勝：＋１，敗：－１，引分：０）
        // }
        // // 最大勝率の手を探す
        // double maxWin = -playoutCount;
        // auto action = legalMoves[0];
        // for(int i = 0; i < legalMoves.size(); ++i){
        //     if(winCounts[i] > maxWin){
        //         maxWin = winCounts[i];
        //         action = legalMoves[i];
        //     }
        // }
        // return action;
    }

    std::string decideHand_Average(){
        auto legalMoves = game.getLegalMove1st();
        auto patterns = getLegalPattern(game);
        const int playout = std::max(static_cast<int>(playoutCount / patterns.size()), 1);
        std::vector<double> rewards(legalMoves.size(), 0.0);
        
        for(int l = 0; l < legalMoves.size(); ++l){
            auto m = legalMoves[l];
            SIMULATOR s(game);
            s.root.move(m);
            for(auto&& pattern: patterns){
                rewards[l] += s.run(pattern, playout);
            }
        }

        auto&& max_iter = std::max_element(rewards.begin(), rewards.end());
        int index = std::distance(rewards.begin(), max_iter);
        return legalMoves[index];
    }

    std::string decideHand_MinMax(){
        const auto patterns = getLegalPattern(game);
        const int playout = std::max(static_cast<int>(playoutCount / patterns.size()), 1);
        const auto legalMoves = game.getLegalMove1st();
        std::vector<double> rewards(legalMoves.size(), 0.0);
        
        for(int l = 0; l < legalMoves.size(); ++l){
            auto m = legalMoves[l];
            SIMULATOR s(game);
            s.root.move(m);

            std::vector<double> patternRewards(patterns.size(), 0.0);
            for(size_t p = 0; p < patterns.size(); ++p){
                patternRewards[p] = s.run(patterns[p], playout);
            }
            rewards[l] = *std::min(patternRewards.begin(), patternRewards.end());
        }
        
        auto&& max_iter = std::max_element(rewards.begin(), rewards.end());
        int index = std::distance(rewards.begin(), max_iter);
        return legalMoves[index];
    }

    std::string decideHand_Vote(){
        auto patterns = getLegalPattern(game);
        const int playout = playoutCount / patterns.size();
        Simulator sim(game);
        auto legalMoves = game.getLegalMove1st();
        auto action = legalMoves[0];
        std::vector<int> voteCounts;
        for(int i = 0; i < legalMoves.size(); ++i){
            voteCounts.push_back(0);
        }
        for(auto p: patterns){
            std::vector<double> winCounts;
            for(int i = 0; i < legalMoves.size(); ++i){
                winCounts.push_back(0.0);
            }
            for(int i = 0; i < playout; ++i){
                auto m = legalMoves[i % legalMoves.size()];
                auto win = 0.0;
                Simulator s(game, p);
                s.root.move(m);
                winCounts[i % legalMoves.size()] += s.run();
            }
            double maxWin = -playoutCount;
            int vote = 0;
            for(int i = 0; i < legalMoves.size(); ++i){
                if(winCounts[i] > maxWin){
                    maxWin = winCounts[i];
                    vote = i;
                }
            }
            voteCounts[vote]++;
        }
        int maxVote = 0;
        for(int i = 0; i < legalMoves.size(); ++i){
            if(voteCounts[i] > maxVote){
                maxVote = voteCounts[i];
                action = legalMoves[i];
            }
        }
        return action;
    }

public:
    MonteCarloPlayer(): mt(rd()){
    }

    virtual std::string decideRed(){
        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string res){
        game = Geister(res);
        return DECIDEHAND();
    }
};