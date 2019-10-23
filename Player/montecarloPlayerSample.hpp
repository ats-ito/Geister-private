#include "../Player.hpp"
#include "../random.hpp"
#include "../Simulator.hpp"
#include <vector>
#include <iostream>

#ifndef PLAYOUT_COUNT
#define PLAYOUT_COUNT 1000
#endif


class MonteCarloPlayerSample: public Player{
    Simulator sim;
    int playoutCount = PLAYOUT_COUNT;
    cpprefjp::random_device rd;
    std::mt19937 mt; 
public:
    MonteCarloPlayerSample(): mt(rd()){
    }

    virtual std::string decideRed(){
        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string res){
        game = Geister(res);
        // 合法手の列挙
        auto legalMoves = game.getLegalMove1st();
        // 勝率計算用配列
        std::vector<double> winCounts;
        for(int i = 0; i < legalMoves.size(); ++i){
            winCounts.push_back(0.0);
        }
        // 規定回数のプレイアウトを実行
        for(int i = 0; i < playoutCount; ++i){
            auto m = legalMoves[i % legalMoves.size()];// 全着手を順番に選択
            sim.init(game);// シミュレータの初期化（相手の駒パターンの設定）
            sim.geister.move(m.unit.name, m.direct.toChar());// 指定の着手を指した後の局面を作成
            winCounts[i % legalMoves.size()] += sim.playout();// プレイアウトの実行と結果の加算（勝：＋１，敗：－１，引分：０）
        }
        // 最大勝率の手を探す
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
};