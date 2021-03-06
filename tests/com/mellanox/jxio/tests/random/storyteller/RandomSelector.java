/*
 ** Copyright (C) 2013 Mellanox Technologies
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at:
 **
 ** http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 ** either express or implied. See the License for the specific language
 ** governing permissions and  limitations under the License.
 **
 */
package com.mellanox.jxio.tests.random.storyteller;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

class RandomSelector<T> {

	List<Item<T>> items    = new ArrayList<Item<T>>();
	Random        rand     = new Random();
	int           totalSum = 0;

	/**
	 * Construct a new RandomSelector.
	 * @param items A list of items to randomize.
	 */
	public RandomSelector(List<Item<T>> items) {
		this.items = items;
		for (Item<T> item : items) {
			totalSum = totalSum + item.getProbability();
		}
	}
	
	/**
	 * Construct a new RandomSelector with a fixed random seed.
	 * @param items A list of items to randomize.
	 * @param seed A long that represents the initial seed.
	 */
	public RandomSelector(List<Item<T>> items, Random rand) {
		this(items);
		this.rand = rand;
	}

	/**
	 * @return A random item based on probability.
	 */
	public Item<T> getRandom() {
		int index = rand.nextInt(totalSum);
		int sum = 0;
		int i = 0;
		while (sum <= index) {
			sum += items.get(i++).getProbability();
		}
		return items.get(i - 1);
	}
}