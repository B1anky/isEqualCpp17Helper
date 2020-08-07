/* MIT License
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
#include <cstddef>
#include <tuple>
#include <cmath>
#include <functional>
#include <string>

//Swap 0 to a 1 for active debugging and more diagnostics
#define DEBUGGING 0

#define TOLERANCE 1E-5

//This allows us to test for what we can freely "assume" to be iterable, you can make that more or less strict depending on use cases
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

//This will call the above template, if possible, but if not, will be of std::false_type at compilation time
template <typename T>
std::false_type is_iterable_impl(...);

//naming this No because if No operator== exists, its value will be 0 (Get it? It's like english but worse)
struct No {};
template<typename T, typename Arg> No operator== (const T&, const Arg&);

template<typename T, typename Arg = T>
struct EqualExists
{
  enum { value = !std::is_same<decltype(std::declval<T>() == std::declval<Arg>()), No>::value };
};

//these two are for maps specifically, pretty simple, but we're only using the structs as a wrapper for the constexpr
template <class T>
struct is_map {
    static constexpr bool value = false;
};

template<class Key, class Value>
struct is_map<std::map<Key, Value>> {
    static constexpr bool value = true;
};

//this one is for tuple types specifically
template <typename>      struct is_tuple                   : std::false_type {};
template <typename ...T> struct is_tuple<std::tuple<T...>> : std::true_type  {};

//try to leverage the same behavior for std::pair now
template <typename>               struct is_pair                  : std::false_type {};
template <typename T, typename V> struct is_pair<std::pair<T, V>> : std::true_type  {};

//this one makes iterables more legible when using them in the source code
template <typename T>
using is_iterable = decltype(is_iterable_impl<T>(0));

//The following was found at: https://www.fluentcpp.com/2019/03/08/stl-algorithms-on-tuples/
//This allows us to generically iterate over two tuples simulatenously and perform user-defined operations on it (super cool)
template <class Tuple1, class Tuple2, class F, std::size_t... I>
F for_each_tuple_element_impl(Tuple1&& t1, Tuple2&& t2, F&& f, std::index_sequence<I...>)
{
    return (void)std::initializer_list<int>{(std::forward<F>(f)(std::get<I>(std::forward<Tuple1>(t1)), std::get<I>(std::forward<Tuple2>(t2))),0)...}, f;
}

template <class Tuple1, class Tuple2, class F>
constexpr decltype(auto) for_each_tuple_element(Tuple1&& t1, Tuple2&& t2, F&& f)
{
    return for_each_tuple_element_impl(std::forward<Tuple1>(t1), std::forward<Tuple2>(t2), std::forward<F>(f),
                         std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple1>>::value>{});
}


/*
 * This function is the meat and potatoes of the best implcit ==operator since sliced bread. (Probably)
 * Ever compare two floating points and need a tolerance because the darn rounding error got you?
 * I bet you've opted to use something along the line of fabs(float1 - float2) <= 0.0001 or the likes...
 * Now have you had to compare two containers of floats, which you assumed would be the same, but weren't?
 * This function can essentially do the above concept and apply it to any combination of types implcitly.
 * You have an int and a float? Great we can do that for you in a nice, convenient, small-form factor call.
 * You have two std::vector<float>s? Great we can do that for you too.
 * You have a std::vector<float> and a std::vector<double>, but they're so similar you'd expect them to be equal?
 * Well normally for differing container types likes that, the compiler will sadly make you have to write out
 * the explicit operator== like this:
 *
 *     bool operator==(const std::vector<float>& floatVector, const std::vector<double>& doubleVector){ ... }
 *
 * Hmmm... but what if we want the tolerance to be definable? Oops good luck with that without adding a layer of indirection.
 * Also god-forbid you reversed the order of the float and double, because you'll have to rewrite the entire thing again
 * to get it compiling for flipping your right and left hand side types. With all these layers of indirection, I think Shrek will be seeing you soon.
 * This magical isEqual function can do any-order tolerance comparison for free (generally).
 * You don't want a tolerance? Just supply a 0.0 on a per-case basis, or swap the above TOLERANCE define to: #define TOLERANCE 0.0
 * Yup, it's that easy.
 * B-b-but what about special types like tuples and pairs? Oh boy if you thought the above operator== nightmare with just
 * the std::vectors was bad, imagine if you have a bunch of similar sized tuples (and pairs obviously) and you want to compare them to each other?
 * Ding-ding-ding you're right, you couldn't before, but now you can!
 * Seriously, it's hard to make this not compile, because if it can fall into the else case, it'll just return false.
 * If it can't fall into any case, it just won't compile any of the cases into the template during the preprocessor stage and essentially just generate:
 * bool isEqualRet = false;
 * return isEqualRet;
 * Oh, by the way, be careful with pointer types. They can literally compare safely with anything, including a float, funnily enough, so
 * make sure you're dereferencing things unless they're both pointer types.
 */
