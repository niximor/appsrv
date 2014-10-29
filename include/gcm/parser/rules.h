#pragma once

#include <cctype>
#include <string>
#include <functional>

namespace gcm {
namespace parser {

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
	using iterator = I;
};

template<typename I>
class literal: public rule_base<I> {
public:
	literal(const std::string &literal): literal(literal)
	{}

	bool operator()(I &begin, I &end) {
		I cur = begin;
		for (auto it = literal.begin(); it != literal.end(); ++it, ++cur) {
			if (cur == end) return false;
			if (*cur != *it) return false;
		}
		begin = cur;
	}

protected:
	const std::string literal;
};

template<typename I>
class single: public rule_base<I> {
public:
	single(const char ch): ch(ch)
	{}

	bool operator()(I &begin, I &end) {
		if (begin == end) return false;

		if (*begin == ch) {
			++begin;
			return true;
		}
		return false;
	}

protected:
	const char ch;
};

template<typename I>
class digit: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) {
		if (begin == end) return false;

		if (std::isdigit(*begin)) {
			++begin;
			return true;
		}

		return false;
	}
};

template<typename I>
class alpha: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) {
		if (begin == end) return false;

		if (std::isalpha(*begin)) {
			++begin;
			return true;
		}
		
		return false;
	}
};

template<typename I>
class alnum: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) {
		if (begin == end) return false;

		if (std::isalnum(*begin)) {
			++begin;
			return true;
		}
		
		return false;
	}
};

template<typename I>
class space: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) {
		if (begin == end) return false;

		if (std::isspace(*begin)) {
			++begin;
			return true;
		}
		
		return false;
	}
};

template<typename I>
class print: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) {
		if (begin == end) return false;

		if (std::isprint(*begin)) {
			++begin;
			return true;
		}
		
		return false;
	}
};

template<typename I>
class xdigit: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) {
		if (begin == end) return false;

		if (std::isxdigit(*begin)) {
			++begin;
			return true;
		}
		
		return false;
	}
};

template<typename I>
class any: public rule_base<I> {
public:
	any(rule_type<I> &&r1, rule_type<I> &&r2): r1(r1), r2(r2)
	{}

	bool operator()(I &begin, I &end) {
		return r1(begin, end) || r2(begin, end);
	}

protected:
	rule_type<I> r1;
	rule_type<I> r2;
};

template<typename I>
class seq: public rule_base<I> {
public:
	seq(rule_type<I> &&r1, rule_type<I> &&r2): r1(r1), r2(r2)
	{}

	bool operator()(I &begin, I& end) {
		return r1(begin, end) && r2(begin, end);
	}

protected:
	rule_type<I> r1;
	rule_type<I> r2;
};

template<typename I>
class many: public rule_base<I> {
public:
	many(rule_type<I> &&r): r(r)
	{}

	bool operator()(I &begin, I &end) {
		while (r(begin, end))
		{}
		return true;
	}

protected:
	rule_type<I> r;
};

template<typename I>
class all: public rule_base<I> {
public:
	bool operator()(I &begin, I&end) {
		if (begin == end) return false;
		++begin;
		return true;
	}
};

template<typename I>
class except: public rule_base<I> {
public:
	except(rule_type<I> &&r1, rule_type<I> &&r2): r1(r1), r2(r2)
	{}

	bool operator()(I &begin, I&end) {
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
	rule(rule<I> &&other): r(std::move(other.r))
	{}

	rule(const rule<I> &other): r(other.r)
	{}

	rule(rule_type<I> &&other): r(other)
	{}

	rule(const rule_type<I> &other): r(other)
	{}

	rule(const std::string &lit): r(literal<I>(lit))
	{}

	rule(const char ch): r(single<I>(ch))
	{}

	rule &operator=(const rule<I> other) {
		r = other.r;
		return *this;
	}

	rule &operator=(const rule_type<I> other) {
		r = other;
		return *this;
	}

	rule &operator|(const rule_type<I> &a) {
		r = any<I>(r, a);
		return *this;
	}

	rule &operator&(const rule_type<I> &a) {
		r = seq<I>(r, a);
		return *this;
	}

	rule &operator*() {
		r = many<I>(std::move(r));
		return *this;
	}

	rule &operator-(const rule_type<I> &b) {
		r = except<I>(std::move(r), std::move(b));
		return *this;
	}

	bool operator()(I &begin, I&end) {
		return r(begin, end);
	}

protected:
	rule_type<I> r;
	callback<I> c;
};

template<typename R, typename CB>
class extractor: public rule_base<typename R::iterator> {
public:
	extractor(R rule, CB callback): rule(rule), call(callback)
	{}

	bool operator()(typename R::iterator &begin, typename R::iterator &end) {
		typename R::iterator begin1 = begin;
		typename R::iterator end1 = end;

		if (rule(begin1, end1)) {
			call(begin, end);
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
rule<I> operator&(char ch, R2 rule2) {
	return std::move(rule<I>(seq<I>(single<I>(ch), rule2)));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator&(R1 rule1, char ch) {
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
rule<I> operator|(char ch, R2 rule2) {
	return std::move(rule<I>(any<I>(single<I>(ch), rule2)));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator|(R1 rule1, char ch) {
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
rule<I> operator-(R1 rule1, char ch) {
	return std::move(rule<I>(except<I>(rule1, single<I>(ch))));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator-(char ch, R2 rule2) {
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
	return std::move(rule<I>(extractor<rule<I>, CB>(r, cb)));
}

}

template<typename I>
bool parse(I begin, I end, rule::rule<I> rules) {
	return rules(begin, end);
}

}
}
