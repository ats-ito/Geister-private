#include <string>
#include "random.hpp"
#include "player.hpp"


class RandomPlayer: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
public:
    RandomPlayer(): mt(rd()){
    }

    virtual std::string decideRed(){
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string_view res){
        game.setState(res);

        auto legalMoves = candidateHand();
        std::uniform_int_distribution<int> serector1(0, legalMoves.size() - 1);
        auto action = legalMoves[serector1(mt) % legalMoves.size()];
        return action;
    }

    virtual std::vector<Hand> candidateHand(){
        auto legalMoves = game.getLegalMove1st();
        for(auto&& lm: legalMoves){
            auto& unit = lm.unit;
            auto& direct = lm.direct;
            if(unit.y == 0 && ((unit.x == 0 && direct == Direction::West) || (unit.x == 5 && direct == Direction::East))){
                return {lm};
            }
        }
        return legalMoves;
    }
};