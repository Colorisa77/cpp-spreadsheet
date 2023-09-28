#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {}
    catch (...) {
        throw FormulaException("formula is syntactically incorrect");
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            std::function<double(Position)> args = [&sheet](const Position pos)-> double {
                if(!pos.IsValid()) {
                    throw FormulaError::Category::Ref;
                }

                const auto* cell_ptr = sheet.GetCell(pos);

                if(cell_ptr == nullptr) {
                    return 0.0;
                }

                auto value = cell_ptr->GetValue();

                if(std::holds_alternative<double>(value)) {
                    return std::get<double>(value);
                } else if(std::holds_alternative<std::string>(value)) {
                    std::string str_value = std::get<std::string>(value);

                    if(str_value.empty()) {
                        return 0.0;
                    }

                    bool has_char = std::any_of(str_value.begin(), str_value.end(), [](char ch){
                        return !isdigit(ch);
                    });

                    if(isdigit(str_value[0]) && has_char) {
                        throw FormulaError(FormulaError::Category::Value);
                    }

                    std::istringstream stream(str_value);

                    double result = 0.0;

                    if(!stream.eof() && stream >> result) {
                        return result;
                    } else {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                } else {
                    throw FormulaError(std::get<FormulaError>(cell_ptr->GetValue()));
                }
            };
            return ast_.Execute(args);
        } catch(FormulaError& err) {
            return err;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream ss;
        ast_.PrintFormula(ss);
        return ss.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::set<Position> cells;
        for (const auto& cell : ast_.GetCells()) {
            if (cell.IsValid()) {

                cells.insert(cell);
            }
        }
        return {cells.begin(), cells.end()};
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}