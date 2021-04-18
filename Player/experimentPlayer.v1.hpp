#include <string>
#include "random.hpp"
#include "player.hpp"
#include "treeNode.v1.hpp"
#include "treeNodes.v1.hpp"
#include <algorithm>
#include <numeric>
#include "Simulator/all.hpp"
#include <memory>
#include <optional>
#include <cstring>
#include <limits>

inline bool onExit(const Unit& unit){
    if(unit.id() < 8){
        return (unit.x() == 0 || unit.x() == 5) && unit.y() == 0;
    }
    else{
        return (unit.x() == 0 || unit.x() == 5) && unit.y() == 5;
    }
}

inline bool debug = false;

class PlayerV1: public Player{
    cpprefjp::random_device rd;
    std::mt19937 mt;

    std::array<double, pattern.size()> selfPatternEst;
    std::array<double, pattern.size()> opponentPatternEst;

    std::string selfPattern;

    constexpr inline static std::array<std::array<double, 4>, 4> thBlue = {
        std::array<double, 4>{0.5, 0.3, 0.1, 0.2},
        std::array<double, 4>{0.6, 0.4, 0.1, 0.05},
        std::array<double, 4>{0.7, 0.3, 0.2, 0.1},
        std::array<double, 4>{0.9, 0.7, 0.5, 0.3}
    };

    double selfBluff; // 自分の着手がブラフだと考えられる確率
    double opponentBluff; // 相手の着手がブラフだと考えられる確率

    std::array<bool, 16> convincedRed;

    int turnCount;

public:
    PlayerV1():
	mt(rd())
	{
        initialize();
    }
    
    virtual void initialize(){
        Player::initialize();
        selfPatternEst.fill(1.0 / selfPatternEst.size());
        opponentPatternEst.fill(1.0 / opponentPatternEst.size());
        selfBluff = 0.2;
        opponentBluff = 0.2;
        turnCount = 0;
        convincedRed.fill(false);
    }

    virtual std::string decideRed(){
        cpprefjp::random_device rd;
        std::mt19937 mt(rd());

        std::uniform_int_distribution<int> serector(0, pattern.size() - 1);
		selfPattern = pattern[serector(mt)];
		game.setColor(selfPattern, "");
        // test();
        return selfPattern;
    }

