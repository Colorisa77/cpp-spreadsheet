#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <optional>

size_t PositionHash::operator()(Position pos) const {
    return std::hash<int>{}(pos.row) + (std::hash<int>{}(pos.col) << 1);
}

Sheet::Sheet() = default;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if(!pos.IsValid()) {
        throw InvalidPositionException("Invalid Position");
    }

    if(IsInitialized(pos)) {
        Cell prev_cell = std::move(cell_index_.at(pos));
        try {
            CleanCell(pos);
            cell_index_.at(pos).CopyDependedAndReferencedCells(prev_cell);
            cell_index_.at(pos).Set(text);
        } catch (const CircularDependencyException&) {
            CleanCell(pos);
            cell_index_.at(pos) = std::move(prev_cell);
            throw CircularDependencyException("circular dependency");
        }
        return;
    }

    Cell cell(*this);
    cell_index_.emplace(pos, std::move(cell));

    Cell* cell_ptr = &cell_index_.at(pos);
    cell_ptr->Set(text);

    ++row_elem_count_[pos.row];
    ++col_elem_count_[pos.col];
}

void Sheet::ClearCell(Position pos) {
    if(!pos.IsValid()) {
        throw InvalidPositionException("Invalid Position");
    }

    if(IsInitialized(pos)) {

        --row_elem_count_[pos.row];
        --col_elem_count_[pos.col];
        if(row_elem_count_[pos.row] == 0) {
            row_elem_count_.erase(pos.row);
        }
        if(col_elem_count_[pos.col] == 0) {
            col_elem_count_.erase(pos.col);
        }
        cell_index_.erase(pos);
    }
}

void Sheet::CleanCell(Position pos) {
    if(!pos.IsValid()) {
        throw InvalidPositionException("Invalid Position");
    }
    if(IsInitialized(pos)) {
        cell_index_.at(pos).Clear();
    }
}

const CellInterface* Sheet::GetCellInternal(Position pos) const {
    if(!pos.IsValid()) {
        throw InvalidPositionException("Invalid Position");
    }
    if(cell_index_.count(pos) > 0) {
        return &cell_index_.at(pos);
    }
    return nullptr;
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellInternal(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return const_cast<CellInterface*>(GetCellInternal(pos));
}

std::vector<int> Sheet::GetSortedElements(const std::unordered_map<int, int>& container) {
    std::vector<int> result;
    result.reserve(container.size());

    for(const auto& [key, _] : container) {
        result.push_back(key);
    }

    std::sort(result.begin(), result.end());
    return result;
}

Size Sheet::GetStartPosPrintableZone() const {
    return {0, 0};
}

Size Sheet::GetEndPosPrintableZone() const {
    if(row_elem_count_.empty() && col_elem_count_.empty()) {
        return Size{};
    }
    int max_row = GetSortedElements(row_elem_count_).back();
    int max_col = GetSortedElements(col_elem_count_).back();
    return {max_row, max_col};
}

Size Sheet::GetPrintableSize() const {
    if(cell_index_.empty()) {
        return {0, 0};
    }
    Size start = GetStartPosPrintableZone();
    Size end = GetEndPosPrintableZone();
    return {end.rows - start.rows + 1, end.cols - start.cols + 1};
}

void Sheet::PrintValues(std::ostream& output) const {
    if(cell_index_.empty()) {
        return;
    }
    Size start_pos = GetStartPosPrintableZone();
    Size end_pos = GetEndPosPrintableZone();

    for(int row = start_pos.rows; row <= end_pos.rows; ++row) {
        for(int col = start_pos.cols; col <= end_pos.cols; ++col) {
            const CellInterface* cell_ptr = GetCell({row, col});
            if(cell_ptr != nullptr) {
                std::visit([&output](const auto& obj){
                    output << obj;
                }, cell_ptr->GetValue());
            }
            if(col != end_pos.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream &output) const {
    if(cell_index_.empty()) {
        return;
    }
    Size start_pos = GetStartPosPrintableZone();
    Size end_pos = GetEndPosPrintableZone();

    for(int row = start_pos.rows; row <= end_pos.rows; ++row) {
        for(int col = start_pos.cols; col <= end_pos.cols; ++col) {
            const CellInterface* cell_ptr = GetCell({row, col});
            if(cell_ptr != nullptr) {
                output << cell_ptr->GetText();
            }
            if(col != end_pos.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

bool Sheet::IsInitialized(Position pos) const {
    return cell_index_.count(pos) > 0;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}