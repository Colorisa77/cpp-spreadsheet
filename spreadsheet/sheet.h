#pragma once
#include "cell.h"
#include "common.h"

#include <map>
#include <unordered_map>
#include <functional>
#include <vector>

struct PositionHash {
    size_t operator()(Position pos) const;
};

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;
    void CleanCell(Position pos);

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<Position, Cell, PositionHash> cell_index_;
    std::unordered_map<int, int> row_elem_count_;
    std::unordered_map<int, int> col_elem_count_;

    const CellInterface* GetCellInternal(Position pos) const;

    bool IsInitialized(Position pos) const;

    Size GetStartPosPrintableZone() const;
    Size GetEndPosPrintableZone() const;

    static std::vector<int> GetSortedElements(const std::unordered_map<int, int>& container);
};