    virtual std::string decideHand(std::string_view res){
        turnCount++;
        auto before = game;
        game.setState(res);
        for(int i = 0; i < 16; ++i){
            before.setColor(i, game.allUnit()[i].color());
            if(onExit(before.allUnit()[i])
            && ((i < 8 && before.allUnit()[i].isRed())
            || (i >= 8 && onExit(before.allUnit()[i])))){
                convincedRed[i] = true;
                std::cerr << game.allUnit()[i].name() << " is red!\n";
            }
        }

        const auto units = game.allUnit();

        // 脱出できるなら問答無用で脱出
        if(auto unit_iter = std::find_if(units.begin(), units.begin()+8,
            [&](const Unit& unit){return unit.isBlue() && unit.x() == 0  && unit.y() == 0;}); unit_iter != units.begin()+8){
                return Hand{*unit_iter, Direction::West};
            }
        if(auto unit_iter = std::find_if(units.begin(), units.begin()+8,
            [&](const Unit& unit){return unit.isBlue() && unit.x() == 5 && unit.y() == 0;}); unit_iter != units.begin()+8){
                return Hand{*unit_iter, Direction::East};
            }

        // debug = true;
        // 相手駒推定部
        bool im1st = true; // 先手判定
        if(turnCount == 1){// 一手目かつ相手の駒が動いてなければ先手
            im1st &= (units[8].x() == 4 && units[8].y() == 1);
            im1st &= (units[9].x() == 3 && units[8].y() == 1);
            im1st &= (units[10].x() == 2 && units[8].y() == 1);
            im1st &= (units[11].x() == 1 && units[8].y() == 1);
            im1st &= (units[12].x() == 4 && units[8].y() == 0);
            im1st &= (units[13].x() == 3 && units[8].y() == 0);
            im1st &= (units[14].x() == 2 && units[8].y() == 0);
            im1st &= (units[15].x() == 1 && units[8].y() == 0);
        }
        if(turnCount > 1 || !im1st){ // 一手目かつ先手なら相手は動いていないので推定しない
            before.changeSide();
            game.changeSide();
            for(int i = 0; i < 8; ++i){
                std::swap(convincedRed[i], convincedRed[i+8]);
            }
            auto opponentHand = getBeforeHand(before, game);
            if(opponentHand.has_value()){
                std::cerr << opponentHand.value() << std::endl;
                opponentPatternEst = updateEstimate(before, opponentHand.value(), opponentPatternEst);
            }
            before.changeSide();
            game.changeSide();
            for(int i = 0; i < 8; ++i){
                std::swap(convincedRed[i], convincedRed[i+8]);
            }
            std::cerr << "\n";
        }
        // debug = false;


        // 着手決定部
        auto legalMoves = game.getLegalMove1st(); // 合法手
        auto candidates = candidateHand(); // 候補手
        if(candidates.size() < legalMoves.size()){ //  候補手が合法手より少ない＝必勝手が存在する
            std::for_each(candidates.begin(), candidates.end(), [&](auto& hand){std::cerr << hand << ",";});
            std::cerr << std::endl;
            std::uniform_int_distribution<int> lmSelector(0, candidates.size() - 1);
            return candidates[lmSelector(mt)];
        }
        std::cerr << std::endl;

        // std::for_each(selfPatternEst.begin(), selfPatternEst.end(), [&](double b){std::cerr << b << ",";});
        // std::cerr << std::endl;
        // std::for_each(opponentPatternEst.begin(), opponentPatternEst.end(), [&](double b){std::cerr << b << ",";});
        std::vector<double> blueness(16, 0.5);
        auto selfBluenessAverage = (4.0 - game.takenCount(UnitColor::Blue)) / std::count_if(units.begin(), units.begin()+8, [&](const Unit& u){return u.onBoard();});
        for(int i = 0; i < 8; ++i){
            blueness[i] = calcBlueness(selfPatternEst, 'A'+i);// * 0.25;
            if(const auto& unit = units[i]; !unit.isTaken()){
                blueness[i] = std::max(std::min(blueness[i], 0.99), 0.01);
            }
            else{
                if(unit.isBlue()){
                    blueness[i] = 1.0;
                }
                else if(unit.isRed()){
                    blueness[i] = 0.0;
                }
            }
        }
        std::cerr << "Self: ";
        std::for_each(blueness.begin(), blueness.begin()+8, [&](double b){std::cerr << b << ",";});
        std::cerr << " -> ";
        for(int i = 0; i < 8; ++i){
            if(convincedRed[i]){
                blueness[i] = 0.0;
            }
            else if(const auto& unit = units[i]; unit.onBoard()){
                blueness[i] -= selfBluenessAverage;
                if(blueness[i] > 0.0){
                    blueness[i] /= (1.0 - selfBluenessAverage);
                }
                else if(blueness[i] < 0.0){
                    blueness[i] /= selfBluenessAverage;
                }
                blueness[i] *= 0.5;
                blueness[i] += 0.5;
            }
        }
        std::for_each(blueness.begin(), blueness.begin()+8, [&](double b){std::cerr << b << ",";});
        std::cerr << std::endl;

        auto opponentBluenessAverage = (4.0 - game.takenCount(UnitColor::blue)) / std::count_if(units.begin()+8, units.end(), [&](const Unit& u){return u.onBoard();});
        for(int i = 0; i < 8; ++i){
            blueness[i+8] = calcBlueness(opponentPatternEst, 'A'+i);// * 0.25;
            if(const auto& unit = units[i+8]; !unit.isTaken()){
                blueness[i+8] = std::max(std::min(blueness[i+8], 0.99), 0.01);
            }
            else{
                if(unit.isBlue()){
                    blueness[i+8] = 1.0;
                }
                else if(unit.isRed()){
                    blueness[i+8] = 0.0;
                }
            }
        }
        std::cerr << "Opponent: ";
        std::for_each(blueness.begin()+8, blueness.end(), [&](double b){std::cerr << b << ",";});
        std::cerr << " -> ";
        for(int i = 0; i < 8; ++i){
            if(convincedRed[i+8]){
                blueness[i+8] = 0.0;
            }
            else if(const auto& unit = units[i+8]; unit.onBoard()){
                blueness[i+8] -= opponentBluenessAverage;
                if(blueness[i+8] > 0.0){
                    blueness[i+8] /= (1.0 - opponentBluenessAverage);
                }
                else if(blueness[i+8] < 0.0){
                    blueness[i+8] /= opponentBluenessAverage;
                }
                blueness[i+8] *= 0.5;
                blueness[i+8] += 0.5;
            }
        }
        std::for_each(blueness.begin()+8, blueness.end(), [&](double b){std::cerr << b << ",";});
        std::cerr << std::endl;

        std::unordered_map<std::string, std::array<double, 70>> estAfterHands;
        std::unordered_map<std::string, RootNode> treeAfterHands;
        std::vector<std::vector<double>> bluenessAfterHands;
        for(int i = 0; i < legalMoves.size(); ++i){
            estAfterHands[legalMoves[i]] = updateEstimate(game, legalMoves[i], selfPatternEst);
            bluenessAfterHands.emplace_back(std::vector<double>(16, 0.5));
            for(int j = 0; j < 8; ++j){
                if(convincedRed[j+8]){
                    bluenessAfterHands[i][j] = 0.0;
                }
                else{
                    bluenessAfterHands[i][j] = calcBlueness(opponentPatternEst, 'A'+j);// * 0.25;
                    if(const auto& unit = units[j+8]; unit.onBoard()){
                        bluenessAfterHands[i][j] = std::max(std::min(bluenessAfterHands[i][j], 0.99), 0.01);

                        bluenessAfterHands[i][j] -= opponentBluenessAverage;
                        if(bluenessAfterHands[i][j] > 0.0){
                            bluenessAfterHands[i][j] /= (1.0 - opponentBluenessAverage);
                        }
                        else if(bluenessAfterHands[i][j] < 0.0){
                            bluenessAfterHands[i][j] /= opponentBluenessAverage;
                        }
                        bluenessAfterHands[i][j] *= 0.5;
                        bluenessAfterHands[i][j] += 0.5;
                    }
                    else{
                        if(unit.isBlue()){
                            bluenessAfterHands[i][j] = 1.0;
                        }
                        else if(unit.isRed()){
                            bluenessAfterHands[i][j] = 0.0;
                        }
                    }
                }
            }
            for(int j = 0; j < 8; ++j){
                if(convincedRed[j]){
                    bluenessAfterHands[i][j+8] = 0.0;
                }
                else{
                    bluenessAfterHands[i][j+8] = calcBlueness(estAfterHands[legalMoves[i]], 'A'+j);// * 0.25;
                    if(const auto& unit = units[j]; unit.onBoard()){
                        bluenessAfterHands[i][j+8] = std::max(std::min(bluenessAfterHands[i][j+8], 0.99), 0.01);

                        bluenessAfterHands[i][j+8] -= selfBluenessAverage;
                        if(bluenessAfterHands[i][j+8] > 0.0){
                            bluenessAfterHands[i][j+8] /= (1.0 - selfBluenessAverage);
                        }
                        else if(bluenessAfterHands[i][j+8] < 0.0){
                            bluenessAfterHands[i][j+8] /= selfBluenessAverage;
                        }
                        bluenessAfterHands[i][j+8] *= 0.5;
                        bluenessAfterHands[i][j+8] += 0.5;
                    }
                    else{
                        if(unit.isBlue()){
                            bluenessAfterHands[i][j+8] = 1.0;
                        }
                        else if(unit.isRed()){
                            bluenessAfterHands[i][j+8] = 0.0;
                        }
                    }
                }
            }
            auto tmpGame = game;
            tmpGame.move(legalMoves[i]);
            tmpGame.changeSide();
            treeAfterHands.insert({legalMoves[i], RootNode(tmpGame, bluenessAfterHands[i])});
            // std::cerr << legalMoves[i] << ": ";
            // std::for_each(bluenessAfterHands[i].begin()+8, bluenessAfterHands[i].end(), [&](auto x){std::cerr << x << ",";});
            // std::cerr << ")\n";
            // treeAfterHands[legalMoves[i]] = RootNode(tmpGame, tmpBlueness);
        }
        // 着手直後に駒を取ってるとおかしなことになってる気がする
        auto evalAfterHands = [&](int n){
            std::vector<std::vector<EvalBoard>> ebAfterHands;
            std::vector<EvalBoard> result;
            for(int i = 0; i < legalMoves.size(); ++i){
                treeAfterHands.at(legalMoves[i]).children.clear();
                ebAfterHands.emplace_back(treeAfterHands.at(legalMoves[i]).evalList(n));
                int maxj = 0;
                for(int j = 0; j < ebAfterHands[i].size(); ++j){
                    if(ebAfterHands[i][j].first > ebAfterHands[i][maxj].first){
                        maxj = j;
                    }
                }
                {
                auto stateAfterHand = treeAfterHands.at(legalMoves[i]).state;
                auto& units = stateAfterHand.allUnit();
                for(auto onExitUnit = std::find_if(units.begin(), units.begin()+8,
                    [&](const Unit& u){return u.y() == 0 && (u.x() == 0 || u.x() == 5);});
                    onExitUnit != units.begin()+8;
                    onExitUnit = std::find_if(onExitUnit, units.begin()+8,
                    [&](const Unit& u){return u.y() == 0 && (u.x() == 0 || u.x() == 5);})
                )
                {
                    stateAfterHand.setColor(onExitUnit->id(), UnitColor::Blue);
                    if(onExitUnit->x() == 0){
                        stateAfterHand.move({*onExitUnit, Direction::West});
                    }
                    else if(onExitUnit->x() == 5){
                        stateAfterHand.move({*onExitUnit, Direction::East});
                    }
                    auto onExitUnitIndex = std::distance(units.begin(), onExitUnit);
                    auto eval = treeAfterHands.at(legalMoves[i]).blueness[onExitUnitIndex];
                    // std::cerr << onExitUnit->name() << " is on exit! " << ebAfterHands[i][maxj].first << " -> " << eval << " = ";
                    if(eval > ebAfterHands[i][maxj].first){
                        ebAfterHands[i].emplace_back(eval, stateAfterHand);
                        maxj = ebAfterHands[i].size()-1;
                    }
                    // std::cerr << ebAfterHands[i][maxj].first << std::endl;
                }
                }
                result.emplace_back(ebAfterHands[i][maxj]);
                result.back().first *= -1.0;
                result.back().second.changeSide();
            }
            return result;
        };


        RootNode root(game
            , blueness
        // , {
        //     0.9, 0.5, 0.95, 0.5, 0.5, 0.5, 0.5, 0.5,
        //     0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5}
            );

        std::vector<EvalBoard> el;
        for(int i = 0; i < bluenessAfterHands.size(); ++i){
            for(int j = 0; j < 8; ++j){
                std::swap(bluenessAfterHands[i][j], bluenessAfterHands[i][j+8]);
            }
        }

        int maxDepth = 200; // 探索深さの上限（時間で制限してるので無限でもよい？）←とりあえず引分手番数と同数に
        const int searchLimit = 500; // 探索時間の上限[msec]
        int searchTime = 0; // 現在の合計探索時間[msec]
        for(int i = 1; i <= maxDepth; i+=1){
            std::cerr << i << ",";
            root.children.clear();

            auto start = std::chrono::system_clock::now();
            // el = root.evalList(i);
            // el = evalAfterHands(i);
            el = root.evalList(i, bluenessAfterHands);
            auto end = std::chrono::system_clock::now();
            auto dur = end - start;
            searchTime += std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            std::cerr << searchTime << std::endl;
            if(searchTime >= searchLimit){
                break;
            }
        }
        for(int i = 0; i < bluenessAfterHands.size(); ++i){
            for(int j = 0; j < 8; ++j){
                std::swap(bluenessAfterHands[i][j], bluenessAfterHands[i][j+8]);
            }
        }

        std::vector<double> trueBlueness(16, 0.5);
        for(int i = 0; i < 8; ++i){
            if(char c = 'A'+i; selfPattern.find(c) != std::string::npos){
                trueBlueness[i] = 0.0;
            }
            else{
                trueBlueness[i] = 1.0;
            }
        }
        std::copy(blueness.begin()+8, blueness.end(), trueBlueness.begin()+8);
        std::vector<double> trueEval(el.size());
        for(int i = 0; i < el.size(); ++i){
            std::cerr
            << legalMoves[i] << ":"
            << el[i].first << "=" << el[i].second << "(";
            std::for_each(bluenessAfterHands[i].begin()+8, bluenessAfterHands[i].end(), [&](auto x){std::cerr << x << ",";});
            std::cerr << ")\n";
            trueEval[i] = evaluate(el[i].second, trueBlueness);
            std::cerr << "true eval = " << trueEval[i] << "(";
            std::for_each(trueBlueness.begin(), trueBlueness.end(), [&](auto x){std::cerr << x << ",";});
            std::cerr << ")\n";
        }
        std::cerr << std::endl;
        
        std::vector<Hand> action_candidates;
        auto maxEval = *std::max_element(trueEval.begin(), trueEval.end());
        auto allowEval = 0.0;
        int addCount = 0;
        while(addCount < legalMoves.size() * 0.3 && allowEval < 0.2){
            addCount = 0;
            for(int i = 0; i < trueEval.size(); ++i){
                if(trueEval[i] >= maxEval - allowEval){
                    action_candidates.emplace_back(legalMoves[i]);
                    addCount++;
                }
            }
            allowEval += 0.01;
        }
        std::uniform_int_distribution<int> action_selector(0, action_candidates.size() - 1);
        auto action = action_candidates[action_selector(mt)];
        selfPatternEst = estAfterHands[action];//updateEstimate(game, action, selfPatternEst);
        game.move(action);
        return action;
    }

