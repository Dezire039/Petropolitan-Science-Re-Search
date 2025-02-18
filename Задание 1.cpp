
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <random>
#include <mutex>

using namespace std;

// Константы для представления доски и фигур
const int BOARD_SIZE = 8;
const char EMPTY = '.';
const char WHITE = 'W';
const char BLACK = 'B';
const char WHITE_KING = 'X';
const char BLACK_KING = 'Y';

// Структура для представления хода
struct Move {
    int from_row;
    int from_col;
    int to_row;
    int to_col;
    vector<pair<int, int>> captures; // Список захваченных шашек (row, col)
};

// Функция для вывода доски на консоль
void printBoard(const vector<vector<char>>& board, char playerColor) {
    cout << "  | A B C D E F G H" << endl;
    cout << "--+-----------------" << endl;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        cout << i + 1 << " | ";
        for (int j = 0; j < BOARD_SIZE; ++j) {
            char piece = board[i][j];
            if (playerColor == BLACK) {
                // Переворачиваем доску для черных
                piece = board[BOARD_SIZE - 1 - i][BOARD_SIZE - 1 - j];
            }

            cout << piece << " ";
        }
        cout << endl;
    }

    cout << endl;
}

// Функция для преобразования координат вида "A6" в индексы массива (row, col)
pair<int, int> parseCoordinates(const string& coordinates, char playerColor) {
    int col = coordinates[0] - 'A';
    int row = coordinates[1] - '1';

    if (playerColor == BLACK)
    {
        col = BOARD_SIZE - 1 - col;
        row = BOARD_SIZE - 1 - row;
    }

    return {row, col};
}

// Функция для преобразования индексов массива (row, col) в координаты вида "A6"
string formatCoordinates(int row, int col, char playerColor) {
    if (playerColor == BLACK)
    {
        col = BOARD_SIZE - 1 - col;
        row = BOARD_SIZE - 1 - row;
    }

    char colChar = 'A' + col;
    char rowChar = '1' + row;

    return string(1, colChar) + string(1, rowChar);
}


