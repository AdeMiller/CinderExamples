#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <algorithm>
#include <array>
#include <cctype>
#include <iostream>
#include <numeric>
#include <stack>
#include <string>
#include <sstream>
#include <unordered_map>

using namespace ::std;

namespace Sudoku
{
    typedef unsigned short Cell;
    typedef array<Cell, 81> Board;
    typedef array<char, 9> Group;

    static const unsigned short value_mask =  0b0000000111111111;
    static const unsigned short locked_mask = 0b0000001000000000;
    static const unsigned short guess_mask =  0b0000010000000000;

    int cell_value(Cell c);
    bool cell_guess(Cell c);
    bool cell_locked(Cell c);

    class SudokuSolver
    {
    private:
        stack<Board> boards;
        static const array<Group, 27> group_offsets;

    public:
        SudokuSolver() : boards() {}

        inline Cell get_cell(const int row, const int col) const { return boards.top()[row * 9 + col]; }

        bool load_sdm(const string &data)
        {
            if (data.length() != 81)
                return false;

            Board board;
            transform(cbegin(data), cend(data), begin(board), [this](const char c) {
                auto v = atoi(&c);
                return v == 0 ? value_mask : (0b1 << (v - 1) | locked_mask);
            });
            boards = stack<Board>({board});
            return true;
        }

        bool is_correct() const { return all_of(cbegin(group_offsets), cend(group_offsets), [this](const Group &g) { return is_group_correct(g); }); }

        bool solve()
        {
            cout << "===================================================" << endl;

            bool changed = false;
            for (const auto &g: SudokuSolver::group_offsets)
                changed |= solve_group(g);

            if (changed)
                return true;

            if (is_correct())
                return false;

            if (!is_complete()) {
                make_guesses();
                return true;
            }
            cout << "Bad guess, unrolling." << endl;
            boards.pop();
            return true;
        }

    private:
        bool make_guesses()
        {
            auto i = distance(cbegin(boards.top()), min_element(cbegin(boards.top()), cend(boards.top()), SudokuSolver::is_lowest));

            auto current_board = boards.top();
            boards.pop();
            for (auto m = 0b1; m < 512; m <<= 1)
                if (current_board[i] & m) {
                    boards.push(current_board);
                    boards.top()[i] = m | guess_mask;
                    cout << "[" << (i / 9 + 1) << ", " << (i % 9 + 1) << "]: " << to_string(current_board[i]) << " => " << to_string(m) << " <------------ GUESS" << endl;
                }
        }

        bool solve_group(const Group &g)
        {
            unordered_map<unsigned short, int> values_counts(9);
            for (const auto i: g)
                values_counts[boards.top()[i] & value_mask]++;

            bool changed = false;
            for (const auto &c: values_counts)
                if (__builtin_popcount(c.first) == c.second)
                    for (const auto i: g)
                        if (__builtin_popcount(boards.top()[i] & value_mask) != 1 && boards.top()[i] != c.first &&
                            boards.top()[i] & c.first) {
                            boards.top()[i] &= ~c.first;
                            if (cell_value(boards.top()[i]))
                                cout << "[" << (i / 9 + 1) << ", " << (i % 9 + 1) << "]: => " << to_string(boards.top()[i]) << endl;
                            changed = true;
                        }
            return changed;
        }

        bool is_complete() const { return all_of(cbegin(boards.top()), cend(boards.top()), [this](const Cell &c) { return cell_value(c); }); }

        bool is_group_correct(const Group &group) const
        {
            auto r = accumulate(cbegin(group), cend(group), 0b0, [this](const unsigned short v, const char i) {
                return v | (cell_value(boards.top()[i]) ? boards.top()[i] : 0);
            });
            return (r & value_mask) == value_mask;
        }

        string to_string(const unsigned short v) const
        {
            stringstream buf;
            for (auto m = 0b1; m < 512; m <<= 1)
                if (v & m)
                    buf << __builtin_ffs(m);
            return buf.str();
        }

        static bool is_lowest(const Cell &i, const Cell &j)
        {
            auto vi = __builtin_popcount(i & value_mask);
            auto vj = __builtin_popcount(j & value_mask);
            return (vi > 1 ? vi : 10) < (vj > 1 ? vj : 10);
        }
    };
};

#endif //SUDOKU_SUDOKUSOLVER_H