    virtual void finalize(std::string_view endState){
        // for(int i = 0; i < selfPatternEst.size(); ++i){
        //     std::cerr << pattern[i] << ": " << selfPatternEst[i] << "\n";
        // }
        // std::cerr << discoverRate(selfPattern, selfPatternEst);
    }

protected:
    double calcBlueness(std::array<double, 70> patternEstimates, char target){
        double result = 1.0;
        for(int i = 0; i < pattern.size(); ++i){
            if(std::strchr(pattern[i], target)){
                result -= patternEstimates[i];
            }
        }
        return std::min(1.0, std::max(0.0, result));
    }

    std::optional<Hand> getBeforeHand(const Geister& before, const Geister& after){
        std::optional<Hand> result;
        auto& unitsB = before.allUnit();
        auto& unitsA = after.allUnit();
        for(int i = 0; i < 16; ++i){
            auto& ub = unitsB[i];
            auto& ua = unitsA[i];
            if(ub.x() != ua.x() || ub.y() != ua.y()){
                if(ua.y() - ub.y() == -1){
                    result.emplace(ub, Direction::North);
                    return result;
                }
                if(ua.x() - ub.x() == 1){
                    result.emplace(ub, Direction::East);
                    return result;
                }
                if(ua.x() - ub.x() == -1){
                    result.emplace(ub, Direction::West);
                    return result;
                }
                if(ua.y() - ub.y() == 1){
                    result.emplace(ub, Direction::South);
                    return result;
                }
            }
        }
        return result;
    }

