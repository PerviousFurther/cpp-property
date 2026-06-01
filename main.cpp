#include <vector>
#include <unordered_map>
#include <string>
#include <concepts>
#include <functional>
#include <any>

template<typename...Ts>
struct signal {
	constexpr void emit(Ts...args) {
		for (auto c : ctx_) c(args...);
	}
	constexpr void operator()(Ts...args) {
		this->emit(static_cast<Ts&&>(args)...);
	}

	template<typename T>
	constexpr void connect(T other)
		requires(::std::invocable<T, Ts...>) {
		ctx_.push_back(::std::function<void(Ts...)>(::std::move(other)));
	}
	template<typename T>
	constexpr void connect(T& other, void(T::*invoker)(Ts...)) {
		ctx_.push_back(::std::function<void(Ts...)>([&other, invoker](Ts...args) {
			(other.*invoker)(args...);
		}));
	}

private:
	::std::vector<::std::function<void(Ts...)>> ctx_;
};

template<typename T, auto...vs>
struct property { static_assert(::std::is_void_v<T> ? 0 : 0, "Not valid property."); };

template<typename M, typename T>
struct member_property {
	T(M::* getter)();
	void(M::* setter)(T);
	signal<>(M::* signal);
};
template<typename M, typename T>
member_property(T(M::* getter)(), void(M::* setter)(T), signal<>(M::*)) -> member_property<M, T>;
template<typename M, typename T>
member_property(T(M::* getter)(), ::std::nullptr_t, signal<>(M::*)) -> member_property<M, T>;
template<typename M, typename T>
member_property(::std::nullptr_t, void(M::* setter)(T), signal<>(M::*)) -> member_property<M, T>;
template<typename M, typename T>
member_property(T(M::* getter)(), void(M::* setter)(T), ::std::nullptr_t) -> member_property<M, T>;
template<typename M, typename T>
member_property(T(M::* getter)(), ::std::nullptr_t, ::std::nullptr_t) -> member_property<M, T>;
template<typename M, typename T>
member_property(::std::nullptr_t, void(M::* setter)(T), ::std::nullptr_t) -> member_property<M, T>;

template<typename M, typename T, member_property<M, T> field>
struct property<T, field> {
	constexpr property(M* ptr) : ptr_{ptr} {}
	
	constexpr property& operator=(property&& other) {
		if constexpr (field.setter != nullptr) {
			(ptr_->*field.setter)(::std::move((other.ptr_->*field.getter)()));
		}
		if constexpr (field.signal != nullptr) {
			(ptr_->*field.signal)();
		}
		return *this;
	}
	constexpr property& operator=(property const& other) {
		if constexpr (field.setter != nullptr) {
			(ptr_->*field.setter)((other.ptr_->*field.getter)());
		}
		if constexpr (field.signal != nullptr) {
			(ptr_->*field.signal)();
		}
		return *this;
	}

	constexpr void set(T value) 
		requires(field.setter != nullptr) {
		(ptr_->*field.setter)(::std::move(value));
		if constexpr (field.signal != nullptr) {
			(ptr_->*field.signal)();
		}
	}
	constexpr operator T() requires(field.getter != nullptr) {
		return (ptr_->*field.getter)();
	}

	template<typename C>
	constexpr void bind(C value) requires(field.signal != nullptr) { (ptr_->*field.signal).connect(::std::move(value)); }
	template<typename O, typename C>
	constexpr void bind(O& value, C member) requires(field.signal != nullptr) { (ptr_->*field.signal).connect(value, member); }

private:
	M* ptr_;
};
template<typename M, typename T, T(M::*read)()>
struct property<T, read> : property<T, member_property{read, nullptr, nullptr}> {};
template<typename M, typename T, T(M::* read)()>
struct property<T, read, nullptr> : property<T, member_property{ read, nullptr, nullptr }> {};
template<typename M, typename T, void(M::*setter)(T)>
struct property<T, nullptr, setter> : property<T, member_property{nullptr, setter, nullptr}> {};
template<typename M, typename T, T(M::*getter)(), void(M::* setter)(T)>
struct property<T, getter, setter> : property<T, member_property{getter, setter, nullptr}> {};
template<typename M, typename T, void(M::* setter)(T), auto signal>
struct property<T, nullptr, setter, signal> : property<T, member_property{nullptr, setter, signal}> {};
template<typename M, typename T, T(M::* getter)(), auto signal>
struct property<T, getter, nullptr, signal> : property<T, member_property{getter, nullptr, signal}> {};
template<typename M, typename T, T(M::* getter)(), void(M::* setter)(T), auto signal>
struct property<T, getter, setter, signal> : property<T, member_property{getter, setter, signal}> {};



#include <iostream>

class uploader {
	int money_ = 0;

	int read() {
		::std::cout << "查看Up主钱包" << money_ << "\n";
		return money_;
	}
	void write(int value) { ::std::cout << "受到投币" << (money_ += value) << "\n"; }

public:
	uploader() 
		: read_no_tip(this)
		, read_and_tip(this)
		, tip(this)
		, tip_and_share(this)
		, read_and_share(this)
		, read_tip_and_share(this)
	{}

	signal<> notify;
	property<int, &uploader::read, nullptr> read_no_tip;
	property<int, &uploader::read, &uploader::write> read_and_tip;
	property<int, nullptr, &uploader::write> tip;
	property<int, nullptr, &uploader::write, &uploader::notify> tip_and_share;
	property<int, &uploader::read, nullptr, &uploader::notify> read_and_share;
	property<int, &uploader::read, &uploader::write, &uploader::notify> read_tip_and_share;
};


struct elite {
	void commit() {
		::std::cout << "点赞+关注+收藏" << "\n";
	}
};
struct studier {
	void commit() {
		::std::cout << "UP主你用的什么编译器啊？" << "\n";
	}
};
struct passerby {
	void commit() {
		::std::cout << "没搞懂有什么用" << "\n";
	}
};

int main() {
	system("chcp 65001");
	uploader up;
	elite elite;
	studier studier;
	passerby passerby;
	up.read_and_share.bind([]() { ::std::cout << "难绷，怎么还在玩TMP" << "\n"; });
	up.read_tip_and_share.bind(elite, &elite::commit);
	up.read_tip_and_share.bind(studier, &studier::commit);
	up.read_and_share.bind(passerby, &passerby::commit);
	
	up.tip.set(2); // not share.
	int c = up.read_no_tip;

	up.tip_and_share.set(2); // share.
	c = up.read_no_tip;
}
