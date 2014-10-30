#pragma once

#include <cctype>
#include <string>
#include <functional>

#include <gcm/logging/logging.h>

namespace gcm {
namespace parser {

constexpr bool ParserDebug = true;

// FIXME: Segfault
//auto &log = gcm::logging::getLogger("GCM.Parser");

namespace rule {

template<typename I>
using rule_type = std::function<bool(I&,I&)>;

template<typename I>
using callback = std::function<void(I,I)>;

template<typename I>
class rule;

template<typename I>
class rule_base {
public:
	rule_base(): log(gcm::logging::getLogger("GCM.Parser"))
	{}

	using iterator = I;

protected:
	gcm::logging::Logger &log;
};

template<typename I>
class literal: public rule_base<I> {
public:
	literal(const std::string &literal): rule_base<I>(), literal(literal)
	{}

	bool operator()(I &begin, I &end) const {
		I cur = begin;

		if (ParserDebug) {
			DEBUG(this->log) << "Tryint to match literal \"" << literal << "\" against \"" << std::string(begin, end) << "\".";
		}

		for (auto it = literal.begin(); it != literal.end(); ++it, ++cur) {
			if (cur == end) return false;
			if (*cur != *it) return false;
		}

		if (ParserDebug) {
			DEBUG(this->log) << "Matched " << std::string(begin, cur) << ".";
		}

		begin = cur;
		return true;
	}

protected:
	const std::string literal;
};

template<typename I>
class single: public rule_base<I> {
public:
	single(const char ch): rule_base<I>(), ch(ch)
	{}

	bool operator()(I &begin, I &end) const {
		if (ParserDebug) {
			DEBUG(this->log) << "Tryint to match char '" << ch << "' against \"" << std::string(begin, end) << "\".";
		}

		if (begin == end) return false;

		if (*begin == ch) {
			++begin;

			if (ParserDebug) {
				DEBUG(this->log) << "Matched.";
			}
			return true;
		}
		return false;
	}

protected:
	const char ch;
};

template<typename I, int (&predicate)(int)>
class ctype: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) const {
		if (ParserDebug) {
			DEBUG(this->log) << "Trying to match ctype against \"" << std::string(begin, end) << "\".";
		}

		if (begin == end) return false;

		if (predicate(*begin)) {
			++begin;

			if (ParserDebug) {
				DEBUG(this->log) << "Matched.";
			}
			return true;
		}

		return false;
	}

protected:
};

template<typename I>
using digit = ctype<I, std::isdigit>;

template<typename I>
using xdigit = ctype<I, std::isxdigit>;

template<typename I>
using alpha = ctype<I, std::isalpha>;

template<typename I>
using alnum = ctype<I, std::isalnum>;

template<typename I>
using space = ctype<I, std::isspace>;

template<typename I>
using print = ctype<I, std::isprint>;

template<typename I>
class any: public rule_base<I> {
public:
	any(rule_type<I> &&r1, rule_type<I> &&r2): rule_base<I>(), r1(std::move(r1)), r2(std::move(r2))
	{}

	any(const rule_type<I> &r1, const rule_type<I> &r2): rule_base<I>(), r1(r1), r2(r2)
	{}

	bool operator()(I &begin, I &end) const {
		return r1(begin, end) || r2(begin, end);
	}

protected:
	rule_type<I> r1;
	rule_type<I> r2;
};

template<typename I>
class seq: public rule_base<I> {
public:
	seq(rule_type<I> &&r1, rule_type<I> &&r2): rule_base<I>(), r1(std::move(r1)), r2(std::move(r2))
	{}

	seq(const rule_type<I> &r1, const rule_type<I> &r2): rule_base<I>(), r1(r1), r2(r2)
	{}

	bool operator()(I &begin, I& end) const {
		return r1(begin, end) && r2(begin, end);
	}

protected:
	rule_type<I> r1;
	rule_type<I> r2;
};

template<typename I>
class many: public rule_base<I> {
public:
	many(rule_type<I> &&r, int min = 0, int max = -1): rule_base<I>(), r(std::move(r)), min(min), max(max)
	{}

	many(const rule_type<I> &r, int min = 0, int max = -1): rule_base<I>(), r(r), min(min), max(max)
	{}

	bool operator()(I &begin, I &end) const {
		if (ParserDebug) {
			DEBUG(this->log) << "Trying to match at least " << min << " and most " << max << " rules of type " << r.target_type().name() << " on " << std::string(begin, end);
		}

		I begin1 = begin;
		I end1 = end;
		int i = 0;
		while (r(begin1, end1))
		{
			++i;
			if (max >= 0 && i == max) break;
		}

		if (min >= 0 && min < i) return false;

		begin = begin1;
		end = end1;

		return true;
	}

protected:
	rule_type<I> r;
	int min;
	int max;
};

template<typename I>
class all: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) const {
		if (begin == end) return false;
		++begin;
		return true;
	}
};

template<typename I>
class except: public rule_base<I> {
public:
	except(rule_type<I> &&r1, rule_type<I> &&r2): rule_base<I>(), r1(std::move(r1)), r2(r2)
	{}

	bool operator()(I &begin, I&end) const {
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
	rule_type<I> r1;
	rule_type<I> r2;
};


template<typename I>
class rule: public rule_base<I> {
public:
	rule(): rule_base<I>()
	{}