// Функция для проверки, находится ли индекс в пределах доски
bool isValidIndex(int row, int col) {
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

// Функция для получения всех возможных ходов для определенной шашки
vector<Move> getPossibleMoves(const vector<vector<char>>& board, int row, int col, char playerColor) {
    vector<Move> moves;
    char piece = board[row][col];
    bool isKing = (piece == WHITE_KING || piece == BLACK_KING);
    int direction = (playerColor == WHITE) ? 1 : -1;

    // Простые ходы
    int dr[] = { direction, direction, -direction, -direction };
    int dc[] = { -1, 1, -1, 1 };

    if (isKing)
    {
        dr[0] = 1; dr[1] = 1; dr[2] = -1; dr[3] = -1;
        dc[0] = -1; dc[1] = 1; dc[2] = -1; dc[3] = 1;
    }

    for (int i = 0; i < 4; ++i) {
        int new_row = row + dr[i];
        int new_col = col + dc[i];

        if (isValidIndex(new_row, new_col) && board[new_row][new_col] == EMPTY) {
            moves.push_back({row, col, new_row, new_col, {}});
        }
    }

    // Рубки
    dr[0] = 2 * direction;
    dr[1] = 2 * direction;
    dr[2] = -2 * direction;
    dr[3] = -2 * direction;
    dc[0] = -2;
    dc[1] = 2;
    dc[2] = -2;
    dc[3] = 2;


    if (isKing)
    {
        dr[0] = 2; dr[1] = 2; dr[2] = -2; dr[3] = -2;
        dc[0] = -2; dc[1] = 2; dc[2] = -2; dc[3] = 2;
    }


    for (int i = 0; i < 4; ++i) {
        int new_row = row + dr[i];
        int new_col = col + dc[i];
        int jump_row = row + dr[i] / 2;
        int jump_col = col + dc[i] / 2;

        if (isValidIndex(new_row, new_col) && board[new_row][new_col] == EMPTY &&
            isValidIndex(jump_row, jump_col)) {
            char captured_piece = board[jump_row][jump_col];
            if (captured_piece != EMPTY &&
                ((playerColor == WHITE && (captured_piece == BLACK || captured_piece == BLACK_KING)) ||
                 (playerColor == BLACK && (captured_piece == WHITE || captured_piece == WHITE_KING))))
            {
                moves.push_back({row, col, new_row, new_col, {{jump_row, jump_col}}});

                // Продолжение рубки (рекурсивно)
                vector<vector<char>> tempBoard = board;
                tempBoard[row][col] = EMPTY;
                tempBoard[jump_row][jump_col] = EMPTY;
                tempBoard[new_row][new_col] = piece;

                vector<Move> subsequentCaptures = getPossibleMoves(tempBoard, new_row, new_col, playerColor);
                for (const auto& subsequentMove : subsequentCaptures)
                {
                    if (!subsequentMove.captures.empty()) //Если есть куда рубить еще
                    {
                        Move combinedMove = {row, col, subsequentMove.to_row, subsequentMove.to_col, {}};
                        combinedMove.captures.push_back({jump_row, jump_col});
                        combinedMove.captures.insert(combinedMove.captures.end(), subsequentMove.captures.begin(), subsequentMove.captures.end());
                        moves.push_back(combinedMove);
                    }
                }
            }
        }
    }

    return moves;
}

// Функция для получения всех возможных ходов для игрока
vector<Move> getAllPossibleMoves(const vector<vector<char>>& board, char playerColor) {
    vector<Move> allMoves;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (((playerColor == WHITE && (board[i][j] == WHITE || board[i][j] == WHITE_KING)) || (playerColor == BLACK && (board[i][j] == BLACK || board[i][j] == BLACK_KING))))
            {
                vector<Move> moves = getPossibleMoves(board, i, j, playerColor);
                allMoves.insert(allMoves.end(), moves.begin(), moves.end());
            }
        }
    }

    // Проверка на принудительную рубку
    bool hasCapture = false;
    for (const auto& move : allMoves) {
        if (!move.captures.empty()) {
            hasCapture = true;
            break;
        }
    }

    if (hasCapture) {
        // Удаляем все ходы без рубки
        allMoves.erase(remove_if(allMoves.begin(), allMoves.end(), [](const Move& move) {
            return move.captures.empty();
            }), allMoves.end());
    }

    return allMoves;
}

// Функция для выполнения хода на доске
void makeMove(vector<vector<char>>& board, const Move& move) {
    char piece = board[move.from_row][move.from_col];
    board[move.from_row][move.from_col] = EMPTY;
    board[move.to_row][move.to_col] = piece;

    // Рубка шашек противника
    for (const auto& capture : move.captures) {
        board[capture.first][capture.second] = EMPTY;
    }

    // Превращение в дамку
    if (piece == WHITE && move.to_row == BOARD_SIZE - 1) {
        board[move.to_row][move.to_col] = WHITE_KING;
    } else if (piece == BLACK && move.to_row == 0) {
        board[move.to_row][move.to_col] = BLACK_KING;
    }
}

// Функция для отмены хода на доске
void unmakeMove(vector<vector<char>>& board, const Move& move, char capturedPiece) {
     char piece = board[move.to_row][move.to_col];
     board[move.to_row][move.to_col] = EMPTY;
     board[move.from_row][move.from_col] = piece;

    //Восстановление захваченых шашек.  НУЖНО ПРОРАБОТАТЬ ЛУЧШЕ, ПОКА НЕ РАБОТАЕТ ПОЛНОЦЕННО
    for (const auto& capture : move.captures)
    {
        board[capture.first][capture.second] = (piece == WHITE || piece == WHITE_KING) ? BLACK : WHITE;
    }

     //Отмена превращения в дамку.
     if (piece == WHITE_KING && move.to_row == BOARD_SIZE - 1)
     {
        board[move.to_row][move.to_col] = WHITE;
     }
     else if (piece == BLACK_KING && move.to_row == 0)
     {
         board[move.to_row][move.to_col] = BLACK;
     }
}

