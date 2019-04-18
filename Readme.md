# EthereumOnXDK (EOX)

This is the companion code for Connecting XDKs to Ethereum Network. The code allows the users to set up CoAP communication
between two XDKs and to execute a SmartContract running on Ethereum Nodes.  
It enables RSA secured data exchange against payment between two Bosch XDK's over the Ethereum Blockchain node. 


## Purpose of the project

This software is a research prototype, solely developed for and published as
part of the publication of XDK example codes. It will neither be maintained nor
monitored in any way.

## License
The software and documentation is released under the MIT license.
See the [LICENSE](LICENSE) file for details. For a list of other open source 
components included in EOX, see the file [3rd-party-licenses.txt](3rd-party-licenses.txt).

## Requirements, how to build, test, install, use, etc.
The program was written with XDK workbench release version 3.3.1. 

To get it running you also need an Ethereum Blockchain setup which includes at least two nodes with unlocked 
accounts, JSON-RPC open and mining enabled (for a local setup, I recommend using Truffle and Ganache (Version 
1.2.0 is used here)).

Step by step instructions:
1. Get the source code
2. Import the project into the XDK workbench with the import wizard (existing code as makefile project). Choose BCDS XDK Toolchain for indexer settings.
3. Adapt common XDK makefiles as described in the HowToMbedTLS folder
4. Adapt UserConfig.h header file to your needs. Wi-Fi credentials and Ethereum blockchain information (Contract address, Producer/Consumer account address) has to be adapted. 
5. Increase ServalPal stack size to 1024 Bytes #define TASK_STACK_SIZE_SERVALPAL_CMD_PROC UINT32_C(1024) in file SDK\xdk110\Common\legacy\source\ServalPAL_WiFi.c.
6. Clean, Build and flash the project for Producer and Consumer. Producer and Consumer build can be started via the build variables ENABLE_PRODUCER/ENABLE_CONSUMER in UserConfig.h file. 
	

To build the **Producer** you have to enable the ``#define ENABLE_PRODUCER`` macro in *source\UserConfig.h* and disable ``#define ENABLE_CONSUMER``.

To build the **Consumer** you have to enable the ``#define ENABLE_CONSUMER`` macro in *source\UserConfig.h* and disable ``#define ENABLE_PRODUCER``.

