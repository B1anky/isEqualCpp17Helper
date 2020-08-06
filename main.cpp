/*MIT License
 *
 * Copyright (c) 2020 B1anky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#include <iostream>
#include <type_traits>
#include <vector>
#include <list>
#include <map>
#include <cmath>

//Swap 0 to a 1 for active debugging and more diagnostics
#define DEBUGGING 0

namespace detail
{
    // To allow ADL with custom begin/end
    using std::begin;
    using std::end;

    template <typename T>
    auto is_iterable_impl(int)
    -> decltype (
        begin(std::declval<T&>()) != end(std::declval<T&>()),   // begin/end and operator !=
        void(),                                                 // Handle evil operator
        size(std::declval<T&>()),                               // Handle safely getting the size, since most iterables have this function
        ++std::declval<decltype(begin(std::declval<T&>()))&>(), // operator ++
        --std::declval<decltype(begin(std::declval<T&>()))&>(), // operator --
        void(*begin(std::declval<T&>())),                       // operator*
        std::true_type{});

    template <typename T>
    std::false_type is_iterable_impl(...);

    struct No {};
    template<typename T, typename Arg> No operator== (const T&, const Arg&);

    template<typename T, typename Arg = T>
    struct EqualExists
    {
      enum { value = !std::is_same<decltype(std::declval<T>() == std::declval<Arg>()), No>::value };
    };

}

template <typename T>
using is_iterable = decltype(detail::is_iterable_impl<T>(0));

template <typename Comparable1, typename Comparable2>
static inline bool isEqual(Comparable1 comparable1, Comparable2 comparable2, float tolerance = 1E-5){ //0.00001

    bool isEqual = true;

    if constexpr(is_iterable<Comparable1>::value && is_iterable<Comparable2>::value){

        //We should have a strict requirement of having a size since there's no standard for every container type
        isEqual = (comparable1.size() == comparable2.size());

        //We also had a strict requirement that the begin() and end() operators were to be defined
        for(auto comparable1Iter = comparable1.begin(),
                 comparable2Iter = comparable2.begin() ;
            isEqual && (comparable1Iter != comparable1.end()) && (comparable2Iter != comparable2.end());
            comparable1Iter++, comparable2Iter++){

            //We also needed the *operator defined so we can dereference the iterator
            isEqual = (fabs(*comparable1Iter - *comparable2Iter) <= tolerance);

            #if DEBUGGING

            if(!isEqual){

                std::cout << fabs(*comparable1Iter - *comparable2Iter)     << " is not within tolerance threshold: " << tolerance << std::endl;
                std::cout << *comparable1Iter << " - " << *comparable2Iter << " = " << *comparable1Iter - *comparable2Iter << std::endl;

            }

            #endif //DEBUGGING

        }

    }else if constexpr( !is_iterable<Comparable1>::value && !is_iterable<Comparable2>::value && std::is_fundamental<Comparable1>::value && std::is_fundamental<Comparable2>::value ){

        isEqual = (fabs(comparable1 - comparable2) <= tolerance);

        #if DEBUGGING

        if(!isEqual){

            std::cout << fabs(comparable1 - comparable2) << " is not within tolerance threshold: " << tolerance << std::endl;
            std::cout << comparable1 << " - " << comparable2 << " = " << comparable1 - comparable2 << std::endl;

        }

        #endif //DEBUGGING

    }else if constexpr( detail::EqualExists<Comparable1>::value && detail::EqualExists<Comparable2>::value && std::is_same<Comparable1, Comparable2>::value && !std::is_fundamental<Comparable1>::value ){

        (void) tolerance;
        isEqual = comparable1 == comparable2;

    }
    //Either Comparable1 is iterable and Comparable2 is not, or vice versa
    else{

        //Unused warning suppression for when the else case is compiled
        (void) comparable1;
        (void) comparable2;
        (void) tolerance;

        return false;

    }

    return isEqual;

}

class ImplictlyCompareMeCorrectly{

public:

    ImplictlyCompareMeCorrectly()
        : i(0),
          f(0.0),
          d(0.0){

        /* NOP */

    }

    ImplictlyCompareMeCorrectly(int iIn, float fIn, double dIn, std::vector<double> doubleVectorIn)
        :       i(iIn),
                f(fIn),
                d(dIn),
          dVector(doubleVectorIn){

        /* NOP */

    }

    ImplictlyCompareMeCorrectly(const ImplictlyCompareMeCorrectly& other)
        :       i(other.i),
                f(other.f),
                d(other.d),
          dVector(other.dVector){

        /* NOP */

    }

    ImplictlyCompareMeCorrectly& operator=(const ImplictlyCompareMeCorrectly& other){

        if(this != &other){

                  i = other.i;
                  f = other.f;
                  d = other.d;
            dVector = other.dVector;

        }

        return *this;

    }

    bool operator==(const ImplictlyCompareMeCorrectly& other) const{

        return isEqual(i,       other.i)        &&
               isEqual(f,       other.f)        &&
               isEqual(d,       other.d)        &&
               isEqual(dVector, other.dVector);

    }

    bool operator!=(const ImplictlyCompareMeCorrectly& other) const{

        return !(*this == other);

    }

