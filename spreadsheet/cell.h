#pragma once

#include "common.h"
#include "formula.h"
#include <optional>
#include <iostream>
#include <set>

class Cell : public CellInterface {
    using FormulaInterfacePtr = std::unique_ptr<FormulaInterface>;

public:
    Cell(SheetInterface& sheet_ref);

    Cell(const Cell& other) = delete;

    Cell(Cell&& other) noexcept;

    Cell& operator=(const Cell& other) = delete;
    Cell& operator=(Cell&& other) noexcept;

    bool operator==(const Cell& other) const;

    ~Cell() override;

    bool IsInitialized() const;

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    void CopyDependedAndReferencedCells(Cell& other);

private:
    FormulaInterfacePtr cell_value_ = nullptr;
    std::string text_;

    SheetInterface& sheet_ref_;

    mutable std::optional<FormulaInterface::Value> cache_;

    std::set<Cell*> dependent_cells_;
    std::set<Cell*> referenced_cells_;

    static bool IsText(std::string text);
    bool IsFormula() const;

    void Swap(Cell& other);

    bool FindCircularDependency(const std::vector<Position>& ref) const;

    bool HasCache() const;
    Value GetCache() const;

    void Refresh();
    void InvalidateCache();
};