    double evaluate(const Geister& state, std::vector<double> blueness){
        auto& units = state.allUnit();
        // std::for_each(blueness.begin(), blueness.end(), [&](auto x){std::cerr << x << ",";});
        // std::cerr << "\n";

        double takenBlue = 0.0;
        double takenRed = 0.0;
        for(int i = 0; i < 8; ++i){
            if(units[i].isTaken()){
                takenBlue += blueness[i];
                takenRed += (1.0 - blueness[i]);
            }
        }
        std::cerr << "takenBlue:" << takenBlue << ",takenRed:" << takenRed << "_";
        double takeBlue = 0.0;
        double takeRed = 0.0;
        for(int i = 8; i < 16; ++i){
            if(units[i].isTaken()){
                takeBlue += blueness[i];
                takeRed += (1.0 - blueness[i]);
            }
        }
        std::cerr << "takeBlue:" << takeBlue << ",takeRed:" << takeRed << "_";

        if(auto result = state.result(); result != Result::OnPlay){
            if(result == Result::Draw){
                std::cerr << "Draw!";
                return 0.0;
            }
            if(result == Result::Escape1st){
                auto escapeUnit_iter = std::find_if(units.begin(), units.end(), [&](const Unit& u){return u.isEscape();});
                auto escapeUnit = std::distance(units.begin(), escapeUnit_iter);
                auto escapeProb = blueness[escapeUnit];
                if((takeRed - 3.0) >= 0.5){
                    std::cerr << "TakenRed2nd?(" << -(takeRed - 3.0) << ")";
                    return -(takeRed - 3.0);
                }
                std::cerr << "Escape1st?(" << escapeProb << ")";
                return 1.0 * blueness[escapeUnit];
            }
            // if(result == Result::TakeBlue1st){
            //     std::cerr << "TakeBlue1st!";
            //     return 1.0;
            // }
            // if(result == Result::TakenRed1st){
            //     std::cerr << "TakenRed1st!";
            //     return 1.0;
            // }
            if(result == Result::Escape2nd){
                auto escapeUnit_iter = std::find_if(units.begin(), units.end(), [&](const Unit& u){return u.isEscape();});
                auto escapeUnit = std::distance(units.begin(), escapeUnit_iter);
                auto escapeProb = blueness[escapeUnit];
                std::cerr << "Escape2nd?(" << escapeProb << ")";
                return -1.0 * blueness[escapeUnit];
            }
            // if(result == Result::TakeBlue2nd){
            //     std::cerr << "TakeBlue2nd!";
            //     return -1.0;
            // }
            // if(result == Result::TakenRed2nd){
            //     std::cerr << "TakenRed2nd!";
            //     return -1.0;
            // }
        }
        if(takeBlue > 3.0 && takeRed > 3.0){
            if(takeBlue >= takeRed){
                std::cerr << "TakeBlue1st?(" << takeBlue - 3.0 << ")";
                return takeBlue - 3.0;
            }
            else{
                std::cerr << "TakenRed2nd?(" << -(takeRed - 3.0) << ")";
                return -(takeRed - 3.0);
            }
        }
        if(takeBlue > 3.0){
            std::cerr << "TakeBlue1st?(" << takeBlue - 3.0 << ")";
            return takeBlue - 3.0;
        }
        if(takeRed > 3.0){
            std::cerr << "TakenRed2nd?(" << -(takeRed - 3.0) << ")";
            return -(takeRed - 3.0);
        }
        if(takenBlue > 3.0 && takenRed > 3.0){
            if(takenBlue >= takenRed){
                std::cerr << "TakeBlue2nd?(" << takenBlue - 3.0 << ")";
                return -(takenBlue - 3.0);
            }
            else{
                std::cerr << "TakenRed1st?(" << -(takenRed - 3.0) << ")";
                return takenRed - 3.0;
            }
        }
        if(takenBlue > 3.0){
            std::cerr << "TakeBlue2nd?(" << takenBlue - 3.0 << ")";
            return -(takenBlue - 3.0);
        }
        if(takenRed > 3.0){
            std::cerr << "TakenRed1st?(" << -(takenRed - 3.0) << ")";
            return takenRed - 3.0;
        }
        // return {0.0, state};

        double value = 0.0;

        double takenValue = 0.0;
        for(int i = 0; i < 8; ++i){
            auto& unit = units[i];
            if(unit.isTaken()){
                takenValue -= 0.1 * (blueness[i] - 0.5) * 2;
            }
        }
        for(int i = 8; i < 16; ++i){
            auto& unit = units[i];
            if(unit.isTaken()){
                takenValue += 0.1 * (blueness[i] - 0.5) * 2;
            }
        }

        double positionValue = 0.0;
        static constexpr std::array<double, 36> posVal = {
            0.300, 0.200, 0.100, 0.100, 0.200, 0.300,
            0.100, 0.070, 0.030, 0.030, 0.070, 0.100,
            0.050, 0.020, 0.010, 0.010, 0.020, 0.050,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000
        };

        for(int i = 0; i < 8; ++i){
            auto& unit = units[i];
            if(unit.onBoard()){
                positionValue += posVal[unit.x() + unit.y()*6];
            }
        }
        for(int i = 8; i < 16; ++i){
            auto& unit = units[i];
            if(unit.onBoard()){
                positionValue -= posVal[unit.x() + (5-unit.y())*6];
            }
        }//*/

        value =
        takenValue * 0.5
        +
        positionValue * 0.5
        ;
		return std::max(-0.999, std::min(0.999, value));
    }

