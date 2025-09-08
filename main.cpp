#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include <map>
#include <iostream>
#include <string>
#include <chrono>

using namespace std;

enum GameState {STOPPED, RUNNING, MAZING, EXIT};
enum CellState {EMPTY, WALL, START, FINISH, OPEN, CLOSED, PATH, PREVIOUS};

struct Position
{
    Position(int, int, bool);
    void setPosition(int, int);
    array<int, 2> getPosition();
    int x;
    int y;
    bool hasPosition;
};

unsigned int screenWidth = sf::VideoMode::getDesktopMode().width;
unsigned int screenHeight = sf::VideoMode::getDesktopMode().height;
sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "A* Pathfinding Algorithm");
int cellsW = 20;
int cellsH = 20;
int thickness = 4;
int costD = 14;
int costN = 10;
bool pressedL = false;
bool pressedR = false;
bool startedP = false;
bool startedM = false;
GameState gameState = STOPPED;
vector<vector<CellState>> cells;
sf::Event event;
Position start(0, 0, false);
Position finish(0, 0, false);
vector<array<int, 2>> open;
vector<array<int, 2>> closed;
map<array<int, 2>, int> gCost;
map<array<int, 2>, int> hCost;
map<array<int, 2>, int> fCost;
map<array<int, 2>, array<int, 2>> parent;
map<array<int, 2>, bool> inOpen;
map<array<int, 2>, bool> inClosed;
map<array<int, 2>, bool> visited;
map<GameState, string> stateToString;
map<array<int, 2>, CellState> prevState;
int visitedCells = 0;
array<int, 2> currCell;
chrono::system_clock::duration startTime, endTime;

sf::Text text;
sf::Font arial;

void draw();
void events();
void processing();

void changeCellState(int, int, CellState);
void resizeCells();
void startPathAlgorithm();
void doPathAlgorithm();
void doMazeAlgorithm();
void restartCells();
void resetCells(CellState);
void startMazeAlgorithm();

array<array<int, 2>, 2> lowestFCost(const vector<array<int, 2>>&);
array<int, 2> randomNeighbor(const array<int, 2>&);

inline int toColumn(int);
inline int toRow(int);
int gCostCalc(int, int, int);
int hCostCalc(int, int);

int main()
{
    srand(time(NULL));
    stateToString[STOPPED] = "STOPPED";
    stateToString[RUNNING] = "RUNNING";
    stateToString[MAZING] = "MAZING";
    stateToString[EXIT] = "EXIT";
    arial.loadFromFile("arial.ttf");
    text.setFont(arial);
    text.setFillColor(sf::Color::Magenta);
    cells.resize(cellsH);
    for (auto &row : cells) row.resize(cellsW);
    for (auto &row : cells) for (auto &cellState : row) cellState = EMPTY;

    while (gameState != EXIT)
    {
        if (gameState != MAZING && gameState != RUNNING) draw();
        events();
        processing();
        // chrono::microseconds result = chrono::duration_cast<chrono::microseconds>(endTime - startTime);
        // cout << result.count() / 1000.0 << endl;
    }

    return 0;
}

void draw()
{
    sf::RectangleShape rect;
    //! Cells
    rect.setSize(sf::Vector2f(screenWidth / (double)cellsW, screenHeight / (double)cellsH));
    rect.setOrigin(0, 0);
    for (int i = 0; i < cellsH; i++)
    {
        for (int j = 0; j < cellsW; j++)
        {
            switch (cells.at(i).at(j))
            {
                case WALL:
                    rect.setFillColor(sf::Color(0, 0, 0));
                    rect.setPosition(j * screenWidth / cellsW, i * screenHeight / cellsH);
                    window.draw(rect);
                    break;
                case START:
                    rect.setFillColor(sf::Color(0, 0, 255));
                    rect.setPosition(j * screenWidth / cellsW, i * screenHeight / cellsH);
                    window.draw(rect);
                    break;
                case FINISH:
                    rect.setFillColor(sf::Color(255, 0, 0));
                    rect.setPosition(j * screenWidth / cellsW, i * screenHeight / cellsH);
                    window.draw(rect);
                    break;
                case OPEN:
                    rect.setFillColor(sf::Color(255, 255, 0));
                    rect.setPosition(j * screenWidth / cellsW, i * screenHeight / cellsH);
                    window.draw(rect);
                    break;
                case CLOSED:
                    rect.setFillColor(sf::Color(0, 255, 0));
                    rect.setPosition(j * screenWidth / cellsW, i * screenHeight / cellsH);
                    window.draw(rect);
                    break;
                case PATH:
                    rect.setFillColor(sf::Color::Cyan);
                    rect.setPosition(j * screenWidth / cellsW, i * screenHeight / cellsH);
                    window.draw(rect);
                    break;
            }
        }
    }
    window.display();
    window.clear(sf::Color(255, 255, 255));
}