template <typename Comparable1, typename Comparable2>
static inline bool isEqual(Comparable1 comparable1, Comparable2 comparable2, float tolerance = TOLERANCE){ //0.00001

    bool isEqualRet = false;

    //If we have mismatched map types but technically "could" be compared (i.e. a std::map<int, float> with a std::map<int, double> or a std::map<int, int>)
    if constexpr(is_map<Comparable1>::value && is_map<Comparable2>::value){

            //We should have a strict requirement of having a size since there's no standard for every container type
            isEqualRet = (comparable1.size() == comparable2.size());

            size_t indexError(0);

            //We also had a strict requirement that the begin() and end() operators were to be defined
            for(auto comparable1Iter = comparable1.begin(),
                     comparable2Iter = comparable2.begin() ;
                isEqualRet && (comparable1Iter != comparable1.end()) && (comparable2Iter != comparable2.end());
                comparable1Iter++, comparable2Iter++){

                //This is crazy that we can call ourself
                isEqualRet = isEqual(*comparable1Iter, *comparable2Iter, tolerance);

                ++indexError;

            }

            #if DEBUGGING

            if(!isEqualRet){

                std::cout << "is_map returned false starting at index: " << indexError << " of both maps. Could be type-mismatched or actually not equal." << std::endl;

            }

            #endif //DEBUGGING

    }
    //iterable containers, like std::vector or std::lists can natively be compared to each other here
    else if constexpr(is_iterable<Comparable1>::value && is_iterable<Comparable2>::value){

        //We should have a strict requirement of having a size since there's no standard for every container type
        isEqualRet = (comparable1.size() == comparable2.size());

        //We also had a strict requirement that the begin() and end() operators were to be defined
        for(auto comparable1Iter = comparable1.begin(),
                 comparable2Iter = comparable2.begin() ;
            isEqualRet && (comparable1Iter != comparable1.end()) && (comparable2Iter != comparable2.end());
            comparable1Iter++, comparable2Iter++){

            //We also needed the *operator defined so we can dereference the iterator
            isEqualRet = isEqual(*comparable1Iter, *comparable2Iter, tolerance);

            #if DEBUGGING

            if(!isEqualRet){

                std::cout << "false (is_iterable)" << std::endl;
                std::cout << fabs(*comparable1Iter - *comparable2Iter)     << " is not within tolerance threshold: " << tolerance << std::endl;
                std::cout << *comparable1Iter << " - " << *comparable2Iter << " = " << *comparable1Iter - *comparable2Iter << std::endl;

            }

            #endif //DEBUGGING

        }

    }else if constexpr( std::is_fundamental<Comparable1>::value && std::is_fundamental<Comparable2>::value ){

        isEqualRet = (fabs(comparable1 - comparable2) <= tolerance);

        #if DEBUGGING

        if(!isEqualRet){

            std::cout << "false (is_fundamental)" << std::endl;
            std::cout << fabs(comparable1 - comparable2) << " is not within tolerance threshold: " << tolerance << " to " << comparable2 << std::endl;
            std::cout << comparable1 << " - " << comparable2 << " = " << comparable1 - comparable2 << std::endl;

        }

        #endif //DEBUGGING

    }
    //The tuple or pair can be of any type we want, even with differing lengths and we can handle it through the following constexprs
    else if constexpr( (is_pair<Comparable1>::value || is_tuple<Comparable1>::value) || (is_pair<Comparable2>::value || is_tuple<Comparable2>::value) ){

        isEqualRet = std::tuple_size<Comparable1>::value == std::tuple_size<Comparable2>::value;

        size_t indexError(0);

        if constexpr( std::tuple_size<Comparable1>::value == std::tuple_size<Comparable2>::value){

            for_each_tuple_element(comparable1, comparable2, [&isEqualRet, &tolerance, &indexError](auto&& i, auto&& s){
                isEqualRet &= isEqual(i, s, tolerance);
                if(isEqualRet) ++indexError;
            });

        }

        #if DEBUGGING

        if(!isEqualRet){

            std::cout << "false (is_tuple / is_pair)" << std::endl;
            std::cout << "is_tuple returned false starting at index: " << indexError << " of both tuples. Could be type-mismatched or actually not equal." << std::endl;

        }

        #endif //DEBUGGING


    }
    else if constexpr( EqualExists<Comparable1>::value && std::is_same<Comparable1, Comparable2>::value && !std::is_fundamental<Comparable1>::value){

            (void) tolerance;
            isEqualRet = comparable1 == comparable2;

            #if DEBUGGING

            if(!isEqualRet){

                std::cout << "false (==operator)" << std::endl;

            }

            #endif //DEBUGGING

    }
    //Either Comparable1 is iterable and Comparable2 is not, or vice versa
    else{        

        //Unused warning suppression for when the else case is compiled
        (void) comparable1;
        (void) comparable2;
        (void) tolerance;        

        #if DEBUGGING

        std::cout << "false (Types are incompatible, this will always return false.)" << std::endl;

        #endif //DEBUGGING

        isEqualRet = false;

    }

    return isEqualRet;

}