	rule(rule<I> &&other): rule_base<I>(), r(std::move(other.r))
	{}

	rule(const rule<I> &other): rule_base<I>(), r(other.r)
	{}

	rule(rule_type<I> &&other): rule_base<I>(), r(other)
	{}

	rule(const rule_type<I> &other): rule_base<I>(), r(other)
	{}

	rule(const std::string &lit): rule_base<I>(), r(literal<I>(lit))
	{}

	rule(const char ch): rule_base<I>(), r(single<I>(ch))
	{}

	rule &operator=(const rule<I> other) {
		r = other.r;
		return *this;
	}

	rule &operator=(const rule_type<I> other) {
		r = other;
		return *this;
	}

	rule operator|(const rule_type<I> &a) {
		return rule<I>(any<I>(r, a));
	}

	rule operator&(const rule_type<I> &a) const {
		return rule<I>(seq<I>(r, a));
	}

	rule operator*() const {
		return rule<I>(many<I>(r));
	}

	rule operator-(const rule_type<I> &b) {
		return rule<I>(except<I>(std::move(r), std::move(b)));
	}

	rule operator-() const {
		return rule<I>(many<I>(r, 0, 1));
	}

	rule operator+() const {
		return rule<I>(many<I>(r, 1));
	}

	bool operator()(I &begin, I&end) const {
		return r(begin, end);
	}

protected:
	rule_type<I> r;
};

template<typename R, typename CB>
class extractor: public rule_base<typename R::iterator> {
public:
	extractor(R rule, CB callback): rule_base<typename R::iterator>(), rule(rule), call(callback)
	{}

	bool operator()(typename R::iterator &begin, typename R::iterator &end) const {
		if (ParserDebug) {
			DEBUG(this->log) << "Extractor called for rule " << typeid(rule).name() << ".";
		}

		typename R::iterator begin1 = begin;
		typename R::iterator end1 = end;

		if (rule(begin1, end1)) {
			DEBUG(this->log) << "Rule absorbed " << std::string(begin, begin1) << ".";
			call(begin, begin1);
			begin = begin1;
			end = end1;
			return true;
		} else {
			return false;
		}
	}

protected:
	R rule;
	CB call;
};

template<typename R2, typename I = typename R2::iterator>
rule<I> operator&(const char ch, R2 rule2) {
	return std::move(rule<I>(seq<I>(single<I>(ch), rule2)));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator&(R1 rule1, const char ch) {
	return std::move(rule<I>(seq<I>(rule1, single<I>(ch))));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator&(const std::string &lit, R2 rule2) {
	return std::move(rule<I>(seq<I>(literal<I>(lit), rule2)));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator&(R1 rule1, const std::string &lit) {
	return std::move(rule<I>(seq<I>(rule1, literal<I>(lit))));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator|(const char ch, R2 rule2) {
	return std::move(rule<I>(any<I>(single<I>(ch), rule2)));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator|(R1 rule1, const char ch) {
	return std::move(rule<I>(any<I>(rule1, single<I>(ch))));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator|(const std::string &lit, R2 rule2) {
	return std::move(rule<I>(any<I>(literal<I>(lit), rule2)));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator|(R1 rule1, const std::string &lit) {
	return std::move(rule<I>(any<I>(rule1, literal<I>(lit))));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator-(R1 rule1, const char ch) {
	return std::move(rule<I>(except<I>(rule1, single<I>(ch))));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator-(const char ch, R2 rule2) {
	return std::move(rule<I>(except<I>(single<I>(ch), rule2)));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator-(R1 rule1, const std::string &lit) {
	return std::move(rule<I>(except<I>(rule1, literal<I>(lit))));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator-(const std::string &lit, R2 rule2) {
	return std::move(rule<I>(except<I>(literal<I>(lit), rule2)));
}

template<typename I, typename CB>
rule<I> operator>>(rule<I> &&r, CB &&cb) {
	return std::move(rule<I>(extractor<rule<I>, CB>(std::move(r), cb)));
}

}

template<typename I>
class syntax {
public:
	using r = rule::rule<I>;

	syntax():
		digit(rule::digit<I>()),
		alpha(rule::alpha<I>()),
		alnum(rule::alnum<I>()),
		space(rule::space<I>()),
		print(rule::print<I>()),
		xdigit(rule::xdigit<I>()),
		any(rule::all<I>())
	{}

	r literal(const std::string &lit) {
		return r(rule::literal<I>(lit));
	}

	r ch(const char ch) {
		return r(rule::single<I>(ch));
	}

	r many(r &&rule, int min = 0, int max = -1) {
		return r(rule::many<I>(std::move(rule), min, max));
	}

	r either(r &&rule1, r &&rule2) {
		return r(rule::any<I>(std::move(rule1), std::move(rule2)));
	}

	r after(r &&rule1, r &&rule2) {
		return r(rule::seq<I>(std::move(rule1), std::move(rule2)));
	}

	r except(r &&rule1, r &&rule2) {
		return r(rule::except<I>(std::move(rule1), std::move(rule2)));
	}

	const r digit;
	const r alpha;
	const r alnum;
	const r space;
	const r print;
	const r xdigit;
	const r any;
};

template<typename I>
bool parse(I begin, I end, rule::rule<I> rules) {
	return rules(begin, end);
}

}
}
