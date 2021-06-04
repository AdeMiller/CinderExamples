#include "SudokuSolver.h"

namespace Sudoku
{
    int cell_value(Cell c) { return (__builtin_popcount(c & value_mask) == 1) ? __builtin_ffs(c & value_mask) : 0; }

    ostream& operator<<(ostream& os, const CellStrm& c)
    {
        Cell v = c.v & value_mask;
        if (__builtin_popcount(v) == 1) {
            os << __builtin_ffs(v);
            return os;
        }
        os << "{ ";
        for (auto m = 0b1; m < 512; m <<= 1)
            if (v & m)
                os << __builtin_ffs(m) << " ";
        os << "}";
        return os;
    }

    ostream& operator<<(ostream& os, const CoordStrm& i)
    {
        os << "r" << (i.v / kGridSize + 1) << "c" << (i.v % kGridSize + 1);
        return os;
    }

    ostream& operator<<(ostream& os, const Board& b)
    {
        for (auto c: b)
            os << setw(1) << cell_value(c);
        os << endl;
    }

    const array<int, 10> SudokuSolver::certainty_map = array<int, 10>({ 10, 10, 2, 3, 4, 5, 6, 7, 8, 9 });

    const array<Group, kGroupCount> SudokuSolver::group_offsets = array<Group, 27>({

        // Rows
        Group({  0,  1,  2,  3,  4,  5,  6,  7,  8 }), Group({  9, 10, 11, 12, 13, 14, 15, 16, 17 }),
        Group({ 18, 19, 20, 21, 22, 23, 24, 25, 26 }), Group({ 27, 28, 29, 30, 31, 32, 33, 34, 35 }),
        Group({ 36, 37, 38, 39, 40, 41, 42, 43, 44 }), Group({ 45, 46, 47, 48, 49, 50, 51, 52, 53 }),
        Group({ 54, 55, 56, 57, 58, 59, 60, 61, 62 }), Group({ 63, 64, 65, 66, 67, 68, 69, 70, 71 }),
        Group({ 72, 73, 74, 75, 76, 77, 78, 79, 80 }),

        // Columns
        Group({  0,  9, 18, 27, 36, 45, 54, 63, 72 }), Group({  1, 10, 19, 28, 37, 46, 55, 64, 73 }),
        Group({  2, 11, 20, 29, 38, 47, 56, 65, 74 }), Group({  3, 12, 21, 30, 39, 48, 57, 66, 75 }),
        Group({  4, 13, 22, 31, 40, 49, 58, 67, 76 }), Group({  5, 14, 23, 32, 41, 50, 59, 68, 77 }),
        Group({  6, 15, 24, 33, 42, 51, 60, 69, 78 }), Group({  7, 16, 25, 34, 43, 52, 61, 70, 79 }),
        Group({  8, 17, 26, 35, 44, 53, 62, 71, 80 }),

        // Boxes
        Group({  0,  1,  2,  9, 10, 11, 18, 19, 20 }), Group({  3,  4,  5, 12, 13, 14, 21, 22, 23 }),
        Group({  6,  7,  8, 15, 16, 17, 24, 25, 26 }), Group({ 27, 28, 29, 36, 37, 38, 45, 46, 47 }),
        Group({ 30, 31, 32, 39, 40, 41, 48, 49, 50 }), Group({ 33, 34, 35, 42, 43, 44, 51, 52, 53 }),
        Group({ 54, 55, 56, 63, 64, 65, 72, 73, 74 }), Group({ 57, 58, 59, 66, 67, 68, 75, 76, 77 }),
        Group({ 60, 61, 62, 69, 70, 71, 78, 79, 80 })
    });

    const array<string, kGroupCount> SudokuSolver::group_names = array<string, 27>({

        // Rows
        string("Row 1"),
        string("Row 2"),
        string("Row 3"),
        string("Row 4"),
        string("Row 5"),
        string("Row 6"),
        string("Row 7"),
        string("Row 8"),
        string("Row 9"),

        // Columns
        string("Col 1"),
        string("Col 2"),
        string("Col 3"),
        string("Col 4"),
        string("Col 5"),
        string("Col 6"),
        string("Col 7"),
        string("Col 8"),
        string("Col 9"),

        // Boxes
        string("Box 1"),
        string("Box 2"),
        string("Box 3"),
        string("Box 4"),
        string("Box 5"),
        string("Box 6"),
        string("Box 7"),
        string("Box 8"),
        string("Box 9"),
    });
};
