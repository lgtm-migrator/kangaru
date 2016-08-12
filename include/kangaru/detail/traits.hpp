#ifndef KGR_INCLUDE_KANGARU_DETAIL_TRAITS_HPP
#define KGR_INCLUDE_KANGARU_DETAIL_TRAITS_HPP

#include "function_traits.hpp"
#include "utils.hpp"

#include <type_traits>
#include <tuple>

namespace kgr {
namespace detail {

// void_t implementation
template<typename...>
struct voider { using type = void; };

template<typename... Ts> using void_t = typename voider<Ts...>::type;

// things missing from c++11 (to be removed when switching to c++14)
template <bool b, typename T = void>
using enable_if_t = typename std::enable_if<b, T>::type;

template<typename T>
using decay_t = typename std::decay<T>::type;

template<std::size_t S, typename T>
using tuple_element_t = typename std::tuple_element<S, T>::type;

template<std::size_t ...>
struct seq {};

template<std::size_t n, std::size_t ...S>
struct seq_gen : seq_gen<n-1, n-1, S...> {};

template<std::size_t ...S>
struct seq_gen<0, S...> {
	using type = seq<S...>;
};

template<typename Tuple>
struct TupleSeqGen : seq_gen<std::tuple_size<Tuple>::value> {};

template<>
struct TupleSeqGen<std::tuple<>> : seq_gen<0> {};

template<typename Tuple>
using tuple_seq = typename TupleSeqGen<Tuple>::type;

template<typename Tuple, int n>
using tuple_seq_minus = typename detail::seq_gen<std::tuple_size<Tuple>::value - (n > std::tuple_size<Tuple>::value ? std::tuple_size<Tuple>::value : n)>::type;

// SFINAE utilities
template<typename T, typename = void>
struct has_invoke : std::false_type {};

template<typename T>
struct has_invoke<T, void_t<typename T::invoke>> : std::true_type {};

template<typename T, typename = void>
struct has_next : std::false_type {};

template<typename T>
struct has_next<T, void_t<typename T::Next>> : std::true_type {};

template<typename T, typename = void>
struct has_forward : std::false_type {};

template<typename T>
struct has_forward<T, void_t<decltype(std::declval<T>().forward())>> : std::true_type {};

template<typename T, typename = void>
struct is_service : std::false_type {};

template<typename T>
struct is_service<T, enable_if_t<(!std::is_polymorphic<T>::value || std::is_abstract<T>::value) && has_forward<T>::value>> : std::true_type {};

template<typename T, typename = void>
struct has_construct : std::false_type {};

template<typename T>
struct has_construct<T, void_t<decltype(&T::construct)>> : std::true_type {};

template<typename T, typename = void>
struct is_invoke_call : std::false_type {};

template<typename T>
struct is_invoke_call<T, void_t<typename T::Params>> : std::true_type {};

template<template<typename> class Map, typename T, typename = void>
struct is_complete_map : std::false_type {};

template<template<typename> class Map, typename T>
struct is_complete_map<Map, T, void_t<typename Map<T>::Service>> : std::true_type {};

template<typename T, typename... Args>
struct is_brace_constructible_helper {
private:
	template<typename U, typename... As>
	static decltype(static_cast<void>(U{std::declval<As>()...}), std::true_type{}) test(int);

	template<typename...>
	static std::false_type test(...);
	
public:
	using type = decltype(test<T, Args...>(0));
};

template<typename T, typename... Args>
struct has_template_construct_helper {
private:
	template<typename U, typename... As>
	static std::true_type test(decltype(&U::template construct<As...>)* = nullptr);

	template<typename...>
	static std::false_type test(...);
	
public:
	using type = decltype(test<T, Args...>(nullptr));
};

template<typename T, typename... Args>
struct has_emplace_helper {
private:
	template<typename U, typename... As>
	static decltype(static_cast<void>(std::declval<U>().emplace(std::declval<As>()...)), std::true_type{}) test(int);
	
	template<typename U, typename... As>
	static std::false_type test(...);
	
public:
	using type = decltype(test<T, Args...>(0));
};

template<template<typename> class, typename, typename, typename...>
struct is_invokable_helper;

template<template<typename> class Map, typename T, typename... Args, std::size_t... S>
struct is_invokable_helper<Map, T, seq<S...>, Args...> {
private:
	template<typename U, typename... As>
	static decltype(
		static_cast<void>(std::declval<U>()(std::declval<ServiceType<service_map_t<Map, function_argument_t<S, decay_t<U>>>>>()..., std::declval<As>()...)),
		std::true_type{}
	) test(int);
	
	template<typename U, typename... As>
	static std::false_type test(...);
	
public:
	using type = decltype(test<T, Args...>(0));
};

template<typename T, typename... Args>
struct has_emplace : has_emplace_helper<T, Args...>::type {};

template<typename T, typename... Args>
struct is_brace_constructible : is_brace_constructible_helper<T, Args...>::type {};

template<typename T> struct remove_rvalue_reference { using type = T; };
template<typename T> struct remove_rvalue_reference<T&> { using type = T&; };
template<typename T> struct remove_rvalue_reference<T&&> { using type = T; };

template<typename T> using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

template<typename... Ts>
struct has_template_construct : has_template_construct_helper<Ts...>::type {};

template<typename... Ts>
using is_someway_constructible = std::integral_constant<bool, is_brace_constructible<Ts...>::value || std::is_constructible<Ts...>::value>;

template<typename T, typename... Args>
using is_emplaceable = typename std::conditional<std::is_default_constructible<T>::value && has_emplace<T, Args...>::value, std::true_type, std::false_type>::type;

template<template<typename> class Map, typename U, typename... Args>
struct is_invokable : is_invokable_helper<Map, U, tuple_seq_minus<function_arguments_t<U>, sizeof...(Args)>, Args...>::type {};

} // namespace detail
} // namespace kgr

#endif // KGR_INCLUDE_KANGARU_DETAIL_TRAITS_HPP
