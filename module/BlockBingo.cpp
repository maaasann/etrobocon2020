/**
 *  @file   BlockBingo.cpp
 *  @brief  ビンゴエリアを攻略する
 *  @author mutotaka0426
 */
#include "BlockBingo.h"

using namespace std;

BlockBingo::BlockBingo(Controller& controller_, bool isLeftCourse_)
  : controller(controller_), isLeftCourse(isLeftCourse_), blockBingoData(controller, isLeftCourse)
{
}

void BlockBingo::runBlockBingo()
{
  RouteCalculator routeCalculator(blockBingoData);
  MotionSequencer motionSequencer(blockBingoData);
  Navigator navigator(controller, isLeftCourse);

  blockBingoData.initBlockBingoData();
  array<array<Coordinate, 2>, 5> blockColorList;       // 色別のブロック座標リスト
  array<array<Coordinate, 2>, 5> circleColorList;      // 色別のサークルの座標リスト
  vector<pair<Coordinate, Coordinate>> transportList;  // 運搬先リスト

  // 運搬先リストを生成
  setColorList(blockColorList, circleColorList);
  transportList = transportCalculate(blockColorList, circleColorList);

  // ビンゴエリアに進入
  int entranceX = isLeftCourse ? 2 : 4;
  int nearPoint = popCoordinate(transportList);
  if(transportList[nearPoint].first.x == entranceX) {
    // (2,6)に進入
    navigator.enterStraight();
    blockBingoData.setDirection(Direction::North);
    blockBingoData.setCoordinate(Coordinate(entranceX, 6));
  } else if(transportList[nearPoint].first.x < entranceX) {
    // (1,6)に進入
    navigator.enterLeft();
    blockBingoData.setDirection(Direction::NWest);
    blockBingoData.setCoordinate(Coordinate(entranceX - 1, 6));
  } else {
    // (3,6)に進入
    navigator.enterRight();
    blockBingoData.setDirection(Direction::NEast);
    blockBingoData.setCoordinate(Coordinate(entranceX + 1, 6));
  }

  // 実際に運搬を開始する
  Coordinate start, goal;                   // スタート座標,ゴール座標
  Coordinate current;                       // スタート座標,ゴール座標
  vector<Coordinate> routeList;             // 経路計算結果を格納するリスト
  vector<MotionCommand> motionCommandList;  // 動作変換結果を格納するリスト
  Direction direction;
  Coordinate afterGoal;  // ブロックを運搬後にいる座標
  while((int)transportList.size() > 1) {
    nearPoint = popCoordinate(transportList);
    start = transportList[nearPoint].first;
    goal = transportList[nearPoint].second;
    transportList.erase(transportList.begin() + nearPoint);

    // ブロックまで移動
    current = blockBingoData.getCoordinate();                    // 現在地を取得
    routeCalculator.solveBlockBingo(routeList, current, start);  //現在地→ブロックの経路計算
    // ここは後で消せよーーーーーーーーーーーーーーーーーーーーーー
    printf("(%d,%d)", routeList[0].x, routeList[0].y);
    for(int k = 1; k < (int)routeList.size(); k++) {
      printf("→(%d,%d)", routeList[k].x, routeList[k].y);
    }
    printf("\n");
    // ここまで消せよーーーーーーーーーーーーーーーーーーーーーーー
    motionCommandList.clear();
    direction = motionSequencer.route2MotionCommand(routeList, motionCommandList);
    navigator.execMotion(motionCommandList);
    blockBingoData.setCoordinate(start);     // 現在地を更新
    blockBingoData.setDirection(direction);  // 現在の向きを更新

    // ブロックを運搬する
    current = blockBingoData.getCoordinate();                 // 現在地を取得
    routeCalculator.solveBlockBingo(routeList, start, goal);  //ブロック→運搬先の経路計算
    motionCommandList.clear();
    direction = motionSequencer.route2MotionCommand(routeList, motionCommandList);
    navigator.execMotion(motionCommandList);
    afterGoal = routeList[(int)routeList.size() - 2];  // 運搬先の一つ前にいた座標
    blockBingoData.setCoordinate(afterGoal);           // 現在地を更新
    blockBingoData.setDirection(direction);            // 現在の向きを更新

    blockBingoData.moveBlock(start, goal);  // ブロックの座標を更新する
  }

  // ガレージ駐車の準備をする
  current = blockBingoData.getCoordinate();  // 現在地を取得
  Coordinate last = isLeftCourse ? Coordinate(0, 6) : Coordinate(6, 6);
  routeCalculator.solveBlockBingo(routeList, current, last);  //現在地→ガレージ直前座標の経路計算
  motionCommandList.clear();
  direction = motionSequencer.route2MotionCommand(routeList, motionCommandList);
  navigator.execMotion(motionCommandList);
  blockBingoData.setCoordinate(start);     // 現在地を更新
  blockBingoData.setDirection(direction);  // 現在の向きを更新
  Direction lastDirection = isLeftCourse ? Direction::South : Direction::East;
  int rotationCount = blockBingoData.calcRotationCount(direction, lastDirection);
  motionCommandList.clear();
  if(rotationCount > 0) {
    for(int count = 0; count < abs(rotationCount); count++) {
      motionCommandList.push_back(MotionCommand::RT);
    }
  } else if(rotationCount < 0) {
    for(int count = 0; count < abs(rotationCount); count++) {
      motionCommandList.push_back(MotionCommand::RF);
    }
  }
  navigator.execMotion(motionCommandList);
  blockBingoData.setDirection(lastDirection);  // 現在の向きを更新
}

