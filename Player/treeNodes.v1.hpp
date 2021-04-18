#include "geister.hpp"
#include <future>
#include <thread>
#include <limits>
#include <unordered_map>

class BaseNode;
class RootNode;
class StandardNode;
class TakeNode;
class EscapeNode;

using EvalBoard = std::pair<double, Geister>;

int cutCount = 0;

inline bool isTakeMove(const Geister& game, const Hand& hand){
    int nowX = hand.unit.x();
    int nowY = hand.unit.y();
    int nextX = nowX;
    int nextY = nowY;
    if(hand.direct == Direction::North) nextY--;
    else if(hand.direct == Direction::East) nextX++;
    else if(hand.direct == Direction::West) nextX--;
    else if(hand.direct == Direction::South) nextY++;

    const auto target = game.getUnitByPos(nextX, nextY);
    return target && (hand.unit.is1st() ^ target->is1st());
}

class BaseNode{
public:
    Geister state; // 局面情報
    std::vector<std::shared_ptr<BaseNode>> children; // 子ノードリスト
    std::vector<double> blueness; // 青らしさ

protected:
    // BaseNodeは直接は使わない
    BaseNode(const Geister& game):
    state(game),
    blueness(16, 0.5) // 青らしさは指定がなければ0.5で初期化
    {
    }

    BaseNode(const Geister& game, std::vector<double> blueness):
    state(game),
    blueness(blueness)
    {
    }

    virtual bool expand(){
        if(state.isEnd()){
            return false;
        }
        auto lm = state.getLegalMove1st();
        for(auto&& m: lm){
            if(isTakeMove(state, m)){ // 駒を取る手の場合
                children.emplace_back(std::make_shared<TakeNode>(this->state, m, blueness));
            }
            else{
                children.emplace_back(std::make_shared<StandardNode>(this->state, m, blueness));
            }
        }
        for(int i = 0; i < 8; ++i){
            if(auto& unit = state.allUnit()[i]; blueness[unit.id()] > 0.0){
                if(unit.y() == 0){
                    if(unit.x() == 0){
                        children.emplace_back(std::make_shared<EscapeNode>(this->state, Hand{unit, Direction::West}, blueness));
                    }
                    else if(unit.x() == 5){
                        children.emplace_back(std::make_shared<EscapeNode>(this->state, Hand{unit, Direction::East}, blueness));
                    }
                }
            }
        }
        return true;
    }

    // 評価関数
    virtual double evaluate(int depth){
        auto& units = state.allUnit();
        // std::for_each(blueness.begin(), blueness.end(), [&](auto x){std::cerr << x << ",";});
        // std::cerr << "\n";

        switch (state.result()){
        case Result::Draw: return 0.0;
        case Result::Escape1st: return (1.0+(static_cast<double>(depth)/20.0));
        case Result::TakeBlue1st: return 1.0+(static_cast<double>(depth)/20.0);
        case Result::TakenRed1st: return 1.0+(static_cast<double>(depth)/20.0);
        case Result::Escape2nd: return -1.0-(static_cast<double>(depth)/20.0);
        case Result::TakeBlue2nd: return -1.0-(static_cast<double>(depth)/20.0);
        case Result::TakenRed2nd: return -1.0-(static_cast<double>(depth)/20.0);
        default: break;
        }
        // return {0.0, state};

        double value = 0.0;
        

        //*/
        double takenValue = 0.0;
        int takenBlue1st = state.takenCount(UnitColor::Blue);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is1st() && u.isBlue() && u.onBoard();});
        int takenRed1st = state.takenCount(UnitColor::Red);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is1st() && u.isRed() && u.onBoard();});
        int takenBlue2nd = state.takenCount(UnitColor::blue);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is2nd() && u.isBlue() && u.onBoard();});
        int takenRed2nd = state.takenCount(UnitColor::red);// std::count_if(units.begin(), units.end(), [](auto& u){ return u.is2nd() && u.isRed() && u.onBoard();});

        takenValue += takenBlue1st * -0.1;
        takenValue += takenRed1st * 0.1;
        takenValue += takenBlue2nd * 0.1;
        takenValue += takenRed2nd * -0.1;
        //*/
        // return value;

        //*
        double positionValue = 0.0;
        static constexpr std::array<double, 36> bluePos = {
            0.300, 0.200, 0.100, 0.100, 0.200, 0.300,
            0.100, 0.070, 0.030, 0.030, 0.070, 0.100,
            0.050, 0.020, 0.010, 0.010, 0.020, 0.050,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000
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
                positionValue -= bluePos[unit.x() + (5-unit.y())*6];
            }
        }//*/

        value = takenValue * 0.5 + positionValue * 0.5;
		return std::max(-0.999999, std::min(0.999999, value));
    }

