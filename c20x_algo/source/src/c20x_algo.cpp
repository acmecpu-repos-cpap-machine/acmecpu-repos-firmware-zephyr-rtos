/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 04-Mar-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <string>

#include "PressureFlowRate.h"
#include "Breath.h"

using namespace std;

static PressureFlowRate pressureFlow;
static int baselineFlowRate = 0;

int baselineFlowrateGet()
{
	return baselineFlowRate;
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("incorrect number of arguments!\n");
		printf("usage: c20xalgo sample_file.csv\n");
		return -1;
	}

	printf("number of argv = %d\n", argc);
	printf("input data file: %s\n", argv[1]);
	printf("==============================\n");

	/* read the entire data file and add to the inhale FIFO */
	std::ifstream file(argv[1]);
	std::string line;
	std::string delimiter = ",";
	int tmp;
	while (std::getline(file, line)) {
		size_t pos = 0;
		std::vector<int> data;
		while ((pos = line.find(delimiter)) != std::string::npos) {
			std::string token = line.substr(0, pos);
			tmp = atoi(token.c_str());
			data.push_back(tmp);
			line.erase(0, pos + delimiter.length());
		}
		tmp = atoi(line.c_str());
		data.push_back(tmp);

		/* the vector will contain only 2 data, we may need to change this depending on the input file */
		int ms = data.at(0);
		int flow = data.at(1);
//		cout << ms << "," << flow << endl;

		data.clear();

		/* add the flow rate and timestamp to inhale FIFO */
		pressureFlow.inhalePressureFlowRateUpdate(0, 0, flow, ms);
	}

	cout << "FIFO size = " << pressureFlow.inhaleFIFOSize() <<endl;

	/* calculate parameters for each breath */
	Breath breath(5000);
	struct inhaleFilteredData ifd;
	struct BreathParams bp, bp_prev;
	int ret=0, first=0;

	while (pressureFlow.inhaleFIFONextObjGet(&ifd) == 0) {
//		cout << ifd.flowRate << ", " << ifd.flowRate_ms <<endl;

		if (first == 0) {
			ret = breath.InhaleParamsCalculate(ifd.flowRate, ifd.flowRate_ms, &bp, NULL);
			if (ret == 0) {
//				cout << "Breath Params:\n";
//				cout << "Inhale start: " << bp.inhaleStartMs <<endl;
//				cout << "Inhale duration: " << bp.inhaleDurationMs <<endl;
//				cout << "Inhale peak: " << bp.inhaleFlowRatePeak <<endl;
//				cout << "Inhale peak duration: " << bp.inhalePeakDurationMs <<endl;
//				cout << "Breath duration: " << bp.breathDurationMs <<endl;
				first = 1;
				memcpy(&bp_prev, &bp, sizeof(struct BreathParams));
			}
		} else {
			ret = breath.InhaleParamsCalculate(ifd.flowRate, ifd.flowRate_ms, &bp, &bp_prev);
			if (ret == 0) {
				cout << "Breath Params:\n";
				cout << "Inhale start: " << bp_prev.inhaleStartMs <<endl;
				cout << "Inhale duration: " << bp_prev.inhaleDurationMs <<endl;
				cout << "Inhale peak: " << bp_prev.inhaleFlowRatePeak <<endl;
				cout << "Inhale peak duration: " << bp_prev.inhalePeakDurationMs <<endl;
				cout << "Breath duration: " << bp_prev.breathDurationMs <<endl;
				cout << "-----------------------------\n";
				memcpy(&bp_prev, &bp, sizeof(struct BreathParams));
			}
		}
	}

	return 0;
}
