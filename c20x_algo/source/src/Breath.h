/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 06-Mar-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_BREATH_H_
#define SRC_BREATH_H_

#include <stdint.h>
#include <time.h>

#include "PressureFlowRate.h"

using namespace std;

/* Structure to hold configurations for breath detection */
struct BreathConfig {
	int minNumSamples;	/* minimum number of samples required to do computation */
	float inhaleStartThresh;	/* threshold in percentage */
	int maxStorageBufferSize;	/* maximum number of breath samples to hold in ram as a part of a double buffer system */
};

/* Structure used for inhale parameter calculation */
struct FlowInfo {
	int flow;
	time_t flow_ms;
	float percent_of_bl;	/* amount the flow rate is greater or lesser than the baseline flow in percentage */
};

struct BreathParams {
	time_t inhaleStartMs;		/* time in ms, flow rate increases by CONFIG_INHALE_START_DETECTION_PERCENTAGE % over the baseline */
	time_t inhaleDurationMs;	/* time in ms, flow rate increases by CONFIG_INHALE_START_DETECTION_PERCENTAGE % over the baseline until time flow rate returns o baseline */
	int inhaleFlowRatePeak;		/* highest flow rate value over last breath */
	time_t inhalePeakDurationMs;	/* number of ms that flow rate is within n % of inhaleFlowRatePeak during the last breath */
	time_t breathDurationMs;		/* duration inhale started from last inhale start */
};

typedef enum {
	BREATH_DETECTION_RESTART=0,
	INHALE_START,
	INHALE_DURATION_OF_INHALE,
	INHALE_PEAK,
	INHALE_PEAK_DURATION,
	BREATH_DURATION,
	BREATH_STATE_UNKNOWN,
} BREATH_STATES;

class Breath : PressureFlowRate {
public:
	int baselineFlowRate;		/* minimum flow rate over last 60 secs, this is calculated by caller */
	BREATH_STATES breathState;	/* holds the current state of the breath detection process */
	struct BreathConfig config;	/* holds the configurations used for breath detection and its parameter calculations */

	/**
	 * @brief	Constructor
	 */
	Breath();

	/**
	 * @brief	Constructor to set a default baseline flow rate
	 * @param	baselineFR	initial baseline flow rate. This will be periodically updated by BaselineFlowRateUpdate
	 */
	Breath(int baselineFR);

	/**
	 * @brief	Destructor
	 */
	virtual ~Breath();

	/**
	 * @brief	Function to update the baseline flow rate. Baseline is calculated by caller
	 * @param	baselineFR	This will be periodically updated as the baseline may change over time
	 */
	void BaselineFlowRateUpdate(int baselineFR);

	/**
	 * @brief
	 * 	This function computes the below parameters of each breath:
	 * 		1. time of inhale start
	 * 		2. duration of inhale
	 * 		3. peak inhale flow rate
	 * 		4. duration of peak inhale flow rate
	 * 		5. breath duration
	 *	This function should be called from a thread.
	 *
	 * @param flowRateMLPM[in]		flow rate in milli liters per minute (ml/min)
	 * @param flowRateMs[in]	flow rate acquisition time in msec
	 * @param bp[out]			current breath parameters populated by this function
	 * @param bp_prev[out]		breath duration of previous breath populated by this function
	 *
	 * @return
	 * 		0 	Success
	 * 		1	send next data
	 * 		-1	Fail
	 */
	int InhaleParamsCalculate(int flowRateMLPM, time_t flowRateMs, struct BreathParams *bp, struct BreathParams *bp_prev);

	/**
	 * @brief	Checks if a vector is sorted in ascending order or not
	 * @param vec[in]	vector to check
	 * @param n			size of the vector
	 * @return
	 * 		1	sorted
	 * 		0	not sorted
	 */
	int vectorSortedAscOrNot(vector<FlowInfo> &vec, int n);

	/**
	 * @brief	Find out the sample closest to and greater than thresh% of baselineFlow value
	 * @param vec[in]		vector to check
	 * @param thresh[in]	threshold in percentage
	 * @param baselineFlow[in] baseline flow rate
	 * @return
	 * 		idx of the vector
	 * 		-1 if not found
	 */
	int findInhaleStartIdx(vector<FlowInfo> &vec, float threshPercent, int baselineFlow);

	/**
	 * @brief	store data into double buffer
	 * @param fi[in] populated struct FlowInfo variable
	 */
	void fillDoubleBuffer(FlowInfo *fi);

private:
	vector<FlowInfo> flowInfoV;		/* vector of FlowInfo used for breath param calculation */

	vector<FlowInfo> v1;		/* vector of a double buffer system used for storage */
	vector<FlowInfo> v2;		/* vector of a double buffer system used for storage */

	bool isSnoringPresent;		/* whether snoring is detected */
};

#endif /* SRC_BREATH_H_ */
