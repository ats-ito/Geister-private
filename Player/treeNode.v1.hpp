#include "geister.hpp"
#include <future>
#include <thread>
#include <limits>
#include <unordered_map>

class TreeNode{
public:
    Geister state;
    std::vector<TreeNode> children;

    std::vector<double> rootAlpha;
    std::vector<double> rootBeta;

    inline static std::unordered_map<std::string, std::pair<double, int>> visited;

    TreeNode();
    TreeNode(const Geister& game):
    state(game),
    rootAlpha(32, -10.0),
    rootBeta(32, 10.0)
    {
    }

    double evaluate(int depth){
        switch (state.result())
        {
        case Result::Draw:
            return 0.0;
        case Result::Escape1st:
            return 1.0+(static_cast<double>(depth)/20.0);
        case Result::TakeBlue1st:
            return 1.0+(static_cast<double>(depth)/20.0);
        case Result::TakenRed1st:
            return 1.0+(static_cast<double>(depth)/20.0);
        case Result::Escape2nd:
            return -1.0-(static_cast<double>(depth)/20.0);
        case Result::TakeBlue2nd:
            return -1.0-(static_cast<double>(depth)/20.0);
        case Result::TakenRed2nd:
            return -1.0-(static_cast<double>(depth)/20.0);
        default:
            break;
        }
        // return 0.0;

        double value = 0.0;

        auto& units = state.allUnit();

        //*/
        double takenValue = 0.0;
        int takenBlue1st = state.takenCount(UnitColor::Blue);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is1st() && u.isBlue() && u.onBoard();});
        int takenRed1st = state.takenCount(UnitColor::Red);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is1st() && u.isRed() && u.onBoard();});
        int takenBlue2nd = state.takenCount(UnitColor::blue);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is2nd() && u.isBlue() && u.onBoard();});
        int takenRed2nd = state.takenCount(UnitColor::red);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is2nd() && u.isRed() && u.onBoard();});

        takenValue += takenBlue1st * -0.1;
        takenValue += takenRed1st * 0.1;
        takenValue += takenBlue2nd * 0.1;
        takenValue += takenRed2nd * -0.1;//*/
        
        for(int i = 0; i < 8; ++i){
            if(std::find_if(units.begin() + 8, units.end(), [&](auto& u){
                return u.isBlue() && u.y() == 5 && (u.x() == 0 || u.x() == 5);
                }) != units.end()){
                    break;
                }
            auto& unit = units[i];
            if(unit.isBlue() && unit.y() == 0
            && ((unit.x() == 0 && std::find_if(units.begin() + 8, units.end(), [&](auto& u){
                return (u.x() == 0 && u.y() == 1) || (u.x() == 1 && u.y() == 0);
                }) != units.end())
            || (unit.x() == 5 && std::find_if(units.begin() + 8, units.end(), [&](auto& u){
                return (u.x() == 5 && u.y() == 1) || (u.x() == 4 && u.y() == 0);
                }) != units.end())))
            {
                value = 1.0;
            }
        }

        // return value;

        //*
        double positionValue = 0.0;
        static constexpr std::array<double, 36> bluePos = {
            0.300, 0.200, 0.100, 0.100, 0.200, 0.300,
            0.100, 0.070, 0.030, 0.030, 0.070, 0.100,
            0.050, 0.020, 0.010, 0.010, 0.020, 0.050,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        };

        static constexpr std::array<double, 36> redPos = {
            -0.500, 0.0, 0.0, 0.0, 0.0, -0.500,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        };

        for(int i = 0; i < 8; ++i){
            auto& unit = units[i];
            if(unit.onBoard()){
                positionValue += bluePos[unit.x() + unit.y()*6];
            }
            // if(unit.isBlue()){
            //     value += bluePos[unit.x() + unit.y()*6];
            // }
            // if(unit.isRed()){
            //     value += redPos[unit.x() + unit.y()*6];
            // }
        }
        for(int i = 8; i < 16; ++i){
            auto& unit = units[i];
            if(unit.onBoard()){
                positionValue += bluePos[unit.x() + (5-unit.y())*6];
            }
        }//*/

        value = takenValue * 0.5 + positionValue * 0.5;
        value = std::max(-0.999999, std::min(0.999999, value));
        // if(!std::isfinite(value)){
        //     std::cerr << state << ":" << value;
        // }
		return value;
    }

    bool expand(){
        if(state.isEnd())
            return false;
        auto lm = state.getLegalMove1st();
        for(auto&& m: lm){
            // std::cout << m << ",";
            children.emplace_back(this->state);
            auto& child = children.back();
            child.state.move(m);
        }
        // std::cout << ": " << state << std::endl;
        return true;
    }

    double search(int depth, double alpha, double beta){
        // state.printBoard();
        // std::cout << state << std::endl;
        // if(visited.count(state) > 0 && depth <= visited[state].second){
        //     std::cerr << "visited: " << state.toString() << " = " << visited[state].first << std::endl;
        //     // state.printBoard();
        //     return -visited[state].first;
        // }
        if(depth == 0 || state.isEnd()){ 
            auto eval = evaluate(depth);
            // state.printBoard();
            // std::cout << eval << std::endl;
            // visited[state] = {eval, depth};
            return eval;
        }
        // state.printBoard();
        state.changeSide();
        expand();
        // if(!expand()){
        //     state.changeSide();
        //     auto eval = evaluate();
        //     // state.printBoard();
        //     // std::cout << eval << std::endl;
        //     return eval;
        // }
        // state.changeSide();
        int childCount = children.size();
        
        /* 枝刈りなし
        double bestval = -1.0;
        for(int i = childCount - 1; i >= 0; --i){
            bestval = std::max(bestval, children.back().search(depth - 1, beta, alpha));
            children.pop_back();
        }
        return -bestval;/**/

        // state.changeSide();
        // state.printBoard();
        // std::cout << state.getLegalMove1st()[best] << std::endl;
        // state.changeSide();
        // std::cout << "alpha:" << alpha << ", beta:" << beta << std::endl;

        // std::cerr << alpha << "[" << depth << "]" << state << std::endl;
        // if(!std::isfinite(alpha)){
        //     std::cerr << "alpha is nan\n";
        //     exit(1);
        // }
        
        for(int i = childCount - 1; i >= 0; --i){
            auto childVal = children.back().search(depth - 1, -beta, -alpha);
            // std::cerr << "child " << i << " val" << childVal << std::endl;
            alpha = std::max(alpha, childVal);
            // if(!std::isfinite(alpha)){
            //     std::cerr << state << "[" << depth << "]:" << alpha;
            // }
            if(alpha >= beta){
                // std::cout << "alpha:" << alpha << ", beta:" << beta << std::endl;
                // visited[state] = {alpha, depth};
                return -alpha;
            }
            children.pop_back();
        }
        // visited[state] = {alpha, depth};
        return -alpha;
    }

    static double static_search(TreeNode* tn, int depth, double alpha, double beta){
        return tn->search(depth, alpha, beta);
    }

	std::vector<double> evalList(int depth = 5){
        expand();
        constexpr int parralellCount = 4;
        int best = 0;
		std::vector<double> result;
        double bestval = -100.0;
        for(int i = 0; i < children.size();){
            std::vector<std::future<double>> res;
            for(int t = 0; t < parralellCount; ++t){
                if(i < children.size()){
                    // std::cout << i << std::endl;
                    auto& child = children[i];
                    res.emplace_back(std::async(std::launch::async, [&](){return static_search(&child, depth-1, -rootBeta[i], -rootAlpha[i]);}));
                    i++;
                }
            }
            for(auto&& r: res){
                result.emplace_back(r.get());
                // std::cerr << result.back() << std::endl;
                // if(!std::isfinite(result.back())){
                //     std::cerr << result.back() << std::endl;
                // }
            }

            // auto& child = children[i];
            // double eval = child.search(depth-1, alpha, beta);
            // result.emplace_back(eval);
            // std::cout << i++ << std::endl;
            // std::cout << eval << std::endl;
            // child.state.printBoard();
        }
        // std::cout << lm[best] << ":" << best << "(" << bestval << ")" << std::endl;
        // if(std::find_if(result.begin(), result.end(), [&](auto x){return !std::isfinite(x);}) != result.end()){
        //     std::cerr << state << std::endl;
        //     for(int i = 0; i < result.size(); ++i){
        //         std::cerr << children[i].state << ":" << result[i] << ",";
        //     }
        //     std::cerr << std::endl;
        //     exit(1);
        // }
        return result;
	}

    std::string bestHand(int depth = 5){
        auto lm = state.getLegalMove1st();

		// auto vals = evalList(depth);
		// auto iter = std::max_element(vals.begin(), vals.end());
		// auto index = std::distance(vals.begin(), iter);
		// return lm[index];

        expand();
        int best = 0;
        double bestval = -100.0;
        double alpha = -1.0;
        double beta = 1.0;
        // for(int i = 0; i < children.size(); ++i){
        //     auto& child = children[i];
        //     double eval = child.search(depth, alpha, beta);
        //     // child.state.printBoard();
        //     // std::cout << eval << std::endl;
        //     if(eval > bestval){
        //         bestval = eval;
        //         best = i;
        //     }
        // }
        for(int i = 0; i < children.size(); ++i){
            auto& child = children[i];
            alpha = std::max(alpha, child.search(depth-1, -beta, -alpha));
            // child.state.printBoard();
            // std::cout << lm[i] << ": ";
            // std::cout << alpha << std::endl;
            if(alpha > bestval){
                bestval = alpha;
                best = i;
            }
        }
        // std::cout << lm[best] << ":" << best << "(" << bestval << ")" << std::endl;
        return lm[best];
    }
};