void events()
{
    while (window.pollEvent(event))
    {
        switch (event.type)
        {
            case sf::Event::Closed:
                window.close();
                gameState = EXIT;
                break;
            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) pressedL = true;
                else if (event.mouseButton.button == sf::Mouse::Right) pressedR = true;
                break;
            case sf::Event::MouseButtonReleased:
                pressedL = false; 
                pressedR = false;
                break;
            case sf::Event::KeyPressed:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
                {
                    if (gameState == STOPPED)
                    {
                        cellsW += 2;
                        resizeCells();
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
                {
                    if (gameState == STOPPED)
                    {
                        if (cellsW >= 4) 
                        {
                            cellsW -= 2;
                            resizeCells();
                        }
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                {
                    if (gameState == STOPPED)
                    {
                        cellsH += 2;
                        resizeCells(); 
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
                {
                    if (gameState == STOPPED)
                    {
                        if (cellsH >= 4) 
                        {
                            cellsH -= 2;
                            resizeCells();
                        }
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1))
                {
                    if (gameState == STOPPED)
                    {
                        int x = toColumn(sf::Mouse::getPosition(window).x);
                        int y = toRow(sf::Mouse::getPosition(window).y);
                        if (x >= 0 && x < cellsW && y >= 0 && y < cellsH)
                        {
                            if (start.hasPosition) changeCellState(start.x, start.y, PREVIOUS);
                            changeCellState(x, y, START);
                            start.setPosition(x, y);
                        }
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2))
                {
                    if (gameState == STOPPED)
                    {
                        int x = toColumn(sf::Mouse::getPosition(window).x);
                        int y = toRow(sf::Mouse::getPosition(window).y);
                        if (x >= 0 && x < cellsW && y >= 0 && y < cellsH)
                        {
                            if (finish.hasPosition) changeCellState(finish.x, finish.y, PREVIOUS);
                            changeCellState(x, y, FINISH);
                            finish.setPosition(x, y);
                        }
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter))
                {
                    if (gameState == STOPPED && start.hasPosition && finish.hasPosition)
                    {
                        if (!startedP) startPathAlgorithm();
                        gameState = RUNNING;
                        window.setTitle("A* Pathfinding Algorithm, State: " + stateToString[gameState]);
                    }
                    else if (gameState == RUNNING) 
                    {
                        gameState = STOPPED;
                        window.setTitle("A* Pathfinding Algorithm, State: " + stateToString[gameState]);
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
                {
                    if (gameState == STOPPED) 
                    {
                        if (!startedM) startMazeAlgorithm();
                        gameState = MAZING;
                        window.setTitle("A* Pathfinding Algorithm, State: " + stateToString[gameState]);
                    }
                    else if (gameState == MAZING) 
                    {
                        gameState = STOPPED;
                        window.setTitle("A* Pathfinding Algorithm, State: " + stateToString[gameState]);
                    }
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Delete))
                {
                    if (gameState == STOPPED) restartCells();
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Backspace))
                {
                    if (gameState == STOPPED) resetCells(EMPTY);
                }
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Tab))
                {
                    if (gameState == STOPPED) 
                    {
                        if (!startedP) startPathAlgorithm();
                        doPathAlgorithm();
                    }
                }
                break;
        }
    }
}

void processing()
{
    switch (gameState)
    {
        case RUNNING:
            doPathAlgorithm();
            break;
        case MAZING:
            doMazeAlgorithm();
            break;
        case STOPPED:
            int x;
            int y;
            if (pressedL)
            {
                x = toColumn(sf::Mouse::getPosition(window).x);
                y = toRow(sf::Mouse::getPosition(window).y);
                if (x >= 0 && x < cellsW && y >= 0 && y < cellsH) changeCellState(x, y, WALL);
            }
            if (pressedR)
            {
                x = toColumn(sf::Mouse::getPosition(window).x);
                y = toRow(sf::Mouse::getPosition(window).y);
                if (x >= 0 && x < cellsW && y >= 0 && y < cellsH) changeCellState(x, y, EMPTY);
            }
            break;
    }
}

void startPathAlgorithm()
{
    array<int, 2> pos = start.getPosition();
    open.push_back(pos);
    gCost[pos] = 0;
    hCost[pos] = hCostCalc(start.x, start.y);
    fCost[pos] = gCost[pos] + hCost[pos];
    startedP = true;   
}

void doPathAlgorithm()
{
    array<array<int, 2>, 2> total = lowestFCost(open);
    array<int, 2> current = total.at(0);
    if (current != finish.getPosition())
    {
        open.erase(open.begin() + total.at(1).at(0));
        inOpen[current] = false;
        closed.push_back(current);
        inClosed[current] = true;
        CellState &currentState = cells.at(current.at(1)).at(current.at(0));
        if (currentState != START && currentState != FINISH) currentState = CLOSED;
        //! Loop for neighbors
        for (int i = -1; i <= 1; i++)
        {
            for (int j = -1; j <= 1; j++)
            {
                //! Each neighbor
                if (i == 0 && j == 0) continue;
                array<int, 2> neighbor = {current.at(0) + j, current.at(1) + i};
                //* If Neighbor is out of bounds
                if (neighbor.at(0) < 0 || neighbor.at(0) >= cellsW || neighbor.at(1) < 0 || neighbor.at(1) >= cellsH) continue;
                CellState &cell = cells.at(neighbor.at(1)).at(neighbor.at(0));
                //* If Neighbor is wall
                if (cell == WALL) continue;
                //* If Neighbor is already evaluated
                if (inClosed[neighbor]) continue;
                int tempGCost = gCostCalc(i, j, gCost[current]);
                int tempHCost = hCostCalc(neighbor.at(0), neighbor.at(1));
                int tempFCost = tempGCost + tempHCost;
                if (!inOpen[neighbor] || tempFCost < fCost[neighbor])
                {
                    gCost[neighbor] = tempGCost;
                    hCost[neighbor] = tempHCost;
                    fCost[neighbor] = tempFCost;
                    if (!inOpen[neighbor])
                    {
                        open.push_back(neighbor);
                        inOpen[neighbor] = true;
                        parent[neighbor] = current;
                        if (cell != START && cell != FINISH) cell = OPEN;
                    }
                }
            }
        }
    }
    else 
    {
        array<int, 2> tile = current;
        while (1)
        {
            tile = parent[tile];
            if (cells.at(tile.at(1)).at(tile.at(0)) == START) break;
            cells.at(tile.at(1)).at(tile.at(0)) = PATH;
        }
        gameState = STOPPED;
        window.setTitle("A* Pathfinding Algorithm, State: " + stateToString[gameState]);
    }
}

void startMazeAlgorithm()
{
    for (int i = 0; i < cellsH; i++)
    {
        for (int j = 0; j < cellsW; j++)
        {
            if (i % 2 == 1 || j % 2 == 1) cells.at(i).at(j) = WALL;
            else cells.at(i).at(j) = EMPTY;
        }
    }
    currCell = {rand() % (cellsW / 2) * 2, rand() % (cellsH / 2) * 2};
    visited[currCell] = true;
    visitedCells = 1;
    startedM = true;
}

void doMazeAlgorithm()
{
    if (visitedCells < cellsW * cellsH / 4)
    {
        array<int, 2> randNeighbor = randomNeighbor(currCell);
        if (!visited[randNeighbor])
        {
            cells.at((currCell.at(1) + randNeighbor.at(1)) / 2).at((currCell.at(0) + randNeighbor.at(0)) / 2) = EMPTY;
            visited[randNeighbor] = true;
            visitedCells++;
        }
        currCell = randNeighbor;
    }
    else 
    {
        gameState = STOPPED;
        window.setTitle("A* Pathfinding Algorithm, State: " + stateToString[gameState]);
    }
}

void restartCells()
{
    for (auto &row : cells)
    {
        for (CellState &cellState : row)
        {
            if (cellState != START && cellState != FINISH && cellState != WALL) cellState = EMPTY;
        }
    }
    open.clear();
    closed.clear();
    gCost.clear();
    hCost.clear();
    fCost.clear();
    parent.clear();
    inOpen.clear();
    inClosed.clear();
    startedP = false;
}

void resetCells(CellState state)
{
    for (auto &row : cells)
    {
        for (CellState &cellState : row)
        {
            cellState = state;
        }
    }
    start.hasPosition = false;
    finish.hasPosition = false;
    open.clear();
    closed.clear();
    gCost.clear();
    hCost.clear();
    fCost.clear();
    parent.clear();
    inOpen.clear();
    inClosed.clear();
    visited.clear();
    startedP = false;
    startedM = false;
}

array<int, 2> randomNeighbor(const array<int, 2> &og)
{
    map<int, array<int, 2>> numToArray;
    int possible = 0;
    for (int i = 0; i < 4; i ++)
    {
        int x;
        int y;
        switch (i)
        {
            case 0:
                x = og.at(0);
                y = og.at(1) - 2;
                break;
            case 1:
                x = og.at(0);
                y = og.at(1) + 2;
                break;
            case 2:
                x = og.at(0) - 2;
                y = og.at(1);
                break;
            case 3:
                x = og.at(0) + 2;
                y = og.at(1);
                break;
        }
        if (x < 0 || x >= cellsW || y < 0 || y >= cellsH) continue;
        numToArray[possible] = {x, y};
        possible++;
    }
    return numToArray[rand() % possible];
}

void changeCellState(int x, int y, CellState newState)
{
    CellState &val = cells.at(y).at(x);
    if (val == START) start.hasPosition = false;
    else if (val == FINISH) finish.hasPosition = false; 
    if (newState == PREVIOUS) 
    {
        val = prevState[{x, y}];
        prevState[{x, y}] = EMPTY;
    }
    else 
    {
        prevState[{x, y}] = val;
        val = newState;
    }
}

array<array<int, 2>, 2> lowestFCost(const vector<array<int, 2>> &openList)
{
    array<array<int, 2>, 2> returnVal;
    int lowestFCost;
    array<int, 2> lowestFCostArray;
    int lowestFCostIndex;
    for (int i = 0; i < openList.size(); i++)
    {
        if (i == 0)
        {
            lowestFCost = fCost[openList.at(i)];
            lowestFCostArray = openList.at(i);
            lowestFCostIndex = i;
        }
        else
        {
            if (fCost[openList.at(i)] < lowestFCost)
            {
                lowestFCost = fCost[openList.at(i)];
                lowestFCostArray = openList.at(i);
                lowestFCostIndex = i;
            }
        }
    }
    returnVal.at(0) = lowestFCostArray;
    returnVal.at(1) = {lowestFCostIndex, lowestFCost};
    return returnVal;
}

void resizeCells()
{
    cells.resize(cellsH);
    for (auto &row : cells)
    {
        row.resize(cellsW);
    }
}

inline int gCostCalc(int i, int j, int prevGCost)
{
    if (abs(i) == 1 && abs(j) == 1) return prevGCost + costD;
    return prevGCost + costN;
}

inline int hCostCalc(int x, int y)
{
    int max1 = max(abs(x - finish.x), abs(y - finish.y));
    int min1 = min(abs(x - finish.x), abs(y - finish.y));
    return costD * min1 + costN * (max1 - min1);
}

inline int toColumn(int x)
{
    return x / ((double)window.getSize().x / cellsW);
}

inline int toRow(int y)
{
    return y / ((double)window.getSize().y / cellsH);
}

Position::Position(int a, int b, bool start)
{
    x = a;
    y = b;
    hasPosition = start;
};

void Position::setPosition(int a, int b)
{
    x = a;
    y = b;
    hasPosition = true;
};

array<int, 2> Position::getPosition()
{
    array<int, 2> pos = {x, y};
    return pos;
}