    int matchCharCount(std::string_view a, std::string_view b){
        return std::count_if(a.begin(), a.end(), [&](char c){return std::find(b.begin(), b.end(), c) != b.end();});
    }
    
    // 駒色の判明度
    double discoverRate(std::string_view correctPattern, std::array<double, pattern.size()> patternProb){
        double result = 0.0;
        
        for(int i = 0; i < pattern.size(); ++i){
            // std::cerr << pattern[i] << ": " << result << " += " << matchCharCount(correctPattern, pattern[i]) << " * " << patternProb[i] << std::endl;
            result += matchCharCount(correctPattern, pattern[i]) / 4.0 * patternProb[i];
        }

        return result;
    }

    void test(){
        if(false){
            Geister g("99B99R99R55B99R00R05B99B99r99r99b99b99b22u33u99r");
            auto a = decideHand(g);
            std::cerr << a << std::endl;
            g.move({g.allUnit()[4], Direction::North});
            g.changeSide();
            g.move({g.allUnit()[7], Direction::North});
            g.changeSide();
            auto b = decideHand(g);
            std::cerr << b << std::endl;
            return;
        }
        if(false){// 脱出可能な位置に青駒があるときの推定動作の確認
            Geister g("99r99r99b50B99r13R05B99b99r99r12u99b99b05u20u99r");
            g.changeSide();
            g.printAll();
            auto opponentHand = Hand(g.allUnit()[6], Direction::North);
            std::cerr << opponentHand << std::endl;
            std::array<double, 70> tmpPtn;
            for(int i = 0; i < tmpPtn.size(); ++i){
                tmpPtn[i] = 1.0 / tmpPtn.size();
            }
            std::for_each(tmpPtn.begin(), tmpPtn.end(), [&](auto x){std::cerr << x << ",";});
            std::cerr << std::endl;
            tmpPtn = updateEstimate(g, opponentHand, tmpPtn);
            std::for_each(tmpPtn.begin(), tmpPtn.end(), [&](auto x){std::cerr << x << ",";});
            std::cerr << std::endl;
            g.changeSide();
            return;
        }

        // Geister g("12R99R99R99R15B25B35B45B41u31u21u11u40u30u20u10u");
        // Geister g("14R99R99R99R15B25B35B45B99r99r21u11u99b99b20u10u");
        // Geister g("14R99R99R99R15B25B35B45B41u31u21u11u40u30u20u05u");
        // Geister g("11R31R22R42R13B33B24B44B21u41u12u32u23u43u14u34u");
        // Geister g("11R31R22R42R13B01B24B44B21u41u12u32u23u43u14u34u");
        // Geister g("11R31R22R42R13B33B24B44B21u41u12u32u23u43u14u34u");
        Geister g("11B31R22R42R13B33B24B44R21u41u12u32u23u43u14u34u");
        // Geister g("14R24R34R44R15B25B35B45B41u31u21u11u40u30u20u10u");
        g.printBoard();

        // for(int i = 0; i < pattern.size(); ++i){
            // for(int j = 0; j < pattern.size(); ++j){
            // auto i = 0;
            //     std::cerr << pattern[i] << "," <<  pattern[j] << "\n";
            //     g.setColor(pattern[i], pattern[j]);
            //     TreeNode t(g);
            //     t.evalList(4);
            // }
        // }
        // return;

        Hand hand{g.allUnit()[7], Direction::North};
        // auto afterEst = updateEstimate(g, hand, selfPatternEst);
        // for(int i = 0; i < pattern.size(); ++i){
        //     std::cerr << pattern[i] << ": " << afterEst[i] << "\n";
        // }
        // return;
        // g.changeSide();
        // g.printBoard();
        RootNode root(g
        , {
            // 0.9, 0.5, 0.95, 0.5, 0.5, 0.5, 0.5, 0.5,
            0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
            0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5}
            );
        std::vector<EvalBoard> el;

        int maxDepth = 5; // 探索深さの上限（時間で制限してるので無限でもよい？）←とりあえず引分手番数と同数に
        const int searchLimit = 10000; // 探索時間の上限[msec]
        int searchTime = 0; // 現在の合計探索時間[msec]
        for(int i = 1; i <= maxDepth; i+=1){
            std::cerr << i << "=";
            root.children.clear();

            auto start = std::chrono::system_clock::now();
            el = root.evalList(i);
            auto end = std::chrono::system_clock::now();
            auto dur = end - start;
            searchTime += std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            std::cerr << searchTime << std::endl;
            if(searchTime >= searchLimit){
                break;
            }
        }

        auto lm = g.getLegalMove1st();
        // std::cerr << lm.size() << "-" << el.size() << std::endl;
        for(int i = 0; i < el.size(); ++i){
            std::cerr
            << lm[i] << ":"
            << el[i].first << "=" << el[i].second << "\n";
            std::cerr << "true eval = " << evaluate(el[i].second, 
            {
                // 0, 0, 0, 0, 1, 1, 1, 1,
                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5,
                0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5
            }
            ) << "\n";
            // el[i].second.printBoard();
        }
        std::cerr << std::endl;
        return;
        // auto g = Geister("51R99R99R99R99B99B99B15B04u40u99r99b99r99b99b99r");
        // auto g = Geister("23R99R99R99R99B99B99B44B21u12u99r99b99r99b14u32u");
        // auto g = Geister("23R99R99R99R99B99B99B01B21u12u99r99b99r99b14u32u");
        g.printBoard();
        patternHandTable(g);
        std::array<double, pattern.size()> before = selfPatternEst;
        for(int i = 0; i < pattern.size(); ++i){
            std::cerr << pattern[i] << ": " << before[i] << "\t";
        }
        std::cerr << std::endl;
        Hand h{g.allUnit()[7], Direction::North};
        auto after = updateEstimate(g, h, before);
        for(int i = 0; i < pattern.size(); ++i){
            std::cerr << pattern[i] << ": " << after[i] << "\t";
        }
        std::cerr << std::endl;
        after = updateEstimate(g, h, after);
        for(int i = 0; i < pattern.size(); ++i){
            std::cerr << pattern[i] << ": " << after[i] << "\t";
        }
        std::cerr << std::endl;
        double dr = discoverRate("ABCD", after);
        std::cerr << "discoverRate: " << dr << std::endl;
    }

