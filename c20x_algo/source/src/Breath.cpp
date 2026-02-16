/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 06-Mar-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdint.h>
#include <iostream>
#include <errno.h>

#include "Breath.h"
#include "config.h"

#define MIN_NUM_SAMPLES		(CONFIG_MIN_NUM_SAMPLES)
#define INHALE_THRESHOLD	(CONFIG_INHALE_START_THRESH_PERCENT)
#define MAX_BUFFER_SIZE		(CONFIG_MAX_BUFFER_SIZE)

Breath::Breath(int baselineFR)
{
	baselineFlowRate = baselineFR;
//	inhaleStartMs = 0;
//	inhaleDurationMs = 0;
//	inhaleFlowRatePeak = 0;
//	inhalePeakDurationMs = 0;
//	breathDurationMs = 0;
	isSnoringPresent = false;
	breathState = BREATH_DETECTION_RESTART;

	config.minNumSamples = MIN_NUM_SAMPLES;
	config.inhaleStartThresh = INHALE_THRESHOLD;
	config.maxStorageBufferSize = MAX_BUFFER_SIZE;
}

Breath::~Breath()
{
	// TODO Auto-generated destructor stub
}

void Breath::BaselineFlowRateUpdate(int baselineFR)
{
	baselineFlowRate = baselineFR;
}

int Breath::vectorSortedAscOrNot(vector<FlowInfo> &vec, int n)
{
	if ((n == 1) || (n == 0))
		return 1;

	if (vec[n-1].flow < vec[n-2].flow) {
		return 0;
	}

	return vectorSortedAscOrNot(vec, n-1);
}

int Breath::findInhaleStartIdx(vector<FlowInfo> &vec, float threshPercent, int baselineFlow)
{
	for (int i=0; i < vec.size(); i++) {
		float diffPercent = ((vec[i].flow - baselineFlow) * 100) / baselineFlow;
		if ((diffPercent >= threshPercent) && (diffPercent < threshPercent+10)) {
			return i;
		}
	}
	return -1;
}

void Breath::fillDoubleBuffer(struct FlowInfo *fi)
{
	if ((v1.size() < config.maxStorageBufferSize) && (v2.size() < config.maxStorageBufferSize)) {
		v1.push_back(*fi);
	} else if ((v1.size() == config.maxStorageBufferSize) && (v2.size() < config.maxStorageBufferSize)) {
		v2.push_back(*fi);
	} else if ((v1.size() == config.maxStorageBufferSize) && (v2.size() == config.maxStorageBufferSize)) {
		v1.clear();
		v1.push_back(*fi);
	} else if ((v1.size() < config.maxStorageBufferSize) && (v2.size() == config.maxStorageBufferSize)) {
		v2.clear();
		v2.push_back(*fi);
	}
}

int Breath::InhaleParamsCalculate(int flowRateMLPM, time_t flowRateMs, struct BreathParams *bp, struct BreathParams *bp_prev)
{
	int ret=1;

	if (bp == NULL) {
		return -EINVAL;
	}

	struct FlowInfo fi;
	fi.flow = flowRateMLPM;
	fi.flow_ms = flowRateMs;
	fi.percent_of_bl = ((flowRateMLPM - baselineFlowRate) * 100) / baselineFlowRate;

	/* fill the double buffer */
	fillDoubleBuffer(&fi);

	flowInfoV.push_back(fi);

	switch (breathState) {
	case BREATH_DETECTION_RESTART:
	{
		if (flowInfoV.size() < config.minNumSamples) {
			return 1;	// need more samples to start the calculations
		} else {
			breathState = INHALE_START;
		}
		break;
	}

	case INHALE_START:
	{
		/* 1. Find out inhale start */

		/* 1.1 	Check if the samples are in ascending or not.
		 * 		The samples should be in ascending order when user is inhaling
		 * */
		int res = vectorSortedAscOrNot(flowInfoV, flowInfoV.size());
		if (res == 1) {
			/* 1.2 Find the smallest vector index which is greater than the threshold percentage of baseline flow
			 * */
			int inhaleStartIdx = findInhaleStartIdx(flowInfoV, config.inhaleStartThresh, baselineFlowRate);
			if (inhaleStartIdx < 0) {
//				cout << "findInhaleStartIdx failed\n";
				return -1;
			} else {
				/* 1.3 	Save the inhale start timestamp into the BreathParams out structure
				 * 		continue to get more samples
				 * */
				cout << "Inhale Start " << flowInfoV[inhaleStartIdx].flow << " : " << flowInfoV[inhaleStartIdx].flow_ms <<endl;
				bp->inhaleStartMs = flowInfoV[inhaleStartIdx].flow_ms;
				if (bp_prev != NULL) {
					bp_prev->breathDurationMs = bp->inhaleStartMs - bp_prev->inhaleStartMs;
				}
				breathState = INHALE_PEAK;
				return 0;
			}
		} else {
			flowInfoV.clear();
			return 1;
		}
		break;
	}

	case INHALE_PEAK:
	{
		bp->inhaleFlowRatePeak = 0;
		breathState = INHALE_PEAK_DURATION;
		break;
	}

	case INHALE_PEAK_DURATION:
	{
		bp->inhalePeakDurationMs = 0;
		breathState = INHALE_DURATION_OF_INHALE;
		break;
	}

	case INHALE_DURATION_OF_INHALE:
	{
		bp->inhaleDurationMs = 0;
		breathState = BREATH_DETECTION_RESTART;
		break;
	}

	case BREATH_DURATION:
	{
		break;
	}

	default:
		break;
	}

	return ret;
}
