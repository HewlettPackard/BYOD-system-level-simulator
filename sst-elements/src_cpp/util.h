// #pragma once
#ifndef _UTIL_H
#define _UTIL_H

#include <cstdint>
#include <iomanip>
#include <vector>
#include <xtensor/containers/xarray.hpp>

template <typename T> std::vector<T> xarray2vector(xt::xarray<T> inp) {
	std::vector<T> dst(inp.size());
	std::copy(inp.cbegin(), inp.cend(), dst.begin());

	return dst;
}

template <typename T> std::vector<uint8_t> castToMemVector(std::vector<T> x) {
	uint8_t bytes = sizeof(T) / sizeof(uint8_t);
	std::vector<uint8_t> res(x.size() * sizeof(T));

	for (size_t i = 0; i < x.size(); ++i) {
		//uint8_t *ptr = reinterpret_cast<uint8_t *>(&x[i]);
		//for (size_t j = 0; j < bytes; ++j) {
		//	res[bytes * i + j] = ptr[j];
		//}
		std::memcpy(&res[bytes * i], &x[i], sizeof(T));
	}

	return res;
}

template <typename T> std::vector<T> castFromMemVector(std::vector<uint8_t> x) {
	uint8_t bytes = sizeof(T) / sizeof(uint8_t);
	std::vector<T> res(x.size() / sizeof(T));
	for (size_t i = 0; i < x.size(); ++i) {
		//float tmp;
		std::memcpy(&res[i], &x[bytes * i], sizeof(T));
		//res[i] = tmp;
	}
	return res;
}

// std::string pad_string(std::string &s, int n) {
// 	std::ostringstream oss;
// 	oss << std::setw(n) << s;
// 	return oss.str();
// }

#endif