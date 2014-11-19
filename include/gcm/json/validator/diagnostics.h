#pragma once

namespace gcm {
namespace json {
namespace validator {

enum class ProblemCode {
    WrongType, CannotBeNull, MustBePresent, UnexpectedParam
};

class Problem {
public:
    Problem(const std::string &item, ProblemCode code, const std::string &description):
        item(item),
        code(code),
        description(description)
    {}

protected:
    const std::string item;
    ProblemCode code;
    const std::string description;
};

class Diagnostics: public std::exception {
public:
    void add_problem(Problem &&problem) {
        problems.emplace_back(std::forward<Problem>(problem));
    }

    void add_problem(const std::string &item, ProblemCode code, const std::string &description) {
        problems.emplace_back(
            item,
            code,
            description
        );
    }

protected:
    std::vector<Problem> problems;
};

} // namespace gcm
} // namespace json
} // namespace validator
