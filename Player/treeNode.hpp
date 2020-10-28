#include "geister.hpp"

class TreeNode{
public:
    Geister state;
    std::vector<TreeNode> children;

    TreeNode();
    TreeNode(const Geister& game): state(game){
    }

    double evaluate(){
        switch (state.result())
        {
        case Result::Draw:
            return 0.0;
        case Result::Escape1st:
            return 1.0;
        case Result::TakeBlue1st:
            return 1.0;
        case Result::TakenRed1st:
            return 1.0;
        case Result::Escape2nd:
            return -1.0;
        case Result::TakeBlue2nd:
            return -1.0;
        case Result::TakenRed2nd:
            return -1.0;
        default:
            break;
        }
		return 0.0;
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
        if(depth == 0){
            auto eval = evaluate();
            // state.printBoard();
            // std::cout << eval << std::endl;
            return eval;
        }
        // state.printBoard();
        // std::cout << "alpha:" << alpha << ", beta:" << beta << std::endl;
        state.changeSide();
        if(!expand()){
            state.changeSide();
            auto eval = evaluate();
            // state.printBoard();
            // std::cout << eval << std::endl;
            return eval;
        }
        state.changeSide();
        int childCount = children.size();
        
        // /* 
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
        for(int i = childCount - 1; i >= 0; --i){
            alpha = std::max(alpha, -children.back().search(depth - 1, -beta, -alpha));
            if(alpha >= beta){
                // std::cout << "alpha:" << alpha << ", beta:" << beta << std::endl;
                return alpha;
            }
            children.pop_back();
        }
        return alpha;
    }

	std::vector<double> evalList(int depth = 5){
        expand();
        int best = 0;
		std::vector<double> result;
        double bestval = -1.0;
        double alpha = -1.0;
        double beta = 1.0;
        for(int i = 0; i < children.size(); ++i){
            auto& child = children[i];
            double eval = child.search(depth, alpha, beta);
            // child.state.printBoard();
            // std::cout << eval << std::endl;
            result.emplace_back(eval);
        }
        // std::cout << lm[best] << ":" << best << "(" << bestval << ")" << std::endl;
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
        double bestval = -1.0;
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
            alpha = std::max(alpha, child.search(depth, alpha, beta));
            // child.state.printBoard();
            std::cout << alpha << std::endl;
            if(alpha > bestval){
                bestval = alpha;
                best = i;
            }
        }
        // std::cout << lm[best] << ":" << best << "(" << bestval << ")" << std::endl;
        return lm[best];
    }
};