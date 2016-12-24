#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <iomanip>
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
    static const unsigned short bad_mask =    0b0000100000000000;

    int cell_value(Cell c);

    struct CellStrm {
        Cell v;

        CellStrm(Cell c): v(c) {}
    };

    ostream& operator<<(ostream& os, const CellStrm& c);

    ostream& operator<<(ostream& os, const Group& g);

    class SudokuSolver
    {
    private:
        stack<Board> boards;
        int move_count;
        static const array<Group, 27> group_offsets;

    public:
        SudokuSolver() : boards(), move_count(0) {}

        inline Cell get_cell(const int row, const int col) const { return boards.top()[row * 9 + col]; }

        bool load_sdm(const string &data)
        {
            cout << "Loading =============================================================" << endl << data << endl;
            if (data.length() != 81)
                return false;

            Board board;
            transform(cbegin(data), cend(data), begin(board), [this](const char c) {
                auto v = atoi(&c);
                return v == 0 ? value_mask : (0b1 << (v - 1) | locked_mask);
            });
            boards = stack<Board>({board});
            move_count = 0;
            return true;
        }

        inline bool is_finished() const
        {
            return is_complete() && is_groups_correct();
        }

        bool solve()
        {
            if (is_finished()) {
                cout << "FINISHED !!" << endl;
                return false;
            }

            cout << "Move " << setw(2) << ++move_count <<
                " on board " << setw(2) << boards.size() <<
                " =================================================" << endl;

            if (!is_groups_correct() and boards.size() > 1) {
                cout << "BAD GUESS. Unrolling." << endl;
                boards.pop();
                return true;
            }

            // Use the basic solver to remove possibilities based on existing values. If this results in
            // changes then consider the move over.

            bool changed = false;
            for (const auto &g: SudokuSolver::group_offsets)
                changed |= solve_group(g);

            // If changed look for incorrect groups and mark their cells.

            if (changed) {
                for (const auto &g: SudokuSolver::group_offsets)
                    if (!is_group_correct(g)) {
                        cout << "Group " << g << " incorrect" << endl;
                        for (const auto i: g)
                            boards.top()[i] |= bad_mask;
                    }
                return true;
            }

            if (!is_complete()) {
                make_guesses();
                return true;
            }

            cout << "BAD GUESS. Unrolling." << endl;
            boards.pop();
            return true;
        }

    private:
        bool make_guesses()
        {
            auto i = find_guess_candidate();
            auto current_board = boards.top();
            boards.pop();
            for (auto m = 0b1; m < 512; m <<= 1)
                if (current_board[i] & m) {
                    boards.push(current_board);
                    boards.top()[i] = m | guess_mask;
                    cout << "[" << (i / 9 + 1) << ", " << (i % 9 + 1) << "]: " << CellStrm(current_board[i]) << " => " << CellStrm(m) << " <------------ GUESS" << endl;
                }
        }

        int find_guess_candidate() const
        {
            // Get group with the lowest number of possible values.

            array<int, 27> group_certainties;
            transform(cbegin(SudokuSolver::group_offsets), cend(SudokuSolver::group_offsets), begin(group_certainties), [this](const Group& g) {
                int p = accumulate(cbegin(g), cend(g), 0, [this](int p, const char& i) { return p + __builtin_popcount(boards.top()[i] & value_mask); });
                return  (p != 9) ? p : 82;
            });
            auto i = distance(cbegin(group_certainties), min_element(cbegin(group_certainties), cend(group_certainties), less<int>()));
            cout << "Group " << SudokuSolver::group_offsets[i] << endl;

            // Find the cell with the lowest number of possible vales within the group.

            return *min_element(cbegin(SudokuSolver::group_offsets[i]), cend(SudokuSolver::group_offsets[i]), [this](const char& i, const char& j) { return cell_certainty(i, j); });
        }

        bool solve_group(const Group &g)
        {
            // Count the number of occurences of each possibility set values within a group.

            unordered_map<unsigned short, int> values_counts(9);
            for (const auto i: g)
                values_counts[boards.top()[i] & value_mask]++;

            // If the number of occurences of a possibility set is the same as the set size then remove those
            // possible values from all other cells in the group.

            bool changed = false;
            for (const auto &c: values_counts)
                if (__builtin_popcount(c.first) == c.second)
                    for (const auto i: g)
                        if (__builtin_popcount(boards.top()[i] & value_mask) != 1 && boards.top()[i] != c.first && boards.top()[i] & c.first) {
                            cout << "[" << (i / 9 + 1) << ", " << (i % 9 + 1) << "]: " << CellStrm(boards.top()[i]);
                            boards.top()[i] &= ~c.first;
                            cout << " => " << CellStrm(boards.top()[i]) << endl;
                            changed = true;
                        }
            return changed;
        }

        // The board is complete when all cells have a value.

        inline bool is_complete() const { return all_of(cbegin(boards.top()), cend(boards.top()), [this](const Cell &c) { return cell_value(c); }); }

        inline bool is_groups_correct() const
        {
            return all_of(cbegin(group_offsets), cend(group_offsets), [this](const Group &g) { return is_group_correct(g); });
        }

        bool is_group_correct(const Group &group) const
        {
            // A group is correct if all the cells that have a value are themselves unique.

            array<int, 10> counts = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            for (const auto i: group)
                counts[cell_value(boards.top()[i])]++;
            return none_of(cbegin(counts) + 1, cend(counts), [] (const int& v) { return v > 1; });
        }

        bool cell_certainty(const char &i, const char &j) const
        {
            auto vi = __builtin_popcount(boards.top()[i] & value_mask);
            auto vj = __builtin_popcount(boards.top()[j] & value_mask);
            return (vi > 1 ? vi : 10) < (vj > 1 ? vj : 10);
        }
    };
};

#endif //SUDOKU_SUDOKUSOLVER_H
