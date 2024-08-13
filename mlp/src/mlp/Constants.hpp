#pragma once

namespace mlp
{
    template<class T>
    constexpr T pi = T(3.1415926535897932385L);
    template<class T>
    constexpr T twopi = T(2) * pi<T>;
    template<class T>
    constexpr T pi_2 = T(0.5) * pi<T>;

}