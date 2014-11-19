#pragma once

namespace gcm {
namespace json {
namespace validator {
namespace detail {

template <typename... Args>
class ParamDefinitions_t {
public:
    ParamDefinitions_t(Args... params): params(std::forward_as_tuple(params...)), num_args(sizeof...(params))
    {}

    bool validate(json::Array &value) {
        auto res = call_func(value.begin(), value.end(), std::index_sequence_for<Args...>{});
        if (!res) {
            throw diag;
        }
        return true;
    }

protected:
    std::tuple<Args...> params;
    std::size_t num_args;
    std::size_t actual_args;
    Diagnostics diag;

    template<typename I, typename Head, typename... Tail>
    bool int_validator(I begin, I end, Head head, Tail... tail) {
        // Argument missing (no more items in array)
        if (begin == end) {
            diag.add_problem(head.get_item(), ProblemCode::MustBePresent, "Missing argument for function.");

            // Validate rest to generate missing arguments for all other arguments.
            int_validator(begin, end, tail...);

            return false;

        } else {
            // Try to validate this argument
            return
                head(diag, *begin)
                && int_validator(begin + 1, end, tail...);
        }
    }

    template<typename I>
    bool int_validator(I begin, I end) {
        if (begin != end) {
            diag.add_problem("", ProblemCode::UnexpectedParam,
                "Function accepts " + std::to_string(num_args) + " arguments, " + std::to_string(actual_args) + " given.");
            return false;
        } else {
            return true;
        }
    }

    template<typename Iterator, std::size_t ...I>
    bool call_func(Iterator begin, Iterator end, std::index_sequence<I...>) {
        actual_args = std::distance(begin, end);
        return int_validator(begin, end, std::get<I>(params)...);
    }
};

} // namespace detail
} // namespace validate
} // namespace json
} // namespace gcm