public:

    int                 i;
    float               f;
    double              d;
    std::vector<double> dVector;

};

int main(){

    std::vector<float>    floatVector  = {1.2, 36.6, 25.11, 22.44};
    std::vector<double>   doubleVector = {1.2, 36.6, 25.11, 22.44};

    std::list<int>        intList      = {2, 36, 25, 22};
    std::map<int, double> map1         = { {1, 1.2}, {36, 36.6}, {25, 25.11}, {22, 22.44} };
    std::map<int, double> map2         = { {1, 1.2}, {36, 36.6}, {25, 25.11}, {22, 22.44} };
    std::array<int, 3>    stdArray1    = { 1, 2, 3 };
    std::array<int, 3>    stdArray2    = { 1, 2, 3 };
    std::array<double, 1> stdArray3    = { 1.0 };

    std::cout << std::boolalpha;

    std::cout << "if a std::vector<float> and a std::vector<double>\nwith equivalent initializer list are equal: "
              << isEqual(floatVector, doubleVector, 1E-5) << std::endl << std::endl;

    std::cout << "if 1 and 0 are equal: " << isEqual(1, 0) << std::endl << std::endl;

    std::cout << "if 1 and 1 are equal: " << isEqual(1, 1) << std::endl << std::endl;

    std::cout << "if 1.0 and 2.9 are equal with a tolerance of ~1.0: " << isEqual(1.0, 2.9, 1.0) << std::endl << std::endl;

    std::cout << "if 1.0 and 1.0 are equal with a tolerance of ~0.0: " << isEqual(1.0, 1.0, 0.0) << std::endl << std::endl;

    std::cout << "if a std::vector<float> and a float are equal: " << isEqual(floatVector, 2.9) << std::endl << std::endl;

    std::cout << "if a std::vector<float> and a std::list<int>\nwith similar initializer lists are equal with a default tolerance: "
              << isEqual(floatVector, intList) << std::endl << std::endl;

    std::cout << "if a std::vector<float> and a std::list<int>\nwith similar initializer lists are equal with a 1.0 tolerance: "
              << isEqual(floatVector, intList, 1.0) << std::endl << std::endl;


    /*
     * Would be nice if we could somehow get implicit std::tuple / pair operator equals such that we could "iterate" over it somehow and compare them like arrays,
     * using isEqual to compare the two elements
     *
    std::cout << "if a std::map<int, double>'s values and a std::vector<double>\nwith equivalent initializer lists are equal and a default tolerance: "
              << isEqual(map1, map2) << std::endl << std::endl;
    */
    ImplictlyCompareMeCorrectly test1(1, 2.3, 64.36435, {1, 2, 3});
    ImplictlyCompareMeCorrectly test2(1, 2.3, 64.36435, {1, 2, 3});

    std::cout << isEqual(test1, 1) << std::endl << std::endl;

    std::cout << isEqual(stdArray1, stdArray2) << std::endl << std::endl;
    std::cout << isEqual(stdArray1, stdArray3) << std::endl << std::endl;

    return 0;

}
