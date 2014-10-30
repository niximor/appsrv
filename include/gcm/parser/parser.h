#pragma once

#include <cctype>
#include <string>
#include <functional>
#include <utility>

#include <gcm/logging/logging.h>

namespace gcm {
namespace parser {

constexpr bool ParserDebug = true;

template<typename Rule1, typename Rule2>
class and_rule {
public:
    and_rule(Rule1 &&r1, Rule2 &&r2): r1(r1), r2(r2)
    {}
    and_rule(const and_rule<Rule1, Rule2> &) = default;
    and_rule(and_rule<Rule1, Rule2> &&) = default;

    and_rule<Rule1, Rule2> &operator=(const and_rule<Rule1, Rule2> &) = default;

    template<typename I>
    bool operator()(I &begin, I &end) {
        return r1(begin, end) && r2(begin, end);
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

template<typename Rule1, typename Rule2>
class or_rule {
public:
    or_rule(Rule1 &&r1, Rule2 &&r2): r1(r1), r2(r2)
    {}
    or_rule(const or_rule<Rule1, Rule2> &) = default;
    or_rule(or_rule<Rule1, Rule2> &&) = default;

    or_rule<Rule1, Rule2> &operator=(const or_rule<Rule1, Rule2> &) = default;

    template<typename I>
    bool operator()(I &begin, I &end) {
        return r1(begin, end) || r2(begin, end);
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

class any_rule {
public:
    template<typename I>
    bool operator()(I &begin, I&end) {
        if (begin == end) {
            return false;
        }

        ++begin;
        return true;
    }
};

class char_rule {
public:
    char_rule(const char ch): ch(ch)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) {
        if (begin == end) {
            return false;
        }

        if (*begin == ch) {
            ++begin;
            return true;
        }
        return false;
    }

protected:
    const char ch;
};

template<typename T>
class ctype_rule {
public:
    ctype_rule(T *func): func(func)
    {}

    ctype_rule(const ctype_rule<T> &) = default;
    ctype_rule(ctype_rule<T> &&) = default;

    ctype_rule<T> &operator=(const ctype_rule<T> &) = default;
    ctype_rule<T> &operator=(T t) {
        func = t;
        return *this;
    }

    template<typename I>
    bool operator()(I &begin, I&end) {
        if (begin == end) return false;

        if (func(*begin) > 0) {
            ++begin;
            return true;
        } else {
            return false;
        }
    }

protected:
    T *func;
};

class literal_rule {
public:
    literal_rule(const std::string &lit): lit(lit)
    {}

    literal_rule(std::string &&lit): lit(std::move(lit))
    {}

    literal_rule(const literal_rule &) = default;
    literal_rule(literal_rule &&) = default;

    literal_rule &operator=(const literal_rule &) = default;

    template<typename I>
    bool operator()(I &begin, I&end) {
        for (auto it = lit.begin(); it != lit.end(); ++it) {
            if (begin == end) {
                return false;
            }

            if (*begin != *it) {
                return false;
            }
        }

        return true;
    }

protected:
    const std::string lit;
};

template<typename Rule1, typename Rule2>
class exception_rule {
public:
    exception_rule(Rule1 &&r1, Rule2 &&r2): r1(r1), r2(r2)
    {}

    exception_rule(const exception_rule<Rule1, Rule2> &) = default;
    exception_rule(exception_rule<Rule1, Rule2> &&) = default;

    exception_rule<Rule1, Rule2> &operator=(const exception_rule<Rule1, Rule2> &) = default;

    template<typename I>
    bool operator()(I &begin, I &end) {
        I begin1 = begin;
        I begin2 = begin;

        if (r1(begin1, end) && !r2(begin2, end)) {
            begin = begin1;
            return true;
        } else {
            return false;
        }
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

template<typename Rule, int Min=0, int Max=-1>
class iteration_rule {
public:
    iteration_rule(Rule &&rule): rule(rule)
    {}

    iteration_rule(const iteration_rule<Rule, Min, Max> &) = default;
    iteration_rule(iteration_rule<Rule, Min, Max> &&) = default;

    iteration_rule<Rule, Min, Max> &operator=(const iteration_rule<Rule, Min, Max> &) = default;

    template<typename I>
    bool operator()(I &begin, I &end) {
        int count = 0;

        I begin1 = begin;
        while (count < Max && rule(begin1, end))
        {}

        if (count >= Min) {
            begin = begin1;
            return true;
        }

        return false;
    }

protected:
    Rule rule;
};

template<typename Rule, typename Extractor>
class extractor_rule {
public:
    extractor_rule(Rule &&rule, Extractor &&extractor): rule(rule), extractor(extractor)
    {}

    extractor_rule(const extractor_rule<Rule, Extractor> &) = default;
    extractor_rule(extractor_rule<Rule, Extractor> &&) = default;

    extractor_rule<Rule, Extractor> &operator=(const extractor_rule<Rule, Extractor> &) = default;

    template<typename I>
    bool operator()(I &begin, I &end) {
        I begin1 = begin;
        if (rule(begin1, end)) {
            extractor(begin, begin1);
            return true;
        }

        return false;
    }

protected:
    Rule rule;
    Extractor extractor;
};

template<typename I>
class rule {
public:
    // Default constructor, no action in this rule.
    rule(): func(nullptr)
    {}

    rule(const rule &) = default;
    rule(rule&&) = default;

    template<class Rule>
    rule(Rule &&r): func(std::forward<Rule>(r))
    {}

    // Assignment operator
    rule<I> &operator=(const rule<I> &) = default;

    template<class Rule>
    rule<I> &operator=(Rule &&r) {
        func = std::move(r);
        return *this;
    }

    // When calling rule, evaluate the function that actualy does the matching.
    bool operator()(I &begin, I &end) {
        if (func != nullptr) {
            return func(begin, end);
        } else {
            return true;
        }
    }

protected:
    std::function<bool(I&,I&)> func;
};

template<typename Rule1, typename Rule2>
or_rule<Rule1, Rule2> operator|(Rule1 &&r1, Rule2 &&r2) {
    return or_rule<Rule1, Rule2>(std::forward<Rule1>(r1), std::forward<Rule2>(r2));
}

template<typename Rule1>
and_rule<Rule1, char_rule> operator&(Rule1 &&r1, char ch) {
    return and_rule<Rule1, char_rule>(std::forward<Rule1>(r1), char_rule(ch));
}

template<typename Rule2>
and_rule<char_rule, Rule2> operator&(char ch, Rule2 &&r2) {
    return and_rule<char_rule, Rule2>(char_rule(ch), std::forward<Rule2>(r2));
}

template<typename Rule1, typename Rule2>
and_rule<Rule1, Rule2> operator&(Rule1 &&r1, Rule2 &&r2) {
    return and_rule<Rule1, Rule2>(std::forward<Rule1>(r1), std::forward<Rule2>(r2));
}

template<typename Rule, int Min=0, int Max=-1>
iteration_rule<Rule, Min, Max> operator*(Rule &&rule) {
    return iteration_rule<Rule, Min, Max>(std::forward<Rule>(rule));
}

template<typename Rule>
iteration_rule<Rule, 1, -1> operator+(Rule &&rule) {
    return iteration_rule<Rule, 1, -1>(std::forward<Rule>(rule));
}

template<typename Rule>
iteration_rule<Rule, 0, 1> operator-(Rule &&rule) {
    return iteration_rule<Rule, 0, 1>(std::forward<Rule>(rule));
}

template<typename Rule1, typename Rule2>
exception_rule<Rule1, Rule2> operator-(Rule1 &&r1, Rule2 &&r2) {
    return exception_rule<Rule1, Rule2>(std::forward<Rule1>(r1), std::forward<Rule2>(r2));
}

template<typename Rule, typename Extractor>
extractor_rule<Rule, Extractor> operator>>(Rule &&rule, Extractor &&extractor) {
    return extractor_rule<Rule, Extractor>(std::forward<Rule>(rule), std::forward<Extractor>(extractor));
}

template<typename I>
class syntax {
public:
    syntax():
        space(ctype_rule<int(int)>(std::isspace)),
        alpha(ctype_rule<int(int)>(std::isalpha)),
        alnum(ctype_rule<int(int)>(std::isalnum)),
        digit(ctype_rule<int(int)>(std::isdigit)),
        xdigit(ctype_rule<int(int)>(std::isxdigit)),
        print(ctype_rule<int(int)>(std::isprint)),
        any(any_rule())
    {}

    char_rule ch(const char ch) {
        return char_rule(ch);
    }

    rule<I> space;
    rule<I> alpha;
    rule<I> alnum;
    rule<I> digit;
    rule<I> xdigit;
    rule<I> print;

    any_rule any;
};

} // namespace parser
} // namespace gcm