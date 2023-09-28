#include "cell.h"
#include <cmath>

Cell::Cell(SheetInterface& sheet_ref)
 : sheet_ref_(sheet_ref){
}

Cell::Cell(Cell&& other) noexcept
    : cell_value_(std::move(other.cell_value_))
    , text_(std::move(other.text_))
    , sheet_ref_(other.sheet_ref_)
    , dependent_cells_(std::move(other.dependent_cells_))
    , referenced_cells_(std::move(other.referenced_cells_)){
    other.Clear();
}

bool Cell::operator==(const Cell& other) const {
    return (IsFormula() == other.IsFormula()) &&
            (this->cell_value_ == other.cell_value_) &&
            (this->text_ == other.text_);
}

Cell& Cell::operator=(Cell &&other) noexcept {
    if(this != &other) {
        Swap(other);
        other.Clear();
    }
    return *this;
}

Cell::~Cell() = default;

void Cell::Clear() {
    text_ = "";
    cell_value_ = nullptr;
    referenced_cells_.clear();
}

bool Cell::IsInitialized() const {
    return cell_value_ || !text_.empty();
}

bool Cell::IsText(std::string text){
    return text[0] != FORMULA_SIGN;
}

void Cell::Set(std::string text) {
    if(IsInitialized()) {
        Clear();
    }
    FormulaInterfacePtr temp_cell_value;
    if(text.size() >= 2) {
        if(!IsText(text)) {
            if(text[1] == ESCAPE_SIGN) {
                temp_cell_value = ParseFormula(text.substr(2));
            } else {
                temp_cell_value = ParseFormula(text.substr(1));
            }
            text_ = FORMULA_SIGN + temp_cell_value->GetExpression();

            Position formula_pos;
            formula_pos = formula_pos.FromString(text_);

            if(formula_pos.IsValid()) {
                Cell* formula_cell = reinterpret_cast<Cell*>(sheet_ref_.GetCell(formula_pos));
                if(formula_cell == nullptr) {
                    sheet_ref_.SetCell(formula_pos, "");
                    formula_cell = reinterpret_cast<Cell*>(sheet_ref_.GetCell(formula_pos));
                }
                referenced_cells_.insert(formula_cell);
                formula_cell->dependent_cells_.insert(this);
            }
        } else {
            text_ = text;
            return;
        }
    }  else {
        text_ = text;
        return;
    }

    const auto temp_ref_cells = temp_cell_value->GetReferencedCells();

    if (!temp_ref_cells.empty()) {
        bool is_found = FindCircularDependency(temp_ref_cells);
        if(is_found) {
            throw CircularDependencyException("circular dependency");
        }
    }
    cell_value_ = std::move(temp_cell_value);

    Refresh();

    if(HasCache()) {
        InvalidateCache();

        for (Cell* dependent : dependent_cells_) {
            dependent->InvalidateCache();
        }
    }
}

Cell::Value Cell::GetValue() const {
    if(HasCache()) {
        return GetCache();
    }
    Value result;
    if(IsFormula()) {
        auto value_result = cell_value_->Evaluate(sheet_ref_);
        if(std::holds_alternative<double>(value_result)) {
            double double_result = std::get<double>(value_result);
            if(std::isinf(double_result)) {
                cache_ = FormulaError::Category::Div0;
                return FormulaError::Category::Div0;
            }
            cache_ = double_result;
            result = double_result;
        } else {
            FormulaError err = std::get<FormulaError>(value_result);
            cache_ = err;
            return err;
        }
    } else {
        if(!text_.empty() && text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }
    return result;
}

Cell::Value Cell::GetCache() const {
    if(IsFormula()) {
        auto value_result = cache_.value();
        if(std::holds_alternative<double>(value_result)) {
            return std::get<double>(value_result);
        }
        return std::get<FormulaError>(value_result);
    } else {
        if(!text_.empty() && text_[0] == ESCAPE_SIGN) {
            return text_.substr(1);
        }
        return text_;
    }
}

std::vector<Position> Cell::GetReferencedCells() const {
    if(cell_value_ != nullptr) {
        return cell_value_->GetReferencedCells();
    }
    return {};
}

std::string Cell::GetText() const {
    /*if(is_formula_) {
        return FORMULA_SIGN + text_;
    }*/
    return text_;
}

void Cell::Refresh() {
    for (Cell* refrence : referenced_cells_) {
        refrence->dependent_cells_.erase(this);
    }
    referenced_cells_.clear();

    for (const auto position : cell_value_->GetReferencedCells()) {

        Cell* ref = reinterpret_cast<Cell*>(sheet_ref_.GetCell(position));

        if (!ref) {
            sheet_ref_.SetCell(position, "");
            ref = reinterpret_cast<Cell*>(sheet_ref_.GetCell(position));
        }

        referenced_cells_.insert(ref);
        ref->dependent_cells_.insert(this);
    }
}

void Cell::CopyDependedAndReferencedCells(Cell& other) {
    dependent_cells_ = other.dependent_cells_;
    referenced_cells_ = other.referenced_cells_;
}

void Cell::InvalidateCache() {
    if(cache_.has_value()) {
        cache_.reset();
    }
}

bool Cell::HasCache() const {
    return cache_.has_value();
}

bool Cell::IsFormula() const {
    if(text_.empty()) {
        return false;
    }
    return text_.at(0) == '=';
}

bool Cell::FindCircularDependency(const std::vector<Position>& ref) const {
    std::set<const Cell*> ref_container;
    std::set<const Cell*> enter_container;
    std::vector<const Cell*> to_enter_collection;

    for (auto position : ref) {
        ref_container.insert(reinterpret_cast<Cell*>(sheet_ref_.GetCell(position)));
    }

    to_enter_collection.push_back(this);

    while (!to_enter_collection.empty()) {

        const Cell* curr_cell = to_enter_collection.back();

        to_enter_collection.pop_back();
        enter_container.insert(curr_cell);

        if (ref_container.find(curr_cell) == ref_container.end()) {

            for (const Cell* dependent : curr_cell->dependent_cells_) {

                if (enter_container.find(dependent) == enter_container.end()) {
                    to_enter_collection.push_back(dependent);
                }
            }

        } else {
            return true;
        }
    }
    return false;
}

void Cell::Swap(Cell& other) {
    if(this != &other) {
        std::swap(this->cell_value_, other.cell_value_);
        std::swap(this->text_, other.text_);
        this->sheet_ref_ = other.sheet_ref_;
        std::swap(this->dependent_cells_, other.referenced_cells_);
        std::swap(this->referenced_cells_, other.referenced_cells_);
    }
}