void BlockBingo::setColorList(array<array<Coordinate, 2>, 5>& blockList,
                              array<array<Coordinate, 2>, 5>& circleList)
{
  array<int, 5> blockListHead;
  array<int, 5> circleListHead;
  fill(blockListHead.begin(), blockListHead.end(), 0);
  fill(circleListHead.begin(), circleListHead.end(), 0);
  Coordinate coordinate;
  Color color;
  int castColor;

  // 各ブロックの座標を色ごとにblockListに保持する
  for(int j = 0; j < AREASIZE; j++) {
    for(int i = 0; i < AREASIZE; i++) {
      coordinate = { i, j };
      if(blockBingoData.checkNode(coordinate) == NodeType::crossCircle) {
        color = blockBingoData.getBlock(coordinate).blockColor;
      } else if(blockBingoData.checkNode(coordinate) == NodeType::blockCircle) {
        color = blockBingoData.getBlock(coordinate).blockColor;
      } else {
        color = Color::none;
      }
      castColor = static_cast<int>(color);
      if((castColor >= 0) && (castColor < 5)) {
        blockList[castColor][blockListHead[castColor]++] = coordinate;
      }
    }
  }

  // 各ブロックサークルの座標を色ごとにcircleListに保持する
  for(int k = 1; k <= 8; k++) {
    coordinate = blockBingoData.numberToCoordinate(k);
    color = blockBingoData.getBlockCircleColor(k);
    castColor = static_cast<int>(color);
    if((castColor >= 0) && (castColor < 5)) {
      circleList[castColor][circleListHead[castColor]++] = coordinate;
    }
  }
}

vector<pair<Coordinate, Coordinate>> BlockBingo::transportCalculate(
    array<array<Coordinate, 2>, 5> blockList, array<array<Coordinate, 2>, 5> circleList)
{
  vector<pair<Coordinate, Coordinate>> transportList;
  Coordinate coordinateFrom, coordinateTo;
  pair<Coordinate, Coordinate> setPair1, setPair2;
  int candidate1, candidate2;
  int cardNumber = blockBingoData.getCardNumber();

  setPair1 = { blockList[0][0], blockBingoData.numberToCoordinate(cardNumber) };
  transportList.push_back(setPair1);

  for(int i = 1; i < 5; i++) {
    candidate1 = manhattanDistance(blockList[i][0], circleList[i][0])
                 + manhattanDistance(blockList[i][1], circleList[i][1]);
    candidate2 = manhattanDistance(blockList[i][0], circleList[i][1])
                 + manhattanDistance(blockList[i][1], circleList[i][0]);
    if(candidate1 < candidate2) {
      setPair1 = { blockList[i][0], circleList[i][0] };
      setPair2 = { blockList[i][1], circleList[i][1] };
    } else {
      setPair1 = { blockList[i][0], circleList[i][1] };
      setPair2 = { blockList[i][1], circleList[i][0] };
    }
    // 運搬元と運搬先が同じ座標の場合は既に設置されているのでリストに追加しない
    if(setPair1.first != setPair1.second) transportList.push_back(setPair1);
    if(setPair2.first != setPair2.second) transportList.push_back(setPair2);
  }

  return transportList;
}

int BlockBingo::manhattanDistance(Coordinate coordinateFrom, Coordinate coordinateTo)
{
  int diffX = abs(coordinateTo.x - coordinateFrom.x);
  int diffY = abs(coordinateTo.y - coordinateFrom.y);
  return diffX + diffY;
}

int BlockBingo::popCoordinate(vector<pair<Coordinate, Coordinate>> const& transportList)
{
  Coordinate currentCoordinate = blockBingoData.getCoordinate();  //現在座標を取得
  int minPoint = 1;  // リスト番号（0は黒ブロックの運搬先が入ってるのでスキップする）
  int min = manhattanDistance(currentCoordinate, transportList[minPoint].first);
  int temp;
  Block blockFrom, blockTo;
  for(int i = minPoint + 1; i < (int)transportList.size(); i++) {
    temp = manhattanDistance(currentCoordinate, transportList[i].first);
    if(temp < min) {
      // 運搬先にカラーブロックが置かれている場合は後回しにする
      blockTo = blockBingoData.getBlock(transportList[i].second);
      if((blockTo.blockColor == Color::none) || (blockTo.blockColor == Color::black)) {
        min = temp;
        minPoint = i;
      }
    }
  }
  return minPoint;
}

BlockBingoData& BlockBingo::getBlockBingoData()
{
  return blockBingoData;
}