//Random test class with some varying types and an operator== overload
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

    std::cout << std::boolalpha;

    //Standard primitive tests
    std::cout << "if 1 and 0 are equal: "
              << isEqual(1, 0) << std::endl << std::endl;
    std::cout << "if 1 and 1 are equal: "
              << isEqual(1, 1) << std::endl << std::endl;
    std::cout << "if 1 and 2 are equal with a tolerance of ~1.0: "
              << isEqual(1, 2, 1.0) << std::endl << std::endl;
    std::cout << "if 1.0 and 2.9 are equal with a tolerance of ~1.0: "
              << isEqual(1.0, 2.9, 1.0) << std::endl << std::endl;
    std::cout << "if 1.0 and 1.0 are equal with a tolerance of ~0.0: "
              << isEqual(1.0, 1.0, 0.0) << std::endl << std::endl;

    std::vector<float>    floatVector  = {1.2, 36.6, 25.11, 22.44};
    std::vector<double>   doubleVector = {1.2, 36.6, 25.11, 22.44};
    std::list<int>        intList      = {2, 36, 25, 22};

    //Iterable tests
    std::cout << "if a std::vector<float> and a std::vector<double>\nwith equivalent initializer list are equal: "
              << isEqual(floatVector, doubleVector, 1E-5) << std::endl << std::endl;

    //This will be the else case that always returns false since we're comparing an iterable with a non-iterable
    std::cout << "if a std::vector<float> and a float are equal: "
              << isEqual(floatVector, 2.9) << std::endl << std::endl;

    std::cout << "if a std::vector<float> and a std::list<int>\nwith similar initializer lists are equal with a default tolerance: "
              << isEqual(floatVector, intList) << std::endl << std::endl;

    std::cout << "if a std::vector<float> and a std::list<int>\nwith similar initializer lists are equal with a 1.0 tolerance: "
              << isEqual(floatVector, intList, 1.0) << std::endl << std::endl;


    ImplictlyCompareMeCorrectly test1(1, 2.3,  64.36435, {1, 2, 3});
    ImplictlyCompareMeCorrectly test2(1, 2.3,  64.36435, {1, 2, 3});
    ImplictlyCompareMeCorrectly test3(2, 4.6, 128.72870, {4, 5, 6});

    //Custom classes with an operator== overload defined
    std::cout << "if a custom class with an operator== is well-defined\nwith two similarly initialized classes are equal: "
              << isEqual(test1, test2) << std::endl << std::endl;
    std::cout << "if a custom class with an operator== is well-defined\nwith two differently initialized classes are equal: "
              << isEqual(test1, test3) << std::endl << std::endl;

    std::tuple<int, int, int> tupleTest1 = {1, 2, 3};
    std::tuple<int, int, int> tupleTest2 = {1, 2, 3};
    std::tuple<int, short, ImplictlyCompareMeCorrectly> tupleTest3 = {2, 5, test1};
    std::tuple<int, short, ImplictlyCompareMeCorrectly> tupleTest4 = {2, 5, test1};

    //std::tuple tests
    std::cout << "if two std::tuple<int, int, int>s initialized with the same initializer lists are equal with a default tolerance: "
              << isEqual(tupleTest1, tupleTest2) << std::endl << std::endl;
    std::cout << "if two std::tuple<int, short, ImplictlyCompareMeCorrectly>s initialized with the same initializer lists are equal with a default tolerance: "
              << isEqual(tupleTest3, tupleTest3) << std::endl << std::endl;

    std::pair<int, int>      pairTest1 = {1  , 2  };
    std::pair<int, int>      pairTest2 = {1  , 2  };
    std::pair<double, float> pairTest3 = {1.0, 2.0};
    std::pair<ImplictlyCompareMeCorrectly, float> pairTest4 = {test1, 5.0};
    std::pair<ImplictlyCompareMeCorrectly, int>   pairTest5 = {test1, 5  };

    //std::pair tests
    std::cout << "if two std::pair<int, int>s initialized with the same initializer lists are equal with a default tolerance: "
              << isEqual(pairTest1, pairTest2) << std::endl << std::endl;
    std::cout << "if a std::pair<int, int> and std::pair<double, float> with very similar initializer lists are equal with a default tolerance: "
              << isEqual(pairTest1, pairTest3) << std::endl << std::endl;
    std::cout << "if a std::pair<ImplictlyCompareMeCorrectly, float> and a std::pair<ImplictlyCompareMeCorrectly, int> "
                 "with very similar initializer lists are equal with a default tolerance: "
              << isEqual(pairTest4, pairTest5) << std::endl << std::endl;

    std::map<int, double> map1 = { {1, 1.2}, {36, 36.6}, {25, 25.11}, {22, 22.44} };
    std::map<int, double> map2 = { {1, 1.2}, {36, 36.6}, {25, 25.11}, {22, 22.44} };
    std::map<int, float>  map3 = { {1, 1.2}, {36, 36.6}, {25, 25.11}, {22, 22.44} };

    //std::map tests
    std::cout << "if a std::map<int, double>'s values and a std::map<int, double>'s\nwith equivalent initializer lists are equal with a default tolerance: "
              << isEqual(map1, map2) << std::endl << std::endl;
    std::cout << "if a std::map<int, double>'s values and a std::map<int, float>'s\nwith equivalent initializer lists are equal with a default tolerance: "
              << isEqual(map1, map3) << std::endl << std::endl;

    std::tuple<int, int> fakeIntPair = {5, 10};
    std::pair<int, int>  realIntPair = {5, 10};

    //The real icing on the cake it probably the behavior that a std::tuple of two elemenst and an std::pair are essentially comparable implicitly
    //Uncomment this and have fun with the compiler generated errors
    //std::cout << (fakeIntPair == realIntPair);
    //Here it just works, like magic
    std::cout << "if a std::tuple<int, int> and a std::pair<int, int> with equivalent initializer lists are equal with a default tolerance: "
              << isEqual(fakeIntPair, realIntPair) << std::endl << std::endl;

    //If you want to play with the pointers, go ahead, uncomment this block

    /*
     *  int* intPtr      = new int(5);
     *  int* otherIntPtr = intPtr;
     *  isEqual(intPtr, otherIntPtr);  //will be true
     *  isEqual(nullptr, otherIntPtr); //will be false
     *  isEqual(nullptr, nullptr);     //one of the few things that won't compile, keep it commented out or your compiler will go Brrrrrrrrr
     *
     *  int* ourNullptr      = nullptr;
     *  int* ourOtherNullptr = nullptr;
     *  isEqual(ourNullptr, ourOtherNullptr); //Luckily returns true
     *
     *  int* ourUninitializedPtr;
     *  int* ourOtherUninitializedPtr;
     *  isEqual(ourUninitializedPtr, ourOtherUninitializedPtr); //This one depends on if your system nulls out memory for you, so it's undefined behavior
     */

    return 0;

}