**Note:** If your blockchain uses other account/contract addresses, then you have to adapt those in the source code in the *UserConfig.h* file. If you want to use a WIFI enterprise network, you have to update the WIFI chip. A HowTo can be found in the Bosch XDK community (https://xdk.bosch-connectivity.com/community/-/message_boards/message/260455).


**Important:** Increase ServalPal stack size to 1024 Bytes ``#define TASK_STACK_SIZE_SERVALPAL_CMD_PROC UINT32_C(1024)`` in file *xdk110\Common\legacy\source\ServalPAL_WiFi.c*. 

*  **debug_Consumer/Producer folders** contain the Consumer and Producer debug binaries.
*  **HowToMbedTLS** folder contains a HowTo for compiling and including the mbedTLS library in the Bosch XDK project. It also contains two configuration makefiles which can be used as a template.
*  **mbedtls** folder contains the compiled mbedtls libraries as well as the include files. 
*  **RSA_1024_Bit_Keypairs** contains the generated private/public key pairs as .pem files.
*  **SmartContract** folder contains the Ethereum smart contract implemented in solidity. 
*  **Source** folder contains the XDK application code.
*  **SED_ConsoleOutput.txt** shows the console output of one communication cycle between Producer and Consumer. 



## Use-Case
### Short version
* One edge device called *Producer* (Bosch XDK1) collects and sells acceleration data during a given period of time. The second edge device called *Consumer* (Bosch XDK2) is willing to buy this acceleration data from the *Producer*.
* The Producer provides a smart contract running on the Ethereum blockchain to provide data integrity, a secure payment channel, an incentive-based evaluation system and RSA key exchange along with Consumer authentication.
* Off-chain data is encrypted via the RSA algorithm and is directly exchanged over the CoAP protocol between Producer and Consumer.
* On-chain communication between the XDKs and the Ethereum node RPC server is done via the http protocol. 
* The XDK implementation as well as the smart contract implementation are limited to two communication partners at a time. 

### Long version
Two constraint embedded devices securely exchange encrypted sensor data against a payment. One edge device called *Producer* (Bosch XDK1) collects acceleration data during a given period of time. 
The second edge device called *Consumer* (Bosch XDK2) is willing to buy this acceler-ation data from the *Producer*.

The two parties go through five successive phases. Starting with a *Registration* phase, continue through a *Search* and *Enquiry* phase, to a *Settlement* and finally a *Post-Settlement* phase. 
On the *Producer* and *Consumer* side, the RSA keypair is stored in local buffers to enable encrypted off-chain communication. For now, those keys are generated offline once and then stored on the XDK. 
The remote nodes which are used for on-chain communication are registered to the blockchain in the *Registration* phase. 
Here, the *Producer* communicates via *Node1* whereas the *Consumer* communicates via *Node2* with the blockchain network. After this, the unlocked *Producer/Consumer* Ethereum accounts with a unique 
address as well as the XDKs are setup for on-/off-chain communication. This registration process only takes place once at the beginning. For testing purposes, this means a private Ethereum blockchain setup, 
including two nodes with at least one unlocked account on each node with mining enabled is required. 

The smart contract is deployed to the blockchain through *Node1* which represents the *Producer* account. As a result, the smart contract gets stored on the distributed ledger with his own unique Ethereum address. 
The contract sets the creator account as the smart contract owner. Everyone participating in the network can interact with the contract through the generated address, but some contract functions are access 
restricted to the contract owner. *Node2* is now able to find the contract if it knows the address in advance. An automatic search function to somehow find the contract address without knowing the 
address in advance is not supported. The *Registration* and *Search* phases are used to setup the entire communication environment.

As mentioned previously, the *Consumer* must know the contract address to interact with it. For this, the first step of the *Enquiry* phase is to provide the contract address to the *Consumer*. 
As the *Producer* is the contract owner, he provides this information directly through the off-chain communication channel. After the contract address is received, the *Consumer* pushes his RSA public key
along with a pre-payment through *Node2* into the smart contract. *Node2* is responsible to prepare and forward the json-rpc message to the distributed ledger network. This requires a specific hash and sign
procedure with the Ethereum account keypair. The public key, the payment and the corresponding *Consumer* account address are stored in the smart contract. So, the contract stores a current state of the *Consumer*,
including his payed amount of ether and the associated public key.

After the *Consumer* receives a transaction confirmation from the blockchain, he requests sensor data directly from the *Producer*. The *Producer* reads the provided information out of the contract and 
is now able to encrypt data locally with the *Consumer* public key from the blockchain. Furthermore, the *Producer* also stores the associated unique Ethereum account address and is therefore able to authenticate the *Consumer*. 
With this information locally available at the *Producer*, it removes the need to rewrite the *Consumers* RSA public key into the blockchain during a later communication cycle. This means, after the public key is written once, 
the authenticated *Consumer* can now directly communicate with the *Producer*. So, the *Consumer* pays only once during the write public key transaction for the data until the *Producer* decides to delete the locally stored 
*Consumer* information.

To provide data integrity, the *Producer* calculates the hash of the encrypted data. The hash value gets written into the smart contract through *Node1*, which is again responsible to process and forward the json-rpc message 
of the *Producer* to the ledger network.

Finally, the *Producer* sends the encrypted acceleration sensor data off-chain to the *Consumer*. Because only the *Consumer* owns and knows the private RSA key, only he can decrypt the received sensor data. 
To verify the received data, the *Consumer* reads the data hash out of the smart contract and compares it with his self-calculated data hash. If something went wrong during transaction or if anyone manipulated the data, 
the *Consumer* can immediately detect the tampered data. 

Along with the RSA public key write operation, the *Consumer* must provide a specified amount of *ether* as a pre-payment. During the write operation, the smart contract checks if enough ether is sent along the transaction. 
If this is the case, the pre-payment gets stored in the smart contract along with the public key. This payment is now only accessible by the smart contract on the blockchain, which means neither the *Producer* 
nor the *Consumer* owns it. The *Settlement* phase is executed in parallel to the *Enquiry* phase but is only executed if the right amount of *ether* is sent along with the write public key transaction.

Finally, the *Post-settlement* phase is executed after the encrypted sensor data has been exchanged and verified. After the *Consumer* has verified the received sensor data based on the data hash from the blockchain, 
as a final step he evaluates the transaction with the *Producer*. A positive or a negative evaluation option is available. Only a *Consumer* who previously made a payment is allowed to vote. 
The *Producer* itself is also excluded from the voting procedure. 

This evaluation system provides incentive for the *Producer* and the *Consumer* to act “fair” during the entire process. It automatically releases the pre-payment of the *Settlement* phase to the *Producer Node1* if the 
evaluation result is positive and refunds the *Consumer Node2* with half of his initial payment. If the evaluation result is negative, the *Consumer* as well as the *Producer* don’t receive any funds from the smart contract. 

Furthermore, other *Consumers* can read the current *Producer* rating out of the smart contract as a percentage value. This gives the *Producer* one more incentive to provide unaltered and correct data. 

## HowTo use the implemented demonstrator

1. Press button1 on XDK1 (Producer) to record accelerometer data before communication
	* XDK1 calculates the acceleration data during the first 1000 ticks after button pressed on XDK1
	* Yellow LED of XDK1 lights up if acceleration data < 5g, red if accel-data > 5g 
2. By clicking button1 on XDK2 (Consumer), it asks XDK1 about the smart contract address
3. XDK1 sends the SC address
4. XDK2 writes pubKey2 to SC, SC stores address of XDK2 along with pubKey2
5. By clicking button1 on XDK2, it initializes the acceleration sensor data preparation
6. XDK1 reads pubKey2 and prepares the sensor data 
7. By clicking button1 on XDK2, it requests acceleration data of XDK1 
8. If data processing is finished, XDK1 sends the encrypted sensor data
9. Yellow LED of XDK2 lights up if received accel-data < 5g, red if accel-data > 5g 
10. XDK2 sends evaluation voting to SC