public:

    virtual EvalBoard search(int depth, double alpha, double beta){
        // 葉ノードの場合は親ノードと局面の向きが一致するため評価値を反転しない
        if(depth == 0 || state.isEnd()){
            auto eval = evaluate(depth);
            return {eval, state};
        }
        state.changeSide();
        for(int i = 0; i < 8; ++i){
            std::swap(blueness[i], blueness[i+8]);
        }
        expand();
        int childCount = children.size();
        Geister g;// = state;
        // g.changeSide();

        int pruneMode = 3;
        if(pruneMode == 0){
        alpha = -100.0;
        for(int i = childCount - 1; i >= 0; --i){
            auto child = children.back();
            auto eb = child->search(depth - 1, -beta, -alpha);
            if(eb.first > alpha){
                alpha = eb.first;
                g = eb.second;
                g.changeSide();
            }
            children.pop_back();
        }
        }
        else if(pruneMode == 1){
        alpha = -100.0;
        for(int i = childCount - 1; i >= 0; --i){
            auto child = children.back();
            auto eb = child->search(depth - 1, -beta, -alpha);
            if(eb.first > alpha){
                alpha = eb.first;
                g = eb.second;
                g.changeSide();
                if(alpha >= beta){
                    cutCount++;
                    return {-alpha, g};
                }
            }
            // alpha = std::max(alpha, eb.first);
            children.pop_back();
        }
        }
        else if(pruneMode == 2){
        // alpha = -100.0;
        double maxEval = -100.0;
        for(int i = childCount - 1; i >= 0; --i){
            auto child = children.back();
            auto eb = child->search(depth - 1, -beta, -alpha);
            // if(eb.first > alpha){
            if(eb.first > maxEval){
                // alpha = eb.first;
                maxEval = eb.first;
                alpha = std::max(alpha, maxEval);
                g = eb.second;
                g.changeSide();
                if(alpha >= beta){
                    cutCount++;
                    return {-alpha, g};
                }
            }
            // alpha = std::max(alpha, eb.first);
            children.pop_back();
        }
        }
        else if(pruneMode == 3){
        double maxEval = alpha;
        for(int i = childCount - 1; i >= 0; --i){
            auto child = children.back();
            auto eb = child->search(depth - 1, -beta, -maxEval);
            if(eb.first > maxEval){
                maxEval = eb.first;
                g = eb.second;
                g.changeSide();
                if(maxEval >= beta){
                    cutCount++;
                    return {-maxEval, g};
                }
            }
            // alpha = std::max(alpha, eb.first);
            children.pop_back();
        }
        if(maxEval == alpha){
            // std::cerr << "better hand is not found in depth " << depth << ". (" << alpha << ")\n";
            maxEval = -100.0;
        }
        alpha = maxEval;
        }

        // return {-maxEval, g};
        return {-alpha, g};
    }
};

// 非同期実行用の探索呼び出し関数
EvalBoard static_search(std::shared_ptr<BaseNode> tn, int depth, double alpha, double beta){
    return tn->search(depth, alpha, beta);
}

// 根局面
class RootNode: public BaseNode{
public:
    std::vector<double> rootAlpha;
    std::vector<double> rootBeta;

    constexpr static uint32_t parallelCount = 8;

    RootNode(const Geister& game):
    BaseNode(game),
    rootAlpha(32, -10.0),
    rootBeta(32, 10.0)
    {
    }

