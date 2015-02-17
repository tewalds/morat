
#pragma once

/*
Given weights for indexes, returns a random index according to the weights.
This is useful for softmax and similar, used in the rollout policy.
*/

#include "bits.h"
#include "xorshift.h"

namespace Morat {

// O(1) updates, O(s) choose
class WeightedRandScan {
	mutable XORShift_double unitrand;
	unsigned int size, allocsize;
	double sum;
	double * weights;
public:
	WeightedRandScan()      : size(0), allocsize(0), sum(0), weights(NULL) { }
	WeightedRandScan(unsigned int s) : allocsize(0), sum(0), weights(NULL) { resize(s); }

	~WeightedRandScan(){
		if(weights)
			delete[] weights;
		weights = NULL;
	}

	//resize and clear the tree
	void resize(unsigned int s){
		size = s;

		if(size > allocsize){
			if(weights)
				delete[] weights;

			allocsize = size;
			weights = new double[allocsize];
		}

		clear();
	}

	//reset all weights to 0, O(s)
	void clear(){
		sum = 0;
		for(unsigned int i = 0; i < size; i++)
			weights[i] = 0;
	}

	//get an individual weight, O(1)
	double get_weight(unsigned int i) const {
		return weights[i];
	}

	//get the sum of the weights, O(1)
	double sum_weight() const {
		return sum;
	}

	//rebuilds the sum based on the weights, O(s)
	void rebuild() {
		sum = 0;
		for(unsigned int i = 0; i < size; i++)
			sum += weights[i];
	}

	//sets the weight but doesn't update the sum, needs to be fixed with rebuild(), O(1)
	void set_weight_fast(unsigned int i, double w){
		weights[i] = w;
	}

	//sets the weight and updates the sum, O(1)
	void set_weight(unsigned int i, double w){
		sum += w - weights[i];
		weights[i] = w;
	}

	void print() const {
		printf("weights, sum: %f; ", sum);
		for(unsigned int i = 0; i < size; i++)
			if(weights[i] > 0)
				printf("%u: %f; ", i, weights[i]);
		printf("\n");
	}

	//return a weighted random index, O(s)
	unsigned int choose() {
		if(sum <= 0)
			rebuild();
		int count = 2;
		while(sum > 0 && count--){
			double r = unitrand() * sum;
			for(unsigned int i = 0; i < size; i++){
				if(r <= weights[i])
					return i;
				r -= weights[i];
			}
			rebuild();
		}
		printf("No moves found...\n");
		print();
		return -1;
	}
};

}; // namespace Morat
