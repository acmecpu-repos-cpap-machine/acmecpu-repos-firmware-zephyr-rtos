/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-May-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_FRAME_H_
#define SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_FRAME_H_

#define UART_M2M_FRAME_SIZE_MAX		CONFIG_UART_M2M_BUFFER_SIZE
#define UART_M2M_HEADER_SIZE_MAX		16
#define UART_M2M_PAYLOAD_SIZE_MAX		(UART_M2M_FRAME_SIZE_MAX - UART_M2M_HEADER_SIZE_MAX)
#define UART_M2M_START_OF_FRAME			0xAC53	/* start of frame */

typedef enum {
	UART_M2M_FRAME_SINGLE_REQ=0,	/* data transfer request with acknowledgment */
	UART_M2M_FRAME_SINGLE_RESP,		/* data transfer response with acknowledgment */
	UART_M2M_FRAME_STREAM_REQ,		/* data stream request */
	UART_M2M_FRAME_STREAM_RESP,		/* data stream response without acknowledgment */
	UART_M2M_FRAME_DATA_REQ,		/* bulk data transfer request */
	UART_M2M_FRAME_DATA_RESP,		/* bulk data transfer response */
	UART_M2M_FRAME_DATA_ACK,		/* bulk data transfer acknowledgment */
} UART_M2M_FRAME_TYPE;

struct m2m_frame_t {
	uint16_t sof;					/* start of frame (UART_M2M_START_OF_FRAME) */
	uint8_t type;					/* type of frame */
	uint32_t sequence;				/* sequence number for transfers with multiple packets */
	uint32_t ack;					/* acknowledgment number = sequence number + 1 */
	uint8_t checksum;				/* XOR checksum of header, with checksum and payload data, while computing checksum, the checksum field itself should be 0 */
	uint32_t payload_len;			/* length of payload data */
	uint8_t payload[UART_M2M_PAYLOAD_SIZE_MAX];	/* payload data */
};


int m2m_comm_frame_serialize(uint8_t *sbuf, uint32_t sbuf_len, struct m2m_frame_t *frame, uint32_t *sdata_len);

/**
 * @brief		This function decodes the byte stream into a struct m2m_frame_t object
 * 				After decoding, it also verifies the checksum.
 * @param sbuf[in]		Input buffer
 * @param len[in]		Length of buffer
 * @param frame[out]	Decoded frame
 * @return
 * 	0			Successfully decoded the buffer into frame variable and verified the checksum
 * 	-EINVAL		Invalid input parameters
 * 	-EPROTO		Input data's checksum did not match with calculated checksum
 * 	-E2BIG		Payload length is greater than the maximum configured length (UART_M2M_PAYLOAD_SIZE_MAX)
 */
int m2m_comm_frame_decode(uint8_t *sbuf, uint32_t len, struct m2m_frame_t *frame);

void m2m_comm_frame_header_single_req_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_single_resp_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_stream_req_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_stream_resp_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_data_req_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_data_resp_make(struct m2m_frame_t *frame);

/**
 * @brief	This function allocates memory for a frame and then serializes it for transmission
 * 			This function calls m2m_comm_frame_serialize() internally.
 * @param frame[in]		The frame containing the data
 * @param buf_len[out]	Serialized buffer length in bytes
 * @return
 * 		Success - address of the serialized buffer
 * 		NULL if failed to allocate memory
 */
uint8_t* m2m_comm_frame_alloc_serialize(struct m2m_frame_t *frame, size_t *buf_len);

/**
 * @brief	Verify the checksum of an incomming frame
 * @param frame[in]	The frame having the checksum field populated
 * @return
 * 		0	checksum matches successfully
 * 		-1	checksum did not match or other errors
 * 		-EINVAL	if frame is null
 */
int m2m_comm_frame_checksum_verify(struct m2m_frame_t *frame);

/**
 * @brief		Computes the checksum of a packet and copies the checksum byte
 * 				in to the appropriate field
 * @param frame[in/out] The frame whose checksum should be calculated and set.
 * @return
 */
int m2m_comm_frame_checksum_compute(struct m2m_frame_t *frame);


#endif /* SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_FRAME_H_ */
