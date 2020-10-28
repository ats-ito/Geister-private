#include <vector>
#include <iostream>
#include "random.hpp"
#include "player.hpp"
#include "TreeNode.hpp"


class TreePlayer: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;
public:
    TreePlayer(): mt(rd()){
    }

    virtual std::string decideRed(){
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
        return pattern[serector(mt)];
    }

    virtual std::string decideHand(std::string res){
        game.setState(res);
        for(auto&& u: game.allUnit()){
            if(u.color() == UnitColor::unknown)
                game.setColor(u.name(), UnitColor::purple);
        }
        // game.printAll();
        auto tn = TreeNode(game);
        return tn.bestHand(5);
    }
};