// Функция для оценки позиции доски (простая эвристика)
int evaluateBoard(const vector<vector<char>>& board) {
    int whiteCount = 0;
    int blackCount = 0;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (board[i][j] == WHITE) {
                whiteCount += 1;
            } else if (board[i][j] == BLACK) {
                blackCount += 1;
            } else if (board[i][j] == WHITE_KING) {
                whiteCount += 3; // Дамки ценнее
            } else if (board[i][j] == BLACK_KING) {
                blackCount += 3; // Дамки ценнее
            }
        }
    }
    return whiteCount - blackCount; // Положительное значение - преимущество белых
}

// Функция Minimax с альфа-бета отсечением
int minimax(vector<vector<char>>& board, int depth, int alpha, int beta, bool maximizingPlayer, char maximizingColor) {
    if (depth == 0) {
        return evaluateBoard(board);
    }

    vector<Move> moves = getAllPossibleMoves(board, maximizingPlayer ? maximizingColor : (maximizingColor == WHITE ? BLACK : WHITE));

    if (moves.empty())
    {
        //Нет ходов - оцениваем как проигрыш
        return maximizingPlayer ? -1000 : 1000;
    }


    if (maximizingPlayer) {
        int maxEval = -10000;
        for (const auto& move : moves) {
            vector<vector<char>> tempBoard = board;
            makeMove(tempBoard, move);
            int eval = minimax(tempBoard, depth - 1, alpha, beta, false, maximizingColor);
            maxEval = max(maxEval, eval);
            alpha = max(alpha, eval);
            if (beta <= alpha) {
                break; // Бета-отсечение
            }
        }
        return maxEval;
    } else {
        int minEval = 10000;
        for (const auto& move : moves) {
            vector<vector<char>> tempBoard = board;
            makeMove(tempBoard, move);
            int eval = minimax(tempBoard, depth - 1, alpha, beta, true, maximizingColor);
            minEval = min(minEval, eval);
            beta = min(beta, eval);
            if (beta <= alpha) {
                break; // Альфа-отсечение
            }
        }
        return minEval;
    }
}

// Функция для нахождения лучшего хода (используем Minimax)
Move findBestMove(vector<vector<char>>& board, char aiColor, int depth) {
    vector<Move> moves = getAllPossibleMoves(board, aiColor);
    if (moves.empty()) {
        //нет доступных ходов - возвращаем пустой ход
        return {};
    }

    int bestEval = -10000;
    Move bestMove = moves[0]; // Initialize with the first move

    // Параллельный цикл для оценки ходов
    vector<int> evaluations(moves.size());
    vector<thread> threads;
    mutex mtx; // Mutex for protecting shared variables

    int num_threads = thread::hardware_concurrency();
    num_threads = (num_threads > 0) ? num_threads : 1; // ensure at least 1 thread

    int moves_per_thread = moves.size() / num_threads;
    int remainder = moves.size() % num_threads; // for distributing the remaining moves

    for (int t = 0; t < num_threads; ++t) {
        int start = t * moves_per_thread;
        int end = start + moves_per_thread;

        // Distribute the remainder among the first few threads
        if (t < remainder) {
            start += t;
            end += (t + 1);
        } else {
            start += remainder;
            end += remainder;
        }

        threads.emplace_back([&, start, end, t]() {
            for (int i = start; i < end; ++i) {
                vector<vector<char>> tempBoard = board;
                makeMove(tempBoard, moves[i]);
                evaluations[i] = minimax(tempBoard, depth - 1, -10000, 10000, false, aiColor);
            }

            // Find the best move (within the thread's range) and update the shared variables
            int localBestEval = -10000;
            Move localBestMove = moves[start];

            for (int i = start; i < end; ++i) {
                if (evaluations[i] > localBestEval) {
                    localBestEval = evaluations[i];
                    localBestMove = moves[i];
                }
            }

            // Protect access to the shared best move
            lock_guard<mutex> lock(mtx);
            if (localBestEval > bestEval) {
                bestEval = localBestEval;
                bestMove = localBestMove;
            }
        });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    return bestMove;
}

// Функция для проверки победителя
char checkWinner(const vector<vector<char>>& board) {
    bool whiteExists = false;
    bool blackExists = false;

    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (board[i][j] == WHITE || board[i][j] == WHITE_KING) {
                whiteExists = true;
            } else if (board[i][j] == BLACK || board[i][j] == BLACK_KING) {
                blackExists = true;
            }
        }
    }

    if (!whiteExists) {
        return BLACK;
    } else if (!blackExists) {
        return WHITE;
    } else {
        return EMPTY; // Игра продолжается
    }
}