    RootNode(const Geister& game, std::vector<double> blueness):
    BaseNode(game, blueness),
    rootAlpha(32, -10.0),
    rootBeta(32, 10.0)
    {
    }

    virtual bool expand(){
        if(state.isEnd()){
            return false;
        }
        auto lm = state.getLegalMove1st();
        for(auto&& m: lm){
            if(isTakeMove(state, m)){ // 駒を取る手の場合
                children.emplace_back(std::make_shared<TakeNode>(this->state, m, blueness));
            }
            else{
                children.emplace_back(std::make_shared<StandardNode>(this->state, m, blueness));
            }
        }
        return true;
    }

	std::vector<EvalBoard> evalList(int depth = 5){
        cutCount = 0;
        expand();
		std::vector<EvalBoard> result;
        for(int i = 0; i < children.size();){
            std::vector<std::future<EvalBoard>> res;
            for(int t = 0; t < parallelCount && i < children.size(); ++t, ++i){
                // std::cerr << i << "," << -rootBeta[i] << "," << -rootAlpha[i] << "\n";
                std::shared_ptr<BaseNode> child = children[i];
                // スマートポインタを参照キャプチャするとマルチスレッド実行時に落ちる
                res.emplace_back(std::async(std::launch::async, [=](){return child->search(depth-1, -rootBeta[i], -rootAlpha[i]);}));
            }
            for(auto& r: res){
                result.emplace_back(r.get());
                // result.back().second.printBoard();
            }
        }
        // std::cerr << cutCount << " cut\n";
        return result;
	}

	std::vector<EvalBoard> evalList(int depth, std::vector<std::vector<double>> childBlueness){
        cutCount = 0;
        expand();
        for(int i = 0; i < children.size(); ++i){
            auto child = children[i];
            child->blueness = childBlueness[i];
        }
		std::vector<EvalBoard> result;
        for(int i = 0; i < children.size();){
            std::vector<std::future<EvalBoard>> res;
            for(int t = 0; t < parallelCount && i < children.size(); ++t, ++i){
                // std::cerr << i << "," << -rootBeta[i] << "," << -rootAlpha[i] << "\n";
                std::shared_ptr<BaseNode> child = children[i];
                // スマートポインタを参照キャプチャするとマルチスレッド実行時に落ちる
                res.emplace_back(std::async(std::launch::async, [=](){return child->search(depth-1, -rootBeta[i], -rootAlpha[i]);}));
            }
            for(auto& r: res){
                result.emplace_back(r.get());
                // result.back().second.printBoard();
            }
        }
        // std::cerr << cutCount << " cut\n";
        return result;
	}

    std::string bestHand(int depth = 5){
        auto lm = state.getLegalMove1st();
        std::for_each(lm.begin(), lm.end(), [&](auto& hand){std::cerr << hand << ",";});
        std::cerr << std::endl;

        expand();
        int best = 0;
        double bestval = -100.0;
        double alpha = -10.0;
        double beta = 10.0;
        for(int i = 0; i < children.size(); ++i){
            auto& child = children[i];
            auto eb = child->search(depth-1, -beta, -alpha);
            alpha = std::max(alpha, eb.first);
            if(alpha > bestval){
                bestval = alpha;
                best = i;
            }
        }
        std::cerr << best << ":" << lm[best] << std::endl;
        return lm[best];
    }
};

// 通常の局面
class StandardNode: public BaseNode{
public:
    Hand beforeHand;
    StandardNode(const Geister& state, const Hand& hand):
    BaseNode(state),
    beforeHand(hand)
    {
        this->state.move(hand);
    }
    StandardNode(const Geister& state, const Hand& hand, std::vector<double> blueness):
    BaseNode(state, blueness),
    beforeHand(hand)
    {
        this->state.move(hand);
    }
};

// 駒取りが発生する局面
class TakeNode: public BaseNode{
protected:
    std::shared_ptr<StandardNode> blueNode;
    std::shared_ptr<StandardNode> redNode;

