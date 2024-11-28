#pragma once

#include <tuple>

template<typename Functor, typename ...Args>
class Lambda {
    Functor f;
    std::tuple<Args...> state;

    template<std::size_t I>
    constexpr auto &getArg(auto &&invocationArgs) {
        constexpr auto STATE_ARGS_SIZE = std::tuple_size_v<decltype(this->state)>;

        if constexpr (I < STATE_ARGS_SIZE) return std::get<I>(this->state);
        else return std::get<I - STATE_ARGS_SIZE>(invocationArgs);
    }

    template<std::size_t ...I>
    constexpr auto invoke(auto &&invocationArgs, std::index_sequence<I...>) -> decltype(this->f(this->getArg<I>(invocationArgs)...)) {
        return this->f(this->getArg<I>(invocationArgs)...);
    }

public:
    constexpr Lambda() = default;

    constexpr explicit Lambda(Functor f, Args ...args) : f(f), state(std::forward_as_tuple(args...)) {}

    // The auto return type affects how concepts are checked
    // i.e. when the variable doesn't meet the requirements of a concept,
    // a hard compile error is emitted.
    //
    // The '-> decltype' syntax gives the compiler the hand it needs
    // to deduce the return type without throwing a compile error.
    // https://stackoverflow.com/questions/64186621/why-doesnt-stdis-invocable-work-with-templated-operator-which-return-type-i
    template<typename ...InvArgs>
    constexpr auto operator()(InvArgs &&...invArgs) -> decltype(this->invoke(std::forward_as_tuple(invArgs...), std::make_index_sequence<std::tuple_size_v<decltype(this->state)> + std::tuple_size_v<decltype(std::forward_as_tuple(invArgs...))>>{})) {
        auto invocationArgs = std::forward_as_tuple(invArgs...);

        constexpr auto STATE_ARGS_SIZE = std::tuple_size_v<decltype(this->state)>;
        constexpr auto INVOCATION_ARGS_SIZE = std::tuple_size_v<decltype(invocationArgs)>;

        return this->invoke(invocationArgs, std::make_index_sequence<STATE_ARGS_SIZE + INVOCATION_ARGS_SIZE>{});
    }
};