int main() {
    // Инициализация доски
    vector<vector<char>> board(BOARD_SIZE, vector<char>(BOARD_SIZE, EMPTY));
    for (int i = 0; i < 3; ++i) {
        for (int j = (i % 2 == 0) ? 1 : 0; j < BOARD_SIZE; j += 2) {
            board[i][j] = BLACK;
        }
    }
    for (int i = BOARD_SIZE - 3; i < BOARD_SIZE; ++i) {
        for (int j = (i % 2 == 0) ? 1 : 0; j < BOARD_SIZE; j += 2) {
            board[i][j] = WHITE;
        }
    }

    // Выбор стороны игрока
    char playerColor;
    cout << "Choose a side (W - white, B - black): ";
    cin >> playerColor;
    playerColor = toupper(playerColor);
    char aiColor = (playerColor == WHITE) ? BLACK : WHITE;

    if (playerColor != WHITE && playerColor != BLACK) {
        cout << "Incorrect input. You play for the whites." << endl;
        playerColor = WHITE;
        aiColor = BLACK;
    }

    // Основной цикл игры
    bool gameRunning = true;
    char winner = EMPTY;

    if (playerColor == BLACK) {
        // Первый ход за AI
        Move aiMove = findBestMove(board, aiColor, 4); // Глубина поиска
        makeMove(board, aiMove);
        cout << "Computer progress: " << formatCoordinates(aiMove.from_row, aiMove.from_col, WHITE)  << " " << formatCoordinates(aiMove.to_row, aiMove.to_col, WHITE) << endl;
    }

    while (gameRunning) {
        printBoard(board, playerColor);

        // Ход игрока
        string moveStr;
        cout << "Your move (for example, A6 B5): ";
        cin >> moveStr;

        try {
            pair<int, int> from = parseCoordinates(moveStr.substr(0, 2), playerColor);
            pair<int, int> to = parseCoordinates(moveStr.substr(3, 2), playerColor);

            Move playerMove;
            playerMove.from_row = from.first;
            playerMove.from_col = from.second;
            playerMove.to_row = to.first;
            playerMove.to_col = to.second;

            // Получаем *ВСЕ* возможные ходы для игрока
            vector<Move> possibleMoves = getAllPossibleMoves(board, playerColor);

            // Ищем введенный ход в списке всех возможных ходов
            auto it = find_if(possibleMoves.begin(), possibleMoves.end(), [&playerMove, &from](const Move& move) {
                return move.from_row == from.first && move.from_col == from.second &&
                    move.to_row == playerMove.to_row && move.to_col == playerMove.to_col;
                });

            if (it == possibleMoves.end()) {
                cout << "Incorrect move." << endl;
                continue;
            }

            makeMove(board, *it);
        }
        catch (const std::exception& e) {
            cout << "Incorrect move." << endl;
            continue;
        }

        // Проверка победителя
        winner = checkWinner(board);
        if (winner != EMPTY) {
            gameRunning = false;
            break;
        }

        // Ход AI
        Move aiMove = findBestMove(board, aiColor, 4); // Глубина поиска
        if (aiMove.captures.size() == 0 && aiMove.to_col == 0 && aiMove.to_row == 0 && aiMove.from_row == 0 && aiMove.from_col == 0)
        {
            winner = playerColor;
            gameRunning = false;
            cout << "The computer has no available moves." << endl;
            break;
        }
        makeMove(board, aiMove);
        cout << "Computer progress: " << formatCoordinates(aiMove.from_row, aiMove.from_col, playerColor) << " " << formatCoordinates(aiMove.to_row, aiMove.to_col, playerColor) << endl;

        // Проверка победителя
        winner = checkWinner(board);
        if (winner != EMPTY) {
            gameRunning = false;
            break;
        }
    }

    // Вывод результата
    if (winner == playerColor) {
        cout << "Congratulations! You've won!" << endl;
    } else {
        cout << "You've lost." << endl;
    }

    printBoard(board, playerColor);

    return 0;
}
