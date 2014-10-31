#pragma once

#include <cctype>
#include <string>
#include <functional>
#include <utility>
#include <type_traits>

#define PARSER_DEBUG

#ifdef PARSER_DEBUG
#   include <gcm/logging/logging.h>
#endif

namespace gcm {
namespace parser {

class rule_base {
public:
#ifdef PARSER_DEBUG
    rule_base(): log(gcm::logging::getLogger("GCM.Parser"))
    {}

    gcm::logging::Logger &log;
#else
    rule_base() {}
#endif
};

template<typename Rule1, typename Rule2>
class and_rule: public rule_base {
public:
    and_rule(Rule1 &&r1, Rule2 &&r2): rule_base(), r1(r1), r2(r2)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif
        return r1(begin, end) && r2(begin, end);
    }

    operator std::string() {
        return "a(" + (std::string)r1 + " & " + (std::string)r2 + ")";
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

template<typename Rule1, typename Rule2>
class or_rule: public rule_base {
public:
    or_rule(Rule1 &&r1, Rule2 &&r2): rule_base(), r1(r1), r2(r2)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        return r1(begin, end) || r2(begin, end);
    }

    operator std::string() {
        return "o(" + (std::string)r1 + " | " + (std::string)r2 + ")";
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

class any_rule: public rule_base {
public:
    template<typename I>
    bool operator()(I &begin, I&end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (begin == end) {
            return false;
        }

        ++begin;
        return true;
    }

    operator std::string() {
        return "any";
    }
};

class char_rule: public rule_base {
public:
    char_rule(const char ch): rule_base(), ch(ch)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (begin == end) {
            return false;
        }

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Got input '" << *begin << "', matching against " << ch;
#endif

        if (*begin == ch) {
            ++begin;

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "OK";
#endif

            return true;
        }
        return false;
    }

    operator std::string() {
        return std::string("'") + ch + "'";
    }

protected:
    const char ch;
};

using call_type = int(int);

template<call_type call>
class ctype_rule: public rule_base {
public:
    template<typename I>
    bool operator()(I &begin, I&end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (begin == end) return false;

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Got input '" << *begin << "', matching against ctype.";
#endif

        if (call(*begin) > 0) {
            ++begin;

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "OK";
#endif

            return true;
        } else {
            return false;
        }
    }

    operator std::string() {
        return std::string("ctype(") + typeid(call).name() + ")";
    }
};

class literal_rule: public rule_base {
public:
    literal_rule(const std::string &lit): rule_base(), lit(lit)
    {}

    literal_rule(std::string &&lit): rule_base(), lit(std::move(lit))
    {}

    template<typename I>
    bool operator()(I &begin, I&end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

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

    operator std::string() {
        return "\"" + lit + "\"";
    }

protected:
    const std::string lit;
};

template<typename Rule1, typename Rule2>
class exception_rule: public rule_base {
public:
    exception_rule(Rule1 &&r1, Rule2 &&r2): rule_base(), r1(r1), r2(r2)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        I begin1 = begin;
        I begin2 = begin;

        if (r1(begin1, end) && !r2(begin2, end)) {
            begin = begin1;
            return true;
        } else {
            return false;
        }
    }

    operator std::string() {
        return "e(" + (std::string)r1 + " - " + (std::string)r2 + ")";
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

template<typename Rule, int Min=0, int Max=-1>
class iteration_rule: public rule_base {
public:
    iteration_rule(Rule &&rule): rule_base(), rule(rule)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        int count = 0;

        I begin1 = begin;

        if (Max >= 0) {
            while (count < Max && rule(begin1, end)) {
                ++count;
            }
        } else {
            while (rule(begin1, end)) {
                ++count;
            }
        }

        if (count >= Min) {
            begin = begin1;
            return true;
        }

        return false;
    }

    operator std::string() {
        return "*(" + (std::string)rule + ")";
    }

protected:
    Rule rule;
};

template<typename Rule, typename Extractor>
class extractor_rule: public rule_base {
public:
    extractor_rule(Rule &&rule, Extractor &&extractor): rule_base(), rule(rule), extractor(extractor)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        I begin1 = begin;
        if (rule(begin1, end)) {
            extractor(begin, begin1);
            return true;
        }

        return false;
    }

    operator std::string() {
        return "(" + (std::string)rule + ") >> extract";
    }

protected:
    Rule rule;
    Extractor extractor;
};

template<typename I>
class rule: public rule_base {
public:
    // Default constructor, no action in this rule.
    rule(): rule_base(), func(nullptr)
    {}

    template<typename Rule>
    rule(Rule &&r): rule_base(), func(std::forward<Rule>(r))
    {}

    // Assignment operator
    rule<I> &operator=(const rule<I> &) = default;

    template<typename Rule>
    rule<I> &operator=(Rule &&r) {
        func = std::move(r);
        return *this;
    }

    // When calling rule, evaluate the function that actualy does the matching.
    bool operator()(I &begin, I &end) {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (func != nullptr) {
            return func(begin, end);
        } else {
            return true;
        }
    }

    operator std::string() {
        return "generic_rule";
    }

protected:
    std::function<bool(I&,I&)> func;
};

template<typename T>
struct is_rule {
    static constexpr bool value = std::is_base_of<rule_base, typename std::remove_reference<T>::type>::value;
};

template<typename Rule1, typename Rule2>
or_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator|(Rule1 &&r1, Rule2 &&r2) {
    return or_rule<Rule1, Rule2>(std::forward<Rule1>(r1), std::forward<Rule2>(r2));
}

template<typename Rule1>
or_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    char_rule
> operator|(Rule1 &&r1, const char ch) {
    return or_rule<Rule1, char_rule>(std::forward<Rule1>(r1), char_rule(ch));
}

template<typename Rule2>
or_rule<
    char_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator|(const char ch, Rule2 &&r2) {
    return or_rule<char_rule, Rule2>(char_rule(ch), std::forward<Rule2>(r2));
}

template<typename Rule1>
and_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    char_rule
> operator&(Rule1 &&r1, char ch) {
    return and_rule<Rule1, char_rule>(std::forward<Rule1>(r1), char_rule(ch));
}

template<typename Rule2>
and_rule<
    char_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator&(char ch, Rule2 &&r2) {
    return and_rule<char_rule, Rule2>(char_rule(ch), std::forward<Rule2>(r2));
}

template<typename Rule1, typename Rule2>
and_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator&(Rule1 &&r1, Rule2 &&r2) {
    return and_rule<Rule1, Rule2>(std::forward<Rule1>(r1), std::forward<Rule2>(r2));
}

template<typename Rule>
iteration_rule<
    std::enable_if_t<is_rule<Rule>::value, Rule>,
    0, -1
> operator*(Rule &&rule) {
    return iteration_rule<Rule, 0, -1>(std::forward<Rule>(rule));
}

template<typename Rule>
iteration_rule<
    std::enable_if_t<is_rule<Rule>::value, Rule>,
    1, -1
> operator+(Rule &&rule) {
    return iteration_rule<Rule, 1, -1>(std::forward<Rule>(rule));
}

template<typename Rule>
iteration_rule<
    std::enable_if_t<is_rule<Rule>::value, Rule>,
    0, 1
> operator-(Rule &&rule) {
    return iteration_rule<Rule, 0, 1>(std::forward<Rule>(rule));
}

template<typename Rule1, typename Rule2>
exception_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator-(Rule1 &&r1, Rule2 &&r2) {
    return exception_rule<Rule1, Rule2>(std::forward<Rule1>(r1), std::forward<Rule2>(r2));
}

template<typename Rule1>
exception_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    char_rule
> operator-(Rule1 &&r1, const char ch) {
    return exception_rule<Rule1, char_rule>(std::forward<Rule1>(r1), char_rule(ch));
}

template<typename Rule2>
exception_rule<
    char_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator-(const char ch, Rule2 &&r2) {
    return exception_rule<char_rule, Rule2>(char_rule(ch), std::forward<Rule2>(r2));
}

template<typename Rule, typename Extractor>
extractor_rule<
    std::enable_if<is_rule<Rule>::value, Rule>,
    Extractor
> operator>>(Rule &&rule, Extractor &&extractor) {
    return extractor_rule<Rule, Extractor>(std::forward<Rule>(rule), std::forward<Extractor>(extractor));
}

using space = ctype_rule<std::isspace>;
using alpha = ctype_rule<std::isalpha>;
using alnum = ctype_rule<std::isalnum>;
using digit = ctype_rule<std::isdigit>;
using xdigit = ctype_rule<std::isxdigit>;
using print = ctype_rule<std::isprint>;

} // namespace parser
} // namespace gcm