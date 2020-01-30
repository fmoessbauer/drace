/*
 * Copyright 2019 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * Example based on ideas from the excellent CPP training
 * "Praktikum Advanced Software Development with Modern C++" at LMU Munich, Winter 2016
 * by Tobias Fuchs
 */

/*
* TASKS:
* 1.  Understand control and data flow
* 2.  Is your program free of data-races?
* 2.1 check with DRace (see handout or slides)
* 2.2 fix the concurrency issues and try again
* 3.  Optimize locking pattern
* 3.1 check with DRace
* 4.  Think about the following: DRace reports 0 races => Program is race-free
*/

#include <iostream>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <set>
#include <string>
#include <chrono>
#include <vector>
#include <iterator>

#define CHECKOUT 0
#define TIMEOUT  1
#define PATIENCE 100
#define BUDGET   200

enum WalmartProduct : int {
	Couch,
	FlatscreenTV,
	Hammock,
	CofeeMachine,
	Ammunition
};

enum ToysRUsProduct : int {
	Pikachu,
	DoraTheExplorer,
	TrumpActionToy,
	Firetruck,
	TickleMeElmo
};

enum KmartProduct : int {
	Diesel,
	Fertilizer,
	Bleach,
	RubberGlove,
	Soap
};

template<typename ProductT>
std::pair<ProductT, int> make() {
	return std::make_pair(
		static_cast<ProductT>(std::rand() % 5),
		std::rand() % 100);
}

template<typename shop_t>
int shopping(std::timed_mutex & buy_mx,
	shop_t & shop,
	const std::string & shopname,
	int & budget,
	const int threadid)
{
	bool checkout = false;

	while (!checkout) {
		// try to buy things, but only if shop is not empty
		std::unique_lock<std::timed_mutex> buy_lock(buy_mx, std::defer_lock);

		if (!buy_lock.try_lock_for(std::chrono::milliseconds(PATIENCE))) {
			std::cout << "[T " << threadid << ", " << shopname << "] timeout" << std::endl;
			return TIMEOUT;
		}

		int size = shop.size();
		if (size != 0) {
			auto elit = shop.begin();
			// find random product in shop
			std::advance(elit, (std::rand() % size));
			if (std::get<1>(*elit) < budget) {
				budget -= std::get<1>(*elit);

				// remove item from shop
				shop.erase(elit);
				std::cout << "[T " << threadid << ", " << shopname << "] bought item, new budget " << budget << std::endl;
			}
			else {
				std::cout << "[T " << threadid << "] budget exhausted" << std::endl;
				checkout = true;
			}
		}
		else {
			std::cout << "[T " << threadid << "] no more products in this shop" << std::endl;
			checkout = true;
		}
	}
	return CHECKOUT;
}

template<typename walmart_t,
	typename toysrus_t,
	typename kmart_t>
	void go_wild(int         threadid,
		walmart_t & walmart,
		toysrus_t & toysrus,
		kmart_t   & kmart,
		int         budget) {
	std::cout << "[T " << threadid << "] started" << std::endl;

	static std::timed_mutex walmart_mx;
	static std::timed_mutex toysrus_mx;
	static std::timed_mutex kmart_mx;

	// reason why a customer returns from a shop
	int ret_reason = -1;
	while (ret_reason != CHECKOUT) {
		int shopnr = std::rand() % 3;
		switch (shopnr) {
		case 0: ret_reason = shopping(walmart_mx, walmart, "walmart", budget, threadid); break;
		case 1: ret_reason = shopping(toysrus_mx, toysrus, "toysrus", budget, threadid); break;
		case 2: ret_reason = shopping(kmart_mx, kmart, "kmart", budget, threadid);
		}
	}

	std::cout << "[T " << threadid << "] finished" << std::endl;
}

void test_fun() {
	std::cout << "Thread started" << std::endl;
}

int main(int argc, char** argv)
{
	std::cout<< "Setup playground" << std::endl;

	static const int horde_size = 20;
	static const int walmart_capacity = 100000;
	static const int toysrus_capacity = 40000;
	static const int kmart_capacity = 50000;

	// The shopping centers:
	std::multiset<std::pair<WalmartProduct, int>> walmart;
	std::multiset<std::pair<ToysRUsProduct, int>> toysrus;
	std::multiset<std::pair<KmartProduct, int>> kmart;

	// The dear customers:
	std::vector<std::thread> horde;

	// some tweaks
	horde.reserve(horde_size);

	std::generate_n(std::inserter(walmart, walmart.begin()), walmart_capacity, make<WalmartProduct>);
	std::generate_n(std::inserter(toysrus, toysrus.begin()), toysrus_capacity, make<ToysRUsProduct>);
	std::generate_n(std::inserter(kmart, kmart.begin()), kmart_capacity, make<KmartProduct>);

	int num_total_products = walmart.size() + toysrus.size() + kmart.size();
	std::cout << "Total number of products " << num_total_products << std::endl;

	for (int i = 0; i < horde_size; ++i) {
		horde.push_back(std::thread(go_wild<decltype(walmart),
			decltype(toysrus),
			decltype(kmart)>,
			i,
			std::ref(walmart),
			std::ref(toysrus),
			std::ref(kmart),
			BUDGET));
	}

	for (auto & t : horde) {
		t.join();
	}
	std::cout << "Black friday ended" << std::endl;
}