    // 可能な相手駒のパターンを列挙
    std::vector<std::string> getLegalPatternSelf(Geister state){
        std::vector<std::string> res;
        std::vector<char> blue;
        std::vector<char> red;
        std::vector<char> unknown;
        // 色ごとに判明済みの駒をリスト化
        for(int u = 0; u < 8; ++u){
            if(convincedRed[u]){
                red.emplace_back(u + 'A');
            }
            else if(state.allUnit()[u].onBoard()){
                unknown.emplace_back(u + 'A');
            }
            else{
                if(state.allUnit()[u].isBlue()){
                    blue.emplace_back(u + 'A');
                }
                else if(state.allUnit()[u].isRed()){
                    red.emplace_back(u + 'A');
                }
            }
        }
        // 判明している情報と矛盾するパターンを除外
        for(auto p: pattern){
            auto ptn = std::string(p);
            if(!blue.empty()){
                // 青と分かっている駒を含むパターンを除外
                int countB = 0;
                for(auto b: blue){
                    countB += std::count_if(ptn.begin(), ptn.end(), [&](auto c){return c == b;});
                }
                if(countB > 0) continue;
            }
            if(!red.empty()){
                // 赤と分かっている駒を含まないパターンを除外
                int countR = 0;
                for(auto r: red){
                    countR += std::count_if(ptn.begin(), ptn.end(), [&](auto c){return c == r;});
                }
                if(countR < red.size()) continue;
            }
            res.emplace_back(ptn);
        }
        return res;
    }

