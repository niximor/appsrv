#pragma once

namespace gcm {
namespace parser {

namespace rule {

template<typename I>
using rule_type = std::function<bool(I&,I&)>;

template<typename I>
class rule;

template<typename I>
class rule_base {
public:
	using iterator = I;

	operator rule<I>() {
		return rule<I>(*this);
	}
};

template<typename I>
class literal: public rule_base<I> {
public:
	literal(const std::string &literal): literal(literal)
	{}

	bool operator()(I &begin, I &end) {
		I cur = begin;
		for (auto it = literal.begin(); it != literal.end(); ++it, ++cur) {
			std::cout << "Trying to match " << *cur << " against " << *it << std::endl;
			if (cur == end) return false;
			if (*cur != *it) return false;
			std::cout << "Success." << std::endl;
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
		std::cout << "Trying to match " << *begin << " against " << ch << std::endl;
		if (*begin == ch) {
			++begin;
			std::cout << "Success." << std::endl;
			return true;
		}
		return false;
	}


	operator rule<I>() {
		return rule<I>(*this);
	}

protected:
	const char ch;
};

template<typename I>
class any: public rule_base<I> {
public:
	any(rule_type<I> r1, rule_type<I> r2): r1(r1), r2(r2)
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
	seq(rule_type<I> r1, rule_type<I> r2): r1(r1), r2(r2)
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
	many(rule_type<I> r): r(r)
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
class rule: public rule_base<I> {
public:
	rule(rule<I> &&r): r(std::move(r))
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

	bool operator()(I &begin, I&end) {
		return r(begin, end);
	}

protected:
	rule_type<I> r;
};

template<typename R2, typename I = typename R2::iterator>
rule<I> operator&(char ch, R2 rule2) {
	return rule<I>(seq<I>(single<I>(ch), rule2));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator&(R1 rule1, char ch) {
	return rule<I>(seq<I>(rule1, single<I>(ch)));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator&(const std::string &lit, R2 rule2) {
	return rule<I>(seq<I>(literal<I>(lit), rule2));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator&(R1 rule1, const std::string &lit) {
	return rule<I>(seq<I>(rule1, literal<I>(lit)));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator|(char ch, R2 rule2) {
	return rule<I>(any<I>(single<I>(ch), rule2));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator|(R1 rule1, char ch) {
	return rule<I>(any<I>(rule1, single<I>(ch)));
}

template<typename R2, typename I = typename R2::iterator>
rule<I> operator|(const std::string &lit, R2 rule2) {
	return rule<I>(any<I>(literal<I>(lit), rule2));
}

template<typename R1, typename I = typename R1::iterator>
rule<I> operator|(R1 rule1, const std::string &lit) {
	return rule<I>(any<I>(rule1, literal<I>(lit)));
}


}

template<typename I>
bool parse(I begin, I end, rule::rule<I> rules) {
	return rules(begin, end);
}

}
}
