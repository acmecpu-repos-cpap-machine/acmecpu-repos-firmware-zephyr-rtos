/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 04-Mar-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <iostream>
#include <numeric>
#include <string.h>

#include "PressureFlowRate.h"

PressureFlowRate::PressureFlowRate()
{
	cout << "PressureFlowRate Constructor" <<endl;
	inhalePressureWall = 0;
	inhalePressureCenter = 0;
//	inhaleFlowRate.fill(0);
//	inhaleFlowRate_ms.fill(0);
}

PressureFlowRate::~PressureFlowRate()
{
	cout << "PressureFlowRate Destructor" <<endl;

	/* delete the data in the vector */
//	inhaleFlowRate.clear();
//	inhaleFlowRate_ms.clear();
	vector<int>().swap(inhaleFlowRate);	// https://stackoverflow.com/questions/10464992/c-delete-vector-objects-free-memory
	vector<time_t>().swap(inhaleFlowRate_ms);

	/* delete the inhale FIFO */
	queue<struct inhaleFilteredData>().swap(inhaleFIFO);
}

int PressureFlowRate::inhalePressureFlowRateUpdate(uint32_t pressWallPa,
		uint32_t pressCenterPa, int flowRateMLPM, time_t flowRateMs)
{
	int ret = 0;

	inhaleFlowRate.push_back(flowRateMLPM);
	inhaleFlowRate_ms.push_back(flowRateMs);

	if (inhaleFlowRate.size() == LOWPASS_COUNT) {
		/* average out the flowrate and timestamp */
		struct inhaleFilteredData ifd;
		ifd.flowRate = accumulate(inhaleFlowRate.begin(), inhaleFlowRate.end(), 0) / inhaleFlowRate.size();
		ifd.flowRate_ms = accumulate(inhaleFlowRate_ms.begin(), inhaleFlowRate_ms.end(), 0) / inhaleFlowRate_ms.size();;

		/* delete the data in the vector */
		inhaleFlowRate.clear();
		inhaleFlowRate_ms.clear();

		/* add the averaged result in the inhale FIFO */
//		cout << "Adding to inhale FIFO: " << ifd.flowRate << ", " << ifd.flowRate_ms <<endl;
		inhaleFIFO.push(ifd);
	}
	return ret;
}

void PressureFlowRate::inhaleFIFOPrint()
{
	cout << "inhaleFIFOPrint is not implemented!" <<endl;
}

size_t PressureFlowRate::inhaleFIFOSize()
{
	return inhaleFIFO.size();
}

int PressureFlowRate::inhaleFIFONextObjGet(struct inhaleFilteredData *ifd)
{
	if (ifd == NULL) {
		return -EINVAL;
	}

	if (inhaleFIFO.empty()) {
		return -1;
	}

	/* copy the next element and delete it from the queue */
//	memcpy(ifd, inhaleFIFO.front(), sizeof(struct inhaleFilteredData));
	*ifd = inhaleFIFO.front();
	inhaleFIFO.pop();

	return 0;
}

//queue<struct inhaleFilteredData>& PressureFlowRate::inhaleFIFORefGet()
//{
//	return &inhaleFIFO;
//}


