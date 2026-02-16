/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 04-Mar-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef PRESSURE_AND_FLOW_H_
#define PRESSURE_AND_FLOW_H_

#include <queue>
#include <array>

#include <stdint.h>
#include <time.h>

#include "config.h"

using namespace std;

#define LOWPASS_COUNT		CONFIG_LP_AVG_COUNT			/* data averaging count */

struct inhaleFilteredData {
	int flowRate;
	time_t flowRate_ms;
};

class PressureFlowRate {
public:

	queue<struct inhaleFilteredData> inhaleFIFO;

	/**
	 * @brief	Constructor
	 */
	PressureFlowRate();

	/**
	 * @brief	Destructor
	 */
	~PressureFlowRate();

	/**
	 * @brief	update the inhale channel pressure and flow rate into the inhale channel FIFO
	 * 			this function MUST be called periodically e.g. every 10ms. After 'LOWPASS_COUNT' calls to this function
	 * 			the values obtained are averaged and added to the FIFO
	 *
	 * @param pressWallPa[in]	pressure in Pascal measured at the wall of the tube
	 * @param pressCenterPa[in] pressure in Pascal measured at the center of the tube
	 * @param flowRateMLPM[in]	flow rate in milli liters per minute (ml/min)
	 * @param flowRateMs[in]	flow rate acquisition time in msec
	 * @return	0 Success
	 * 			-1 Fail
	 */
	int inhalePressureFlowRateUpdate(uint32_t pressWallPa, uint32_t pressCenterPa, int flowRateMLPM, time_t flowRateMs);

	/**
	 * @brief Print the contents of the inhale FIFO
	 */
	void inhaleFIFOPrint();

	/**
	 * @brief Get the size of the inhale FIFO
	 */
	size_t inhaleFIFOSize();

	/**
	 * @brief	Gets the next object from the inhale FIFO and copies it to address pointed by ifd
	 * @param ifd[out]	Mst have a valid address. The dequeued object is copied here
	 * @return
	 * 		0 	success
	 * 		-EINVAL	if ifd is NULL
	 * 		-1	if not data is present in the inhale FIFO
	 */
	int inhaleFIFONextObjGet(struct inhaleFilteredData *ifd);

	/**
	 * @brief Get the reference to the inhale FIFO
	 */
//	queue<struct inhaleFilteredData>& inhaleFIFORefGet();
private:
	uint32_t inhalePressureWall;		/* inhale channel wall pressure measured in Pascal */
	uint32_t inhalePressureCenter;		/* inhale channel center pressure measured in Pascal */
	vector<int> inhaleFlowRate;			/* inhale channel flow rate in ml/min */
	vector<time_t>inhaleFlowRate_ms;	/* inhale channel flow rate measurement time in msec */
};



#endif /* PRESSURE_AND_FLOW_H_ */