    Hand beforeHand;
    int takeUnitID;
public:
    TakeNode(const Geister& state, const Hand& hand):
    BaseNode(state),
    beforeHand(hand)
    { 
        setup();
    }
    TakeNode(const Geister& state, const Hand& hand, std::vector<double> blueness):
    BaseNode(state, blueness),
    beforeHand(hand)
    {
        setup();
    }

    void setup(){
        int nowX = beforeHand.unit.x();
        int nowY = beforeHand.unit.y();
        int nextX = nowX;
        int nextY = nowY;
        if(beforeHand.direct == Direction::North) nextY--;
        else if(beforeHand.direct == Direction::East) nextX++;
        else if(beforeHand.direct == Direction::West) nextX--;
        else if(beforeHand.direct == Direction::South) nextY++;

        takeUnitID = state.getUnitByPos(nextX, nextY)->id();

        if(blueness[takeUnitID] > 0.0 && state.takenCount(UnitColor::blue) < 4){
            state.setColor(takeUnitID, UnitColor::blue);
            blueNode = std::make_shared<StandardNode>(state, beforeHand, blueness);
        }
        else{
            // std::cerr << state.allUnit()[takeUnitID].name() << " is red!\n";
            blueNode = nullptr;
        }
        if(blueness[takeUnitID] < 1.0 && state.takenCount(UnitColor::red) < 4){
            state.setColor(takeUnitID, UnitColor::red);
            redNode = std::make_shared<StandardNode>(state, beforeHand, blueness);
        }
        else{
            redNode = nullptr;
        }
        // if(takeUnitID == 8 && blueNode){
        //     blueNode->state.printBoard();
        // }
    }

    virtual EvalBoard search(int depth, double alpha, double beta) override
    {
        if(!redNode){
            auto eb = blueNode->search(depth, alpha, beta);
            return {eb.first * blueness[takeUnitID], eb.second};
        }
        if(!blueNode){
            auto eb = redNode->search(depth, alpha, beta);
            return {eb.first * (1.0 - blueness[takeUnitID]), eb.second};
        }
        auto bn = blueNode->search(depth, alpha, beta);
        auto rn = redNode->search(depth, alpha, beta);
        // std::cerr << bn.second << std::endl;
        // bn.second.printBoard();
        if(blueness[takeUnitID] > 0.5){
            bn.second.setColor(takeUnitID, UnitColor::unknown);
            return {bn.first * blueness[takeUnitID] + rn.first * (1.0 - blueness[takeUnitID]), bn.second};
        }
        else{
            rn.second.setColor(takeUnitID, UnitColor::unknown);
            return {bn.first * blueness[takeUnitID] + rn.first * (1.0 - blueness[takeUnitID]), rn.second};
        }
    }
};

class EscapeNode: public BaseNode{
public:
    std::unique_ptr<StandardNode> escaped;
    Hand beforeHand;

    EscapeNode(const Geister& state, const Hand& hand):
    BaseNode(state),
    beforeHand(hand)
    {
        this->state.setColor(hand.unit.id(), UnitColor::Blue);
        escaped = std::make_unique<StandardNode>(this->state, hand, blueness);
    }

    EscapeNode(const Geister& state, const Hand& hand, std::vector<double> blueness):
    BaseNode(state, blueness),
    beforeHand(hand)
    {
        this->state.setColor(hand.unit.id(), UnitColor::Blue);
        escaped = std::make_unique<StandardNode>(this->state, hand, blueness);
        // escaped->state.printBoard();
        // std::cerr << hand << std::endl;
    }

    virtual EvalBoard search(int depth, double alpha, double beta) override
    {
        auto val = escaped->search(depth, alpha, beta);
        return {val.first * blueness[beforeHand.unit.id()], val.second};
        std::for_each(blueness.begin(), blueness.end(), [&](auto x){std::cerr << x << ",";});
        std::cerr << val.first << "*" << blueness[beforeHand.unit.id()];
        std::cerr << std::endl;
        return {val.first * blueness[beforeHand.unit.id()], val.second};
    }
};