/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __PAIR_H
#define __PAIR_H

#if defined(ENABLE_STL)
#include <utility>
using namespace std;

#define Pair pair
// Dummy namespace
namespace haggle {}

#else
namespace haggle {
/**
   This is a simple implementation of a Pair class. It is more or less the
   same as found in most STL libraries.
  
   @author Erik Nordström  
*/

template <typename T1, typename T2>
class Pair {
public:
	T1 first;
	T2 second;
	Pair() : first(), second() {}
	Pair(const T1& _first, const T2& _second) : first(_first), second(_second) {}
	template <typename TT1, typename TT2>
	Pair(const Pair<TT1, TT2>& p) : first(p.first), second(p.second) {}
};
	
template <typename T1, typename T2>
inline Pair<T1, T2> make_pair(T1 x, T2 y) {
	return Pair<T1, T2>(x, y);
}
	
template <typename T1, typename T2>
inline bool operator==(const Pair<T1, T2>& p1, const Pair<T1, T2>& p2) {
	return p1.first == p2.first && p1.second == p2.second;
}
	
template <typename T1, typename T2>
inline bool operator!=(const Pair<T1, T2>& p1, const Pair<T1, T2>& p2) {
	return !(p1 == p2);
}
	
template <typename T1, typename T2>
inline bool operator<(const Pair<T1, T2>& p1, const Pair<T1, T2>& p2) {
	return p1.first < p2.first || (!(p2.first < p1.first) && p1.second < p2.second);
}
	
template <typename T1, typename T2>
inline bool operator<=(const Pair<T1, T2>& p1, const Pair<T1, T2>& p2) {
	return !(p2 < p1);
}
	
template <typename T1, typename T2>
inline bool operator>(const Pair<T1, T2>& p1, const Pair<T1, T2>& p2) {
	return p2 < p1;
}
	
template <typename T1, typename T2>
inline bool operator>=(const Pair<T1, T2>& p1, const Pair<T1, T2>& p2) {
	return !(p1 < p2);
}

};

#endif /* ENABLE_STL */

#endif /* __PAIR_H */