    // 駒色パターンごとの着手選択確率
    std::unordered_map<std::string, std::unordered_map<std::string, double>>
    // std::vector<std::vector<double>> 
    patternHandTable(Geister state){
        if(debug)
            std::cerr << "a";
        static std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, double>>> memo;
        if(memo.count(state)){
            std::cerr << "this state already exist!" << std::endl;
            return memo[state];
        }
        if(debug)
            std::cerr << "b";
        // std::cerr << "pattern";

        // for(int i = 0; i < 8; ++i){
        //     if(state.allUnit()[i].onBoard()){
        //         state.setColor(i, UnitColor::Unknown);
        //     }
        // }
        // for(int i = 8; i < 16; ++i){
        //     if(state.allUnit()[i].onBoard()){
        //         state.setColor(i, UnitColor::unknown);
        //     }
        // }
        auto legalMoves = state.getLegalMove1st();
        for(auto lm = legalMoves.begin(); lm != legalMoves.end(); ++lm){
            if(auto& unit = lm->unit;
            unit.y() == 0 && (
                (unit.x() == 0 && lm->direct == Direction::West) ||
                (unit.x() == 5 && lm->direct == Direction::East)
            )
            ){
                legalMoves.erase(lm);
            }
        }
        if(debug){
            std::cerr << "c" << legalMoves.size();
            std::for_each(legalMoves.begin(), legalMoves.end(), [&](auto& x){std::cerr << x;});
        }
        auto legalPatterns = getLegalPatternSelf(state);
        if(debug)
            std::cerr << "d" << legalPatterns.size();

        // std::vector<std::vector<double>> result(legalMoves.size(), std::vector<double>(legalPatterns.size()));
        std::unordered_map<std::string, std::unordered_map<std::string, double>> result;

        for(size_t p = 0; p < legalPatterns.size(); ++p){
            // std::cerr << " ha";
            auto& lp = legalPatterns[p];
            if(debug)
                std::cerr << "x" << lp;
            state.setColor(lp, "");
            
            TreeNode tree(state);
            // tree.state.printBoard();
            auto evals = tree.evalList(2);
            if(debug)
                std::cerr << "y" << evals.size();
            // std::cerr << legalPatterns[p] << ":";
            // for(size_t m = 0; m < legalMoves.size(); ++m){
            //     auto move = legalMoves[m];
            //     std::cerr << move << "=" << evals[m];
            // }
            // std::cerr << std::endl;

            // 報酬値から着手選択確率を計算
            if(auto winHand = std::find_if(evals.begin(), evals.end(), [&](double e){return e >= 1.0;});
                winHand != evals.end()){
                std::for_each(evals.begin(), evals.end(), [&](double& e){e = 0.0;});
                evals[std::distance(evals.begin(), winHand)] = 1.0;
            }
            else{
                for(auto& eval: evals){
                    eval += 1.0;
                    eval *= 0.5;
                    eval = std::pow(eval, 5);
                }
            }
            if(debug)
                std::cerr << "z";
            // std::cerr << legalPatterns[p] << ":";
            // for(size_t m = 0; m < legalMoves.size(); ++m){
            //     auto move = legalMoves[m];
            //     std::cerr << move << "=" << evals[m];
            // }
            // std::cerr << std::endl;
            auto sumEvals = std::accumulate(evals.begin(), evals.end(), 0.0);
            std::vector<double> probs(evals.size());
            std::transform(evals.begin(), evals.end(), probs.begin(), [&](double r){return r / sumEvals;});
            // std::cerr << "nd";
            if(debug){
                std::cerr << "w" << probs.size();
                std::cerr << legalMoves.size();
            }
            
            // std::cerr << lp << ":";
            for(size_t m = 0; m < legalMoves.size(); ++m){
                auto move = legalMoves[m];
                if(debug)
                    std::cerr << "m" << m << move;
                result[lp][move] = probs[m];
                // if(probs[m] == 0 || !std::isfinite(probs[m])){
                //     std::cerr << evals[m] << std::endl;
                // }
                // std::cerr << move << "=" << evals[m];
                // std::cerr << move << "=" << probs[m];
                if(debug)
                    std::cerr << "v";
            }
            // std::cerr << std::endl;
        }
        // std::cerr << " table" << std::endl;

        memo[state] = result;
        if(debug)
            std::cerr << "e";
        return result;
    }

