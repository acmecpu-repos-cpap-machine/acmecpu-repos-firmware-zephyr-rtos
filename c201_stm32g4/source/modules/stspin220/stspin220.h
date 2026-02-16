/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_STSPIN220_STSPIN220_H_
#define MODULES_STSPIN220_STSPIN220_H_

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdint.h>
#include <zephyr/device.h>

#define STSPIN220_STEP_MODE_STEP_FULL		0x00	/* Full step */
#define STSPIN220_STEP_MODE_STEP_2			0x0A	/* 1/2 step */
#define STSPIN220_STEP_MODE_STEP_4			0x05	/* 1/4th step */
#define STSPIN220_STEP_MODE_STEP_8			0x0B	/* 1/8th step */
#define STSPIN220_STEP_MODE_STEP_16			0x0F	/* 1/16 step */
#define STSPIN220_STEP_MODE_STEP_32			0x01	/* 1/32nd step */
#define STSPIN220_STEP_MODE_STEP_64			0x07	/* 1/64th step */
#define STSPIN220_STEP_MODE_STEP_128		0x02	/* 1/128th step */
#define STSPIN220_STEP_MODE_STEP_256		0x03	/* 1/256th step */

#define STSPIN220_DIR_CLOCKWISE				0
#define STSPIN220_DIR_ANTICLOCKWISE			1

#define STSPIN220_STEPPER_RUNNING			1
#define STSPIN220_STEPPER_NOT_RUNNING		0

/* application callback function type */
typedef void (*stspin220_stop_handler_t)(const struct device *dev, uint32_t steps_completed);

/* API type defines */
typedef int (*stspin220_dir_set_t)(const struct device*, const uint8_t);
typedef int (*stspin220_step_clock_set_t)(const struct device*, const uint32_t);
typedef int (*stspin220_num_step_set_t)(const struct device*, const uint32_t);
typedef int (*stspin220_vref_pwm_set_t)(const struct device*, const uint32_t, const uint32_t);
typedef int (*stspin220_step_mode_set_t)(const struct device*, const uint8_t);
typedef int (*stspin220_fault_get_t)(const struct device *);
typedef int (*stspin220_enable_t)(const struct device *);
typedef int (*stspin220_disable_t)(const struct device *);
typedef int (*stspin220_standby_t)(const struct device *);
typedef int (*stspin220_resume_t)(const struct device *);
typedef int (*stspin220_start_t)(const struct device *);
typedef int (*stspin220_stop_t)(const struct device *);
typedef int (*stspin220_status_get_t)(const struct device *);
typedef int (*stspin220_stop_handler_set_t)(const struct device *, stspin220_stop_handler_t);

struct stspin220_driver_api {
	/**
	 * Function signature:
	 * static int stspin220_direction_set(const struct device *dev, const uint8_t dir)
	 *
	 * @brief: 	sets the direction of rotation
	 * @param:	dev	device driver instance obtained from device_get_binding()
	 * 			dir direction, 0 - clockwise, 1 - anti-clockwise
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_dir_set_t dir_set;

	/**
	 * Function signature:
	 * static int stspin220_step_clock_set(const struct device *dev, const uint32_t period_us)
	 *
	 * @brief: 	Sets the step clock (STCK) input frequency (fSTCK)
	 * 			fSTCK max = 1MHz
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 * 			period_us 	time period of 1 clock cycle in micro-seconds
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_step_clock_set_t step_clock_set;

	/**
	 * Function signature:
	 * static int stspin220_num_step_set(const struct device *dev, const uint32_t num_steps)
	 *
	 * @brief: 	Sets the number of steps the motor should take when started
	 * 			Stepping should be started by stspin220_start()
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 * 			num_steps	number of steps to take
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_num_step_set_t num_step_set;

	/**
	 * Function signature:
	 * static int stspin220_vref_pwm_set(const struct device *dev, const uint32_t period_us, const uint32_t duty_cycle)
	 *
	 * @brief: 	Sets the reference voltage at STSPIN220 REF pin via PWM.
	 * 			The reference voltage value Vref, must be selected according to the
	 * 			load current target value (peak value) and the sense resistor value
	 * 			Vref = Rsnsx . Iload,peak
	 *
	 * 			if, Rsnsx = 2.2ohm and Iload,peak = 200mA
	 * 			Vref = 2.2 * 0.2 = 0.44V
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 * 			period_us 	pwm period in micro-seconds
	 * 			duty_cycle	pwm duty cycle in percentage
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_vref_pwm_set_t vref_pwm_set;

	/**
	 * Function signature:
	 * static int stspin220_step_mode_set(const struct device *dev, const uint8_t step_mode)
	 *
	 * @brief: 	Sets the step mode through MODEx pins
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 * 			step_mode 	full step to 1/256th step mode, use STSPIN220_STEP_MODE_STEP_x MACROS
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_step_mode_set_t step_mode_set;

	/**
	 * Function signature:
	 * static int stspin220_fault_get(const struct device *dev)
	 *
	 * @brief: 	Get the EN/FAULT pin level
	 * 			When the over-current or short-circuit protection is triggered the
	 * 			EN/FAULT input is forced LOW
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	0 when fault occurs
	 * 			1 in normal condition
	 * */
	stspin220_fault_get_t fault_get;

	/**
	 * Function signature:
	 * static int stspin220_enable(const struct device *dev)
	 *
	 * @brief: 	Sets EN/FAULT and STBY/RESET pin HIGH
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_enable_t enable;

	/**
	 * Function signature:
	 * static int stspin220_disable(const struct device *dev)
	 *
	 * @brief: 	Sets EN/FAULT and STBY/RESET pin LOW
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_disable_t disable;

	/**
	 * Function signature:
	 * static int stspin220_standby(const struct device *dev)
	 *
	 * @brief: 	Sets STBY/RESET pin LOW
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_standby_t standby;

	/**
	 * Function signature:
	 * static int stspin220_resume(const struct device *dev)
	 *
	 * @brief: 	Sets STBY/RESET pin HIGH
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_resume_t resume;

	/**
	 * Function signature:
	 * static int stspin220_start(const struct device *dev)
	 *
	 * @brief: 	Starts the step clock (STCK) to start spinning the motor
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_start_t start;

	/**
	 * Function signature:
	 * static int stspin220_stop(const struct device *dev)
	 *
	 * @brief: 	Stops the step clock (STCK) to stop spinning the motor
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	stspin220_stop_t stop;

	/**
	 * Function signature:
	 * static int stspin220_status_get(const struct device *dev)
	 *
	 * @brief: 	Returns the status of the stepper motor, running or nor running
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 *
	 * @return:	1	STSPIN220_STEPPER_RUNNING
	 * 			0	STSPIN220_STEPPER_NOT_RUNNING
	 * */
	stspin220_status_get_t status_get;

	/**
	 * Function signature:
	 * static int stspin220_stop_handler_set(const struct device *dev, stspin220_stop_handler_t handler)
	 *
	 * @brief: 	The application can register a callback function by calling this api.
	 * 			The call back must be of type stspin220_stop_handler_t
	 * 			Whenever the stepper motor is stopped, either by num_steps expiry or via user called api
	 * 			the handler function will get called providing the number of steps made by the motor
	 *
	 * @param:	dev			device driver instance obtained from device_get_binding()
	 * 			handler		callback function that will get called when a stop condition occurs
	 *
	 * @return:	0	Success
	 * 			-ERRNO for failure
	 * */
	stspin220_stop_handler_set_t stop_handler_set;
};

//#ifdef __cplusplus
//}
//#endif

#endif /* MODULES_STSPIN220_STSPIN220_H_ */
