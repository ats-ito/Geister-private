#include "simulator.hpp"


class Simulator1: public Simulator{
    using Simulator::Simulator;

    std::vector<Hand> getCandidateMove1st(){
        auto legalMoves = current.getLegalMove1st();
        for(auto&& lm: legalMoves){
            auto& unit = lm.unit;
            auto& direct = lm.direct;
            if(unit.y() == 0 && ((unit.x() == 0 && direct == Direction::West) || (unit.x() == 5 && direct == Direction::East))){
                return {lm};
            }
        }
        return legalMoves;
    }
    std::vector<Hand> getCandidateMove2nd(){
        auto legalMoves = current.getLegalMove2nd();
        for(auto&& lm: legalMoves){
            auto& unit = lm.unit;
            auto& direct = lm.direct;
            if(unit.y() == 5 && ((unit.x() == 0 && direct == Direction::West) || (unit.x() == 5 && direct == Direction::East))){
                return {lm};
            }
        }
        return legalMoves;
    }
    
    double playout(){
        std::vector<Hand> lm;
        while(true){
            if(current.result() != Result::OnPlay)
                break;
            // 相手の手番
            lm = getCandidateMove2nd();
            std::uniform_int_distribution<int> selector1(0, lm.size() - 1);
            auto m = lm[selector1(mt)];
            current.move(m);
            if(current.result() != Result::OnPlay)
                break;
            // 自分の手番
            lm = getCandidateMove1st();
            std::uniform_int_distribution<int> selector2(0, lm.size() - 1);
            m = lm[selector2(mt)];
            current.move(m);
        }
        return evaluate();
    }

};