    std::array<double, pattern.size()> updateEstimate(const Geister& state, Hand hand, const std::array<double, pattern.size()>& before){
        auto after = before;
        auto lp = getLegalPatternSelf(state);
        for(int i = 0; i < pattern.size(); ++i){
            if(std::find(lp.begin(), lp.end(), pattern[i]) == lp.end()){
                after[i] = 0.0;
            }
        }
        auto table = patternHandTable(state);

        std::array<double, pattern.size()> lh; // 尤度
        for(int i = 0; i < lh.size(); ++i){
            lh[i] = 0.0;
        }
        for(auto& p: table){
            auto evalSum = 0.0;
            for(auto& h: p.second){
                evalSum += h.second;
            }
            auto ptn_iter = std::find(pattern.begin(), pattern.end(), p.first);
            auto ptn_index = std::distance(pattern.begin(), ptn_iter);
            lh[ptn_index] = p.second[hand] / evalSum;
            // std::cerr << p.second[hand] << "/" << evalSum;
            // if(p.second[hand] == 0){
            //     state.printBoard();
            //     for(auto& h: p.second){
            //         std::cerr << "-" << h.first << ":" << h.second;
            //     }
            // }
            // std::cerr << ",";
        }
        // std::cerr << std::endl;

        for(size_t i = 0; i < after.size(); ++i){
            // std::cerr << lh[i] << ",";
            after[i] *= lh[i];
        }
        // std::cerr << std::endl;
        // for(size_t i = 0; i < after.size(); ++i){
        //     std::cerr << after[i] << ",";
        // }
        // std::cerr << std::endl;
        auto sum = std::accumulate(after.begin(), after.end(), 0.0);
        // std::cerr << sum << std::endl;
        std::transform(after.begin(), after.end(), after.begin(), [&](auto a){return a / sum;});
        // for(size_t i = 0; i < after.size(); ++i){
        //     std::cerr << after[i] << ",";
        // }
        // std::cerr << std::endl;
        return after;
    }

    // 可能な相手駒のパターンを列挙
    std::vector<std::string> getLegalPattern(Geister state){
        std::vector<std::string> res;
        std::vector<char> blue;
        std::vector<char> red;
        std::vector<char> unknown;
        // 色ごとに判明済みの駒をリスト化
        for(int u = 8; u < 16; ++u){
            if(state.allUnit()[u].color() == UnitColor::blue)
                blue.push_back(u - 8 + 'A');
            else if(state.allUnit()[u].color() == UnitColor::red || convincedRed[u])
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

    virtual std::vector<Hand> candidateHand(){
        auto legalMoves = game.getLegalMove1st();
        // return legalMoves;

        auto pgame(game); // 紫駒化した盤面
        for(int u = 0; u < 16; ++u){
            if(convincedRed[u]){
                pgame.setColor(u, UnitColor::red);
            }
            else if(auto& unit = pgame.allUnit()[u]; unit.color() == UnitColor::unknown){
                pgame.setColor(unit, UnitColor::purple);
            }
        }
        auto root = TreeNode(pgame);
        std::vector<double> evals;
        int maxDepth = 200; // 探索深さの上限（時間で制限してるので無限でもよい？）←とりあえず引分手番数と同数に
        const int searchLimit = 200; // 探索時間の上限[msec]
        int searchTime = 0; // 現在の合計探索時間[msec]
        for(int i = 1; i <= maxDepth; i+=1){
            std::cerr << i << ",";
            root.children.clear();

            auto start = std::chrono::system_clock::now();
            evals = root.evalList(i);
            auto end = std::chrono::system_clock::now();
            auto dur = end - start;
            // auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
            searchTime += std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();

            // 必勝手があればそれだけ返す
            double maxVal = *std::max_element(evals.begin(), evals.end());
            if(maxVal >= 1.0){
                std::vector<Hand> victoryMove;
                for(int i = 0; i < legalMoves.size(); ++i){
                    if(evals[i] >= maxVal){
                        victoryMove.emplace_back(legalMoves[i]);
                    }
                }
                if(!victoryMove.empty()){
                    return victoryMove;
                }
            }
            for(int j = 0; j < evals.size(); ++j){
                if(evals[j] <= -1.0){
                    root.rootBeta[j] = -1.0;
                }
            }
            // if(msec >= searchLimit){
            if(searchTime >= searchLimit){
                break;
            }
        }
        return legalMoves; // 必勝手が見つからなければ全て候補手

        // 必敗手を除外して返す（本当の意味で必敗ではないので除外しないほうがよい？）
        std::vector<Hand> notLoseMove;
        for(int i = 0; i < evals.size(); ++i){
            std::cout << legalMoves[i] << ": " << evals[i] << ", ";
            if(evals[i] > -1.0){
                notLoseMove.emplace_back(legalMoves[i]);
            }
        }
        std::cout << std::endl;
        if(!notLoseMove.empty()){
            return notLoseMove;
        }

        double maxTurnLose = *std::max_element(evals.begin(), evals.end());
        std::vector<Hand> maxTurnLoseHand;
        for(int i = 0; i < legalMoves.size(); ++i){
            if(evals[i] >= maxTurnLose){
                maxTurnLoseHand.emplace_back(legalMoves[i]);
            }
        }
        if(!maxTurnLoseHand.empty()){
            return maxTurnLoseHand;
        }

        for(auto&& lm: legalMoves){
            auto& unit = lm.unit;
            auto& direct = lm.direct;
            if(unit.y() == 0 && ((unit.x() == 0 && direct == Direction::West) || (unit.x() == 5 && direct == Direction::East))){
                return {lm};
            }
        }
        return legalMoves;
    }
};