/*
    Copyright (c) 2019 Robert Bosch GmbH
    All rights reserved.

    This source code is licensed under the MIT license found in the
    LICENSE file in the root directory of this source tree.
*/

#ifndef SOURCE_ENCRYPTION_H_
#define SOURCE_ENCRYPTION_H_

/* global interface task declarations */
xTaskHandle EncryptionTask;
xTaskHandle DecryptionTask;

/* global interface function declarations */
Retcode_T encryptData(uint8_t const *payload, size_t iLength, uint8_t *oBuff, size_t ioBuffLength, size_t *oLength_ptr);
Retcode_T decryptData(uint8_t const *payload_ptr, size_t iLength, uint8_t *oBuff, size_t ioBuffLength, size_t *oLength_ptr);
Retcode_T InitMbedCrypto(void);
Retcode_T CalculateHash(uint8_t const *payload_ptr, size_t iLength, uint8_t *calculatedHash_ptr, size_t iLengthOBuffer);

#endif /* SOURCE_ENCRYPTION_H_ */
